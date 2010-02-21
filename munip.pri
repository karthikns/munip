MOC_DIR = .tmp
UI_DIR = .tmp
RCC_DIR = .tmp
OBJECTS_DIR = .tmp

CONFIG += debug qtestlib
win32 {
    CONFIG += console
}

DEPENDPATH += $$PWD/core
INCLUDEPATH += $$PWD/core
LIBS += -L$$PWD
DESTDIR = $$PWD
