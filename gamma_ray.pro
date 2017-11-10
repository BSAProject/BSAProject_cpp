#-------------------------------------------------
#
# Project created by QtCreator 2016-07-26T21:10:06
#
#-------------------------------------------------

QT       += core gui
QT       += printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = gamma_ray
TEMPLATE = app

SOURCES += main.cpp\
        gamma.cpp \
    GraphicWidget.cpp \
    PlotSettings.cpp \
    qcustomplot.cpp

HEADERS  += gamma.h \
    GraphicWidget.h \
    PlotSettings.h \
    qbsa.h \
    qcustomplot.h

# подключение распараллеливания
QMAKE_CXXFLAGS+=-fopenmp
LIBS+=-lgomp

# подключение c++11
QMAKE_CXXFLAGS += -std=gnu++11

RESOURCES += icon.qrc

FORMS    += gamma.ui
