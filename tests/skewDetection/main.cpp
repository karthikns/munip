#include <QtTest/QtTest>
#include <QImage>
#include <QScopedPointer>
#include <QTextStream>
#include "processstep.h"

qreal norm(qreal angle)
{
    while (angle > 360.0) {
        angle -= 360;
    }
    while (angle < 0.0) {
        angle += 360.0;
    }
    return angle;
}

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

    void cleanupTestCase();

private:
    void pushStat(const QString &fileName, qreal angle, qreal accuracy) {
        stats[fileName] << qMakePair(angle, accuracy);
    }
    qreal calculatedAngle;
    typedef QPair<qreal, qreal> AngleAccuracyPair;
    typedef QHash<QString, QList<AngleAccuracyPair> > Hash;

    Hash stats;
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
        { S("music1.bmp"), -4.94 },
        { S("music2.bmp"), 14.956 },
        { S("music3.bmp"), 0.0 },
        { S("music4.bmp"), -7.99 },
        { S("scan0022.jpg"), 12.66 },
        { S("scan0022small.jpg"), 12.66 },
        { S("scan0025.jpg"), 26.08 }
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
    image.save(QString("plots/Png/%1_%2_beforeSkew.png").arg(QFileInfo(fileName).baseName()).arg(expectedAngle));

    QScopedPointer<Munip::ProcessStep> skew(new Munip::SkewCorrection(image));
    connect(skew.data(), SIGNAL(angleCalculated(qreal)), SLOT(slotCalculatedAngle(qreal)));
    skew->process();

    qreal denominator = 1.0;
    qreal accuracy = qAbs((expectedAngle) - (calculatedAngle)) / denominator;
    pushStat(fileName, expectedAngle, accuracy);
    qDebug() << fileName << this->calculatedAngle << expectedAngle << rotateBy;
    skew->processedImage().save(QString("plots/Png/%1_%2_laterSkew.png").arg(QFileInfo(fileName).baseName()).arg(expectedAngle));

}

void tst_SkewDetection::cleanupTestCase()
{
    static const QString prefix = "plots";
    for(Hash::iterator it = stats.begin(); it != stats.end(); ++it) {
        QFile file(prefix + QChar('/') + QFileInfo(it.key()).baseName() + QString(".dat"));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qFatal("Cannot open plot file %s for writing", qPrintable(file.fileName()));
            return;
        }

        QTextStream stream(&file);
        stream << QString("# ") << it.key() << endl;
        QList<AngleAccuracyPair> &list = it.value();
        qSort(list);
        for(QList<AngleAccuracyPair>::iterator it  = list.begin(); it != list.end(); ++it) {
            stream << (*it).first << " " << (*it).second << endl;
        }

        file.close();
    }
}

QTEST_MAIN(tst_SkewDetection)
#include "main.moc"
