#ifndef MONOIMAGE_H
#define MONOIMAGE_H

#include <QImage>

class MonoImage : public QImage
{
public:
    enum MonoColor {
        Black = 0,
        White = 1
    };

    MonoImage();
    MonoImage(const MonoImage& monoImage);
    MonoImage(const QImage &imageRef);

    MonoImage& operator=(const QImage& other);
    MonoImage& operator=(const MonoImage& other);

    MonoColor pixelValue(int x, int y);
    void setPixelValue(int x, int y, MonoColor color);

private:
    void calcIndicies();

    int m_blackIndex;
    int m_whiteIndex;
};

#endif
