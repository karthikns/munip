#include "tools.h"

namespace Munip
{
    QImage convertToMonochrome(const QImage& image)
    {
        if (image.format() == QImage::Format_Mono) {
            return image;
        }
        // First convert to 8-bit image.
        QImage converted = image.convertToFormat(QImage::Format_Indexed8);

        // Modify colortable to our own monochrome
        QVector<QRgb> colorTable = converted.colorTable();
        const int threshold = 200;
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
}
