#------------------------------#
#                              #
# Project created by QtCreator #
#                              #
#------------------------------#

# QT

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Compilation controls
QMAKE_CXXFLAGS += -std=c++1y
QMAKE_CXXFLAGS += -I/usr/include/opencv
LIBS += -lX11
LIBS += -lopencv_imgproc -lopencv_core -lopencv_features2d # -lopencv_objdetect -lopencv_highgui

# Project Structure

TARGET = controller
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    xcontrol.cpp \
    ai.cpp

HEADERS  += mainwindow.h \
    xcontrol.h \
    ai.h

FORMS    += mainwindow.ui

