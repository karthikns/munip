#ifndef DATAWAREHOUSE_H
#define DATAWAREHOUSE_H
#include "staff.h"
#include<QList>
#include<QPair>
#include<QImage>

namespace Munip {

    class DataWarehouse
    {
    public:
        static DataWarehouse* instance();
        float pageSkew() const;
        void setPageSkew(float skew);

        float pageSkewPrecison() const;
        void setPageSkewPrecision(float precision);

        uint lineSize() const;
        void setLineSize(uint lineSize);

        QSize resolution()  const;

        void appendStaff( Staff staff );
        QList<Staff> staffList() const;

        QImage workImage() const;
        void setWorkImage(const QImage& image);

    private:
        DataWarehouse();
        static DataWarehouse* m_dataWarehouse;
        float m_pageSkew;
        float m_pageSkewPrecision;
        QImage m_workImage;

        uint m_lineSize;
        QList<Staff> m_staffList;
    };
}
#endif
