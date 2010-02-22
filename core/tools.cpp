#include "tools.h"

#include <cmath>

namespace Munip
{
    int IDGenerator::lastID = -1;

    QImage convertToMonochrome(const QImage& image, int threshold)
    {
        if (image.format() == QImage::Format_Mono) {
            return image;
        }

        int h = image.height();
        int w = image.width();

        QImage monochromed(w, h, QImage::Format_Mono);
        uchar *destData = monochromed.bits();
        int destBytes = monochromed.numBytes();
        memset(destData, 0, destBytes);

        const int White = 0, Black = 1;
        // Ensure above index values to color table
        monochromed.setColor(0, 0xffffffff);
        monochromed.setColor(1, 0xff000000);

        for(int x = 0; x < w; ++x) {
            for(int y = 0; y < h; ++y) {
                bool isWhite = (qGray(image.pixel(x, y)) > threshold);
                int color = isWhite ? White : Black;
                monochromed.setPixel(x, y, color);
            }
        }

        return monochromed;
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

    bool segmentSortByWeight(Segment &s1,Segment &s2)
    {
         return s1.weight()>s2.weight();
    }

    bool segmentSortByConnectedComponentID(Segment &s1,Segment &s2)
    {
        return (s1.connectedComponentID() < s2.connectedComponentID())||( (s1.connectedComponentID() == s2.connectedComponentID() ) && (s1.weight() > s2.weight() ) );

    }

    bool segmentSortByPosition(Segment &s1,Segment &s2)
    {
        //return ((s1.startPos().x() < s2.startPos().x())||((s1.startPos().x() == s2.startPos().x()) && (s1.startPos().y() < s2.startPos().y())));

        return ((s1.startPos().y() < s2.startPos().y())||((s1.startPos().y() == s2.startPos().y()) && (s1.startPos().x() < s2.startPos().x())));
    }

    bool staffLineSort(StaffLine &line1,StaffLine &line2)
    {
        return line1.startPos().y() < line2.startPos().y();
    }

    bool symbolRectSort(QRect &symbolRect1,QRect &symbolRect2)
    {
        return ( ( symbolRect1.topLeft().x() < symbolRect2.topLeft().x() ) || ( symbolRect1.topLeft().x() == symbolRect2.topLeft().x() && symbolRect1.topLeft().y() < symbolRect2.topLeft().y() ) );
    }

     double normalizedLineTheta(const QLineF& line)
    {
        double angle = line.angle();
        while (angle < 0.0) angle += 360.0;
        while (angle > 360.0) angle -= 360.0;
        if (angle > 180.0) angle -= 360.0;
        return angle < 0.0 ? -angle : angle;
    }
}
