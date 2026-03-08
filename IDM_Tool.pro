# =============================================================================
#    IDM Activation Tool  –  Project File (.pro)
#    Copyright © 2026  AliSakkaf  |  All Rights Reserved
#    Version : 1.2  |  Built: 2026-03-08
# -----------------------------------------------------------------------------
#    Website  : https://mysterious-dev.com/
#    GitHub   : https://github.com/alisakkaf
#    Facebook : https://www.facebook.com/AliSakkaf.Dev/
# =============================================================================

QT       += core gui widgets winextras network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

TARGET = IDM_Tool
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS
# DEFINES += QT_NO_DEBUG_OUTPUT

# Optimize for release size
CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS += -O3
    QMAKE_LFLAGS += -Wl,--gc-sections
    QMAKE_STRIP += --strip-unneeded
}

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/idmworker.cpp \
    src/registrymanager.cpp \
    src/aboutdialog.cpp \
    src/logwidget.cpp

HEADERS += \
    src/mainwindow.h \
    src/idmworker.h \
    src/registrymanager.h \
    src/aboutdialog.h \
    src/logwidget.h

FORMS += \
    src/mainwindow.ui \
    src/aboutdialog.ui

RESOURCES += \
    resources/resources.qrc

LIBS += -ladvapi32 -luser32 -lshell32 -lws2_32 -lnetapi32 -lshlwapi -lole32 -luuid


RC_FILE = resources/app.rc

DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/.obj
MOC_DIR = $$PWD/build/.moc
RCC_DIR = $$PWD/build/.rcc
UI_DIR = $$PWD/build/.ui
