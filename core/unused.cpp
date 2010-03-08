#include "processstep.h"
#include "staff.h"

#include "cluster.h"
#include "datawarehouse.h"
#include "horizontalrunlengthimage.h"
#include "imagewidget.h"
#include "mainwindow.h"
#include "projection.h"
#include "tools.h"

#include <QAction>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QProcess>
#include <QRgb>
#include <QSet>
#include <QStack>
#include <QTextStream>
#include <QList>

#include <iostream>
#include <cmath>
namespace Munip
{

void SymbolAreaExtraction::extraStuff()
{
    emit started();

    const int Black = m_originalImage.color(0) == 0xffffffff ? 1 : 0;

    DataWarehouse *dw = DataWarehouse::instance();
    QList<Staff> staffList = dw->staffList();

    const int spacing = 50;
    QSize size(0, 0);
    foreach (const Staff& s, staffList) {
        QRect r = s.boundingRect();
        size.rheight() += 2 * r.height() + spacing;
        if (size.width() < r.width()) {
            size.setWidth(r.width());
        }
    }

    m_processedImage = QImage(size, QImage::Format_ARGB32_Premultiplied);
    m_processedImage.fill(0xffffffff);

    int y = 0;
    QImage workImage = m_processedImage;
    QImage newImage(workImage.width(), workImage.height() * 2 + 100,
            QImage::Format_ARGB32_Premultiplied);
    newImage.fill(0xffffffff);
    {
        QPainter p(&newImage);
        p.drawImage(0, 0, workImage);
    }
    QPainter p(&m_processedImage);
    foreach (const Staff& s, staffList) {
        QRect bigger = s.boundingRect();

        QRect r = s.staffBoundingRect();
        QRect staffBoundRect = s.staffBoundingRect();
        r.setLeft(bigger.left());
        r.setRight(bigger.right());

        QRect target(QPoint(0, y), bigger.size());
        p.drawImage(target, m_originalImage, bigger);
        p.fillRect(target, QColor(60, 60, 60, 100));

        y += bigger.height() + .5 * spacing;
        QRect projectionRect = bigger.translated(QPoint(0, y) - bigger.topLeft());
        p.setPen(QColor(Qt::blue));
        p.drawRect(projectionRect.adjusted(-2, -2, 2, 2));
        p.setPen(QColor(Qt::red));

        QList<int> aboveStaffCounts;
        QList<int> counts;
        QList<int> belowStaffCounts;
        for (int x = r.left(), X = projectionRect.left(); x <= r.right(); ++x, ++X) {
            if (x >= m_originalImage.width()) {
                continue;
            }
            int count = 0;
            for (int y = r.top(); y <= r.bottom(); ++y) {
                if (y < m_originalImage.height()) {
                    if (m_originalImage.pixelIndex(x, y) == Black) {
                        count++;
                    }
                }
            }
            if (count) {
                p.drawLine(X, projectionRect.bottom(), X, projectionRect.bottom() - count);
            }
            counts << count;

            int aboveStaffCount = 0;
            for (int y = bigger.top(); y < r.top(); ++y) {
                if (m_originalImage.pixelIndex(x, y) == Black) {
                    aboveStaffCount++;
                }
            }
            aboveStaffCounts << aboveStaffCount;

            int belowStaffCount = 0;
            for (int y = r.bottom()+1; y <= bigger.bottom(); ++y) {
                if (m_originalImage.pixelIndex(x, y) == Black) {
                    belowStaffCount++;
                }
            }
            belowStaffCounts << belowStaffCount;
        }

        // Just a random push so that there is no crash on empty list.
        if (counts.isEmpty()) {
            counts << 5;
        }

        int staffLineHeight = -1;
        {
            StaffParamExtraction *param =
                new StaffParamExtraction(m_originalImage, false, 0);
            param->process();
            Range range = DataWarehouse::instance()->staffLineHeight();
            if (range.size() == 1) {
                staffLineHeight = range.min;
            } else {
                staffLineHeight = int(qRound(.5 * (range.min + range.max)));
            }
            delete param;
        }

        QColor symbolColor(Qt::darkYellow);
        symbolColor.setAlpha(90);

        QColor nonSymbolColor(Qt::darkGray);

        int top = target.top() + (r.top() - bigger.top());
        int bottom = target.bottom() - (bigger.bottom() - r.bottom());

        QList<QRect> symbolRects;

        QRect rect;
        for (int i = 0; i < counts.size(); ++i) {
            if (counts[i] > 2) {
                p.setPen(symbolColor);
                if (rect.isValid()) {
                    rect.setWidth(rect.width() + 1);
                } else {
                    rect.setLeft(staffBoundRect.left() + i);
                    rect.setTop(staffBoundRect.top());
                    rect.setRight(staffBoundRect.left() + i);
                    rect.setBottom(staffBoundRect.bottom());
                }
            } else {
                p.setPen(nonSymbolColor);
                if (rect.isValid()) {
                    symbolRects << rect;
                }
                rect = QRect();
            }
            p.drawLine(target.left() + i, top, target.left() + i, bottom);

            if (aboveStaffCounts[i] > 2) {
                p.setPen(symbolColor);
            } else {
                p.setPen(nonSymbolColor);
            }
            p.drawLine(target.left() + i, target.top(), target.left() + i,
                    top-1);

            if (belowStaffCounts[i] > 2) {
                p.setPen(symbolColor);
            } else {
                p.setPen(nonSymbolColor);
            }
            p.drawLine(target.left() + i, bottom+1, target.left() + i,
                    target.bottom());
        }
        mDebug() << Q_FUNC_INFO << counts;


        SlidingWindow window(workImage, staffBoundRect.height());
        QList<SlidingWindowData> datas;
        QMap<int, int> projectionValues;

        foreach (const QRect& symbolRect, symbolRects) {
            if (symbolRect.width() < 3) continue;

            for (int x = symbolRect.left(); x <= symbolRect.right(); ++x) {
                SlidingWindowData d = window.process(QPoint(x, symbolRect.top()));
                datas << d;
                projectionValues[x] = qMax(projectionValues[x], d.maxH());
            }
        }

        {

            QPainter p(&newImage);
            int offsetY = workImage.height() + 100;
            for (int x = staffBoundRect.left(); x <= staffBoundRect.right(); ++x) {
                p.drawLine(x, offsetY + staffBoundRect.bottom(),
                        x, offsetY + staffBoundRect.bottom() - projectionValues[x]);
            }
        }


        y += bigger.height() + .5 * spacing;


    }

    MainWindow::instance()->addSubWindow(new ImageWidget(newImage));
    emit ended();
}

} // namespace Munip
