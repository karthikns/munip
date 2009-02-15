TEMPLATE = app
TARGET = munip
DEPENDPATH += .
INCLUDEPATH += .

CONFIG += debug console

# Input
HEADERS += imagewidget.h \
           mainwindow.h \
           monoimage.h \
           processstep.h \
           projection.h \
           sidebar.h \
           staff.h \
           tools.h
SOURCES += imagewidget.cpp \
           main.cpp \
           mainwindow.cpp \
           monoimage.cpp \
           processstep.cpp \
           projection.cpp \
           sidebar.cpp \
           staff.cpp \
           tools.cpp
RESOURCES += munipresources.qrc

MOC_DIR = .tmp
UI_DIR = .tmp
RCC_DIR = .tmp
OBJECTS_DIR = .tmp
