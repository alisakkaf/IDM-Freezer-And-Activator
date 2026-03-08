// =============================================================================
//   IDM Tool  –  Worker Header  |  Copyright © 2026 AliSakkaf
//   Pure WinAPI Edition - Zero CMD/PowerShell Dependency
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QByteArray>

// Winsock2 MUST precede windows.h
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winreg.h>
#include <aclapi.h>
#include <accctrl.h>

// Internal struct for CLSID scan results
struct ClsidEntry {
    HKEY         root;
    std::wstring sub;   // path below root (e.g. "Software\\Classes\\WOW...\\CLSID\\{GUID}")
    int          type;  // 1=open/IDM  2=locked(access-denied)
};

class IDMWorker : public QObject
{
    Q_OBJECT

public:
    enum Operation {
        Activate,
        FreezeTrial,
        Reset,
        CheckStatus,
        CheckUpdates,
        Backup
    };

    explicit IDMWorker(Operation op, QObject *parent = nullptr);
    ~IDMWorker() = default;

public slots:
    void run();
    // ── DLL Injection Helpers (Native Memory Patch) ──────────────────────────
    DWORD findProcessId(const QString &processName);
    bool injectDll(DWORD processId, const QString &dllPath);
    void setUseInjector(bool use) { m_useInjector = use; }

signals:
    void operationStarted(const QString &message);
    void logMessage(const QString &message, int type);
    void progressChanged(int value, const QString &label);
    void operationFinished(bool success, const QString &message);
    // 0=unknown  1=registered  2=trial/frozen  3=not installed
    void statusDetected(int statusCode, const QString &version, const QString &detail);

private:
    // ── Operations ───────────────────────────────────────────────────────────
    void doActivate(bool freeze);
    void doReset();
    void doCheckStatus();
    void doCheckUpdates();
    void doBackup();
    bool m_useInjector = false; // المتغير الجديد

    // ── WinAPI environment helpers ────────────────────────────────────────────
    QString getSid();
    QString getArch();
    QString getIDMPath();
    QString getTempDir() const;
    bool    isSynced();
    bool    isIDMInstalled();
    bool    hasInternetConnection();

    // ── IDM control ──────────────────────────────────────────────────────────
    void killIDM();
    void runIDMBriefly(int msec);
    bool triggerDownloads();

    // ── Registry helpers (Pure WinAPI) ───────────────────────────────────────
    bool parseKeyPath(const QString &fullPath, HKEY &rootKey, QString &subPath);
    bool regAdd(const QString &key, const QString &valueName, DWORD type, const QByteArray &data);
    bool regAddString(const QString &key, const QString &valueName, const QString &strValue);
    bool regAddDword(const QString &key, const QString &valueName, DWORD dwValue);
    bool regDelete(const QString &key, const QString &valueName = {});
    bool exportReg(const QString &key, const QString &filePath);

    // ── CLSID operations (Pure WinAPI) ───────────────────────────────────────
    QList<ClsidEntry> scanClsidEntries();               // enumerate + classify IDM CLSID keys
    int  lockAllClsidKeys();                            // apply Deny-Everyone ACL → freezes trial
    int  unlockAllClsidKeys();                          // take ownership + grant + recursive delete

    void deleteRegistrationKeys();
    void deleteKnownClsidKeys();
    void addAdvIntKey();
    void applyRegistration();

    // ── Emit helpers ─────────────────────────────────────────────────────────
    void log(const QString &msg, int type = 0);
    void progress(int val, const QString &label);

    // ── State ────────────────────────────────────────────────────────────────
    Operation m_op;
    QString   m_sid;
    QString   m_arch;
    QString   m_idmPath;
    QString   m_clsid;
    QString   m_clsid2;
    QString   m_hklm;
    bool      m_hkcu_sync;
};
