#include "symbol.h"
#include "datawarehouse.h"
#include "tools.h"

#include <QColor>
#include <QDebug>
#include <QPainter>

namespace Munip
{
    StaffData::StaffData(const QImage& img, const Staff& stf) :
        staff(stf),
        image(img)
    {
        SlidingWindowSize = DataWarehouse::instance()->staffLineHeight().max;
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
        DataWarehouse *dw = DataWarehouse::instance();

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

        return;
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
        int n1_2 = 2 * (dw->staffLineHeight().max);
        noteProjections = filter(Range(1, 100),
                Range(n1_2, n1_2 + dw->staffSpaceHeight().max),
                maxProjections);
        noteProjections = filter(Range(1, 100),
                Range(n1_2 + dw->staffSpaceHeight().min - dw->staffLineHeight().min, 100),
                noteProjections);
        //qDebug() << Q_FUNC_INFO << noteProjections.values();
    }

    void StaffData::findStems()
    {
        DataWarehouse *dw = DataWarehouse::instance();
        int n1_2 = 2 * (dw->staffLineHeight().max);
        stemsProjections = filter(Range(1, 100),
                Range(n1_2 + dw->staffSpaceHeight().max, 100),
                maxProjections);
    }

    QHash<int, int> StaffData::filter(Range , Range height,
            const QHash<int, int> &hash)
    {
        QList<int> allKeys = hash.keys();
        qSort(allKeys);

        QHash<int, int> retval;

        //qDebug() << Q_FUNC_INFO;
        //qDebug() << height.min << height.max;

        for (int i = 0; i < allKeys.size(); ++i) {
            if (hash[allKeys[i]] < height.min) continue;
            if (hash[allKeys[i]] > height.max) {
                retval[allKeys[i]] = retval[allKeys[i]-1];
            } else {
                retval[allKeys[i]] = hash[allKeys[i]];
            }
        }
        //qDebug() << retval;

        return retval;
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
                if (horProjValues[i+runLength] < peak) break;//!= horProjValues[i]) break;
            }

            i += runLength - 1;
            maxRun = qMax(maxRun, runLength);
        }

        //qDebug() << Q_FUNC_INFO << maxRun << endl;
        return maxRun;
    }

    void StaffData::extractNoteHeadSegments()
    {
        noteHeadSegments.clear();
        QList<int> noteKeys = noteProjections.keys();
        qSort(noteKeys);

        int top = staff.boundingRect().top();
        int height = staff.boundingRect().height();
        int noteWidth = 2 * DataWarehouse::instance()->staffSpaceHeight().min;

        //qDebug() << Q_FUNC_INFO;
        for (int i = 0; i < noteKeys.size(); ++i) {
            int runlength = 0;
            int key = noteKeys[i];
            if (noteProjections.value(key) == 0) continue;
            while (i+runlength < noteKeys.size() &&
                    noteProjections.value(noteKeys[i]+runlength, 0) != 0) {
                ++runlength;
            }

            i += runlength - 1;

            int xCenter = key + (runlength >> 1);
            NoteHead n;
            // n.rect = QRect(xCenter - noteWidth, top, noteWidth * 2, height);
            n.rect = QRect(key, top, runlength, height);
            noteHeadSegments << n;
        }

        qSort(noteHeadSegments);
        //qDebug() << noteProjections.keys();
        //qDebug() << noteProjections.values();
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

    QImage StaffData::projectionImage(const QHash<int, int> &hash) const
    {
        //qDebug() << Q_FUNC_INFO;
        const QRect r = staff.boundingRect();
        DataWarehouse *dw = DataWarehouse::instance();

        QImage img(r.size(), QImage::Format_ARGB32_Premultiplied);
        img.fill(0xffffffff);

        int xOffset = -r.left();
        int yOffset = -r.top();

        QPainter p(&img);
        p.setPen(Qt::red);
        for (int x = r.left(); x <= r.right(); ++x) {
            QLineF line(x + xOffset, r.bottom() + yOffset,
                    x + xOffset, r.bottom() + yOffset - hash[x]);
            p.drawLine(line);
        }
        p.end();

        return img;
    }

}
