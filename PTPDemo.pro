#-------------------------------------------------
#
# Project created by QtCreator 2013-10-02T11:35:23
#
#-------------------------------------------------

QT       += widgets printsupport

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

target.path = /opt/ptp
target.files = PTPDemo run_master.sh master.cfg slave.cfg
ptp.path = /opt/ptp
ptp.files = linuxptp-1.3/ptp4l linuxptp-1.3/phc2sys
configfile.path = /opt/ApplicationLauncher/applications/xml
configfile.files = configs/6-ptp.xml
imagefile.path = /opt/ApplicationLauncher/applications/resources
imagefile.files = configs/ptp.png

greaterThan(QT_MAJOR_VERSION, 4) {
  startscript.path = /opt/ptp
  startscript.files = configs/qt5/ptp.sh
}else{
  startscript.path = /opt/ptp
  startscript.files = configs/qt4/ptp.sh
}


INSTALLS += startscript target configfile imagefile ptp
