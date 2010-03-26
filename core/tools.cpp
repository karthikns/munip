#include "tools.h"

#include <cmath>

bool EnableMDebugOutput = true;

namespace Munip
{
    int IDGenerator::lastID = -1;

    static void initializeHoriontalRunlengthImage(const QImage& image, const QColor& color,
            QList<QList<Run> > &dataRef)
    {
        const QRgb data = color.rgb();
        resizeList(dataRef, image.height(), QList<Run>());

        for (int y = 0; y < image.height(); ++y) {
            QList<Run> &row = dataRef[y];
            row.clear();

            for (int x = 0; x < image.width(); ++x) {
                if (image.pixel(x, y) != data) continue;

                int runlength = 0;
                for (; (x + runlength) < image.width(); ++runlength) {
                    if (image.pixel(x + runlength, y) != data) break;
                }

                row << Run(x, runlength);
                x += runlength - 1;
            }
        }
    }

    static void initializeVerticalRunlengthImage(const QImage& image, const QColor& color,
            QList<QList<Run> > &dataRef)
    {
        const QRgb data = color.rgb();
        resizeList(dataRef, image.width(), QList<Run>());

        for (int x = 0; x < image.width(); ++x) {
            QList<Run> &col = dataRef[x];
            col.clear();

            for (int y = 0; y < image.height(); ++y) {
                if (image.pixel(x, y) != data) continue;

                int runlength = 0;
                for (; (y + runlength) < image.height(); ++runlength) {
                    if (image.pixel(x, y + runlength) != data) break;
                }

                col << Run(y, runlength);
                y += runlength - 1;
            }
        }
    }

    const QList<Run> RunlengthImage::InvalidRuns = QList<Run>();

    RunlengthImage::RunlengthImage(const QImage& image,
            Qt::Orientation orientation, const QColor& color) :
        m_orientation(orientation),
        m_size(image.size())
    {
        if (m_orientation == Qt::Horizontal) {
            initializeHoriontalRunlengthImage(image, color, m_data);
        } else {
            initializeVerticalRunlengthImage(image, color, m_data);
        }
    }

    RunlengthImage::~RunlengthImage()
    {
    }

    Qt::Orientation RunlengthImage::orientation() const
    {
        return m_orientation;
    }

    QSize RunlengthImage::size() const
    {
        return m_size;
    }

    QRect RunlengthImage::rect() const
    {
        return QRect(QPoint(0, 0), m_size);
    }

    const QList<Run>& RunlengthImage::runs(int index) const
    {
        if (index < 0 || index >= m_data.size()) {
            // needed because, this method returns reference.
            return RunlengthImage::InvalidRuns;
        }

        return m_data.at(index);
    }

    Run RunlengthImage::run(int x, int y) const
    {
        Run retval;
        if (x < 0 || x >= m_size.width()) return retval;
        if (y < 0 || y >= m_size.height()) return retval;

        const QList<Run>& runsRef =
            (m_orientation == Qt::Horizontal ? runs(y) : runs(x));

        if (runsRef.isEmpty()) return retval;

        int searchCoord = (m_orientation == Qt::Horizontal ? x : y);

        int l = 0, h = runsRef.size() - 1, mid = 0;

        while (l <= h) {
            mid = (l + h) / 2;
            const Run midRun = runsRef.at(mid);

            if (searchCoord >= midRun.pos && searchCoord <= midRun.endPos()) {
                retval = midRun;
                break;
            } else if (searchCoord < midRun.pos) {
                h = mid - 1;
            } else {
                l = mid + 1;
            }
        }

        return retval;
    }

    QList<Run> RunlengthImage::adjacentRunsInNextLine(const RunCoord& runCoord) const
    {
        QList<Run> retval;

        if (runCoord.pos < 0 || runCoord.pos >= (m_data.size() - 1)) return retval;

        const int nextCoord = runCoord.pos + 1;

        for (int i = runCoord.run.pos; i < runCoord.run.endPos(); ++i) {
            Run r;
            if (m_orientation == Qt::Horizontal) {
                r = run(i, nextCoord);
            } else {
                r = run(nextCoord, i);
            }

            if (r.isValid()) {
                retval << r;
                i = r.endPos();
            }
        }

        return retval;
    }

    QList<Run> RunlengthImage::adjacentRunsInPreviousLine(const RunCoord& runCoord) const
    {
        QList<Run> retval;

        if (runCoord.pos <= 0 || runCoord.pos >= m_data.size()) return retval;

        const int previousCoord = runCoord.pos - 1;

        for (int i = runCoord.run.pos; i < runCoord.run.endPos(); ++i) {
            Run r;
            if (m_orientation == Qt::Horizontal) {
                r = run(i, previousCoord);
            } else {
                r = run(previousCoord, i);
            }

            if (r.isValid()) {
                retval << r;
                i = r.endPos();
            }
        }

        return retval;
    }

    VerticalRunlengthImage::VerticalRunlengthImage(const QImage& image,
            const QColor& color) : RunlengthImage(image, Qt::Vertical, color)
    {
    }

    VerticalRunlengthImage::~VerticalRunlengthImage()
    {
    }

    const QList<Run>& VerticalRunlengthImage::runsForColumn(int index) const
    {
        return runs(index);
    }

    QList<Run> VerticalRunlengthImage::adjacentRunsInNextColumn(const RunCoord& runCoord) const
    {
        return adjacentRunsInNextLine(runCoord);
    }

    QList<Run> VerticalRunlengthImage::adjacentRunsInPreviousColumn(const RunCoord& runCoord) const
    {
        return adjacentRunsInPreviousLine(runCoord);
    }

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
        monochromed.setColor(White, 0xffffffff);
        monochromed.setColor(Black, 0xff000000);

        for(int x = 0; x < w; ++x) {
            for(int y = 0; y < h; ++y) {
                bool isWhite = (qGray(image.pixel(x, y)) > threshold);
                int colorIndex = isWhite ? White : Black;
                monochromed.setPixel(x, y, colorIndex);
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
        return (s1.connectedComponentID() < s2.connectedComponentID()) ||
            ((s1.connectedComponentID() == s2.connectedComponentID()) &&
             (s1.weight() > s2.weight()));

    }

    bool segmentSortByPosition(Segment &s1,Segment &s2)
    {
        //return ((s1.startPos().x() < s2.startPos().x()) ||
        //((s1.startPos().x() == s2.startPos().x()) &&
        //(s1.startPos().y() < s2.startPos().y())));

        return ((s1.startPos().y() < s2.startPos().y())
                || ((s1.startPos().y() == s2.startPos().y()) &&
                    (s1.startPos().x() < s2.startPos().x())));
    }

    bool staffLineSort(StaffLine &line1,StaffLine &line2)
    {
        return line1.startPos().y() < line2.startPos().y();
    }

    bool symbolRectSort(QRect &symbolRect1,QRect &symbolRect2)
    {
        return ((symbolRect1.topLeft().x() < symbolRect2.topLeft().x()) ||
                (symbolRect1.topLeft().x() == symbolRect2.topLeft().x() &&
                 symbolRect1.topLeft().y() < symbolRect2.topLeft().y()));
    }
}
