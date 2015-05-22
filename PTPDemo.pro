#-------------------------------------------------
#
# Project created by QtCreator 2013-10-02T11:35:23
#
#-------------------------------------------------

QT       += core gui

TARGET = PTPDemo
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    PlottingClass/qcustomplot.cpp \
    pipereader.cpp

HEADERS  += mainwindow.h \
    PlottingClass/qcustomplot.h \
    pipereader.h

FORMS    += mainwindow.ui

RESOURCES += \
    PTPGraphicResources.qrc
