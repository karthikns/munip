#ifndef HORIZONTALRUNLENGTHIMAGE_H
#define HORIZONTALRUNLENGTHIMAGE_H

#include "tools.h"

#include <QPair>
#include <QSize>
#include <QVector>

class QImage;

namespace Munip
{
    class HorizontalRunlengthImage
    {
    public:
        HorizontalRunlengthImage(const QImage& binaryImage, int dataColorIndex);

        int runLength(int y, int index) const;
        int runStart(int y, int index) const;
        Run run(int y, int index) const;

        int runForPixel(int x, int y) const;
        int pixelIndex(int x, int y) const;
        bool isData(int x, int y) const;

    private:

        QVector<QVector<Run> >  m_data;
        int m_dataColorIndex;
        int m_nonDataColorIndex;
        QSize m_imageSize;
    };
}

#endif

