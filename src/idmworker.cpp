// =============================================================================
//   IDM Tool  –  Worker Engine  |  Copyright © 2026 AliSakkaf
//   Pure WinAPI Edition - Professional Registry Logging
// =============================================================================
#include "idmworker.h"
#include <QProcess>
#include <QThread>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>

// WinAPI (winsock2 BEFORE windows.h)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winreg.h>
#include <aclapi.h>
#include <accctrl.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <sddl.h>

#define LOG(m)      log(m,0)
#define LOG_INFO(m) log(m,1)
#define LOG_WARN(m) log(m,2)
#define LOG_ERR(m)  log(m,3)
#define LOG_OK(m)   log(m,4)

// ─── Privilege helper ────────────────────────────────────────────────────────
static void enablePriv(LPCWSTR name)
{
    HANDLE hTok;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTok))
        return;
    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    LookupPrivilegeValueW(nullptr, name, &tp.Privileges[0].Luid);
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hTok, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(hTok);
}

// ─── Recursive registry key delete (handles locked keys via WinAPI) ──────────
static bool regDeleteRecursive(HKEY root, const wchar_t* subKey)
{
    HKEY hk;
    if (RegOpenKeyExW(root, subKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hk) != ERROR_SUCCESS) {
        // Fallback for 32-bit keys on 64-bit OS if needed
        if (RegOpenKeyExW(root, subKey, 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hk) != ERROR_SUCCESS)
            return false;
    }

    wchar_t child[256];
    DWORD len;
    while (true) {
        len = 256;
        if (RegEnumKeyExW(hk, 0, child, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
            break;
        std::wstring full = std::wstring(subKey) + L"\\" + child;
        regDeleteRecursive(root, full.c_str());
    }
    RegCloseKey(hk);
    return RegDeleteKeyExW(root, subKey, KEY_WOW64_64KEY, 0) == ERROR_SUCCESS ||
           RegDeleteKeyExW(root, subKey, KEY_WOW64_32KEY, 0) == ERROR_SUCCESS;
}

// ─── Check if a CLSID subkey is an IDM trial key ────────────────────────────
static int classifyClsidKey(HKEY parent, const wchar_t* name)
{
    HKEY hk;
    LONG res = RegOpenKeyExW(parent, name, 0, KEY_READ | KEY_WOW64_64KEY, &hk);
    if (res == ERROR_ACCESS_DENIED)
        return 2;   // locked by us → is IDM key
    if (res != ERROR_SUCCESS)
        return 0;

    static const wchar_t* kBad[] = { L"InProcServer32", L"LocalServer32", L"InProcHandler32", L"TypeLib", nullptr };
    wchar_t sub[256];
    DWORD sl;
    bool hasBad = false;
    for (DWORD i = 0; ; ++i) {
        sl = 256;
        if (RegEnumKeyExW(hk, i, sub, &sl, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        for (int j = 0; kBad[j]; ++j)
            if (_wcsicmp(sub, kBad[j]) == 0) { hasBad = true; break; }
        if (hasBad) break;
    }
    if (hasBad) { RegCloseKey(hk); return 0; }

    DWORD vcnt = 0, skcnt = 0;
    RegQueryInfoKeyW(hk, nullptr, nullptr, nullptr, &skcnt, nullptr, nullptr, &vcnt, nullptr, nullptr, nullptr, nullptr);

    if (vcnt == 0 && skcnt == 0) { RegCloseKey(hk); return 1; }

    char dv[512]{};
    DWORD dvSz = sizeof(dv);
    if (RegQueryValueExA(hk, "", nullptr, nullptr, (LPBYTE)dv, &dvSz) == ERROR_SUCCESS) {
        QString s = QString::fromLocal8Bit(dv);
        bool allDigits = !s.isEmpty();
        for (QChar c : s) if (!c.isDigit()) { allDigits = false; break; }
        if (allDigits || s.contains('+') || s.contains('=') || s.contains('/'))
        { RegCloseKey(hk); return 1; }
    }

    wchar_t vn[256];
    DWORD vnLen;
    static const wchar_t* kIDM[] = { L"MData", L"Model", L"scansk", L"Therad", L"radxcnt", L"ptrk", nullptr };
    for (DWORD i = 0; ; ++i) {
        vnLen = 256;
        if (RegEnumValueW(hk, i, vn, &vnLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        for (int j = 0; kIDM[j]; ++j)
            if (_wcsicmp(vn, kIDM[j]) == 0) { RegCloseKey(hk); return 1; }
    }

    RegCloseKey(hk);
    return 0;
}

// ─── Build regpath string for SetNamedSecurityInfoW ─────────────────────────
static std::wstring toSecPath(HKEY root, const std::wstring& sub)
{
    std::wstring prefix;
    if (root == HKEY_CURRENT_USER) prefix = L"CURRENT_USER\\";
    else if (root == HKEY_LOCAL_MACHINE) prefix = L"MACHINE\\";
    else if (root == HKEY_USERS)        prefix = L"USERS\\";
    else if (root == HKEY_CLASSES_ROOT) prefix = L"CLASSES_ROOT\\";
    return prefix + sub;
}

// ─── Apply Deny-Everyone ACL to a key ────────────────────────────────────────
static bool denyKey(HKEY root, const std::wstring& sub)
{
    PSID evSid = nullptr;
    SID_IDENTIFIER_AUTHORITY world = SECURITY_WORLD_SID_AUTHORITY;
    if (!AllocateAndInitializeSid(&world, 1, SECURITY_WORLD_RID, 0,0,0,0,0,0,0, &evSid))
        return false;

    EXPLICIT_ACCESSW ea{};
    ea.grfAccessPermissions = KEY_ALL_ACCESS;
    ea.grfAccessMode        = DENY_ACCESS;
    ea.grfInheritance       = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.ptstrName    = (LPWSTR)evSid;

    PACL newDacl = nullptr;
    SetEntriesInAclW(1, &ea, nullptr, &newDacl);

    std::wstring path = toSecPath(root, sub);
    DWORD err = SetNamedSecurityInfoW(
        const_cast<LPWSTR>(path.c_str()), SE_REGISTRY_KEY,
        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
        nullptr, nullptr, newDacl, nullptr);

    FreeSid(evSid);
    if (newDacl) LocalFree(newDacl);
    return err == ERROR_SUCCESS;
}

// ─── Remove all ACL from key then delete it (Smart Check) ────────────────────
static bool unlockAndDelete(HKEY root, const std::wstring& sub)
{
    HKEY testHk;
    LONG res = RegOpenKeyExW(root, sub.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &testHk);
    if (res == ERROR_FILE_NOT_FOUND) {
        return true;
    }
    if (res == ERROR_SUCCESS) RegCloseKey(testHk);

    enablePriv(SE_TAKE_OWNERSHIP_NAME);
    enablePriv(SE_RESTORE_NAME);

    PSID adminSid = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0,0,0,0,0,0, &adminSid);

    std::wstring path = toSecPath(root, sub);
    LPWSTR pathPtr = const_cast<LPWSTR>(path.c_str());

    SetNamedSecurityInfoW(pathPtr, SE_REGISTRY_KEY, OWNER_SECURITY_INFORMATION, adminSid, nullptr, nullptr, nullptr);

    PSID evSid = nullptr;
    SID_IDENTIFIER_AUTHORITY world = SECURITY_WORLD_SID_AUTHORITY;
    AllocateAndInitializeSid(&world, 1, SECURITY_WORLD_RID, 0,0,0,0,0,0,0, &evSid);

    EXPLICIT_ACCESSW ea{};
    ea.grfAccessPermissions = KEY_ALL_ACCESS;
    ea.grfAccessMode        = GRANT_ACCESS;
    ea.grfInheritance       = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.ptstrName    = (LPWSTR)evSid;

    PACL dacl = nullptr;
    SetEntriesInAclW(1, &ea, nullptr, &dacl);
    SetNamedSecurityInfoW(pathPtr, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr, dacl, nullptr);

    if (adminSid) FreeSid(adminSid);
    if (evSid)    FreeSid(evSid);
    if (dacl)     LocalFree(dacl);

    return regDeleteRecursive(root, sub.c_str());
}

// ─── Scan + process CLSID paths ──────────────────────────────────────────────
QList<ClsidEntry> IDMWorker::scanClsidEntries()
{
    QList<ClsidEntry> result;
    struct ScanPath { HKEY root; std::wstring sub; };
    QList<ScanPath> paths;

    bool x64 = (m_arch == "x64");
    std::wstring clsidFrag = x64 ? L"Software\\Classes\\Wow6432Node\\CLSID" : L"Software\\Classes\\CLSID";

    paths.append({ HKEY_CURRENT_USER, clsidFrag });

    if (!m_hkcu_sync && !m_sid.isEmpty()) {
        std::wstring sidW = m_sid.toStdWString();
        paths.append({ HKEY_USERS, sidW + L"\\" + clsidFrag });
    }

    static const QRegExp kGUID("\\{[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}\\}");

    for (const auto& sp : paths) {
        HKEY base;
        if (RegOpenKeyExW(sp.root, sp.sub.c_str(), 0, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY, &base) != ERROR_SUCCESS)
            continue;

        wchar_t name[256];
        DWORD nameLen;
        for (DWORD i = 0; ; ++i) {
            nameLen = 256;
            if (RegEnumKeyExW(base, i, name, &nameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            QString n = QString::fromWCharArray(name);
            if (!kGUID.exactMatch(n)) continue;

            std::wstring childSub = sp.sub + L"\\" + std::wstring(name);
            int cls = classifyClsidKey(base, name);
            if (cls > 0)
                result.append({ sp.root, childSub, cls });
        }
        RegCloseKey(base);
    }
    return result;
}

// =============================================================================
IDMWorker::IDMWorker(Operation op, QObject *parent)
    : QObject(parent), m_op(op), m_hkcu_sync(false) {}

void IDMWorker::run()
{
    m_sid     = getSid();
    m_arch    = getArch();
    m_idmPath = getIDMPath();

    if (m_arch == "x86") {
        m_clsid  = "HKCU\\Software\\Classes\\CLSID";
        m_clsid2 = QString("HKU\\%1\\Software\\Classes\\CLSID").arg(m_sid);
        m_hklm   = "HKLM\\Software\\Internet Download Manager";
    } else {
        m_clsid  = "HKCU\\Software\\Classes\\Wow6432Node\\CLSID";
        m_clsid2 = QString("HKU\\%1\\Software\\Classes\\Wow6432Node\\CLSID").arg(m_sid);
        m_hklm   = "HKLM\\SOFTWARE\\Wow6432Node\\Internet Download Manager";
    }
    m_hkcu_sync = isSynced();

    switch (m_op) {
    case Activate:     doActivate(false); break;
    case FreezeTrial:  doActivate(true);  break;
    case Reset:        doReset();         break;
    case CheckStatus:  doCheckStatus();   break;
    case CheckUpdates: doCheckUpdates();  break;
    case Backup:       doBackup();        break;
    }
}


// =============================================================================
// NATIVE DLL INJECTOR (Pure WinAPI)
// =============================================================================

DWORD IDMWorker::findProcessId(const QString& processName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (QString::fromWCharArray(pe32.szExeFile).compare(processName, Qt::CaseInsensitive) == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return 0;
}

bool IDMWorker::injectDll(DWORD processId, const QString& dllPath)
{
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return false;

    // Convert Qt QString path to local 8-bit string for LoadLibraryA
    QByteArray dllPathA = dllPath.toLocal8Bit();
    size_t dllPathLen = dllPathA.size() + 1;

    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, dllPathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pDllPath) { CloseHandle(hProcess); return false; }

    if (!WriteProcessMemory(hProcess, pDllPath, dllPathA.constData(), dllPathLen, NULL)) {
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryA");

    if (!pLoadLibrary) {
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibrary, pDllPath, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Wait for DLL to load
    WaitForSingleObject(hThread, INFINITE);
    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return exitCode != 0; // if exitCode != 0, LoadLibrary was successful
}

// =============================================================================
// ACTIVATE / FREEZE
// =============================================================================
void IDMWorker::doActivate(bool freeze)
{
    emit operationStarted(freeze ? "❄️ Freezing IDM Trial..." : "⚡ Activating IDM...");

    if (!freeze && m_useInjector) {
        LOG_INFO("Operation Mode: Full Activation (Registry + Native DLL Injector)");
    } else if (!freeze) {
        LOG_INFO("Operation Mode: Standard Activation (Registry Only)");
    }

    progress(5, "Checking IDM...");
    if (!isIDMInstalled()) {
        LOG_ERR("IDM is not installed.");
        emit statusDetected(3,{},{});
        emit operationFinished(false, "IDM is not installed.");
        return;
    }
    LOG_OK(QString("📁 IDM Path: %1").arg(m_idmPath));

    progress(10, "Checking internet...");
    if (!hasInternetConnection()) {
        LOG_ERR("No internet connection. Please check your network.");
        emit operationFinished(false, "No internet connection.");
        return;
    }

    progress(15, "Stopping IDM processes...");
    killIDM();

    progress(20, "Backing up Registry Keys...");
    QString ts  = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmssfff");
    QString tmp = getTempDir();
    exportReg(m_clsid, tmp + QString("\\_Bak_CLSID_%1.reg").arg(ts));
    exportReg("HKCU\\Software\\DownloadManager", tmp + QString("\\_Bak_IDM_%1.reg").arg(ts));
    LOG_OK(QString("💾 Backup saved to: %1").arg(tmp));

    progress(28, "Cleaning previous registration data...");
    deleteRegistrationKeys();

    progress(33, "Removing known trial/fake serial CLSID keys...");
    deleteKnownClsidKeys();

    if (!freeze) {
        progress(38, "Writing Activation Serial to Registry...");
        applyRegistration();
    } else {
        progress(38, "Writing Freeze Marker to Registry...");
        regAddString("HKCU\\SOFTWARE\\DownloadManager", "AliSakkaf_Frozen", "1");
    }

    progress(45, "Initializing IDM trial engine (~6 seconds)...");
    LOG_INFO("Starting IDM temporarily to generate fresh CLSID keys...");
    runIDMBriefly(6500);
    killIDM();

    progress(60, "Locking CLSID keys to prevent modification...");
    int locked = lockAllClsidKeys();
    LOG_OK(QString("🔒 Successfully locked %1 CLSID Registry Key(s).").arg(locked));

    if (!freeze) {
        progress(72, "Triggering validation via dummy download...");
        triggerDownloads();
        killIDM(); // Kill IDM after validation
        progress(82, "Re-locking updated CLSID keys...");
        locked += lockAllClsidKeys();

        LOG_INFO("  [Registry] Restoring registration data wiped by IDM validation...");
        applyRegistration();

        // ---------------------------------------------------------------------
        // INJECTOR LOGIC: Extract to IDM path -> Inject
        // ---------------------------------------------------------------------
        if (m_useInjector) {
            progress(88, "Applying Native DLL Memory Patch...");

            killIDM();

            QFileInfo idmFile(m_idmPath);
            QString idmDir = idmFile.absolutePath();
            QString extractDllPath = idmDir + "\\aidm_patch.dll";

            QFile::remove(extractDllPath);

            bool hasDll = false;
            if (QFile::copy(":/aidm.dll", extractDllPath)) {
                LOG_INFO("  [Injector] DLL extracted to IDM directory successfully.");
                hasDll = true;
            } else if (QFile::exists(extractDllPath)) {
                LOG_INFO("  [Injector] DLL already exists in IDM directory. Using it.");
                hasDll = true;
            } else {
                extractDllPath = QCoreApplication::applicationDirPath() + "\\aidm_patch.dll";
                QFile::remove(extractDllPath);
                if (QFile::copy(":/aidm.dll", extractDllPath) || QFile::exists(extractDllPath)) {
                    LOG_INFO("  [Injector] DLL extracted to App directory as fallback.");
                    hasDll = true;
                }
            }

            if (hasDll) {
                STARTUPINFOW si = { sizeof(si) };
                PROCESS_INFORMATION pi;
                std::wstring wExePath = m_idmPath.toStdWString();

                if (CreateProcessW(wExePath.c_str(), nullptr, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
                    LOG_INFO(QString("  [Injector] Process created successfully (PID: %1).").arg(pi.dwProcessId));

                    if (injectDll(pi.dwProcessId, extractDllPath)) {
                        LOG_OK("  ✔ DLL injected successfully into memory.");
                        ResumeThread(pi.hThread);
                        LOG_OK("  ✔ IDM is now running with Fake Serial patch bypassed!");
                    } else {
                        LOG_ERR("  ✖ Injection failed. Terminating process...");
                        TerminateProcess(pi.hProcess, 1);
                    }
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                } else {
                    LOG_ERR("  ✖ Failed to launch IDMan.exe for injection.");
                }
            } else {
                LOG_ERR("  ✖ Failed to extract 'aidm.dll' from resources! Skipping memory patch.");
            }
        }
    }

    progress(90, "Applying Advanced Integration Driver Patches...");
    addAdvIntKey();
    if (!m_useInjector || freeze) killIDM();

    progress(100, "Finalizing Operation...");

    LOG_OK("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    if (freeze) {
        LOG_OK("❄️  IDM Trial Frozen for Lifetime!  🎉");
        emit statusDetected(2,{},{});
        emit operationFinished(true, "IDM trial frozen successfully! ❄️");
    } else {
        if (m_useInjector) LOG_OK("⚡  IDM FULLY Activated (Registry + Memory Patch)!  🎉");
        else LOG_OK("⚡  IDM Activated (Registry Only)!  🎉");

        emit statusDetected(1,{},{});
        emit operationFinished(true, "IDM activated successfully! ✅");
    }
    LOG_OK("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}

// =============================================================================
// RESET
// =============================================================================
void IDMWorker::doReset()
{
    emit operationStarted("🔄 Resetting IDM Configuration...");
    progress(5, "Stopping IDM...");
    killIDM();

    progress(12, "Backing up Registry...");
    QString ts  = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmssfff");
    QString tmp = getTempDir();
    exportReg(m_clsid, tmp + QString("\\_Bak_CLSID_%1.reg").arg(ts));
    if (!m_hkcu_sync)
        exportReg(m_clsid2, tmp + QString("\\_Bak_CLSID2_%1.reg").arg(ts));
    LOG_OK(QString("💾 Backup saved to: %1").arg(tmp));

    progress(30, "Unlocking & deleting IDM CLSID keys...");
    int removed = unlockAllClsidKeys();
    LOG_OK(QString("✔ Removed %1 locked CLSID key(s).").arg(removed));

    progress(60, "Removing known specific IDM CLSID keys...");
    deleteKnownClsidKeys();

    progress(75, "Cleaning up Registration & Trial data...");
    deleteRegistrationKeys();

    progress(88, "Resetting Driver key...");
    addAdvIntKey();
    progress(100, "Reset Complete!");

    LOG_OK("━━━━━━━━━━━━━━━━━");
    LOG_OK("✅  IDM has been fully reset to factory state.");
    LOG_INFO("   Reset by: AliSakkaf Tool v1.2  |  " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    LOG_OK("━━━━━━━━━━━━━━━━━");
    emit statusDetected(0,{},{});
    emit operationFinished(true, "IDM reset completed successfully! 🔄");
}

// =============================================================================
// CHECK STATUS
// =============================================================================
void IDMWorker::doCheckStatus()
{
    emit operationStarted("🔍 Checking IDM Status...");
    progress(10, "Looking for IDM Installation...");

    if (!isIDMInstalled()) {
        LOG_ERR("IDM is not installed on this system.");
        emit statusDetected(3,{},{});
        emit operationFinished(false, "IDM is not installed.");
        return;
    }
    LOG_OK(QString("📁 Found IDM at: %1").arg(m_idmPath));
    progress(35, "Reading Registry Data...");

    HKEY hk;
    int statusCode = 0;
    QString version, detail;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\DownloadManager", 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        auto readStr = [&](const char* n) -> QString {
            char buf[512]{}; DWORD sz = sizeof(buf);
            if (RegQueryValueExA(hk,n,nullptr,nullptr,(LPBYTE)buf,&sz)==ERROR_SUCCESS && sz>1)
                return QString::fromLocal8Bit(buf).trimmed();
            return {};
        };
        auto hasVal = [&](const char* n) -> bool {
            DWORD sz=0;
            return RegQueryValueExA(hk,n,nullptr,nullptr,nullptr,&sz)==ERROR_SUCCESS && sz>0;
        };

        version = readStr("idmvers");
        LOG_INFO(version.isEmpty() ? "⚠ Version unreadable from Registry." : QString("🔢 Installed Version: %1").arg(version));
        progress(65,"Analyzing License Status...");

        QString serial = readStr("Serial");
        bool isFrozen  = hasVal("AliSakkaf_Frozen");
        bool hasTvf    = hasVal("tvfrdt");
        QString lstChk = readStr("LstCheck");
        QString fname  = readStr("FName");
        QString lname  = readStr("LName");

        // 💡 الجزء الذكي: الفحص الإجباري إذا كان الريجستري ممسوح (بعد Reset)
        if (serial.isEmpty() && !isFrozen && !hasTvf) {
            LOG_INFO("  ⚙️ IDM registry is empty. Initializing IDM engine automatically...");
            progress(75, "Starting IDM in background to generate status...");
            runIDMBriefly(4500); // فتح البرنامج بصمت لثواني
            killIDM();           // قفله بالقوة
            hasTvf = hasVal("tvfrdt"); // إعادة قراءة الحالة بعد التشغيل
        }

        LOG(""); LOG_OK("━━━━━━━━━━━━━━━━━");
        if (!serial.isEmpty()) {
            statusCode = 1;
            LOG_OK("✅  Status: IDM is REGISTERED");
            if (!fname.isEmpty()) LOG_INFO(QString("   Registered To: %1 %2").arg(fname,lname));
            LOG_INFO(QString("   Serial Key: %1").arg(serial));
            detail = serial;
        } else if (isFrozen) {
            statusCode = 4;
            LOG_OK("❄️  Status: IDM Trial is FROZEN");
            LOG_INFO("   Trial period is permanently locked by AliSakkaf Tool.");
            detail = "Frozen";
        } else if (hasTvf) {
            statusCode = 2;
            LOG_WARN("⏳  Status: IDM is in TRIAL mode");
            LOG_INFO("   Trial tracking data found in Registry.");
            if (!lstChk.isEmpty()) LOG_INFO(QString("   Last Online Check: %1").arg(lstChk));
            detail = lstChk;
        } else {
            // حالة النظافة التامة (مصنع)
            statusCode = 5;
            LOG_OK("✨  Status: CLEAN / FACTORY RESET");
            LOG_INFO("   IDM is fully clean and ready for a fresh 30-Day Trial.");
            detail = "Clean";
        }
        LOG_OK("━━━━━━━━━━━━━━━━━");
        LOG_INFO("   AliSakkaf IDM Tool v1.2");
        RegCloseKey(hk);
    } else {
        LOG_ERR("Cannot read IDM registry.");
    }

    progress(100,"Check Complete!");
    emit statusDetected(statusCode, version, detail);
    emit operationFinished(true,"Status check complete. ✅");
}

// =============================================================================
// CHECK UPDATES
// =============================================================================
void IDMWorker::doCheckUpdates()
{
    emit operationStarted("🔄 Checking for Tool Updates...");
    progress(10,"Connecting to GitHub...");
    LOG_INFO("Current Version: v1.2 | AliSakkaf Tool");

    // --- 1. SSL & Network Compatibility Check ---
    qDebug() << "\n=== 🔍 SSL & Network Diagnostics ===";
    qDebug() << "Is SSL Supported?:" << (QSslSocket::supportsSsl() ? "Yes ✅" : "No ❌ (Missing DLLs like libcrypto/libssl)");
    qDebug() << "SSL Build Version:" << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << "SSL Runtime Version:" << QSslSocket::sslLibraryVersionString();
    qDebug() << "====================================\n";

    QNetworkAccessManager nam;
    QNetworkRequest req(QUrl("https://raw.githubusercontent.com/alisakkaf/IDM-Freezer-And-Activator/main/version.txt"));
    req.setRawHeader("User-Agent","AliSakkaf-IDMTool/1.2");
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute,true);

    qDebug() << "🌐 Connecting to URL:" << req.url().toString();
    qDebug() << "Connection Scheme:" << req.url().scheme();

    auto *reply = nam.get(req);

    // --- 2. Catch SSL Certificate Errors ---
    connect(reply, &QNetworkReply::sslErrors, [](const QList<QSslError> &errors){
        qDebug() << "⚠️ SSL Certificate Errors Detected:";
        for(const QSslError &error : errors) {
            qDebug() << " -" << error.errorString();
        }
    });

    QEventLoop loop; QTimer tmo;
    tmo.setSingleShot(true); tmo.setInterval(10000);
    connect(&tmo,&QTimer::timeout,&loop,&QEventLoop::quit);
    connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    tmo.start(); loop.exec();

    // --- 3. Server Response Diagnostics ---
    qDebug() << "\n=== 📊 Server Response Details ===";

    if (!tmo.isActive()) {
        qDebug() << "❌ Error: Request timed out. No response within 10 seconds.";
    }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid()) {
        qDebug() << "HTTP Status Code:" << statusCode.toInt();
    }

    QSslConfiguration sslConfig = reply->sslConfiguration();
    qDebug() << "Session Protocol:" << sslConfig.sessionProtocol();
    qDebug() << "Valid Peer Certificate?:" << (!sslConfig.peerCertificate().isNull() ? "Yes ✅" : "No ❌");

    if (reply->error() != QNetworkReply::NoError || !tmo.isActive()) {
        qDebug() << "❌ Error Code:" << reply->error();
        qDebug() << "❌ Error String:" << reply->errorString();
        qDebug() << "====================================\n";

        reply->deleteLater();
        LOG_ERR("Failed to connect to update server.");
        emit operationFinished(false,"Update check failed.");
        return;
    }

    qDebug() << "✅ Connection successful, no errors.";
    qDebug() << "====================================\n";

    QString latest = QString::fromUtf8(reply->readAll()).trimmed();
    reply->deleteLater();
    progress(100,"Check Complete!");

    LOG_OK("━━━━━━━━━━━━━━━━━");
    LOG_INFO(QString("   Current: v1.2  |  Latest Online: v%1").arg(latest.isEmpty()?"?":latest));
    if (latest=="1.2"||latest.isEmpty()) {
        LOG_OK("✅  You are running the latest version!");
        emit operationFinished(true,"Up to date! ✅");
    } else {
        LOG_WARN(QString("🆕  New version v%1 is available!").arg(latest));
        LOG_INFO("   Visit: https://github.com/zinzied/IDM-Freezer");
        emit operationFinished(true,QString("Update v%1 available!").arg(latest));
    }
    LOG_OK("━━━━━━━━━━━━━━━━━");
}


// =============================================================================
// BACKUP
// =============================================================================
void IDMWorker::doBackup()
{
    emit operationStarted("💾 Backing Up IDM Configuration...");
    progress(10,"Preparing Backup Directory...");

    QString dir = QCoreApplication::applicationDirPath() + "\\IDM_Backup";
    QDir().mkpath(dir);
    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

    progress(35,"Exporting HKCU Settings...");
    bool ok = exportReg("HKCU\\Software\\DownloadManager", dir + QString("\\IDM_HKCU_%1.reg").arg(ts));

    progress(65,"Exporting HKLM Settings...");
    exportReg(m_hklm, dir + QString("\\IDM_HKLM_%1.reg").arg(ts));

    progress(100,"Backup Complete!");
    LOG_OK("━━━━━━━━━━━━━━━━━");
    LOG_OK(ok ? "💾  Backup completed successfully!" : "⚠  Backup had minor issues.");
    LOG_INFO(QString("   Saved to → %1").arg(dir));
    LOG_OK("━━━━━━━━━━━━━━━━━");
    emit operationFinished(ok, ok ? "Backup saved! 💾" : "Backup failed.");
}

// =============================================================================
// WINAPI LOCK / UNLOCK (Pure WinAPI)
// =============================================================================

int IDMWorker::lockAllClsidKeys()
{
    enablePriv(SE_TAKE_OWNERSHIP_NAME);
    enablePriv(SE_RESTORE_NAME);

    auto entries = scanClsidEntries();
    int count = 0;
    for (const auto& e : entries) {
        if (denyKey(e.root, e.sub)) {
            LOG_OK("  🔒 [ACL Lock] Secured Key: " + QString::fromStdWString(e.sub));
            ++count;
        } else {
            LOG_WARN("  ⚠ [ACL Lock] Already inaccessible or failed: " + QString::fromStdWString(e.sub));
        }
    }
    return count;
}

int IDMWorker::unlockAllClsidKeys()
{
    enablePriv(SE_TAKE_OWNERSHIP_NAME);
    enablePriv(SE_RESTORE_NAME);

    auto entries = scanClsidEntries();
    int count = 0;
    for (const auto& e : entries) {
        if (unlockAndDelete(e.root, e.sub)) {
            LOG_OK("  ✔ [ACL Unlock & Delete] Removed Key: " + QString::fromStdWString(e.sub));
            ++count;
        } else {
            LOG_WARN("  ⚠ [ACL Unlock] Could not remove Key: " + QString::fromStdWString(e.sub));
        }
    }
    return count;
}

// =============================================================================
// WINAPI ENVIRONMENT HELPERS
// =============================================================================

QString IDMWorker::getSid()
{
    HANDLE hTok = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok)) return {};
    DWORD len = 0;
    GetTokenInformation(hTok, TokenUser, nullptr, 0, &len);
    QByteArray buf(static_cast<int>(len), '\0');
    if (!GetTokenInformation(hTok, TokenUser, buf.data(), len, &len)) {
        CloseHandle(hTok); return {};
    }
    CloseHandle(hTok);
    auto* tu = reinterpret_cast<TOKEN_USER*>(buf.data());
    LPSTR s = nullptr;
    if (!ConvertSidToStringSidA(tu->User.Sid, &s)) return {};
    QString sid = QString::fromLocal8Bit(s);
    LocalFree(s);
    return sid;
}

QString IDMWorker::getArch()
{
    SYSTEM_INFO si; GetNativeSystemInfo(&si);
    return (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) ? "x64" : "x86";
}

QString IDMWorker::getTempDir() const
{
    wchar_t buf[MAX_PATH]{};
    GetTempPathW(MAX_PATH, buf);
    QString p = QString::fromWCharArray(buf);
    while (p.endsWith('\\')) p.chop(1);
    return p;
}

QString IDMWorker::getIDMPath()
{
    // Try via Registry first (WinAPI)
    HKEY hk;
    QString hkuPath = QString("%1\\Software\\DownloadManager").arg(m_sid);
    if (RegOpenKeyExA(HKEY_USERS, hkuPath.toLocal8Bit().constData(), 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        char buf[MAX_PATH]{}; DWORD sz = MAX_PATH;
        if (RegQueryValueExA(hk, "ExePath", nullptr, nullptr, (LPBYTE)buf, &sz) == ERROR_SUCCESS) {
            QString path = QString::fromLocal8Bit(buf).trimmed();
            if (!path.isEmpty() && QFile::exists(path)) {
                RegCloseKey(hk);
                return path;
            }
        }
        RegCloseKey(hk);
    }

    // Fallback to Program Files paths
    wchar_t pf[MAX_PATH]{};
    SHGetSpecialFolderPathW(nullptr, pf, CSIDL_PROGRAM_FILESX86, FALSE);
    QString p86 = QString::fromWCharArray(pf)+"\\Internet Download Manager\\IDMan.exe";
    if (QFile::exists(p86)) return p86;

    SHGetSpecialFolderPathW(nullptr, pf, CSIDL_PROGRAM_FILES, FALSE);
    QString p64 = QString::fromWCharArray(pf)+"\\Internet Download Manager\\IDMan.exe";
    if (QFile::exists(p64)) return p64;
    return {};
}

bool IDMWorker::isSynced()
{
    const char* k = "IDM_ALISAKK_SYNCTEST";
    RegCreateKeyExA(HKEY_CURRENT_USER, k, 0, nullptr, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, nullptr, nullptr, nullptr);
    HKEY hk;
    bool s = RegOpenKeyExA(HKEY_USERS, QString("%1\\%2").arg(m_sid,k).toLocal8Bit(), 0, KEY_READ, &hk) == ERROR_SUCCESS;
    if (s) RegCloseKey(hk);
    RegDeleteKeyA(HKEY_CURRENT_USER, k);
    return s;
}

bool IDMWorker::isIDMInstalled() {
    return !m_idmPath.isEmpty() && QFile::exists(m_idmPath);
}

void IDMWorker::killIDM()
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"IDMan.exe") == 0) {
                    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (h) { TerminateProcess(h, 0); WaitForSingleObject(h, 3000); CloseHandle(h); }
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
    // Backup kill method if handle is overly protected
    QProcess tk;
    tk.start("taskkill", QStringList() << "/f" << "/im" << "idman.exe" << "/t");
    tk.waitForFinished(3000);
    QThread::msleep(600);
}

void IDMWorker::runIDMBriefly(int msec)
{
    if (m_idmPath.isEmpty()) return;
    QProcess::startDetached(m_idmPath, QStringList() << "/n");
    int e = 0;
    while (e < msec) { QThread::msleep(500); e += 500; }
}

bool IDMWorker::hasInternetConnection()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) return false;
    bool ok = false;
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo("internetdownloadmanager.com","80",&hints,&res) == 0) {
        SOCKET s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s != INVALID_SOCKET) {
            DWORD tv = 5000;
            setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&tv,sizeof(tv));
            ok = (::connect(s,res->ai_addr,(int)res->ai_addrlen) == 0);
            closesocket(s);
        }
        freeaddrinfo(res);
    }
    WSACleanup();
    return ok;
}

// =============================================================================
// REGISTRY HELPERS (PURE WINAPI IMPLEMENTATION)
// =============================================================================

bool IDMWorker::parseKeyPath(const QString &fullPath, HKEY &rootKey, QString &subPath)
{
    if      (fullPath.startsWith("HKLM\\")) { rootKey = HKEY_LOCAL_MACHINE; subPath = fullPath.mid(5); }
    else if (fullPath.startsWith("HKCU\\")) { rootKey = HKEY_CURRENT_USER;  subPath = fullPath.mid(5); }
    else if (fullPath.startsWith("HKU\\"))  { rootKey = HKEY_USERS;         subPath = fullPath.mid(4); }
    else return false;
    return true;
}

bool IDMWorker::regAdd(const QString& key, const QString& valueName, DWORD type, const QByteArray& data)
{
    HKEY root; QString sub;
    if (!parseKeyPath(key, root, sub)) return false;

    HKEY hk;
    DWORD accessFlags = KEY_ALL_ACCESS | KEY_WOW64_32KEY;

    if (RegCreateKeyExW(root, sub.toStdWString().c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, accessFlags, nullptr, &hk, nullptr) == ERROR_SUCCESS) {
        bool ok = (RegSetValueExW(hk, valueName.toStdWString().c_str(), 0, type, (const BYTE*)data.constData(), data.size()) == ERROR_SUCCESS);
        RegCloseKey(hk);
        if (ok) LOG_INFO(QString("  [Registry] Created/Set Value: %1 \\ %2").arg(key, valueName));
        else LOG_WARN(QString("  [Registry] Failed to Set Value: %1 \\ %2").arg(key, valueName));
        return ok;
    }
    LOG_WARN(QString("  [Registry] Failed to Open/Create Key: %1").arg(key));
    return false;
}

bool IDMWorker::regAddString(const QString &key, const QString &valueName, const QString &strValue)
{
    std::wstring wstr = strValue.toStdWString();
    QByteArray data((const char*)wstr.c_str(), (wstr.length() + 1) * sizeof(wchar_t));
    return regAdd(key, valueName, REG_SZ, data);
}

bool IDMWorker::regAddDword(const QString &key, const QString &valueName, DWORD dwValue)
{
    QByteArray data((const char*)&dwValue, sizeof(DWORD));
    return regAdd(key, valueName, REG_DWORD, data);
}

bool IDMWorker::regDelete(const QString& key, const QString& valueName)
{
    HKEY root; QString sub;
    if (!parseKeyPath(key, root, sub)) return false;

    if (valueName.isEmpty()) {
        bool ok = regDeleteRecursive(root, sub.toStdWString().c_str());
        if (ok) LOG_INFO(QString("  [Registry] Deleted Entire Key: %1").arg(key));
        return ok;
    } else {
        HKEY hk;
        if (RegOpenKeyExW(root, sub.toStdWString().c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &hk) == ERROR_SUCCESS) {
            bool ok = (RegDeleteValueW(hk, valueName.toStdWString().c_str()) == ERROR_SUCCESS);
            RegCloseKey(hk);
            if (ok) LOG_INFO(QString("  [Registry] Deleted Value: %1 \\ %2").arg(key, valueName));
            return ok;
        }
    }
    return false;
}

bool IDMWorker::exportReg(const QString& key, const QString& file)
{
    // Keeping QProcess ONLY for REG EXPORT because creating properly formatted
    // .reg files natively in C++ WinAPI is exceptionally complex and error-prone.
    QProcess p;
    p.start("reg", QStringList() << "export" << key << file << "/y");
    p.waitForFinished(10000);
    return p.exitCode() == 0;
}

void IDMWorker::deleteRegistrationKeys()
{
    static const char* kV[] = {
        "FName", "LName", "Email", "Serial", "scansk",
        "tvfrdt", "radxcnt", "LstCheck", "ptrk_scdt", "LastCheckQU",
        "AliSakkaf_Frozen", nullptr   // <--- أضفنا هذا هنا
    };

    const QString hkcu = "HKCU\\Software\\DownloadManager";
    for (int i=0; kV[i]; ++i) regDelete(hkcu, kV[i]);

    regDelete(m_hklm); // Deletes the entire HKLM key

    if (!m_hkcu_sync && !m_sid.isEmpty()) {
        QString hku = QString("HKU\\%1\\Software\\DownloadManager").arg(m_sid);
        for (int i=0; kV[i]; ++i) regDelete(hku, kV[i]);
    }
}

void IDMWorker::deleteKnownClsidKeys()
{
    static const char* kG[] = {
        "{6DDF00DB-1234-46EC-8356-27E7B2051192}",
        "{7B8E9164-324D-4A2E-A46D-0165FB2000EC}",
        "{D5B91409-A8CA-4973-9A0B-59F713D25671}",
        "{5ED60779-4DE2-4E07-B862-974CA4FF2E9C}",
        "{07999AC3-058B-40BF-984F-69EB1E554CA7}",
        nullptr
    };
    QString suffix = (m_arch=="x64") ? "Wow6432Node\\CLSID\\" : "CLSID\\";
    static const char* kRoots[] = {
        "HKCU\\Software\\Classes\\",
        "HKLM\\SOFTWARE\\Classes\\", nullptr
    };
    for (int r=0; kRoots[r]; ++r)
        for (int i=0; kG[i]; ++i)
            regDelete(kRoots[r] + suffix + kG[i]);

    if (!m_sid.isEmpty()) {
        QString huBase = "HKU\\" + m_sid + "\\Software\\Classes\\" + suffix;
        for (int i=0; kG[i]; ++i) regDelete(huBase + kG[i]);
    }
}

void IDMWorker::addAdvIntKey()
{
    if (regAddDword(m_hklm, "AdvIntDriverEnabled2", 1)) {
        LOG_OK("  ✔ Driver Patched: AdvIntDriverEnabled2 = 1");
    }
}

void IDMWorker::applyRegistration()
{
    quint32 seed = (quint32)(QDate::currentDate().toJulianDay() ^ 0xA115ACCA);
    QRandomGenerator rng(seed);
    const QString ch = "ABCDEFGHJKLMNPQRSTUVWXYZ2345678";
    auto part = [&]{ QString s; for(int i=0;i<5;++i) s+=ch[rng.bounded(ch.size())]; return s; };
    QString serial = part()+"-"+part()+"-"+part()+"-"+part();

    LOG_INFO("  Registration Name:  Ali SakkaF");
    LOG_INFO("  Registration Email: alisakkaf@tool.dev");
    LOG_INFO(QString("  Generated Serial:   %1").arg(serial));

    const QString hkcu = "HKCU\\SOFTWARE\\DownloadManager";
    regAddString(hkcu, "FName", "Ali");
    regAddString(hkcu, "LName", "SakkaF");
    regAddString(hkcu, "Email", "alisakkaf@tool.dev");
    regAddString(hkcu, "Serial", serial);

    if (!m_hkcu_sync && !m_sid.isEmpty()) {
        QString hku = "HKU\\"+m_sid+"\\SOFTWARE\\DownloadManager";
        regAddString(hku, "FName", "Ali");
        regAddString(hku, "LName", "SakkaF");
        regAddString(hku, "Email", "alisakkaf@tool.dev");
        regAddString(hku, "Serial", serial);
    }
    LOG_OK("  ✔ Registration details applied securely via WinAPI.");
}

bool IDMWorker::triggerDownloads()
{
    if (m_idmPath.isEmpty()) return false;
    LOG_INFO("  [Trigger] Forcing IDM to validate serial with server...");

    QString tmp = getTempDir();
    QString out = tmp + "\\idm_tool_trg.png";
    QFile::remove(out);

    QString url = "https://www.internetdownloadmanager.com/images/idm_box_min.png";

    QStringList args;
    args << "/n" << "/q" << "/d" << url << "/p" << tmp << "/f" << "idm_tool_trg.png";
    QProcess::startDetached(m_idmPath, args);

    LOG_INFO("  ℹ Waiting 6 seconds for server response & registry writing...");

    for (int t = 0; t < 10; ++t) {
        QThread::msleep(1000);
    }

    LOG_OK("  ✔ Validation provoked successfully. Ready to lock.");
    QFile::remove(out);
    return true;
}

// Emit helpers
void IDMWorker::log(const QString& m, int t) { emit logMessage(m,t); }
void IDMWorker::progress(int v, const QString& l) { emit progressChanged(v,l); }
