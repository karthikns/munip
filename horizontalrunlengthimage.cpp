#include "horizontalrunlengthimage.h"

#include <QImage>

HorizontalRunlengthImage::HorizontalRunlengthImage(const QImage& binaryImage,
                                                    int dataColorIndex) :
    m_dataColorIndex(dataColorIndex),
    m_nonDataColorIndex(1 - dataColorIndex)
{
    Q_ASSERT(binaryImage.format() == QImage::Format_Mono);
    m_data.resize(binaryImage.height());
    const int width = binaryImage.width();
    for (int y = 0; y < binaryImage.height(); ++y) {
        int x = 0;
        while (x < width) {
            if (binaryImage.pixelIndex(x, y) == dataColorIndex) {
                int start = x;
                int length = 1;
                while (start + length < width &&
                        binaryImage.pixelIndex(start+length, y) == dataColorIndex)
                    ++length;

                m_data[y].append(LocationRunPair(x, length));
                x += length;
            } else {
                ++x;
            }
        }
    }
}

int HorizontalRunlengthImage::runLength(int y, int index) const
{
    return m_data[y][index].run;
}

int HorizontalRunlengthImage::runStart(int y, int index) const
{
    return m_data[y][index].x;
}

LocationRunPair HorizontalRunlengthImage::run(int y, int index) const
{
    return m_data[y][index];
}

int HorizontalRunlengthImage::pixelIndex(int x, int y) const
{
    const QVector<LocationRunPair> &row = m_data[y];
    int l = 0, h = row.size() - 1, mid = 0;
    while (l < h) {
        if (row[mid].x == x) {
            break;
        } else if (x < row[mid].x) {
            h = mid-1;
        } else {
            l = mid+1;
        }
    }

    if (row[mid].x == x) {
        return m_dataColorIndex;
    }

    if (l-1 < 0) {
        return m_nonDataColorIndex;
    } else if (x < row[l-1].x + row[l-1].run) {
        return m_dataColorIndex;
    } else {
        return m_nonDataColorIndex;
    }
}

bool HorizontalRunlengthImage::isData(int x, int y) const
{
    return pixelIndex(x, y) == m_dataColorIndex;
}

