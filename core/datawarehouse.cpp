#include "datawarehouse.h"
#include "staff.h"
#include "mainwindow.h"
#include "imagewidget.h"

using namespace Munip;

DataWarehouse* DataWarehouse::m_dataWarehouse = 0;

DataWarehouse::DataWarehouse()
{
    m_pageSkewPrecision = 0.3f;
    m_lineSize = (int)qRound(resolution().width()*0.05);
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

void DataWarehouse::setLineSize(uint size )
{
    m_lineSize = size;
}

uint DataWarehouse::lineSize() const
{
    return m_lineSize;
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
