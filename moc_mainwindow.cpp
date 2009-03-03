/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created: Mon Mar 2 20:21:19 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "mainwindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_MainWindow[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x0a,
      23,   11,   11,   11, 0x0a,
      34,   11,   11,   11, 0x0a,
      47,   11,   11,   11, 0x0a,
      59,   11,   11,   11, 0x0a,
      70,   11,   11,   11, 0x0a,
      83,   11,   11,   11, 0x0a,
      99,   97,   11,   11, 0x0a,
     124,   11,   11,   11, 0x0a,
     141,   11,   11,   11, 0x0a,
     165,  158,   11,   11, 0x0a,
     192,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MainWindow[] = {
    "MainWindow\0\0slotOpen()\0slotSave()\0"
    "slotSaveAs()\0slotClose()\0slotQuit()\0"
    "slotZoomIn()\0slotZoomOut()\0b\0"
    "slotToggleShowGrid(bool)\0slotProjection()\0"
    "slotAboutMunip()\0status\0"
    "slotStatusMessage(QString)\0"
    "slotOnSubWindowActivate(QMdiSubWindow*)\0"
};

const QMetaObject MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow,
      qt_meta_data_MainWindow, 0 }
};

const QMetaObject *MainWindow::metaObject() const
{
    return &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow))
	return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: slotOpen(); break;
        case 1: slotSave(); break;
        case 2: slotSaveAs(); break;
        case 3: slotClose(); break;
        case 4: slotQuit(); break;
        case 5: slotZoomIn(); break;
        case 6: slotZoomOut(); break;
        case 7: slotToggleShowGrid((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: slotProjection(); break;
        case 9: slotAboutMunip(); break;
        case 10: slotStatusMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: slotOnSubWindowActivate((*reinterpret_cast< QMdiSubWindow*(*)>(_a[1]))); break;
        }
        _id -= 12;
    }
    return _id;
}
