TEMPLATE = lib
TARGET = core
CONFIG += static

# Input
HEADERS += datawarehouse.h \
    imagewidget.h \
    mainwindow.h \
    processstep.h \
    projection.h \
    segments.h \
    sidebar.h \
    staff.h \
    tools.h \
    cluster.h \
    symbol.h
SOURCES += datawarehouse.cpp \
    imagewidget.cpp \
    mainwindow.cpp \
    processstep.cpp \
    projection.cpp \
    segments.cpp \
    sidebar.cpp \
    staff.cpp \
    tools.cpp \
    cluster.cpp \
    symbol.cpp \
    unused.cpp

MOC_DIR = .tmp
UI_DIR = .tmp
RCC_DIR = .tmp
OBJECTS_DIR = .tmp

CONFIG += debug qtestlib
win32 {
    CONFIG += console
}

DESTDIR = ..
