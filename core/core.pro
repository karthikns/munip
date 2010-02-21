TEMPLATE = lib
TARGET = core
CONFIG += static

# Input
HEADERS += DataWarehouse.h \
    horizontalrunlengthimage.h \
    imagewidget.h \
    mainwindow.h \
    processstep.h \
    projection.h \
    segments.h \
    sidebar.h \
    staff.h \
    tools.h \
    cluster.h
SOURCES += DataWarehouse.cpp \
    horizontalrunlengthimage.cpp \
    imagewidget.cpp \
    mainwindow.cpp \
    processstep.cpp \
    projection.cpp \
    segments.cpp \
    sidebar.cpp \
    staff.cpp \
    tools.cpp \
    cluster.cpp

include(../munip.pri)
