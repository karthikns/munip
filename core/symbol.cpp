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

    StemSegment StaffData::stemSegmentForPoint(const QPoint& p, bool &validOutput)
    {
        for (int i = 0; i < stemSegments.size(); ++i) {
            StemSegment seg = stemSegments.at(i);
            QRect rect = seg.boundingRect;
            rect.setLeft(rect.left() - 2);
            rect.setTop(rect.top() - 1);
            rect.setBottom(rect.bottom() + 1);
            if (rect.contains(p)) {
                validOutput = true;
                return seg;
            }
        }
        validOutput = false;
        return StemSegment();
    }

    void StaffData::findBeamsUsingShortestPathApproach()
    {
        qDebug() << Q_FUNC_INFO;
        const QRgb BlackColor = QColor(Qt::black).rgb();

        QImage image = this->image;
        {
            QPainter p(&image);
            p.setBrush(QColor(Qt::white));
            p.setPen(Qt::NoPen);

            for (int i = 0; i < stemSegments.size(); ++i) {
                p.drawRect(stemSegments[i].boundingRect.adjusted(-1, 0, +1, 0));
            }
        }


        int id = 0;
        QSet<QPoint> visited;

        for (int i = 0; i < stemSegments.size(); ++i) {
            const StemSegment& seg = stemSegments.at(i);

            QRect rect = seg.boundingRect;
            const int yStart = (seg.beamAtTop ? rect.top() : rect.bottom());
            const int yEnd = (seg.beamAtTop ? (rect.bottom()) : (rect.top()));
            const int yStep = (seg.beamAtTop ? +1 : -1);


            for (int x = qMin(rect.right() + 1, image.width() - 1);
                    x <= qMin(rect.right() + 2, image.width() - 1); ++x) {

                for (int y = yStart; (seg.beamAtTop ? (y <= yEnd) : (y >= yEnd)); y += yStep) {
                    const QPoint p(x, y);

                    if (image.pixel(p) != BlackColor) continue;

                    if (visited.contains(p)) continue;

                    // if (beamPoints.contains(p)) continue;

                    QList<QPoint> pathPoints;
                    pathPoints << p;
                    visited << p;

                    while (1) {
                        QPoint lastPoint = pathPoints.last();
                        QPoint newp = QPoint(lastPoint.x() + 1, lastPoint.y());
                        if (newp.x() < image.width() && image.pixel(newp) == BlackColor
                                && !visited.contains(newp)) {
                            pathPoints << newp;
                            visited << newp;
                            continue;
                        }

                        newp = QPoint(lastPoint.x(), lastPoint.y() - 1);
                        if (newp.y() >= 0 && image.pixel(newp) == BlackColor
                                && !visited.contains(newp)) {
                            pathPoints << newp;
                            visited << newp;
                            continue;
                        }

                        newp = QPoint(lastPoint.x(), lastPoint.y() + 1);
                        if (newp.y() < image.height() && image.pixel(newp) == BlackColor
                                && !visited.contains(newp)) {
                            pathPoints << newp;
                            visited << newp;
                            continue;
                        }

                        break;
                    }

                    bool valid = false;
                    StemSegment rightSegment = stemSegmentForPoint(pathPoints.last(), valid);
                    valid = (valid && seg != rightSegment);

                    if (valid) {
                        QList<QPoint> result = solidifyPath(pathPoints, seg, rightSegment, visited);
                        QRect boundRect;
                        foreach (const QPoint& resultPt, result) {
                            beamPoints.insert(resultPt, id);
                            if (boundRect.isNull()) {
                                boundRect = QRect(resultPt, resultPt);
                            } else {
                                boundRect |= QRect(resultPt, resultPt);
                            }
                        }
                        qDebug() << "id = " << id
                            << "num points = " << result.size()
                            << "Bound: " << boundRect.topLeft() << boundRect.bottomRight();
                        ++id;
                    }
                }
            }
        }

        qDebug() << Q_FUNC_INFO << "Num of beam segments = " << id << endl;
    }

    QList<QPoint> StaffData::solidifyPath(const QList<QPoint> &pathPoints,
            const StemSegment& left, const StemSegment& right,
            QSet<QPoint> &visited)
    {
        QList<QPoint> result;
        QList<QPoint> lastPassPoints = pathPoints;
        const QRgb BlackColor = QColor(Qt::black).rgb();

        while (1) {
            QList<QPoint> nextPassPoints;

            foreach (const QPoint& p, lastPassPoints) {
                result << p;
                QPoint l = p, t = p, r = p, b = p;
                l.rx()--; t.ry()--; r.rx()++; b.ry()++;

                // Construct neighbors.
                QList<QPoint> pts;
                pts << l << t << r << b;

                foreach (const QPoint& pt, pts) {
                    if (pt.x() > left.boundingRect.right() &&
                            pt.x() < right.boundingRect.left() &&
                            pt.y() >= 0 &&
                            pt.y() < image.height() &&
                            image.pixel(pt) == BlackColor &&
                            !visited.contains(pt)) {
                        visited << pt;
                        nextPassPoints << pt;
                    }
                }
            }

            if (nextPassPoints.isEmpty()) break;
            lastPassPoints = nextPassPoints;
        }

        return result;
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
            QRect rect = seg.rect;
            rect.setLeft(qMax(0, rect.left() - lineHeight));
            rect.setRight(qMin(image.width() - 1, rect.right() + lineHeight));

            int xWithMaxRunLength = -1;
            Run maxRun;
            for (int x = rect.left(); x <= rect.right(); ++x) {
                // Runlength info for x.
                for (int y = rect.top(); y <= rect.bottom(); ++y) {
                    if (image.pixel(x, y) != BlackColor) continue;

                    int runlength = 0;
                    for (; (y + runlength) <= rect.bottom() &&
                            image.pixel(x, y + runlength) == BlackColor; ++runlength);

                    Run r(y, runlength);

                    if (xWithMaxRunLength < 0 || r > maxRun) {
                        xWithMaxRunLength = x;
                        maxRun = r;
                    }

                    y += runlength - 1;
                }
            }

            const int margin = qRound(.8 * maxRun.length);
            int xLimit = qMin(xWithMaxRunLength + lineHeight, rect.right());
            QRect stemRect(xWithMaxRunLength, maxRun.pos, 1, maxRun.length);
            for (int x = xWithMaxRunLength + 1; x <= xLimit; ++x) {
                int count = 0;
                for (int y = maxRun.pos; y < (maxRun.pos + maxRun.length); ++y) {
                    count += (image.pixel(x, y) == BlackColor);
                }

                if (count >= margin) {
                    stemRect.setRight(x);
                } else {
                    break;
                }
            }

            xLimit = qMax(rect.left(), xWithMaxRunLength - lineHeight);
            for (int x = xWithMaxRunLength - 1; x >= xLimit; --x) {
                int count = 0;
                for (int y = maxRun.pos; y < (maxRun.pos + maxRun.length); ++y) {
                    count += (image.pixel(x, y) == BlackColor);
                }

                if (count >= margin) {
                    stemRect.setLeft(x);
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

    void StaffData::extractChords()
    {
        const QRgb BlackColor = QColor(Qt::black).rgb();
        const QRect r = staff.boundingRect();
        const QPoint delta(-r.left(), -r.top());
        QImage modifiedImage(r.size(), QImage::Format_ARGB32_Premultiplied);
        if (1)
        {
            // Erase beam points
            QPainter p(&modifiedImage);
            p.drawImage(QRect(0, 0, r.width(), r.height()), image, r);
            p.setBrush(QColor(Qt::white));
            p.setPen(QColor(Qt::white));
            QHash<QPoint, int>::const_iterator it = beamPoints.constBegin();
            while (it != beamPoints.constEnd()) {
                p.drawPoint(it.key() + delta);
                ++it;
            }

            // Draw stems
            foreach (const StemSegment& s, stemSegments) {
                QRect r = s.boundingRect.adjusted(-1, 0, +1, 0);
                r.translate(delta.x(), delta.y());
                p.drawRect(r);
            }
            p.end();
        }

        DataWarehouse *dw = DataWarehouse::instance();
        const int margin = dw->staffSpaceHeight().min;
        int verticalMargin = dw->staffSpaceHeight().min;
        qDebug() << "Margins: " << margin << verticalMargin;

        QList<NoteHeadSegment>::iterator it = noteHeadSegments.begin();
        for (; it != noteHeadSegments.end(); ++it) {
            NoteHeadSegment &seg = *it;
            QRect segRect = seg.rect.translated(delta);
            QList<int> projHelper;
            for (int y = segRect.top(); y <= segRect.bottom(); ++y) {
                int count = 0;
                for (int x = segRect.left(); x <= segRect.right(); ++x) {
                    count += (modifiedImage.pixel(x, y) == BlackColor);
                }
                if (count < margin) {
                    projHelper << 0;
                } else {
                    projHelper << count;
                }
            }

            for (int i = 0; i < projHelper.size(); ++i) {
                if (projHelper.at(i) == 0) continue;
                int runlength = 0;
                while ((i + runlength) < projHelper.size()) {
                    if (projHelper.at(i+runlength) == 0) break;
                    ++runlength;
                }

                if (runlength < verticalMargin) {
                    for (int j = 0; j < runlength; ++j) {
                        projHelper[i+j] = 0;
                    }

                } else {
                    int midY = segRect.top() + i + (runlength >> 1);
                    QRect rect(segRect.left(), midY - margin, segRect.width(),
                            margin * 2);
                    rect.translate(-delta.x(), -delta.y());
                    seg.noteRects << rect;
                }

                i += runlength - 1;
            }
        }
    }

    QImage StaffData::noteHeadHorizontalProjectioNImage() const
    {
        const QRgb BlackColor = QColor(Qt::black).rgb();
        const QRect r = staff.boundingRect();
        const QPoint delta(-r.left(), -r.top());
        QImage modifiedImage(r.size(), QImage::Format_ARGB32_Premultiplied);
        if (1)
        {
            // Erase beam points
            QPainter p(&modifiedImage);
            p.drawImage(QRect(0, 0, r.width(), r.height()), image, r);
            p.setBrush(QColor(Qt::white));
            p.setPen(QColor(Qt::white));
            QHash<QPoint, int>::const_iterator it = beamPoints.constBegin();
            while (it != beamPoints.constEnd()) {
                p.drawPoint(it.key() + delta);
                ++it;
            }

            // Draw stems
            foreach (const StemSegment& s, stemSegments) {
                QRect r = s.boundingRect.adjusted(-1, 0, +1, 0);
                r.translate(delta.x(), delta.y());
                p.drawRect(r);
            }
            p.end();
        }

        QImage img(r.size(), QImage::Format_ARGB32_Premultiplied);
        img.fill(0xffffffff);

        QPainter p(&img);

        DataWarehouse *dw = DataWarehouse::instance();
        const int margin = dw->staffSpaceHeight().min;
        int verticalMargin = dw->staffSpaceHeight().min;
        qDebug() << "Margins: " << margin << verticalMargin;
        foreach (const NoteHeadSegment& seg, noteHeadSegments) {
            QRect segRect = seg.rect.translated(delta);
            QList<int> projHelper;
            for (int y = segRect.top(); y <= segRect.bottom(); ++y) {
                int count = 0;
                for (int x = segRect.left(); x <= segRect.right(); ++x) {
                    count += (modifiedImage.pixel(x, y) == BlackColor);
                }
                if (count < margin) {
                    projHelper << 0;
                } else {
                    projHelper << count;
                }
            }

            for (int i = 0; i < projHelper.size(); ++i) {
                if (projHelper.at(i) == 0) continue;
                int runlength = 0;
                while ((i + runlength) < projHelper.size()) {
                    if (projHelper.at(i+runlength) == 0) break;
                    ++runlength;
                }

                if (runlength < verticalMargin) {
                    for (int j = 0; j < runlength; ++j) {
                        projHelper[i+j] = 0;
                    }

                }

                i += runlength - 1;
            }

            for (int i = 0; i < projHelper.size(); ++i) {
                int y = segRect.top() + i;
                int count = projHelper.at(i);
                p.setPen(QColor(Qt::red));
                p.drawLine(segRect.left(), y, segRect.left() + count, y);
                p.setPen(QColor(Qt::blue));
                p.drawRect(segRect);
            }
        }


        p.end();
        return img;
    }

}
