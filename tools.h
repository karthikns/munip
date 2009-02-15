#ifndef TOOLS_H
#define TOOLS_H

#include <QImage>

namespace Munip
{
    struct IDGenerator
    {
        static int lastID;

        static int gen()
        {
            return ++lastID;
        }
    };

    QImage convertToMonochrome(const QImage& image);
    QPointF meanOfPoints(const QList<QPoint>& pixels);
    QList<double> covariance(const QList<QPoint>& blackPixels, QPointF mean);
    double highestEigenValue(const QList<double> &matrix);
}

#endif
