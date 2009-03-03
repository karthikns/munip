#ifndef HORIZONTALRUNLENGTHIMAGE_H
#define HORIZONTALRUNLENGTHIMAGE_H

#include <QPair>
#include <QVector>

class QImage;

struct LocationRunPair : public QPair<int, int>
{
    int &x;
    int &run;

    LocationRunPair(int _x=0, int _run=0) : 
        QPair<int,int>(_x, _run),
        x(first),
        run(second)
    {}

    LocationRunPair(const LocationRunPair& rhs) : 
        QPair<int,int>(rhs),
        x(first),
        run(second)
    {
    }

    LocationRunPair& operator=(const LocationRunPair& rhs)
    {
        QPair<int,int>::operator=(rhs);
        return *this;
    }
};

struct HorizontalRunlengthImage
{
    HorizontalRunlengthImage(const QImage& binaryImage, int dataColorIndex);

    int runLength(int y, int index) const;
    int runStart(int y, int index) const;
    LocationRunPair run(int y, int index) const;

    int pixelIndex(int x, int y) const;
    bool isData(int x, int y) const;

    QVector<QVector<LocationRunPair> >  m_data;
    int m_dataColorIndex;
    int m_nonDataColorIndex;
};

#endif

