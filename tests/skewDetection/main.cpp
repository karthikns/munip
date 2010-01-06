#include <QtTest/QtTest>
#include <QImage>
#include <QScopedPointer>
#include "processstep.h"

class tst_SkewDetection : public QObject
{
Q_OBJECT
public:
    tst_SkewDetection() : calculatedAngle(0.0) {}

public Q_SLOTS:
    void slotCalculatedAngle(qreal angle) { calculatedAngle = angle; }

private Q_SLOTS:
    void skewDetect_data();
    void skewDetect();

private:
    qreal calculatedAngle;
};

void tst_SkewDetection::skewDetect_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QImage>("image");
    QTest::addColumn<qreal>("rotateBy");
    QTest::addColumn<qreal>("expectedAngle");

    struct Data {
        QString fileName;
        qreal actualAngle;
    };

    static const QString prefix = "../../images/";
    #define S QString
    Data data[] = {
        { S("image.bmp"), 0.0 },
        { S("music3.bmp"), 0.0 }
    };
    #undef S

    for (uint i = 0; i < sizeof(data)/sizeof(Data); ++i) {
        QImage image = QImage(prefix + data[i].fileName);
        {
            QScopedPointer<Munip::ProcessStep> mono(new Munip::MonoChromeConversion(image));
            mono->process();
            image = mono->processedImage();
        }

        QString dTag = data[i].fileName + QChar('_') + QString::number(data[i].actualAngle);
        QTest::newRow(qPrintable(dTag)) << data[i].fileName
                                        << image
                                        << 0.0
                                        << data[i].actualAngle;

        const qreal start = -40.0;
        const qreal stop = +40.0;
        const qreal step = 10.0;

        for (qreal s = start; s <= stop; s += step) {
            QString dataTag = data[i].fileName + QChar('_') + QString::number(s);
            QTestData &td = QTest::newRow(qPrintable(dataTag));

            td << data[i].fileName;
            td << image;
            td << s - data[i].actualAngle;
            td << s;
        }
    }
}

void tst_SkewDetection::skewDetect()
{
    QFETCH(QString, fileName);
    QFETCH(QImage, image);
    QFETCH(qreal, rotateBy);
    QFETCH(qreal, expectedAngle);

    QScopedPointer<Munip::ProcessStep> rotate(new Munip::ImageRotation(image, rotateBy));
    rotate->process();
    image = rotate->processedImage();

    QScopedPointer<Munip::ProcessStep> skew(new Munip::SkewCorrection(image));
    connect(skew.data(), SIGNAL(angleCalculated(qreal)), SLOT(slotCalculatedAngle(qreal)));
    skew->process();

    qDebug() << fileName << this->calculatedAngle;
}

QTEST_MAIN(tst_SkewDetection)
#include "main.moc"
