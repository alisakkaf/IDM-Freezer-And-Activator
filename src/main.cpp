#include "mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QFontDatabase>

#include <QIcon>

int main(int argc, char *argv[])
{
    // Enable High DPI Support before QApplication creation
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication a(argc, argv);
    a.setApplicationName("IDM Activation Tool");
    a.setApplicationVersion("1.2");
    a.setOrganizationName("Ali Sakkaf");
    a.setWindowIcon(QIcon(":/icon.ico"));

    // Modern Typography
    QFont defaultFont("Segoe UI", 10);
    defaultFont.setStyleStrategy(QFont::PreferAntialias);
    a.setFont(defaultFont);

    MainWindow w;
    w.show();

    return a.exec();
}
