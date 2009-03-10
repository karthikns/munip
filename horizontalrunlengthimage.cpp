#include "horizontalrunlengthimage.h"

#include <QDebug>
#include <QImage>

HorizontalRunlengthImage::HorizontalRunlengthImage(const QImage& binaryImage,
                                                    int dataColorIndex) :
    m_dataColorIndex(dataColorIndex),
    m_nonDataColorIndex(1 - dataColorIndex),
    m_imageSize(binaryImage.size())
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
    Q_ASSERT(y >= 0 && y < m_imageSize.height());
    Q_ASSERT(index >= 0 && index < m_data[y].size());
    return m_data[y][index].run;
}

int HorizontalRunlengthImage::runStart(int y, int index) const
{
    Q_ASSERT(y >= 0 && y < m_imageSize.height());
    Q_ASSERT(index >= 0 && index < m_data[y].size());
    return m_data[y][index].x;
}

LocationRunPair HorizontalRunlengthImage::run(int y, int index) const
{
    Q_ASSERT(y >= 0 && y < m_imageSize.height());
    Q_ASSERT(index >= 0 && index < m_data[y].size());
    return m_data[y][index];
}

int HorizontalRunlengthImage::runForPixel(int x, int y) const
{
    Q_ASSERT(y >= 0 && y < m_imageSize.height());
    Q_ASSERT(x >= 0 && x < m_imageSize.width());
    const QVector<LocationRunPair> &row = m_data[y];
    int l = 0, h = row.size() - 1, mid = 0;
    while (l < h) {
        mid = (l + h) / 2;
        if (row[mid].x == x) {
            break;
        } else if (x < row[mid].x) {
            h = mid-1;
        } else {
            l = mid+1;
        }
    }

    int i = -1;
    for (i=mid-1; i <= mid+1; ++i) {
        if (i < 0 || i >= row.size()) {
            continue;
        }

        if (x >= row[i].x && x < row[i].x + row[i].run) {
            return i;
        }
    }

    return -1;
}

int HorizontalRunlengthImage::pixelIndex(int x, int y) const
{
    Q_ASSERT(y >= 0 && y < m_imageSize.height());
    Q_ASSERT(x >= 0 && x < m_imageSize.width());
    if (runForPixel(x, y) != -1) {
        return m_dataColorIndex;
    }
    return m_nonDataColorIndex;
}

bool HorizontalRunlengthImage::isData(int x, int y) const
{
    Q_ASSERT(y >= 0 && y < m_imageSize.height());
    Q_ASSERT(x >= 0 && x < m_imageSize.width());
    return pixelIndex(x, y) == m_dataColorIndex;
}

