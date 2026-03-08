/*
 =============================================================================
    IDM Activation Tool  –  Main Window
    Copyright © 2026  AliSakkaf  |  All Rights Reserved
    Version : 1.2  |  Built: 2026-03-08
 -----------------------------------------------------------------------------
    Website  : https://mysterious-dev.com/
    GitHub   : https://github.com/alisakkaf
    Facebook : https://www.facebook.com/AliSakkaf.Dev/
 =============================================================================
*/

#include "mainwindow.h"
#include "src/aboutdialog.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QApplication>
#include <QScreen>
#include <QBitmap>
#include <QFont>
#include <windows.h>
#include <shellapi.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_dragging(false)
    , m_currentPage(0)
    , m_isLightMode(false)
    , m_isSilentOp(false)
    , m_worker(nullptr)
    , m_workerThread(nullptr)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);

    applyTheme();
    setupConnections();
    setupAnimations();

    // Center on screen
    QScreen *screen = QApplication::primaryScreen();
    QRect sg = screen->availableGeometry();
    move(sg.center() - rect().center());

    // Initial status check
    QTimer::singleShot(200, this, &MainWindow::updateStatusBadge);

    // Start on Activate Page by default
    onActivateClicked();
}

MainWindow::~MainWindow()
{
    if (!m_workerThread.isNull()) {
        m_workerThread->quit();
        m_workerThread->wait(3000);
    }
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_isLightMode) {
        // Dark Mode Background gradient
        QLinearGradient bg(0, 0, width(), height());
        bg.setColorAt(0.0, QColor(24, 24, 27));
        bg.setColorAt(0.5, QColor(18, 18, 20));
        bg.setColorAt(1.0, QColor(9, 9, 11));
        painter.fillRect(rect(), bg);

        QRadialGradient orb1(width() * 0.2, height() * 0.2, 200);
        orb1.setColorAt(0.0, QColor(14, 165, 233, 35));
        orb1.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(rect(), orb1);

        QRadialGradient orb2(width() * 0.8, height() * 0.8, 220);
        orb2.setColorAt(0.0, QColor(20, 184, 166, 30));
        orb2.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(rect(), orb2);

        painter.setPen(QPen(QColor(255, 255, 255, 6), 1));
    } else {
        // Light Mode Background gradient
        QLinearGradient bg(0, 0, width(), height());
        bg.setColorAt(0.0, QColor(248, 250, 252));
        bg.setColorAt(1.0, QColor(241, 245, 249));
        painter.fillRect(rect(), bg);

        QRadialGradient orb1(width() * 0.2, height() * 0.2, 300);
        orb1.setColorAt(0.0, QColor(56, 189, 248, 15));
        orb1.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(rect(), orb1);

        QRadialGradient orb2(width() * 0.8, height() * 0.8, 300);
        orb2.setColorAt(0.0, QColor(45, 212, 191, 12));
        orb2.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(rect(), orb2);

        painter.setPen(QPen(QColor(0, 0, 0, 8), 1));
    }

    int step = 40;
    for (int x = 0; x < width(); x += step) painter.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += step) painter.drawLine(0, y, width(), y);

    QPainterPath path;
    path.addRoundedRect(rect(), 16, 16);
    painter.setClipPath(path);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->pos().y() < 60) {
        m_dragging = true;
        m_dragPos = event->globalPos() - frameGeometry().topLeft();
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton))
        move(event->globalPos() - m_dragPos);
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    QBitmap mask(size());
    mask.fill(Qt::color0);
    QPainter p(&mask);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::color1);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 16, 16);
    setMask(mask);
}

void MainWindow::applyTheme()
{
    if (m_isLightMode) {
        ui->btnTheme->setText("🌙");
        setStyleSheet(R"(
            QWidget { background: transparent; color: #1E293B; font-family: 'Segoe UI', sans-serif; font-size: 13px; font-weight: 600; }
            #titleBar { background: rgba(248, 250, 252, 0.85); border-bottom: 1px solid rgba(0,0,0,0.07); }
            #lblLogo { font-size: 22px; font-weight: 700; }
            #lblAppTitle { font-size: 14px; font-weight: 700; color: #0F172A; }
            #lblVersion { font-size: 10px; font-weight: 500; color: #64748B; }
            #lblStatusBadge { background: rgba(14,165,233,0.1); border: 1px solid rgba(14,165,233,0.25); border-radius: 14px; padding: 4px 14px; font-size: 11px; font-weight: 700; color: #0284C7; }
            #btnClose { background: rgba(239,68,68,0.1); border: 1px solid rgba(239,68,68,0.2); border-radius: 12px; color: #DC2626; font-size: 14px; font-weight: bold; min-width:26px; max-width:26px; min-height:26px; max-height:26px; }
            #btnClose:hover { background: #EF4444; color: white; border-color: #EF4444; }
            #btnTheme, #btnMinimize { background: rgba(0,0,0,0.06); border: 1px solid rgba(0,0,0,0.1); border-radius: 12px; font-size: 14px; min-width:26px; max-width:26px; min-height:26px; max-height:26px; }
            #btnTheme:hover, #btnMinimize:hover { background: rgba(0,0,0,0.12); }
            #btnMinimize { color: #475569; font-weight: bold; font-size: 18px; }
            #btnMinimize:hover { color: #0F172A; }
            #sidebar { background: rgba(241,245,249,0.9); border-right: 1px solid rgba(0,0,0,0.06); }
            #lblSection1, #lblSection2, #lblSection3 { color: #94A3B8; font-size: 10px; font-weight: 700; padding: 2px 10px 4px 10px; }
            QPushButton#btnNavActivate, QPushButton#btnNavFreeze, QPushButton#btnNavReset, QPushButton#btnNavStatus, QPushButton#btnNavUpdates, QPushButton#btnNavDownload, QPushButton#btnNavAbout { background: transparent; border: none; border-radius: 8px; text-align: left; padding: 9px 12px; font-size: 12px; font-weight: 600; color: #475569; }
            QPushButton#btnNavActivate:hover, QPushButton#btnNavFreeze:hover, QPushButton#btnNavReset:hover, QPushButton#btnNavStatus:hover, QPushButton#btnNavUpdates:hover, QPushButton#btnNavDownload:hover, QPushButton#btnNavAbout:hover { background: rgba(14,165,233,0.08); color: #0284C7; }
            QPushButton#btnNavActivate:disabled, QPushButton#btnNavFreeze:disabled, QPushButton#btnNavReset:disabled, QPushButton#btnNavStatus:disabled, QPushButton#btnNavUpdates:disabled { color: #CBD5E1; }
            QFrame#sep1, QFrame#sep2 { color: rgba(0,0,0,0.06); margin: 4px 4px; }
            #lblSideFooter { color: #CBD5E1; font-size: 10px; font-weight: 600; padding: 6px; }
            #contentArea { background: rgba(255,255,255,0.6); border-radius: 16px; margin: 8px 8px 8px 4px; border: 1px solid rgba(255,255,255,0.9); }
            #lblPageTitle { font-size: 17px; font-weight: 700; color: #0F172A; }
            #lblPageSubtitle { font-size: 11px; font-weight: 500; color: #64748B; }
            #lblStatusLabel { font-size: 10px; font-weight: 600; color: #94A3B8; }
            #lblStatusValue { font-size: 13px; font-weight: 700; color: #0284C7; }
            QFrame#hSep { color: rgba(0,0,0,0.06); }
            #lblActTitle, #lblFrzTitle, #lblResTitle { font-size: 16px; font-weight: 700; color: #0F172A; }
            #lblActDesc, #lblFrzDesc, #lblResDesc { font-size: 12px; font-weight: 500; color: #64748B; }
            QFrame#cardActivate, QFrame#cardFreeze, QFrame#cardReset { background: rgba(255,255,255,0.9); border: 1px solid rgba(0,0,0,0.07); border-radius: 12px; }
            #lblNoteWarn { background: rgba(245,158,11,0.08); border: 1px solid rgba(245,158,11,0.25); border-radius: 8px; padding: 10px 12px; font-size: 12px; font-weight: 600; color: #B45309; }
            #lblNoteSuccess { background: rgba(16,185,129,0.08); border: 1px solid rgba(16,185,129,0.25); border-radius: 8px; padding: 10px 12px; font-size: 12px; font-weight: 600; color: #047857; }
            #lblNoteDanger { background: rgba(239,68,68,0.07); border: 1px solid rgba(239,68,68,0.2); border-radius: 8px; padding: 10px 12px; font-size: 12px; font-weight: 600; color: #B91C1C; }
            #btnDoActivate { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0284C7, stop:1 #38BDF8); border: 1px solid #0EA5E9; border-radius: 10px; color: white; font-size: 13px; font-weight: 700; padding: 0 26px; }
            #btnDoActivate:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0369A1, stop:1 #0284C7); }
            #btnDoActivate:disabled { background: rgba(0,0,0,0.06); color: #94A3B8; border-color: rgba(0,0,0,0.08); }
            #btnDoFreeze { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0F766E, stop:1 #2DD4BF); border: 1px solid #14B8A6; border-radius: 10px; color: white; font-size: 13px; font-weight: 700; padding: 0 26px; }
            #btnDoFreeze:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #115E59, stop:1 #0F766E); }
            #btnDoFreeze:disabled { background: rgba(0,0,0,0.06); color: #94A3B8; border-color: rgba(0,0,0,0.08); }
            #btnDoReset { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #B91C1C, stop:1 #F87171); border: 1px solid #EF4444; border-radius: 10px; color: white; font-size: 13px; font-weight: 700; padding: 0 26px; }
            #btnDoReset:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #991B1B, stop:1 #B91C1C); }
            #btnDoReset:disabled { background: rgba(0,0,0,0.06); color: #94A3B8; border-color: rgba(0,0,0,0.08); }
            QProgressBar { background: rgba(0,0,0,0.05); border: none; border-radius: 3px; height: 6px; }
            QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #0284C7, stop:1 #38BDF8); border-radius: 3px; }
            #lblProgressLabel { font-size: 11px; font-weight: 600; color: #475569; }
            #logView { background: #F8FAFC; border: 1px solid rgba(0,0,0,0.08); border-radius: 10px; color: #1E293B; font-family: 'Consolas', monospace; font-size: 11px; font-weight: 500; padding: 10px; }
            QScrollBar:vertical { background: transparent; width: 5px; }
            QScrollBar::handle:vertical { background: rgba(0,0,0,0.15); border-radius: 2px; min-height: 24px; }
            QScrollBar::handle:vertical:hover { background: rgba(0,0,0,0.25); }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        )");
    } else {
        ui->btnTheme->setText("☀️");
        setStyleSheet(R"(
            QWidget { background: transparent; color: #F1F5F9; font-family: 'Segoe UI', sans-serif; font-size: 13px; font-weight: 600; }
            #titleBar { background: rgba(15, 23, 42, 0.6); border-bottom: 1px solid rgba(255,255,255,0.05); }
            #lblLogo { font-size: 22px; font-weight: 700; }
            #lblAppTitle { font-size: 14px; font-weight: 700; color: #F8FAFC; }
            #lblVersion { font-size: 10px; font-weight: 500; color: #64748B; }
            #lblStatusBadge { background: rgba(14,165,233,0.12); border: 1px solid rgba(14,165,233,0.25); border-radius: 14px; padding: 4px 14px; font-size: 11px; font-weight: 700; color: #38BDF8; }
            #btnClose { background: rgba(239,68,68,0.1); border: 1px solid rgba(239,68,68,0.2); border-radius: 12px; color: #FCA5A5; font-size: 14px; font-weight: bold; min-width:26px; max-width:26px; min-height:26px; max-height:26px; }
            #btnClose:hover { background: #EF4444; color: white; border-color: #EF4444; }
            #btnTheme, #btnMinimize { background: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.1); border-radius: 12px; font-size: 14px; min-width:26px; max-width:26px; min-height:26px; max-height:26px; }
            #btnTheme:hover, #btnMinimize:hover { background: rgba(255,255,255,0.14); }
            #btnMinimize { color: #94A3B8; font-weight: bold; font-size: 18px; }
            #btnMinimize:hover { color: #F8FAFC; }
            #sidebar { background: rgba(15, 23, 42, 0.4); border-right: 1px solid rgba(255,255,255,0.04); }
            #lblSection1, #lblSection2, #lblSection3 { color: #475569; font-size: 10px; font-weight: 700; padding: 2px 10px 4px 10px; }
            QPushButton#btnNavActivate, QPushButton#btnNavFreeze, QPushButton#btnNavReset, QPushButton#btnNavStatus, QPushButton#btnNavUpdates, QPushButton#btnNavDownload, QPushButton#btnNavAbout { background: transparent; border: none; border-radius: 8px; text-align: left; padding: 9px 12px; font-size: 12px; font-weight: 600; color: #94A3B8; }
            QPushButton#btnNavActivate:hover, QPushButton#btnNavFreeze:hover, QPushButton#btnNavReset:hover, QPushButton#btnNavStatus:hover, QPushButton#btnNavUpdates:hover, QPushButton#btnNavDownload:hover, QPushButton#btnNavAbout:hover { background: rgba(14,165,233,0.1); color: #38BDF8; }
            QPushButton#btnNavActivate:disabled, QPushButton#btnNavFreeze:disabled, QPushButton#btnNavReset:disabled, QPushButton#btnNavStatus:disabled, QPushButton#btnNavUpdates:disabled { color: #334155; }
            QFrame#sep1, QFrame#sep2 { color: rgba(255,255,255,0.05); margin: 4px 4px; }
            #lblSideFooter { color: #334155; font-size: 10px; font-weight: 600; padding: 6px; }
            #contentArea { background: rgba(255,255,255,0.025); border-radius: 16px; margin: 8px 8px 8px 4px; border: 1px solid rgba(255,255,255,0.06); }
            #lblPageTitle { font-size: 17px; font-weight: 700; color: #F8FAFC; }
            #lblPageSubtitle { font-size: 11px; font-weight: 500; color: #64748B; }
            #lblStatusLabel { font-size: 10px; font-weight: 600; color: #475569; }
            #lblStatusValue { font-size: 13px; font-weight: 700; color: #38BDF8; }
            QFrame#hSep { color: rgba(255,255,255,0.05); }
            #lblActTitle, #lblFrzTitle, #lblResTitle { font-size: 16px; font-weight: 700; color: #F8FAFC; }
            #lblActDesc, #lblFrzDesc, #lblResDesc { font-size: 12px; font-weight: 500; color: #64748B; }
            QFrame#cardActivate, QFrame#cardFreeze, QFrame#cardReset { background: rgba(255,255,255,0.04); border: 1px solid rgba(255,255,255,0.07); border-radius: 12px; }
            #lblNoteWarn { background: rgba(245,158,11,0.07); border: 1px solid rgba(245,158,11,0.2); border-radius: 8px; padding: 10px 12px; font-size: 12px; font-weight: 600; color: #FCD34D; }
            #lblNoteSuccess { background: rgba(16,185,129,0.07); border: 1px solid rgba(16,185,129,0.2); border-radius: 8px; padding: 10px 12px; font-size: 12px; font-weight: 600; color: #6EE7B7; }
            #lblNoteDanger { background: rgba(239,68,68,0.07); border: 1px solid rgba(239,68,68,0.18); border-radius: 8px; padding: 10px 12px; font-size: 12px; font-weight: 600; color: #FCA5A5; }
            #btnDoActivate { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0284C7, stop:1 #38BDF8); border: 1px solid rgba(56,189,248,0.4); border-radius: 10px; color: white; font-size: 13px; font-weight: 700; padding: 0 26px; }
            #btnDoActivate:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0369A1, stop:1 #0284C7); }
            #btnDoActivate:disabled { background: rgba(255,255,255,0.05); color: #475569; border-color: rgba(255,255,255,0.08); }
            #btnDoFreeze { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0F766E, stop:1 #2DD4BF); border: 1px solid rgba(45,212,191,0.4); border-radius: 10px; color: white; font-size: 13px; font-weight: 700; padding: 0 26px; }
            #btnDoFreeze:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #115E59, stop:1 #0F766E); }
            #btnDoFreeze:disabled { background: rgba(255,255,255,0.05); color: #475569; border-color: rgba(255,255,255,0.08); }
            #btnDoReset { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #B91C1C, stop:1 #F87171); border: 1px solid rgba(248,113,113,0.4); border-radius: 10px; color: white; font-size: 13px; font-weight: 700; padding: 0 26px; }
            #btnDoReset:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #991B1B, stop:1 #B91C1C); }
            #btnDoReset:disabled { background: rgba(255,255,255,0.05); color: #475569; border-color: rgba(255,255,255,0.08); }
            QProgressBar { background: rgba(255,255,255,0.06); border: none; border-radius: 3px; height: 6px; }
            QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #0284C7, stop:1 #38BDF8); border-radius: 3px; }
            #lblProgressLabel { font-size: 11px; font-weight: 600; color: #94A3B8; }
            #logView { background: rgba(0,0,0,0.35); border: 1px solid rgba(255,255,255,0.06); border-radius: 10px; color: #E2E8F0; font-family: 'Consolas', monospace; font-size: 11px; font-weight: 500; padding: 10px; }
            QScrollBar:vertical { background: transparent; width: 5px; }
            QScrollBar::handle:vertical { background: rgba(255,255,255,0.15); border-radius: 2px; min-height: 24px; }
            QScrollBar::handle:vertical:hover { background: rgba(255,255,255,0.28); }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        )");
    }
    renderLogs();
}

void MainWindow::setupConnections()
{
    // Title bar controls
    connect(ui->btnClose,    &QPushButton::clicked, qApp, &QApplication::quit);
    connect(ui->btnMinimize, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(ui->btnTheme,    &QPushButton::clicked, this, &MainWindow::onThemeClicked);

    // Sidebar navigation (ONLY Changes Tabs, Does NOT Run Operation)
    connect(ui->btnNavActivate, &QPushButton::clicked, this, &MainWindow::onActivateClicked);
    connect(ui->btnNavFreeze,   &QPushButton::clicked, this, &MainWindow::onFreezeClicked);
    connect(ui->btnNavReset,    &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(ui->btnNavStatus,   &QPushButton::clicked, this, &MainWindow::onCheckStatusClicked);
    connect(ui->btnNavUpdates,  &QPushButton::clicked, this, &MainWindow::onCheckUpdatesClicked);
    connect(ui->btnNavDownload, &QPushButton::clicked, this, &MainWindow::onDownloadIDMClicked);
    connect(ui->btnNavAbout,    &QPushButton::clicked, this, &MainWindow::onAboutClicked);

    // Action buttons (Inside the Content Tabs, Executes Worker)
    connect(ui->btnDoActivate, &QPushButton::clicked, this, [this](){
        QDialog dlg(this);
        dlg.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        dlg.setAttribute(Qt::WA_TranslucentBackground);
        dlg.setFixedSize(450, 280);

        QVBoxLayout *layout = new QVBoxLayout(&dlg);
        layout->setContentsMargins(20, 20, 20, 20);

        QFrame *frame = new QFrame(&dlg);
        frame->setStyleSheet(m_isLightMode ?
                                 "QFrame { background: rgba(248, 250, 252, 0.95); border: 1px solid rgba(0,0,0,0.1); border-radius: 12px; }" :
                                 "QFrame { background: rgba(15, 23, 42, 0.95); border: 1px solid rgba(255,255,255,0.1); border-radius: 12px; }");
        QVBoxLayout *frameLayout = new QVBoxLayout(frame);

        QLabel *lblTitle = new QLabel("⚡ Activation Method", frame);
        lblTitle->setStyleSheet(m_isLightMode ? "color: #0F172A; font-size: 16px; font-weight: bold; border: none;" : "color: #F8FAFC; font-size: 16px; font-weight: bold; border: none;");

        QLabel *lblDesc = new QLabel("Choose how you want to activate IDM:\n\n"
                                     "1. Full Activation: Uses Registry + DLL Memory Patch\n   to block fake serial popups permanently. (Recommended)\n\n"
                                     "2. Standard Activation: Uses Registry only.", frame);
        lblDesc->setStyleSheet(m_isLightMode ? "color: #475569; font-size: 12px; border: none;" : "color: #94A3B8; font-size: 12px; border: none;");
        lblDesc->setWordWrap(true);

        QPushButton *btnFull = new QPushButton("🚀 Full Activation (Recommended)", frame);
        btnFull->setStyleSheet(m_isLightMode ?
                                   "QPushButton { background: #0284C7; color: white; border-radius: 8px; padding: 10px; font-weight: bold; border: none; } QPushButton:hover { background: #0369A1; }" :
                                   "QPushButton { background: #0284C7; color: white; border-radius: 8px; padding: 10px; font-weight: bold; border: none; } QPushButton:hover { background: #38BDF8; }");
        btnFull->setCursor(Qt::PointingHandCursor);

        QPushButton *btnStandard = new QPushButton("📄 Standard Activation", frame);
        btnStandard->setStyleSheet(m_isLightMode ?
                                       "QPushButton { background: rgba(0,0,0,0.05); color: #0F172A; border-radius: 8px; padding: 10px; font-weight: bold; border: none; } QPushButton:hover { background: rgba(0,0,0,0.1); }" :
                                       "QPushButton { background: rgba(255,255,255,0.05); color: #F8FAFC; border-radius: 8px; padding: 10px; font-weight: bold; border: none; } QPushButton:hover { background: rgba(255,255,255,0.1); }");
        btnStandard->setCursor(Qt::PointingHandCursor);

        QPushButton *btnCancel = new QPushButton("Cancel", frame);
        btnCancel->setStyleSheet(m_isLightMode ? "color: #DC2626; border: none; background: transparent; font-weight: bold;" : "color: #F87171; border: none; background: transparent; font-weight: bold;");
        btnCancel->setCursor(Qt::PointingHandCursor);

        frameLayout->addWidget(lblTitle);
        frameLayout->addWidget(lblDesc);
        frameLayout->addSpacing(10);
        frameLayout->addWidget(btnFull);
        frameLayout->addWidget(btnStandard);
        frameLayout->addWidget(btnCancel, 0, Qt::AlignCenter);
        layout->addWidget(frame);

        bool useInjector = false;
        connect(btnFull, &QPushButton::clicked, [&](){ useInjector = true; dlg.accept(); });
        connect(btnStandard, &QPushButton::clicked, [&](){ useInjector = false; dlg.accept(); });
        connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

        if (dlg.exec() == QDialog::Accepted) {
            runOperation(IDMWorker::Activate, false, useInjector);
        }
    });

    connect(ui->btnDoFreeze, &QPushButton::clicked, this, [this](){ runOperation(IDMWorker::FreezeTrial, false, false); });
    connect(ui->btnDoReset,  &QPushButton::clicked, this, [this](){ runOperation(IDMWorker::Reset, false, false); });

}

void MainWindow::setupAnimations()
{
    m_pulseTimer = new QTimer(this);
    m_pulseTimer->setInterval(2000);
    connect(m_pulseTimer, &QTimer::timeout, this, &MainWindow::animatePulse);
    m_pulseTimer->start();

    // Remove the 15-second status check to prevent overlapping requests or flickering.
    // updateStatusBadge() native call handles lightweight checks instead.
    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(15000);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::updateStatusBadge);
    m_statusTimer->start();
}

void MainWindow::animatePulse()
{
    auto *anim = new QPropertyAnimation(ui->lblLogo, "geometry", this);
    anim->setDuration(800);
    QRect r = ui->lblLogo->geometry();
    anim->setStartValue(r);
    anim->setKeyValueAt(0.5, r.adjusted(-2,-2,2,2));
    anim->setEndValue(r);
    anim->setEasingCurve(QEasingCurve::SineCurve);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::updateStatusBadge()
{
    if (!m_workerThread.isNull() && m_workerThread->isRunning()) return;

    QString colorSuccess = m_isLightMode ? "#059669" : "#10B981";
    QString colorFreeze  = m_isLightMode ? "#0284C7" : "#38BDF8";
    QString colorWarn    = m_isLightMode ? "#D97706" : "#F59E0B";
    QString colorClean   = m_isLightMode ? "#0D9488" : "#14B8A6";
    QString colorDanger  = m_isLightMode ? "#DC2626" : "#EF4444";
    QString colorMuted   = m_isLightMode ? "#64748B" : "#94A3B8";

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\DownloadManager", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        char buf[512] = {0};
        DWORD sz = sizeof(buf);
        QString verStr = "";
        if (RegQueryValueExA(hKey, "idmvers", nullptr, nullptr, (LPBYTE)buf, &sz) == ERROR_SUCCESS && sz > 1) {
            verStr = QString::fromLocal8Bit(buf).trimmed();
            m_idmVersion = verStr;
        } else {
            verStr = m_idmVersion;
        }

        QString displayVer = verStr.isEmpty() ? "IDM" : ("IDM " + verStr);

        sz = sizeof(buf);
        if (RegQueryValueExA(hKey, "Serial", nullptr, nullptr, (LPBYTE)buf, &sz) == ERROR_SUCCESS) {
            ui->lblStatusValue->setText(QString("🔒 %1 · Registered").arg(displayVer));
            ui->lblStatusValue->setStyleSheet(QString("color: %1; font-weight: bold;").arg(colorSuccess));
            ui->lblStatusBadge->setText("✅  Registered");
            ui->lblStatusBadge->setStyleSheet("color: " + colorSuccess + "; background: rgba(16,185,129,0.18); border-radius:14px;");
        } else {
            sz = sizeof(buf);
            if (RegQueryValueExA(hKey, "AliSakkaf_Frozen", nullptr, nullptr, (LPBYTE)buf, &sz) == ERROR_SUCCESS) {
                ui->lblStatusValue->setText(QString("❄️ %1 · Frozen").arg(displayVer));
                ui->lblStatusValue->setStyleSheet(QString("color: %1; font-weight: bold;").arg(colorFreeze));
                ui->lblStatusBadge->setText("❄️  Trial Frozen");
                ui->lblStatusBadge->setStyleSheet("color: " + colorFreeze + "; background: rgba(14,165,233,0.18); border-radius:14px;");
            } else {
                sz = sizeof(buf);
                if (RegQueryValueExA(hKey, "tvfrdt", nullptr, nullptr, (LPBYTE)buf, &sz) == ERROR_SUCCESS) {
                    ui->lblStatusValue->setText(QString("⏳ %1 · Trial").arg(displayVer));
                    ui->lblStatusValue->setStyleSheet(QString("color: %1; font-weight: bold;").arg(colorWarn));
                    ui->lblStatusBadge->setText("⏳  Trial Mode");
                    ui->lblStatusBadge->setStyleSheet("color: " + colorWarn + "; background: rgba(245,158,11,0.18); border-radius:14px;");
                } else {
                    ui->lblStatusValue->setText(QString("✨ %1 · Ready").arg(displayVer));
                    ui->lblStatusValue->setStyleSheet(QString("color: %1; font-weight: bold;").arg(colorClean));
                    ui->lblStatusBadge->setText("✨  Trial Ready");
                    ui->lblStatusBadge->setStyleSheet("color: " + colorClean + "; background: rgba(20,184,166,0.18); border-radius:14px;");
                }
            }
        }
        RegCloseKey(hKey);
    } else {
        ui->lblStatusValue->setText("❌ IDM Not Installed");
        ui->lblStatusValue->setStyleSheet(QString("color: %1; font-weight: bold;").arg(colorMuted));
        ui->lblStatusBadge->setText("❓  Unknown");
        ui->lblStatusBadge->setStyleSheet("color: " + colorMuted + "; background: rgba(100,116,139,0.15); border-radius:14px;");
    }
}
void MainWindow::runOperation(IDMWorker::Operation op, bool silent, bool useInjector)
{
    if (!m_workerThread.isNull() && m_workerThread->isRunning()) return;

    m_lastOp = op;
    m_isSilentOp = silent;

    if (!silent) {
        m_logHistory.clear();
        ui->logView->clear();
        ui->stackedPages->setCurrentWidget(ui->pageLog);

        setButtonsEnabled(false);
        ui->progressBar->setValue(0);
        ui->progressBar->setVisible(true);
        ui->lblProgressLabel->setVisible(true);
        ui->lblProgressLabel->setText("Initializing...");
    }

    m_workerThread = new QThread(this);
    m_worker = new IDMWorker(op);
    m_worker->setUseInjector(useInjector);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &IDMWorker::run);

    // Only bind UI updates if it's not a silent background check
    if (!silent) {
        connect(m_worker, &IDMWorker::operationStarted, this, &MainWindow::onOperationStarted);
        connect(m_worker, &IDMWorker::logMessage, this, &MainWindow::onLogMessage);
        connect(m_worker, &IDMWorker::progressChanged, this, &MainWindow::onProgressChanged);
    }

    // Always connect finish and status events
    connect(m_worker, &IDMWorker::operationFinished, this, &MainWindow::onOperationFinished);
    connect(m_worker, &IDMWorker::statusDetected, this, &MainWindow::onStatusDetected);

    // Cleanup routines
    connect(m_worker, &IDMWorker::operationFinished, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

    m_workerThread->start();
}

void MainWindow::setButtonsEnabled(bool enabled)
{
    ui->btnNavActivate->setEnabled(enabled);
    ui->btnNavFreeze->setEnabled(enabled);
    ui->btnNavReset->setEnabled(enabled);
    ui->btnNavStatus->setEnabled(enabled);
    ui->btnNavUpdates->setEnabled(enabled);

    ui->btnDoActivate->setEnabled(enabled);
    ui->btnDoFreeze->setEnabled(enabled);
    ui->btnDoReset->setEnabled(enabled);
}

void MainWindow::showPage(int index)
{
    m_currentPage = index;
    ui->stackedPages->setCurrentIndex(index);
}

// ===== SLOTS =====

void MainWindow::onActivateClicked()
{
    showPage(0);
    // ui->lblPageTitle->setText("⚡ Activate IDM");
    ui->lblPageSubtitle->setText("Permanently activate IDM via registry manipulation");
}

void MainWindow::onFreezeClicked()
{
    showPage(1);
    // ui->lblPageTitle->setText("❄️ Freeze Trial");
    ui->lblPageSubtitle->setText("Freeze the 30-day trial period permanently");
}

void MainWindow::onResetClicked()
{
    showPage(2);
    // ui->lblPageTitle->setText("🔄 Reset / Clean");
    ui->lblPageSubtitle->setText("Remove all activation and trial data");
}

void MainWindow::onCheckStatusClicked()
{
    // Ensure we are viewing the log page
    // ui->lblPageTitle->setText("🔍 IDM Status");
    ui->lblPageSubtitle->setText("Checking current activation and installation status...");
    runOperation(IDMWorker::CheckStatus, false);
}

void MainWindow::onCheckUpdatesClicked()
{
    // ui->lblPageTitle->setText("🔔 Check Updates");
    ui->lblPageSubtitle->setText("Checking for newer versions on GitHub...");
    runOperation(IDMWorker::CheckUpdates, false);
}

void MainWindow::onDownloadIDMClicked()
{
    QDesktopServices::openUrl(QUrl("https://www.internetdownloadmanager.com/download.html"));
}

void MainWindow::onThemeClicked()
{
    m_isLightMode = !m_isLightMode;
    applyTheme();
    updateStatusBadge();
    update();
}

void MainWindow::renderLogs()
{
    if(m_isSilentOp) return; // Do not render logs for silent ops

    ui->logView->clear();
    for (const LogEntry &entry : m_logHistory) {
        QString message = entry.message;
        int type = entry.type;

        QString colorText    = m_isLightMode ? "#334155" : "#E2E8F0";
        QString colorPrimary = m_isLightMode ? "#0284C7" : "#38BDF8";
        QString colorWarn    = m_isLightMode ? "#D97706" : "#F59E0B";
        QString colorDanger  = m_isLightMode ? "#DC2626" : "#EF4444";
        QString colorSuccess = m_isLightMode ? "#059669" : "#10B981";

        if (type == 5) {
            ui->logView->append(QString("<span style='color:%1;font-weight:bold;'>▶ %2</span>").arg(colorPrimary, message));
        } else if (type == 6) {
            ui->logView->append(QString("<br><span style='color:%1;font-weight:bold;font-size:14px;'>"
                                        "━━━━━━━━━━━━━━━━━<br>"
                                        "✅  %2<br>"
                                        "━━━━━━━━━━━━━━━━━</span>").arg(colorSuccess, message));
        } else if (type == 7) {
            ui->logView->append(QString("<br><span style='color:%1;font-weight:bold;font-size:14px;'>"
                                        "━━━━━━━━━━━━━━━━━<br>"
                                        "❌  %2<br>"
                                        "━━━━━━━━━━━━━━━━━</span>").arg(colorDanger, message));
        } else {
            QString prefix;
            switch (type) {
            case 0: prefix = "  "; break;
            case 1: prefix = "ℹ "; break;
            case 2: prefix = "⚠ "; break;
            case 3: prefix = "✖ "; break;
            case 4: prefix = "✔ "; break;
            default: prefix = "  "; break;
            }
            QString color = colorText;
            if      (type == 1) color = colorPrimary;
            else if (type == 2) color = colorWarn;
            else if (type == 3) color = colorDanger;
            else if (type == 4) color = colorSuccess;
            ui->logView->append(QString("<span style='color:%1;'>%2%3</span>").arg(color, prefix, message));
        }
    }
    QTextCursor c = ui->logView->textCursor();
    c.movePosition(QTextCursor::End);
    ui->logView->setTextCursor(c);
}

// Worker callbacks
void MainWindow::onOperationStarted(const QString &message)
{
    if (m_isSilentOp) return;
    m_logHistory.append({message, 5});
    renderLogs();
}

void MainWindow::onLogMessage(const QString &message, int type)
{
    if (m_isSilentOp) return;
    m_logHistory.append({message, type});
    renderLogs();
}

void MainWindow::onProgressChanged(int value, const QString &label)
{
    if (m_isSilentOp) return;
    ui->progressBar->setValue(value);
    ui->lblProgressLabel->setText(label);
}

void MainWindow::onOperationFinished(bool success, const QString &message)
{
    if (!m_isSilentOp) {
        setButtonsEnabled(true);
        ui->progressBar->setValue(100);

        m_logHistory.append({message, success ? 6 : 7});
        renderLogs();

        // Run silent check in background to update badge natively
        if (success && (m_lastOp == IDMWorker::Activate || m_lastOp == IDMWorker::FreezeTrial || m_lastOp == IDMWorker::Reset)) {
            QTimer::singleShot(2500, this, [this](){
                runOperation(IDMWorker::CheckStatus, true); // <--- True means SILENT MODE
            });
        }

        QTimer::singleShot(2000, this, [this](){
            ui->progressBar->setVisible(false);
            ui->lblProgressLabel->setVisible(false);
        });
    } else {
        // If it was a silent check, update the native badge based on results without touching the UI logs
        updateStatusBadge();
    }
}

void MainWindow::onStatusDetected(int statusCode, const QString &version, const QString &detail)
{
    Q_UNUSED(detail)

    if (!version.isEmpty()) m_idmVersion = version;

    QString displayVer = m_idmVersion.isEmpty() ? "IDM" : ("IDM " + m_idmVersion);

    QString badge, badgeStyle, statusText, statusStyle;

    switch (statusCode) {
    case 1:
        badge      = "✅  Registered";
        badgeStyle = "background:rgba(16,185,129,0.18);border:1px solid rgba(16,185,129,0.35);border-radius:14px;padding:4px 14px;font-size:11px;font-weight:700;color:" + QString(m_isLightMode ? "#059669" : "#10B981") + ";";
        statusText  = QString("🔒 %1 · Registered").arg(displayVer);
        statusStyle = "color:" + QString(m_isLightMode ? "#059669" : "#10B981") + ";font-weight:bold;";
        break;
    case 2:
        badge      = "⏳  Trial Mode";
        badgeStyle = "background:rgba(245,158,11,0.18);border:1px solid rgba(245,158,11,0.35);border-radius:14px;padding:4px 14px;font-size:11px;font-weight:700;color:" + QString(m_isLightMode ? "#D97706" : "#F59E0B") + ";";
        statusText  = QString("⏳ %1 · Trial").arg(displayVer);
        statusStyle = "color:" + QString(m_isLightMode ? "#D97706" : "#F59E0B") + ";font-weight:bold;";
        break;
    case 3:
        badge      = "❌  Not Found";
        badgeStyle = "background:rgba(239,68,68,0.15);border:1px solid rgba(239,68,68,0.30);border-radius:14px;padding:4px 14px;font-size:11px;font-weight:700;color:" + QString(m_isLightMode ? "#DC2626" : "#EF4444") + ";";
        statusText  = "❌ IDM Not Installed";
        statusStyle = "color:" + QString(m_isLightMode ? "#DC2626" : "#EF4444") + ";font-weight:bold;";
        break;
    case 4:
        badge      = "❄️  Trial Frozen";
        badgeStyle = "background:rgba(14,165,233,0.18);border:1px solid rgba(14,165,233,0.35);border-radius:14px;padding:4px 14px;font-size:11px;font-weight:700;color:" + QString(m_isLightMode ? "#0284C7" : "#38BDF8") + ";";
        statusText  = QString("❄️ %1 · Frozen").arg(displayVer);
        statusStyle = "color:" + QString(m_isLightMode ? "#0284C7" : "#38BDF8") + ";font-weight:bold;";
        break;
    case 5:
        badge      = "✨  Trial Ready";
        badgeStyle = "background:rgba(20,184,166,0.18);border:1px solid rgba(20,184,166,0.35);border-radius:14px;padding:4px 14px;font-size:11px;font-weight:700;color:" + QString(m_isLightMode ? "#0D9488" : "#14B8A6") + ";";
        statusText  = QString("✨ %1 · Ready").arg(displayVer);
        statusStyle = "color:" + QString(m_isLightMode ? "#0D9488" : "#14B8A6") + ";font-weight:bold;";
        break;
    default:
        badge      = "❓  Unknown";
        badgeStyle = "background:rgba(100,116,139,0.15);border:1px solid rgba(100,116,139,0.25);border-radius:14px;padding:4px 14px;font-size:11px;font-weight:700;color:" + QString(m_isLightMode ? "#64748B" : "#94A3B8") + ";";
        statusText  = "❓ Status Unknown";
        statusStyle = "color:" + QString(m_isLightMode ? "#64748B" : "#94A3B8") + ";font-weight:bold;";
        break;
    }

    ui->lblStatusBadge->setText(badge);
    ui->lblStatusBadge->setStyleSheet(badgeStyle);
    if (ui->lblStatusValue) {
        ui->lblStatusValue->setText(statusText);
        ui->lblStatusValue->setStyleSheet(statusStyle);
    }
}
void MainWindow::onAboutClicked()
{
    AboutDialog dlg(m_isLightMode, this);
    dlg.exec();
}
