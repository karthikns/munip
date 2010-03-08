#include "symbol.h"
#include "datawarehouse.h"

#include <QColor>
#include <QDebug>
#include <QPainter>

namespace Munip
{
    const int StaffData::SlidingWindowSize = 3;

    StaffData::StaffData(const QImage& img, const Staff& stf) :
        staff(stf),
        image(img)
    {
    }

    void StaffData::findSymbolRegions()
    {
        QRect r = staff.boundingRect();
        const QRgb blackColor = QColor(Qt::black).rgb();

        QRect symbolRect;
        for (int x = r.left(); x <= r.right(); ++x) {
            int count = 0;
            for (int y = r.top(); y <= r.bottom(); ++y) {
                if (y < image.height()) {
                    if (image.pixel(x, y) == blackColor) {
                        count++;
                    }
                }
            }

            bool isSymbolLine = (count >= 2);
            if (isSymbolLine) {
                if (symbolRect.isNull()) {
                    symbolRect = QRect(QPoint(x, r.top()), QPoint(x, r.bottom()));
                } else {
                    symbolRect.setRight(x);
                }
            } else {
                if (!symbolRect.isNull()) {
                    symbolRects << symbolRect;
                }
                symbolRect = QRect();
            }
        }

        if (!symbolRect.isNull()) {
            symbolRects << symbolRect;
        }
    }

    void StaffData::findMaxProjections()
    {
        QRect r = staff.boundingRect();
        const QRgb BlackColor = QColor(Qt::black).rgb();

        foreach (const QRect &sr, symbolRects) {
            if (sr.width() < SlidingWindowSize) continue;

            for (int x = sr.left(); x <= sr.right() - SlidingWindowSize; ++x) {
                QList<int> projectionHelper;

                for (int y = sr.top(); y <= sr.bottom(); ++y) {
                    int count = 0;

                    for (int i = 0; i < SlidingWindowSize; ++i) {
                        count += (image.pixel(x+i, y) == BlackColor);
                    }
                    projectionHelper << count;

                }

                int peakHValue = determinePeakHValueFrom(projectionHelper);
                for (int i = 0; i < SlidingWindowSize; ++i) {
                    maxProjections[x+i] = qMax(maxProjections[x+i], peakHValue);
                }
            }
        }

        qDebug() << Q_FUNC_INFO;
        QList<int> keys = maxProjections.keys();
        qSort(keys);
        foreach (int k, keys) {
            qDebug() << k << maxProjections[k] << endl;
        }

        qDebug() << endl;
    }

    void StaffData::findNoteHeads()
    {
        DataWarehouse *dw = DataWarehouse::instance();
        noteProjections = extract(dw->staffSpaceHeight().dominantValue());
    }

    void StaffData::findStems()
    {
        DataWarehouse *dw = DataWarehouse::instance();
        stemsProjections = extract(dw->staffLineHeight().dominantValue());
    }

    int StaffData::determinePeakHValueFrom(const QList<int>& horProjValues)
    {
        //qDebug() << Q_FUNC_INFO << horProjValues;
        const int peak = SlidingWindowSize - 1;
        int maxRun = 0;
        for (int i = 0; i < horProjValues.size(); ++i) {
            if (horProjValues[i] < peak) continue;

            int runLength = 0;
            for (; (i + runLength) < horProjValues.size(); ++runLength) {
                if (horProjValues[i+runLength] != horProjValues[i]) break;
            }

            i += runLength - 1;
            maxRun = qMax(maxRun, runLength);
        }

        //qDebug() << Q_FUNC_INFO << maxRun << endl;
        return maxRun;
    }

    QImage StaffData::staffImage() const
    {
        const QRect r = staff.boundingRect();
        QImage img(r.size(), QImage::Format_ARGB32_Premultiplied);
        img.fill(0xffffffff);

        QPainter p(&img);
        p.drawImage(QRect(0, 0, r.width(), r.height()), image, r);
        p.end();

        return img;
    }

    QImage StaffData::projectionImage() const
    {
        qDebug() << Q_FUNC_INFO;
        const QRect r = staff.boundingRect();
        DataWarehouse *dw = DataWarehouse::instance();

        QImage img(r.size(), QImage::Format_ARGB32_Premultiplied);
        img.fill(0xffffffff);

        int xOffset = -r.left();
        int yOffset = -r.top();

        QPainter p(&img);
        p.setPen(Qt::red);
        const int limit = dw->staffSpaceHeight().dominantValue() + 0.5 * dw->staffLineHeight().min;
        for (int x = r.left(); x <= r.right(); ++x) {
            QLineF line(x + xOffset, r.bottom() + yOffset,
                    x + xOffset, r.bottom() + yOffset - maxProjections[x]);
            if (maxProjections[x] < limit) continue;
            p.drawLine(line);
        }
        p.end();

        return img;
    }

    QHash<int, int> StaffData::extract(int width)
    {
        QList<int> allKeys = maxProjections.keys();
        qSort(allKeys);

        QHash<int, int> retval;

        for (int i = 0; i < allKeys.size(); ++i) {
            int runLength = 1;
            for (; (i + runLength) < allKeys.size(); ++runLength) {
                if (maxProjections[allKeys[i+runLength]] < maxProjections[allKeys[i]])
                    break;
            }
            if (runLength >= width) {
                for (int j = 0; j < runLength; ++j) {
                    retval[i+j] = maxProjections[allKeys[i+j]];
                }
            }
            i += runLength - 1;
        }
        return retval;
    }
}
