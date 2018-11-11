#-------------------------------------------------
#
# Project created by QtCreator 2014-10-05T22:45:47
#
#-------------------------------------------------

QT       += core gui multimedia widgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


LIBS +=avformat.lib
LIBS +=avcodec.lib
LIBS +=avutil.lib
LIBS +=swresample.lib
TARGET = LqtePlayers
TEMPLATE = app


SOURCES += main.cpp \
    mainwindow.cpp \
    audiodecodemgr.cpp \
    tableview.cpp \
    audiothread.cpp

HEADERS  += \
    mainwindow.h \
    audiodecodemgr.h \
    tableview.h \
    audiothread.h

FORMS += \
    mainwindow.ui \
    titleBar.ui \
    listgroup.ui \
    ViewCenter.ui \
    miniPlay.ui

RESOURCES += \
    src.qrc

OTHER_FILES +=
