#pragma once

#include <QTextEdit>
#include <QString>

class LogWidget : public QTextEdit
{
    Q_OBJECT

public:
    // تعريف مستويات اللوج لتشمل البداية والنهاية بخطوط فاصلة
    enum LogLevel {
        Normal = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Success = 4,
        Started = 5,
        FinishedSuccess = 6,
        FinishedError = 7
    };

    explicit LogWidget(QWidget *parent = nullptr);

    // دالة لاستقبال الثيم من النافذة الرئيسية لتعديل الألوان
    void setLightMode(bool isLight);

public slots:
    void appendLog(const QString &message, int level = Normal);
    void clearLog();

private:
    void scrollToBottom();
    bool m_isLightMode;
};
