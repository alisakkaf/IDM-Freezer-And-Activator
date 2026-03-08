#include "registrymanager.h"
#include <QProcess>
#include <QFile>

RegistryManager::RegistryManager(QObject *parent) : QObject(parent) {}

HKEY RegistryManager::parseRootKey(const QString &keyPath, QString &subKey)
{
    HKEY root = HKEY_CURRENT_USER;
    if (keyPath.startsWith("HKLM\\") || keyPath.startsWith("HKEY_LOCAL_MACHINE\\")) {
        root = HKEY_LOCAL_MACHINE;
        subKey = keyPath.mid(keyPath.indexOf('\\') + 1);
    } else if (keyPath.startsWith("HKCU\\") || keyPath.startsWith("HKEY_CURRENT_USER\\")) {
        root = HKEY_CURRENT_USER;
        subKey = keyPath.mid(keyPath.indexOf('\\') + 1);
    } else if (keyPath.startsWith("HKU\\") || keyPath.startsWith("HKEY_USERS\\")) {
        root = HKEY_USERS;
        subKey = keyPath.mid(keyPath.indexOf('\\') + 1);
    } else {
        subKey = keyPath;
    }
    return root;
}

bool RegistryManager::setValue(const QString &keyPath, const QString &valueName,
                               DWORD type, const QByteArray &data)
{
    QString subKey;
    HKEY root = parseRootKey(keyPath, subKey);
    HKEY hKey;
    if (RegCreateKeyExA(root, subKey.toLocal8Bit().constData(),
                        0, nullptr, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return false;
    bool ok = RegSetValueExA(hKey, valueName.toLocal8Bit().constData(),
                             0, type, (LPBYTE)data.constData(), data.size()) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return ok;
}

bool RegistryManager::deleteValue(const QString &keyPath, const QString &valueName)
{
    QProcess p;
    p.start("reg", QStringList() << "delete" << keyPath << "/v" << valueName << "/f");
    return p.waitForFinished(5000) && p.exitCode() == 0;
}

bool RegistryManager::deleteKey(const QString &keyPath)
{
    QProcess p;
    p.start("reg", QStringList() << "delete" << keyPath << "/f");
    return p.waitForFinished(5000) && p.exitCode() == 0;
}

QString RegistryManager::readString(const QString &keyPath, const QString &valueName)
{
    QString subKey;
    HKEY root = parseRootKey(keyPath, subKey);
    HKEY hKey;
    if (RegOpenKeyExA(root, subKey.toLocal8Bit().constData(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return {};
    char buf[2048] = {0};
    DWORD sz = sizeof(buf);
    RegQueryValueExA(hKey, valueName.toLocal8Bit().constData(), nullptr, nullptr, (LPBYTE)buf, &sz);
    RegCloseKey(hKey);
    return QString::fromLocal8Bit(buf);
}

bool RegistryManager::keyExists(const QString &keyPath)
{
    QString subKey;
    HKEY root = parseRootKey(keyPath, subKey);
    HKEY hKey;
    bool exists = (RegOpenKeyExA(root, subKey.toLocal8Bit().constData(), 0, KEY_READ, &hKey) == ERROR_SUCCESS);
    if (exists) RegCloseKey(hKey);
    return exists;
}

bool RegistryManager::exportKey(const QString &keyPath, const QString &filePath)
{
    QProcess p;
    p.start("reg", QStringList() << "export" << keyPath << filePath << "/y");
    return p.waitForFinished(10000) && p.exitCode() == 0;
}
