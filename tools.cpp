#include "tools.h"

#include <cmath>

namespace Munip
{
    int IDGenerator::lastID = -1;

    QImage convertToMonochrome(const QImage& image)
    {
        if (image.format() == QImage::Format_Mono) {
            return image;
        }
        // First convert to 8-bit image.
        QImage converted = image.convertToFormat(QImage::Format_Indexed8);

        // Modify colortable to our own monochrome
        QVector<QRgb> colorTable = converted.colorTable();
        const int threshold = 240;
        for(int i = 0; i < colorTable.size(); ++i) {
            int gray = qGray(colorTable[i]);
            if(gray > threshold) {
                gray = 255;
            }
            else {
                gray = 0;
            }
            colorTable[i] = qRgb(gray, gray, gray);
        }
        converted.setColorTable(colorTable);
        // convert to 1-bit monochrome
        converted = converted.convertToFormat(QImage::Format_Mono);
        return converted;
    }

    QPointF meanOfPoints(const QList<QPoint>& pixels)
    {
        QPointF mean;

        foreach(const QPoint &pixel, pixels) {
            mean.rx() += pixel.x();
            mean.ry() += pixel.y();
        }

        if(pixels.size()==0)
            return mean;

        mean /= pixels.size();
        return mean;
    }


    QList<double> covariance(const QList<QPoint>& blackPixels, QPointF mean)
    {
        QList<double> varianceMatrix;
        varianceMatrix << 0 << 0 << 0 << 0;

        double &vxx = varianceMatrix[0];
        double &vxy = varianceMatrix[1];
        double &vyx = varianceMatrix[2];
        double &vyy = varianceMatrix[3];


        foreach(const QPoint& pixel, blackPixels) {
            vxx += ( pixel.x() - mean.x() ) * ( pixel.x() - mean.x() );
            vyx = vxy += ( pixel.x() - mean.x() ) * ( pixel.y() - mean.y() );
            vyy += ( pixel.y() - mean.y() ) * ( pixel.y() - mean.y() );
        }

        if (!blackPixels.isEmpty()) {
            vxx /= blackPixels.size();
            vxy /= blackPixels.size();
            vyx /= blackPixels.size();
            vyy /= blackPixels.size();
        }

        return varianceMatrix;
    }


    double highestEigenValue(const QList<double> &matrix)
    {
        double a = 1;
        double b = -( matrix[0] + matrix[3] );
        double c = (matrix[0]  * matrix[3]) - (matrix[1] * matrix[2]);
        double D = b * b - 4*a*c;
        Q_ASSERT(D >= 0);
        double lambda = (-b + std::sqrt(D))/(2*a);
        return lambda;
    }
}
