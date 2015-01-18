#-------------------------------------------------
#
# Project created by QtCreator 2015-01-15T13:26:35
#
#-------------------------------------------------

CONFIG += c++11
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = stereo_to_depthmap
TEMPLATE = app

LIBPATH += /usr/lib/x86_64-linux-gnu

LIBS     += -lm -lz -lpthread -lavformat -lavcodec -lavutil -lopencv_core -lopencv_calib3d -lopencv_highgui -lopencv_imgproc -lopencv_video -lopencv_objdetect

SOURCES += main.cpp\
           arguments.cpp

HEADERS  += arguments.hpp

