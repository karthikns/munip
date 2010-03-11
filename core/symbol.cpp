#include "symbol.h"
#include "datawarehouse.h"

#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QStack>

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

    void StaffData::findNoteHeadSegments()
    {
        DataWarehouse *dw = DataWarehouse::instance();
        int n1_2 = 2 * (dw->staffLineHeight().max);
        noteProjections = filter(Range(1, 100),
                Range(n1_2, n1_2 + dw->staffSpaceHeight().max),
                maxProjections);
        noteProjections = filter(Range(1, 100),
                Range(n1_2 + dw->staffSpaceHeight().min - dw->staffLineHeight().min, 100),
                noteProjections);

        // Removal of false positives.
        QList<int> keys = noteProjections.keys();
        qSort(keys);

        int lineHeight = dw->staffLineHeight().min;
        for (int i = 1; i < keys.size(); ++i) {
            int diff = keys[i] - keys[i-1];
            if (diff == 0) {
                continue;
            }
            if (diff <= (lineHeight>>1)) {
                int valueToInsert = qMin(noteProjections[keys[i]], noteProjections[keys[i-1]]);
                for (int j = keys[i-1] + 1; j < keys[i]; ++j) {
                    noteProjections[j] = valueToInsert;
                }
            }

        }
        for (int i = 0; i < keys.size(); ++i) {
            int runlength = 0;
            int key = keys[i];
            if (noteProjections.value(key) == 0) continue;
            while (i+runlength < keys.size() &&
                    noteProjections.value(keys[i]+runlength, 0) != 0) {
                ++runlength;
            }

            if (runlength < (dw->staffSpaceHeight().min >>1)
                    || runlength >= (dw->staffSpaceHeight().max << 1)) {
                for (int j = 0; j<runlength;j++) {
                    noteProjections[j+keys[i]] = 0;
                }
            }
            i += runlength - 1;

        }


    }

    void StaffData::findBeams()
    {
        DataWarehouse *dw = DataWarehouse::instance();
        const QRgb BlackColor = QColor(Qt::black).rgb();

        QImage image = this->image;
        {
            QPainter p(&image);
            p.setBrush(QColor(Qt::white));
            p.setPen(Qt::NoPen);

            for (int i = 0; i < stemSegments.size(); ++i) {
                p.drawRect(stemSegments[i].boundingRect);
            }
        }
        image.save("test.png");


        for (int i = 0; i < stemSegments.size(); ++i) {
            const StemSegment& seg = stemSegments.at(i);
            const int id = i;

            QRect rect = seg.boundingRect;
            for (int x = qMin(rect.right() + 1, image.width() - 1);
                    x <= qMin(rect.right() + 2, image.width() - 1); ++x) {

                int space = dw->staffSpaceHeight().max * 2;
                int yStart = (seg.beamAtTop ? rect.top() : rect.bottom());
                int yEnd = (seg.beamAtTop ? (rect.top() + space) : (rect.bottom() - space));
                int yStep = (seg.beamAtTop ? +1 : -1);

                for (int y = yStart; (seg.beamAtTop ? (y <= yEnd) : (y >= yEnd)); y += yStep) {
                    const QPoint p(x, y);

                    if (image.pixel(p) != BlackColor) continue;

                    if (beamPoints.contains(p)) continue;

                    QStack<QPoint> stack;
                    stack.push(p);

                    while (!stack.isEmpty()) {
                        QPoint pop = stack.pop();
                        if (beamPoints.contains(pop)) continue;

                        beamPoints.insert(pop, id);
                        QPoint l = pop, t = pop, r = pop, b = pop;
                        l.rx() = qMax(x, l.x()-1);
                        t.ry() = qMax(0, t.y()-1);
                        r.rx() = qMin(image.width()-1, r.x()+1);
                        b.ry() = qMin(image.height()-1, b.y()+1);

                        if (image.pixel(l) == BlackColor && !beamPoints.contains(l)) {
                            stack.push(l);
                        }
                        if (image.pixel(t) == BlackColor && !beamPoints.contains(t)) {
                            stack.push(t);
                        }
                        if (image.pixel(r) == BlackColor && !beamPoints.contains(r)) {
                            stack.push(r);
                        }
                        if (image.pixel(b) == BlackColor && !beamPoints.contains(b)) {
                            stack.push(b);
                        }
                    }
                }
            }
        }


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
        const QRgb BlackColor = QColor(Qt::black).rgb();
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
            NoteHeadSegment n;
            // n.rect = QRect(xCenter - noteWidth, top, noteWidth * 2, height);
            n.rect = QRect(xCenter - (noteWidth >> 1), top, noteWidth, height);
            // TODO: BoundingRect should include beams, but thats not being drawn. Check that.
            //n.rect = QRect(key, top, runlength, height);

            noteHeadSegments << n;
        }

        qSort(noteHeadSegments);
        //qDebug() << noteProjections.keys();
        //qDebug() << noteProjections.values();
    }

    void StaffData::extractStemSegments()
    {
        const QRgb BlackColor = QColor(Qt::black).rgb();
        DataWarehouse *dw = DataWarehouse::instance();
        const int lineHeight = dw->staffLineHeight().min * 2;

        foreach (const NoteHeadSegment& seg, noteHeadSegments) {
            QList<int> projData;
            int maxCountIndex = -1;
            QRect rect = seg.rect;
            rect.setLeft(qMax(0, rect.left() - lineHeight));
            rect.setRight(qMin(image.width(), rect.right() + lineHeight));

            for (int x = rect.left(); x <= rect.right(); ++x) {
                int count = 0;
                for (int y = rect.top(); y <= rect.bottom(); ++y) {
                    count += (image.pixel(x, y) == BlackColor);
                }
                projData << count;
                if (maxCountIndex < 0) {
                    maxCountIndex = x - rect.left();
                } else {
                    if (count > projData[maxCountIndex]) {
                        maxCountIndex = x - rect.left();
                    }
                }
            }

            QRect stemRect(maxCountIndex + rect.left(), staff.boundingRect().top(),
                    1, staff.boundingRect().height());
            int margin = dw->staffSpaceHeight().min >> 1;
            for (int x = maxCountIndex; x < qMin(maxCountIndex + lineHeight, projData.size()-1); ++x) {
                if (qAbs(projData[x] - projData[maxCountIndex]) <= margin) {
                    stemRect.setRight(x + rect.left());
                } else {
                    break;
                }
            }

            for (int x = maxCountIndex; x >= qMax(maxCountIndex - lineHeight, 0); --x) {
                if (qAbs(projData[x] - projData[maxCountIndex]) <= margin) {
                    stemRect.setLeft(x + rect.left());
                } else {
                    break;
                }
            }

            StemSegment stemSeg;
            stemSeg.boundingRect = stemRect;
            stemSeg.noteHeadSegment = seg;

            int lDist = qAbs(seg.rect.left() - stemSeg.boundingRect.left());
            int rDist = qAbs(seg.rect.right() - stemSeg.boundingRect.left());

            // == cond not thought, but guess not needed.
            stemSeg.beamAtTop = (lDist > rDist);
            stemSegments << stemSeg;

        }

        qSort(stemSegments);
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
