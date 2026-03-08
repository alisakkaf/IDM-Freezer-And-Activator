#pragma once
#include <QObject>
#include <QString>
#include <windows.h>
#include <winreg.h>

// Helper class for Windows registry operations
class RegistryManager : public QObject
{
    Q_OBJECT
public:
    explicit RegistryManager(QObject *parent = nullptr);

    static bool setValue(const QString &keyPath, const QString &valueName,
                         DWORD type, const QByteArray &data);
    static bool deleteValue(const QString &keyPath, const QString &valueName);
    static bool deleteKey(const QString &keyPath);
    static QString readString(const QString &keyPath, const QString &valueName);
    static bool keyExists(const QString &keyPath);
    static bool exportKey(const QString &keyPath, const QString &filePath);

private:
    static HKEY parseRootKey(const QString &keyPath, QString &subKey);
};
