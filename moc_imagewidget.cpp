/****************************************************************************
** Meta object code from reading C++ file 'imagewidget.h'
**
** Created: Mon Mar 2 20:21:17 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "imagewidget.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'imagewidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_ImageWidget[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      20,   13,   12,   12, 0x05,

 // slots: signature, parameters, type, tag, flags
      45,   43,   12,   12, 0x0a,
      67,   12,   12,   12, 0x0a,
      88,   12,   12,   12, 0x0a,
     101,   12,   12,   12, 0x0a,
     115,   12,   12,   12, 0x0a,
     126,   12,   12,   12, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ImageWidget[] = {
    "ImageWidget\0\0string\0statusMessage(QString)\0"
    "b\0slotSetShowGrid(bool)\0slotToggleShowGrid()\0"
    "slotZoomIn()\0slotZoomOut()\0slotSave()\0"
    "slotSaveAs()\0"
};

const QMetaObject ImageWidget::staticMetaObject = {
    { &QGraphicsView::staticMetaObject, qt_meta_stringdata_ImageWidget,
      qt_meta_data_ImageWidget, 0 }
};

const QMetaObject *ImageWidget::metaObject() const
{
    return &staticMetaObject;
}

void *ImageWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ImageWidget))
	return static_cast<void*>(const_cast< ImageWidget*>(this));
    return QGraphicsView::qt_metacast(_clname);
}

int ImageWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: statusMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: slotSetShowGrid((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: slotToggleShowGrid(); break;
        case 3: slotZoomIn(); break;
        case 4: slotZoomOut(); break;
        case 5: slotSave(); break;
        case 6: slotSaveAs(); break;
        }
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void ImageWidget::statusMessage(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
