#pragma once
#include <QDialog>
#include <QPainter>
#include <QLinearGradient>
#include <QMouseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class AboutDialog; }
QT_END_NAMESPACE

class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(bool isLightMode, QWidget *parent = nullptr);
    ~AboutDialog();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    Ui::AboutDialog *ui;
    bool m_drag = false;
    QPoint m_dragPos;
    bool m_isLightMode;
};
