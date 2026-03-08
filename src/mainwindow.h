#pragma once

#include <QMainWindow>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <QThread>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QStackedWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QMovie>
#include <QPointer>
#include "idmworker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct LogEntry {
    QString message;
    int type; // 0: normal, 1: info, 2: warn, 3: err, 4: success, 5: started, 6: finished_success, 7: finished_error
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // Navigation (Sidebar)
    void onActivateClicked();
    void onFreezeClicked();
    void onResetClicked();
    void onCheckStatusClicked();
    void onCheckUpdatesClicked();
    void onDownloadIDMClicked();
    void onAboutClicked();

    // Theme
    void onThemeClicked();

    // Worker signals
    void onOperationStarted(const QString &message);
    void onLogMessage(const QString &message, int type);
    void onOperationFinished(bool success, const QString &message);
    void onProgressChanged(int value, const QString &label);
    void onStatusDetected(int statusCode, const QString &version, const QString &detail);

    // UI animations
    void animatePulse();
    void updateStatusBadge(); // Native quick check

private:
    void setupUI();
    void setupConnections();
    void setupAnimations();
    void applyTheme();
    void renderLogs();

    // Operation Execution
    void runOperation(IDMWorker::Operation op, bool silent = false, bool useInjector = false);
    void setButtonsEnabled(bool enabled);
    void showPage(int index);

    Ui::MainWindow *ui;

    // Worker
    QPointer<IDMWorker> m_worker;
    QPointer<QThread> m_workerThread;

    // UI State
    bool m_dragging;
    QPoint m_dragPos;
    int m_currentPage;
    bool m_isLightMode;
    bool m_isSilentOp; // true when checking status in background to prevent log wiping
    QList<LogEntry> m_logHistory;

    // Animation
    QTimer *m_pulseTimer;
    QTimer *m_statusTimer;

    // Status
    QString m_idmStatus;
    QString m_idmVersion;
    QString m_idmPath;
    IDMWorker::Operation m_lastOp = IDMWorker::CheckStatus;
};
