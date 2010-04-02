#ifndef DATAWAREHOUSE_H
#define DATAWAREHOUSE_H

#include "staff.h"
#include "tools.h"

#include<QList>
#include<QPair>
#include<QImage>

namespace Munip {

    class StaffData;

    class DataWarehouse
    {
    public:
        static DataWarehouse* instance();
        float pageSkew() const;
        void setPageSkew(float skew);

        float pageSkewPrecison() const;
        void setPageSkewPrecision(float precision);

        QSize resolution()  const;

        void clearStaff();
        void appendStaff( Staff staff );
        QList<Staff> staffList() const;

        QImage workImage() const;
        void setWorkImage(const QImage& image);

        Range staffSpaceHeight() const;
        void setStaffSpaceHeight(const Range& value);

        Range staffLineHeight() const;
        void setStaffLineHeight(const Range& value);

        QList<StaffData*> staffDatas() const;
        void setStaffDatas(const QList<StaffData*> &sd);

        QImage imageWithRemovedStaffLinesOnly() const;
        QImage& imageRefWithRemovedStaffLinesOnly();

    private:
        DataWarehouse();
        static DataWarehouse* m_dataWarehouse;
        float m_pageSkew;
        float m_pageSkewPrecision;

        Range m_staffSpaceHeight;
        Range m_staffLineHeight;

        QImage m_workImage;
        QImage m_imageWithRemovedStaffLinesOnly;

        uint m_lineSize;
        QList<Staff> m_staffList;
        QList<StaffData*> m_staffDatas;
    };
}
#endif
