#include "symbol.h"
#include "datawarehouse.h"

#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QStack>

namespace Munip
{
    static bool lessThanNoteSegmentPointers(const NoteSegment *l,
            const NoteSegment *r)
    {
        return l->boundingRect.left() < r->boundingRect.left();
    }

    static bool lessThanStemSegmentPointers(const StemSegment* l,
            const StemSegment* r)
    {
        return l->boundingRect.left() < r->boundingRect.left();
    }

    QDebug operator<<(QDebug dbg, const NoteSegment* seg)
    {
        dbg.nospace() << "NoteSegment: [" << (void*)seg << "] " << seg->boundingRect;
        return dbg.space();
    }

    QDebug operator<<(QDebug dbg, const StemSegment* seg)
    {
        dbg.nospace() << "StemSegment: [" << (void*)seg << "] " << seg->boundingRect;
        return dbg.space();
    }

    StaffData::StaffData(const QImage& img, const Staff& stf) :
        staff(stf),
        image(img)
    {
        // SlidingWindowSize should atleast be 4 pixels wide,
        // else it results in lots of false positives.
        SlidingWindowSize = qMax(4, DataWarehouse::instance()->staffLineHeight().max);
        workImage = staffImage();
    }

    StaffData::~StaffData()
    {
        // First delete stem segments as they store pointer to note segment.
        qDeleteAll(stemSegments);
        stemSegments.clear();

        qDeleteAll(noteSegments);
        noteSegments.clear();
    }

    /**
     * This function is very impt as it decides the order in which the processing has to happen.
     * Each and every step has precise dependency and so this order should be preservered.
     */
    void StaffData::process()
    {
        findSymbolRegions();

        findMaxProjections();

        extractNoteSegments();

        extractStemSegments();

        eraseStems();
        extractBeams();

        eraseBeams();
        extractChords();

        eraseChords();
        extractFlags();

        eraseFlags();
    }

    void StaffData::findSymbolRegions()
    {
        QRect r = workImage.rect();
        const QRgb BlackColor = QColor(Qt::black).rgb();

        QRect symbolRect;
        for (int x = r.left(); x <= r.right(); ++x) {
            int count = 0;
            for (int y = r.top(); y <= r.bottom(); ++y) {
                count += (workImage.pixel(x, y) == BlackColor);
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
        QRect r = workImage.rect();
        const QRgb BlackColor = QColor(Qt::black).rgb();

        foreach (const QRect &sr, symbolRects) {
            if (sr.width() < SlidingWindowSize) continue;

            for (int x = sr.left(); x <= sr.right() - SlidingWindowSize; ++x) {
                QList<int> projectionHelper;

                for (int y = sr.top(); y <= sr.bottom(); ++y) {
                    int count = 0;

                    for (int i = 0; i < SlidingWindowSize && (x+i) <= sr.right(); ++i) {

                        // The following condition eliminates 90% of false positives!!!
                        // It mainly discards the count if it the row begins with white pixel.
                        if (i == 0 && workImage.pixel(x+i, y) != BlackColor) break;

                        count += (workImage.pixel(x+i, y) == BlackColor);
                    }
                    projectionHelper << count;

                }

                int peakHValue = determinePeakHValueFrom(projectionHelper);
                for (int i = 0; i < SlidingWindowSize && (x+i) <= sr.right(); ++i) {
                    maxProjections[x+i] = qMax(maxProjections[x+i], peakHValue);
                }
            }
        }
    }

    int StaffData::determinePeakHValueFrom(const QList<int>& horProjValues)
    {
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

        return maxRun;
    }

    void StaffData::extractNoteSegments()
    {
        DataWarehouse *dw = DataWarehouse::instance();
        int n1_2 = 2 * (dw->staffLineHeight().max);
        noteProjections = filter(Range(1, 100),
                Range(n1_2, n1_2 + dw->staffSpaceHeight().max),
                maxProjections);

        noteProjections = filter(Range(1, 100),
                Range(n1_2 + dw->staffSpaceHeight().min - dw->staffLineHeight().min, 100),
                noteProjections);

        // temp is just for debugging purpose.
        temp = noteProjections;

        // Removal of false positives.
        QList<int> keys = noteProjections.keys();
        qSort(keys);


        // Fill up very thin gaps. (aka the bald note head region ;) )
        const int ThinGapLimit = (dw->staffLineHeight().min >> 1);

        for (int i = keys.first(); i <= keys.last(); ++i) {
            if (noteProjections.value(i, 0) > 0) continue;

            int runlength = 0;
            for (; (i + runlength) <= keys.last(); ++runlength) {
                if (noteProjections.value(i+runlength, 0) > 0) break;
            }

            if (runlength <= ThinGapLimit) {
                // valueToInsert will be min of the values surrounding '0' run.
                int valueToInsert = qMin(noteProjections.value(i-1, 0),
                        noteProjections.value(i+runlength, 0));
                for (int j = i; j < (i+runlength); ++j) {
                    noteProjections[j] = valueToInsert;
                }
            }

            i += runlength - 1;
        }

        // Update the keys variable as we inserted few in above loop.
        keys = noteProjections.keys();
        qSort(keys);

        // Remove peak region which are very thin or very thick.
        // IMPT: 80% the staffSpaceHeight min is really good choice since the regions are now thick,
        // thanks to filter method cutting the taller ones to the limit rather than previous value.
        const int ThinRegionLimit = int(qRound(.8 * dw->staffSpaceHeight().min));
        const int ThickRegionLimit = dw->staffSpaceHeight().max << 1;

        for (int i = keys.first(); i <= keys.last(); ++i) {
            if (noteProjections.value(i, 0) == 0) continue;

            int runlength = 0;
            for (; (i+runlength) <= keys.last(); ++runlength) {
                if (noteProjections.value(i+runlength, 0) == 0) break;
            }

            if (runlength <= ThinRegionLimit || runlength >= ThickRegionLimit) {
                for (int j = i; j < (i+runlength); ++j) {
                    noteProjections[j] = 0;
                }
            }

            i += runlength - 1;
        }

        // Update the keys as we added some new keys
        keys = noteProjections.keys();
        qSort(keys);

        // Now fill up noteSegments list (datastructure construction)
        const QRect r = workImage.rect();
        const int top = r.top();
        const int height = r.height();
        const int noteWidth = 2 * DataWarehouse::instance()->staffSpaceHeight().min;

        for (int i = keys.first(); i <= keys.last(); ++i) {
            if (noteProjections.value(i, 0) == 0) continue;

            int runlength = 0;
            for (; (i+runlength) <= keys.last(); ++runlength) {
                if (noteProjections.value(i+runlength, 0) == 0) break;
            }


            int xCenter = i + (runlength >> 1);
            NoteSegment *n = NoteSegment::create();
            // n->boundingRect = QRect(xCenter - noteWidth, top, noteWidth * 2, height);
            n->boundingRect = QRect(xCenter - (noteWidth >> 1), top, noteWidth, height);
            // TODO: BoundingRect should include beams, but thats not being drawn. Check that.
            //n->boundingRect = QRect(key, top, runlength, height);

            noteSegments << n;

            i += runlength - 1;
        }

        qSort(noteSegments.begin(), noteSegments.end(),
                lessThanNoteSegmentPointers);

        mDebug() << Q_FUNC_INFO << endl << "Note segments <sorted>: ";
        foreach (NoteSegment *seg, noteSegments) {
            mDebug() << seg;
        }
        mDebug();
    }

    QHash<int, int> StaffData::filter(Range , Range height,
            const QHash<int, int> &hash)
    {
        QList<int> allKeys = hash.keys();
        qSort(allKeys);

        QHash<int, int> retval;

        for (int i = 0; i < allKeys.size(); ++i) {
            if (hash[allKeys[i]] < height.min) continue;
            if (hash[allKeys[i]] > height.max) {
                // Setting it height.max destroyes the curve, but emboldens the note head region in
                // a better fashion.
                retval[allKeys[i]] = height.max;
                //retval[allKeys[i]-1];
            } else {
                retval[allKeys[i]] = hash[allKeys[i]];
            }
        }

        return retval;
    }

    void StaffData::extractStemSegments()
    {
        const QRgb BlackColor = QColor(Qt::black).rgb();

        DataWarehouse *dw = DataWarehouse::instance();
        const int lineHeight = dw->staffLineHeight().min * 2;

        foreach (NoteSegment* seg, noteSegments) {
            QRect rect = seg->boundingRect;
            rect.setLeft(qMax(0, rect.left() - lineHeight));
            rect.setRight(qMin(workImage.width() - 1, rect.right() + lineHeight));

            int xWithMaxRunLength = -1;
            Run maxRun;
            for (int x = rect.left(); x <= rect.right(); ++x) {
                // Runlength info for x.
                for (int y = rect.top(); y <= rect.bottom(); ++y) {
                    if (workImage.pixel(x, y) != BlackColor) continue;

                    int runlength = 0;
                    for (; (y + runlength) <= rect.bottom() &&
                            workImage.pixel(x, y + runlength) == BlackColor; ++runlength);

                    Run run(y, runlength);

                    if (xWithMaxRunLength < 0 || run.length > maxRun.length) {
                        xWithMaxRunLength = x;
                        maxRun = run;
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
                    count += (workImage.pixel(x, y) == BlackColor);
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
                    count += (workImage.pixel(x, y) == BlackColor);
                }

                if (count >= margin) {
                    stemRect.setLeft(x);
                } else {
                    break;
                }
            }

            StemSegment *stemSeg = StemSegment::create();
            stemSeg->boundingRect = stemRect;
            stemSeg->noteSegment = seg;

            stemSegments << stemSeg;
        }

        qSort(stemSegments.begin(), stemSegments.end(), lessThanStemSegmentPointers);
    }

    void StaffData::eraseStems()
    {
        QPainter p(&workImage);
        p.setBrush(QColor(Qt::white));
        p.setPen(Qt::NoPen);

        // Erase all stems first
        for (int i = 0; i < stemSegments.size(); ++i) {
            p.drawRect(stemSegments[i]->boundingRect.adjusted(-1, 0, +1, 0));
        }
        p.end();
    }

    void StaffData::extractBeams()
    {
        mDebug() << Q_FUNC_INFO;
        beamsRunCoords.clear();
        VerticalRunlengthImage vRunImage(workImage);

        QSet<RunCoord> visited;
        QHash<RunCoord, RunCoord> prevCoord;

        DataWarehouse *dw = DataWarehouse::instance();
        const int MinimumBeamRunlengthLimit = dw->staffSpaceHeight().min >> 1;

        foreach (StemSegment *seg, stemSegments) {
            const QRect rect = seg->boundingRect;
            const RunCoord stemRunCoord(rect.right() + 1, Run(rect.top(), rect.height()));

            QStack<RunCoord> stack; // For DFS
            stack.push(stemRunCoord);

            while (!stack.isEmpty()) {
                RunCoord runCoord = stack.pop();
                visited << runCoord;

                if (!runCoord.isValid()) continue;

                QList<Run> adjRuns = vRunImage.adjacentRunsInNextColumn(runCoord);

                bool pushed = false;
                foreach (const Run& adjRun, adjRuns) {
                    RunCoord adjRunCoord(runCoord.pos + 1, adjRun);
                    if (adjRunCoord.isValid() &&
                            adjRunCoord.run.length >= MinimumBeamRunlengthLimit &&
                            !visited.contains(adjRunCoord))
                    {
                        prevCoord[adjRunCoord] = runCoord;
                        stack.push(adjRunCoord);
                        pushed = true;
                    }
                }

                if (!pushed) {
                    StemSegment *right = stemSegmentForRunCoord(runCoord);
                    if (right && right != seg) {
                        QList<RunCoord> beamCoords;

                        right->leftFlagCount += 1;
                        seg->rightFlagCount += 1;

                        RunCoord cur = runCoord;
                        // This loop takes care not to push stemRunCoord as that
                        // is the artifact we created for convenience.
                        while (1) {
                            RunCoord prev = prevCoord.value(cur, RunCoord());
                            if (!prev.isValid()) break;

                            beamCoords << cur;

                            cur = prev;
                        }

                        beamsRunCoords << beamCoords;
                    }
                }
            }
        }

        mDebug() << "Num of beam segments = " << beamsRunCoords.size();
    }

    StemSegment* StaffData::stemSegmentForRunCoord(const RunCoord& runCoord)
    {
        const QRect runRect(runCoord.pos, runCoord.run.pos, 1, runCoord.run.length);
        foreach (StemSegment *seg, stemSegments) {
            QRect rect = seg->boundingRect;
            rect.setLeft(rect.left() - 2);
            rect.setTop(rect.top() - 1);
            rect.setBottom(rect.bottom() + 2);
            if (rect.intersects(runRect)) {
                return seg;
            }
        }
        return 0;
    }

    void StaffData::eraseBeams()
    {
        QPainter p(&workImage);

        p.setBrush(Qt::NoBrush);
        p.setPen(QColor(Qt::white));

        foreach (const QList<RunCoord>& oneBeamCoords, beamsRunCoords) {
            foreach (const RunCoord& runCoord, oneBeamCoords) {
                p.drawLine(runCoord.pos, runCoord.run.pos,
                        runCoord.pos, runCoord.run.endPos());
            }
        }
        p.end();
    }

    void StaffData::extractChords()
    {
        const QRgb BlackColor = QColor(Qt::black).rgb();

        DataWarehouse *dw = DataWarehouse::instance();
        const int margin = dw->staffSpaceHeight().min;
        int verticalMargin = dw->staffSpaceHeight().min - dw->staffLineHeight().min;

        for (int i = 0; i < noteSegments.size(); ++i) {
            NoteSegment *seg = noteSegments[i];
            QRect segRect = seg->boundingRect;

            QList<int> projHelper;
            for (int y = segRect.top(); y <= segRect.bottom(); ++y) {
                int count = 0;
                for (int x = segRect.left(); x <= segRect.right(); ++x) {
                    count += (workImage.pixel(x, y) == BlackColor);
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
                    seg->noteRects << rect;
                }

                i += runlength - 1;
            }

            seg->horizontalProjection.clear();
            for (int i = 0; i < projHelper.size(); ++i) {
                seg->horizontalProjection.insert(i + segRect.top(), projHelper.at(i));
            }

        }
    }

    void StaffData::eraseChords()
    {
        QPainter p(&workImage);
        p.setBrush(QBrush(QColor(Qt::white)));
        p.setPen(Qt::NoPen);

        foreach (const NoteSegment* nSeg, noteSegments) {
            foreach (const QRect &chordRect, nSeg->noteRects) {
                p.drawRect(chordRect);
            }
        }
    }

    void StaffData::extractFlags()
    {
        mDebug() << Q_FUNC_INFO;
        VerticalRunlengthImage vRunImage(workImage);

        QHash<RunCoord, RunCoord> prevCoord;

        DataWarehouse *dw = DataWarehouse::instance();
        const int MinimumFlagRunlengthLimit = dw->staffLineHeight().min << 1;
        const int FlagDistanceLimit = int(qRound(.8 * dw->staffSpaceHeight().max));

        foreach (StemSegment *seg, stemSegments) {
            const QRect rect = seg->boundingRect;
            const RunCoord stemRunCoord(rect.right() + 1, Run(rect.top(), rect.height()));

            QRect flagBoundRect;

            QStack<RunCoord> stack; // For DFS
            stack.push(stemRunCoord);

            while (!stack.isEmpty()) {
                RunCoord runCoord = stack.pop();

                if (!runCoord.isValid()) continue;

                QList<Run> adjRuns = vRunImage.adjacentRunsInNextColumn(runCoord);

                bool pushed = false;
                foreach (const Run& adjRun, adjRuns) {
                    RunCoord adjRunCoord(runCoord.pos + 1, adjRun);
                    if (adjRunCoord.isValid() &&
                            adjRunCoord.run.length >= MinimumFlagRunlengthLimit)
                    {
                        prevCoord[adjRunCoord] = runCoord;
                        stack.push(adjRunCoord);
                        pushed = true;
                    }
                }

                if (!pushed) {
                    if ((runCoord.pos - stemRunCoord.pos) >= FlagDistanceLimit) {
                        // This loop takes care not to push stemRunCoord as that
                        // is the artifact we created for convenience.
                        RunCoord cur = runCoord;
                        while (1) {
                            const RunCoord prev = prevCoord.value(cur, RunCoord());
                            if (!prev.isValid()) break;

                            seg->flagRunCoords << cur;
                            const QRect curRect(prev.pos, prev.run.pos, 1, prev.run.length);
                            if (flagBoundRect.isNull()) {
                                flagBoundRect = curRect;
                            } else {
                                flagBoundRect |= curRect;
                            }
                            cur = prev;
                        }
                    }
                }
            }


            if (flagBoundRect.isNull()) continue;

            QList<int> transitionsList;
            for (int x = flagBoundRect.left(); x <= flagBoundRect.right(); ++x) {
                int transitions = 0;
                for (int y = flagBoundRect.top(); y <= flagBoundRect.bottom(); ++y) {
                    Run run = vRunImage.run(x, y);
                    if (!run.isValid()) continue;

                    if (run.length >= MinimumFlagRunlengthLimit) ++transitions;
                    y = run.endPos();
                }
                transitionsList << transitions;
            }

            int maxRunLength = -1;
            int maxRunValue = -1;
            for (int i = 0; i < transitionsList.size(); ++i) {
                int runlength = 0;
                for (; (i + runlength) < transitionsList.size(); ++runlength) {
                    if (transitionsList[i + runlength] != transitionsList[i]) {
                        break;
                    }
                }
                if (runlength > maxRunLength) {
                    maxRunLength = runlength;
                    maxRunValue = transitionsList[i];
                }
                i += runlength - 1;
            }

            if (maxRunLength > 0) {
                seg->rightFlagCount += maxRunValue;
            }
        }
    }

    void StaffData::eraseFlags()
    {
        QPainter p(&workImage);

        p.setPen(QColor(Qt::white));
        p.setBrush(Qt::NoBrush);

        foreach (const StemSegment *seg, stemSegments) {
            foreach (const RunCoord &runCoord, seg->flagRunCoords) {
                p.drawLine(runCoord.pos, runCoord.run.pos, runCoord.pos, runCoord.run.endPos());
            }
        }
    }


#if 0
    void StaffData::extractFlags()
    {
        mDebug() << Q_FUNC_INFO;

        const QRgb BlackColor = QColor(Qt::black).rgb();
        const QRect staffRect = workImage.rect();
        DataWarehouse *dw = DataWarehouse::instance();

        for (int i = 0; i < stemSegments.size(); ++i) {
            StemSegment *seg = stemSegments[i];
            QRect areaToTry(0, 0, dw->staffSpaceHeight().min >> 1,
                    seg->boundingRect.height());
            QPoint positions[2] =
            {
                seg->boundingRect.topRight() + QPoint(2, 0),
                seg->boundingRect.topLeft() - QPoint(areaToTry.width() + 1, 0)
            };
            // J loop for trying both "left" and "right" areas of stem.
            for (int j = 0; j < 2; ++j) {
                areaToTry.moveTo(positions[j]);

                if (areaToTry.left() > staffRect.right()) continue;
                if (areaToTry.right() < staffRect.left()) continue;

                QList<int> numTransitions;
                for (int i = 0; i < areaToTry.width(); ++i) {
                    int x = (j == 0) ? (areaToTry.left() + i) :
                        (areaToTry.right() - i);
                    if (x > staffRect.right() || x < staffRect.left()) break;
                    QList<int> runs;

                    for (int y = areaToTry.top(); y <= areaToTry.bottom(); ++y) {
                        if (workImage.pixel(x, y) != BlackColor) continue;

                        int runlength = 0;
                        for (; (y + runlength) <= areaToTry.bottom(); ++runlength) {
                            if (workImage.pixel(x, y + runlength) != BlackColor) break;
                        }

                        const int margin = (dw->staffLineHeight().min << 1);
                        if (runlength > margin) {
                            runs << runlength;
                        }
                        y += runlength - 1;
                    }
                    // An empty column means there is no more connected components.
                    if (runs.isEmpty()) break;
                    numTransitions << runs.size();
                }

                int avgTransitions = 0;
                if (numTransitions.isEmpty() == false) {
                    int sum = 0;
                    for (int i = 0; i < numTransitions.size(); ++i) {
                        sum += numTransitions.at(i);
                    }
                    avgTransitions = int(qRound(qreal(sum)/numTransitions.size()));
                }

                if (j == 0) {
                    seg->rightFlagCount += avgTransitions;
                } else {
                    seg->leftFlagCount += avgTransitions;
                }
                if (avgTransitions) {
                    mDebug() << seg << numTransitions << endl;
                }
            }
        }
    }
#endif

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
        const QRect r = workImage.rect();

        QImage img(r.size(), QImage::Format_ARGB32_Premultiplied);
        img.fill(0xffffffff);

        QPainter p(&img);
        p.setPen(Qt::red);
        for (int x = r.left(); x <= r.right(); ++x) {
            QLineF line(x, r.bottom(), x, r.bottom() - hash[x]);
            p.drawLine(line);
        }
        p.end();

        return img;
    }

    QImage StaffData::noteHeadHorizontalProjectionImage() const
    {
        QImage img(workImage.size(), QImage::Format_ARGB32_Premultiplied);
        img.fill(0xffffffff);

        QPainter p(&img);

        foreach (const NoteSegment* seg, noteSegments) {
            p.setPen(QColor(Qt::red));
            QRect segRect = seg->boundingRect;

            QHash<int, int>::const_iterator it = seg->horizontalProjection.constBegin();
            while (it != seg->horizontalProjection.constEnd()) {
                p.drawLine(segRect.left(), it.key(),
                        segRect.left() + it.value(), it.key());
                ++it;
            }
            p.setPen(QColor(Qt::blue));
            p.drawRect(segRect);
        }

        p.end();
        return img;
    }
}
