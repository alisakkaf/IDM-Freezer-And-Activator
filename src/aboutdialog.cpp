#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QDesktopServices>
#include <QUrl>
#include <QPainterPath>
#include <QBitmap>
#include <QApplication>
#include <QScreen>
#include <QRadialGradient>

AboutDialog::AboutDialog(bool isLightMode, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
    , m_isLightMode(isLightMode)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 480);

    // Center relative to parent or screen
    if (parent) {
        QPoint topLeft = parent->mapToGlobal(parent->rect().center()) - QPoint(width()/2, height()/2);
        move(topLeft);
    } else {
        QScreen *s = QApplication::primaryScreen();
        move(s->availableGeometry().center() - rect().center());
    }

    // Apply rounded mask
    QBitmap mask(size());
    mask.fill(Qt::color0);
    QPainter p(&mask);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::color1);
    p.drawRoundedRect(rect(), 20, 20);
    setMask(mask);

    // Apply Dynamic Styles based on Light/Dark Mode
    if (m_isLightMode) {
        setStyleSheet(R"(
            QWidget#topBar { background: rgba(255, 255, 255, 0.4); border-bottom: 1px solid rgba(0, 0, 0, 0.06); }
            QLabel { background: transparent; }
            QLabel#lblTopTitle { color: #0F172A; font-size: 13px; font-weight: 600; }
            QPushButton#btnClose { background: rgba(239, 68, 68, 0.1); border: 1px solid rgba(239, 68, 68, 0.2); border-radius: 12px; color: #DC2626; font-size: 12px; min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; }
            QPushButton#btnClose:hover { background: rgba(239, 68, 68, 0.8); color: white; border: 1px solid rgba(239, 68, 68, 1); }
            QLabel#lblIcon { font-size: 42px; }
            QLabel#lblAppName { color: #0F172A; font-size: 18px; font-weight: 600; }
            QLabel#lblAppVersion { color: #64748B; font-size: 12px; }
            QFrame#lineDivider { color: rgba(0, 0, 0, 0.06); }
            QLabel#lblDevInfo { color: #1E293B; font-size: 12px; font-weight: 600; }
            
            QPushButton#btnGithubOwner { background: rgba(0, 0, 0, 0.03); border: 1px solid rgba(0, 0, 0, 0.08); border-radius: 8px; color: #334155; font-size: 12px; padding: 10px; text-align: center; }
            QPushButton#btnGithubOwner:hover { background: rgba(0, 0, 0, 0.08); color: #0F172A; }
            
            QPushButton#btnFacebook { background: rgba(59, 130, 246, 0.1); border: 1px solid rgba(59, 130, 246, 0.2); border-radius: 8px; color: #2563EB; font-size: 12px; padding: 10px; text-align: center; }
            QPushButton#btnFacebook:hover { background: rgba(59, 130, 246, 0.2); border: 1px solid rgba(59, 130, 246, 0.4); color: #1D4ED8; }
            
            QPushButton#btnWebsite { background: rgba(20, 184, 166, 0.1); border: 1px solid rgba(20, 184, 166, 0.2); border-radius: 8px; color: #0D9488; font-size: 12px; padding: 10px; text-align: center; }
            QPushButton#btnWebsite:hover { background: rgba(20, 184, 166, 0.2); border: 1px solid rgba(20, 184, 166, 0.4); color: #0F766E; }
            
        )");
    } else {
        setStyleSheet(R"(
            QWidget#topBar { background: rgba(255, 255, 255, 0.03); border-bottom: 1px solid rgba(255, 255, 255, 0.06); }
            QLabel { background: transparent; }
            QLabel#lblTopTitle { color: #F8FAFC; font-size: 13px; font-weight: 600; }
            QPushButton#btnClose { background: rgba(239, 68, 68, 0.1); border: 1px solid rgba(239, 68, 68, 0.2); border-radius: 12px; color: #FCA5A5; font-size: 12px; min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; }
            QPushButton#btnClose:hover { background: rgba(239, 68, 68, 0.8); color: white; border: 1px solid rgba(239, 68, 68, 1); }
            QLabel#lblIcon { font-size: 42px; }
            QLabel#lblAppName { color: #F8FAFC; font-size: 18px; font-weight: 600; }
            QLabel#lblAppVersion { color: #94A3B8; font-size: 12px; }
            QFrame#lineDivider { color: rgba(255, 255, 255, 0.06); }
            QLabel#lblDevInfo { color: #E2E8F0; font-size: 12px; font-weight: 600; }
            
            QPushButton#btnGithubOwner { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 8px; color: #E2E8F0; font-size: 12px; padding: 10px; text-align: center; }
            QPushButton#btnGithubOwner:hover { background: rgba(255, 255, 255, 0.08); color: white; }
            
            QPushButton#btnFacebook { background: rgba(59, 130, 246, 0.1); border: 1px solid rgba(59, 130, 246, 0.2); border-radius: 8px; color: #93C5FD; font-size: 12px; padding: 10px; text-align: center; }
            QPushButton#btnFacebook:hover { background: rgba(59, 130, 246, 0.2); border: 1px solid rgba(59, 130, 246, 0.4); color: white; }
            
            QPushButton#btnWebsite { background: rgba(20, 184, 166, 0.1); border: 1px solid rgba(20, 184, 166, 0.2); border-radius: 8px; color: #5EEAD4; font-size: 12px; padding: 10px; text-align: center; }
            QPushButton#btnWebsite:hover { background: rgba(20, 184, 166, 0.2); border: 1px solid rgba(20, 184, 166, 0.4); color: white; }

        )");
    }

    // Connect button
    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btnGithubOwner, &QPushButton::clicked, this, [](){
        QDesktopServices::openUrl(QUrl("https://github.com/alisakkaf"));
    });
    connect(ui->btnFacebook, &QPushButton::clicked, this, [](){
        QDesktopServices::openUrl(QUrl("https://www.facebook.com/AliSakkaf.Dev/"));
    });
    connect(ui->btnWebsite, &QPushButton::clicked, this, [](){
        QDesktopServices::openUrl(QUrl("https://mysterious-dev.com/"));
    });
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_isLightMode) {
        // Light Background (Pearl White)
        QLinearGradient bg(0, 0, width(), height());
        bg.setColorAt(0.0, QColor(248, 250, 252)); // Slate 50
        bg.setColorAt(1.0, QColor(241, 245, 249)); // Slate 100
        painter.fillRect(rect(), bg);

        // Orb glow top (Sky Blue)
        QRadialGradient orb(width()/2, 0, 250);
        orb.setColorAt(0.0, QColor(56, 189, 248, 15)); // Sky 400
        orb.setColorAt(1.0, QColor(0,0,0,0));
        painter.fillRect(rect(), orb);

        // Border
        QPainterPath path;
        path.addRoundedRect(QRectF(0.5, 0.5, width()-1, height()-1), 19.5, 19.5);
        painter.setPen(QPen(QColor(0, 0, 0, 15), 1));
        painter.drawPath(path);
    } else {
        // Dark Background (Slate 950)
        QLinearGradient bg(0, 0, width(), height());
        bg.setColorAt(0.0, QColor(24, 24, 27));
        bg.setColorAt(1.0, QColor(9, 9, 11));
        painter.fillRect(rect(), bg);

        // Orb glow top (Sky Blue)
        QRadialGradient orb(width()/2, 0, 250);
        orb.setColorAt(0.0, QColor(14, 165, 233, 40));
        orb.setColorAt(1.0, QColor(0,0,0,0));
        painter.fillRect(rect(), orb);

        // Border
        QPainterPath path;
        path.addRoundedRect(QRectF(0.5, 0.5, width()-1, height()-1), 19.5, 19.5);
        painter.setPen(QPen(QColor(255, 255, 255, 20), 1));
        painter.drawPath(path);
    }
}

void AboutDialog::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && e->pos().y() < 60) {
        m_drag = true;
        m_dragPos = e->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(e);
}
void AboutDialog::mouseMoveEvent(QMouseEvent *e)
{
    if (m_drag) move(e->globalPos() - m_dragPos);
    QDialog::mouseMoveEvent(e);
}
void AboutDialog::mouseReleaseEvent(QMouseEvent *e)
{
    m_drag = false;
    QDialog::mouseReleaseEvent(e);
}
