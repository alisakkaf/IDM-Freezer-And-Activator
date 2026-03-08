#include "logwidget.h"
#include <QScrollBar>
#include <QFont>

LogWidget::LogWidget(QWidget *parent) : QTextEdit(parent), m_isLightMode(false)
{
    setReadOnly(true);
    setObjectName("logView");
    setUndoRedoEnabled(false);

    // 1. تعيين خط عصري، عريض، وأنيق بدلاً من Consolas
    QFont logFont("Segoe UI", 10, QFont::DemiBold);
    logFont.setStyleHint(QFont::SansSerif); // إجبار النظام على استخدام خط عادي وليس Monospace
    setFont(logFont);
}

void LogWidget::setLightMode(bool isLight)
{
    m_isLightMode = isLight;
}

void LogWidget::appendLog(const QString &message, int level)
{
    // 2. ألوان عصرية وهادئة تتغير تلقائياً حسب الثيم (مظلم/مضيء)
    QString colorText    = m_isLightMode ? "#334155" : "#F8FAFC"; // رمادي غامق / أبيض مائل للرمادي
    QString colorPrimary = m_isLightMode ? "#0284C7" : "#38BDF8"; // أزرق سماوي
    QString colorWarn    = m_isLightMode ? "#D97706" : "#FCD34D"; // أصفر مائل للبرتقالي
    QString colorDanger  = m_isLightMode ? "#DC2626" : "#F87171"; // أحمر
    QString colorSuccess = m_isLightMode ? "#059669" : "#34D399"; // أخضر

    // معالجة الرسائل الخاصة (مثل البداية والنهاية)
    if (level == Started) {
        append(QString("<span style='color:%1; font-weight:bold;'>▶ %2</span>").arg(colorPrimary, message));
        scrollToBottom();
        return;
    } else if (level == FinishedSuccess) {
        append(QString("<br><span style='color:%1; font-weight:bold; font-size:13px;'>"
                       "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━<br>"
                       "✅  %2<br>"
                       "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━</span><br>").arg(colorSuccess, message));
        scrollToBottom();
        return;
    } else if (level == FinishedError) {
        append(QString("<br><span style='color:%1; font-weight:bold; font-size:13px;'>"
                       "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━<br>"
                       "❌  %2<br>"
                       "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━</span><br>").arg(colorDanger, message));
        scrollToBottom();
        return;
    }

    // معالجة الرسائل العادية
    QString color, prefix;
    switch (level) {
    case Normal:  color = colorText;    prefix = "  "; break;
    case Info:    color = colorPrimary; prefix = "ℹ "; break;
    case Warning: color = colorWarn;    prefix = "⚠ "; break;
    case Error:   color = colorDanger;  prefix = "✖ "; break;
    case Success: color = colorSuccess; prefix = "✔ "; break;
    default:      color = colorText;    prefix = "  "; break;
    }

    // 3. استخدام append بدلاً من setHtml للحفاظ على السطور السابقة
    append(QString("<span style='color:%1;'>%2%3</span>").arg(color, prefix, message));
    scrollToBottom();
}

void LogWidget::clearLog()
{
    clear();
}

void LogWidget::scrollToBottom()
{
    QScrollBar *vBar = verticalScrollBar();
    vBar->setValue(vBar->maximum());
}
