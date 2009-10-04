#include "DataWarehouse.h"
#include "staff.h"
#include "mainwindow.h"
#include "imagewidget.h"

using namespace Munip;

DataWarehouse* DataWarehouse :: m_dataWarehouse = 0;

DataWarehouse :: DataWarehouse()
{
    m_pageSkewPrecision = 0.3f;
    m_lineSize = (int)resolution().first*0.05;
}

DataWarehouse* DataWarehouse :: instance()
{
    if(!m_dataWarehouse)
        m_dataWarehouse = new DataWarehouse;
     return m_dataWarehouse;
}

void DataWarehouse :: setPageSkew( float skew )
{
    m_pageSkew = skew;
}

float DataWarehouse :: pageSkew()    const
{
    return m_pageSkew;
}

void DataWarehouse :: setPageSkewPrecision( float precision )
{
    m_pageSkewPrecision = precision;
}

float DataWarehouse :: pageSkewPrecison() const
{
    return m_pageSkewPrecision;
}

void DataWarehouse :: setLineSize(uint size )
{
    m_lineSize = size;
}

uint DataWarehouse :: lineSize() const
{
    return m_lineSize;
}

QPair<uint,uint> DataWarehouse :: resolution() const
{
    MainWindow* t = MainWindow::instance();
    if( !t->activeImageWidget() )
        return QPair<uint,uint>(0,0);
    QPair<uint,uint> ob( t->activeImageWidget()->image().width(),t->activeImageWidget()->image().height() );
    return ob;
}

QList<Staff> DataWarehouse :: staffList() const
{
    return m_staffList;
}

void DataWarehouse :: appendStaff( Staff staff )
{
    m_staffList.append(staff);
}
