#include "monoimage.h"

#include "tools.h"
#include <QDebug>

MonoImage::MonoImage()
{
    m_blackIndex = m_whiteIndex = -1;
}

MonoImage::MonoImage(const QImage &image) : QImage(Munip::convertToMonochrome(image))
{
    calcIndicies();
}

MonoImage::MonoImage(const MonoImage &monoImg) : QImage(monoImg)
{
    calcIndicies();
}

MonoImage& MonoImage::operator=(const MonoImage& other)
{
    QImage::operator=(other);
    calcIndicies();
    return *this;
}

MonoImage& MonoImage::operator=(const QImage& other)
{
    QImage::operator=(other);
    calcIndicies();
    return *this;
}

void MonoImage::calcIndicies()
{
    // In colortable index 0 might represent white. To take care
    // of that we have following.
    QVector<QRgb> colorTable = QImage::colorTable();
    Q_ASSERT(colorTable.size() >= 2);
    m_blackIndex = qGray(colorTable[0]) == 0 ? 0 : 1;
    m_whiteIndex = 1 - m_blackIndex;
}

MonoImage::MonoColor MonoImage::pixelValue(int x, int y) const
{
    int pixelIndex = QImage::pixelIndex(x, y);
    return pixelIndex == m_blackIndex ?
        MonoImage::Black :
        MonoImage::White;
}

void MonoImage::setPixelValue(int x, int y, MonoImage::MonoColor color)
{
    int pixelIndex = (color == MonoImage::Black) ? m_blackIndex : m_whiteIndex;
    QImage::setPixel(x, y, pixelIndex);
}
