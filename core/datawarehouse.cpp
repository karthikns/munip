#include "datawarehouse.h"
#include "staff.h"
#include "mainwindow.h"
#include "imagewidget.h"
#include "symbol.h"

using namespace Munip;

DataWarehouse* DataWarehouse::m_dataWarehouse = 0;

DataWarehouse::DataWarehouse()
{
    m_pageSkewPrecision = 0.3f;
    m_lineSize = (int)qRound(resolution().width()*0.05);
    m_staffSpaceHeight = Range(4, 6);
    m_staffLineHeight = Range(1, 2);
}

DataWarehouse* DataWarehouse::instance()
{
    if(!m_dataWarehouse)
        m_dataWarehouse = new DataWarehouse;
     return m_dataWarehouse;
}

void DataWarehouse::setPageSkew( float skew )
{
    m_pageSkew = skew;
}

float DataWarehouse::pageSkew()    const
{
    return m_pageSkew;
}

void DataWarehouse::setPageSkewPrecision( float precision )
{
    m_pageSkewPrecision = precision;
}

float DataWarehouse::pageSkewPrecison() const
{
    return m_pageSkewPrecision;
}

QSize DataWarehouse::resolution() const
{
    MainWindow* t = MainWindow::instance();
    ImageWidget *img = t->activeImageWidget();
    return img ? img->image().size() : QSize(0, 0);
}

void DataWarehouse::clearStaff()
{
    m_staffList.clear();
}

QList<Staff> DataWarehouse::staffList() const
{
    return m_staffList;
}

void DataWarehouse::appendStaff( Staff staff )
{
    m_staffList.append(staff);
}

QImage DataWarehouse::workImage() const
{
    return m_workImage;
}

void DataWarehouse::setWorkImage(const QImage &image)
{
    m_workImage = image;
}

Range DataWarehouse::staffSpaceHeight() const
{
    return m_staffSpaceHeight;
}

void DataWarehouse::setStaffSpaceHeight(const Range& value)
{
    m_staffSpaceHeight = value;
}

Range DataWarehouse::staffLineHeight() const
{
    return m_staffLineHeight;
}

void DataWarehouse::setStaffLineHeight(const Range& value)
{
    m_staffLineHeight = value;
}

QList<StaffData*> DataWarehouse::staffDatas() const
{
    return m_staffDatas;
}

void DataWarehouse::setStaffDatas(const QList<StaffData*> &sd)
{
    m_staffDatas = sd;
}

QImage DataWarehouse::imageWithRemovedStaffLinesOnly() const
{
    return m_imageWithRemovedStaffLinesOnly;
}

QImage& DataWarehouse::imageRefWithRemovedStaffLinesOnly()
{
    return m_imageWithRemovedStaffLinesOnly;
}
