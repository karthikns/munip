#include <QtTest/QtTest>
#include <QImage>
#include <QScopedPointer>
#include <QTextStream>

#include <cmath>

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

    static const QString prefix = "images/";
    #define S QString
    Data data[] = {
        { S("music4.bmp"), -7.99 },

// The #if is for easier faster testing (make it 0 to disable temporarily)
#if 0
        { S("music3.bmp"), 0.0 },
        { S("image.bmp"), 0.0 },
        { S("music2.bmp"), 14.956 },
        { S("scan0022.jpg"), 12.66 },
        { S("scan0022small.jpg"), 12.66 },
        { S("scan0025.jpg"), 26.08 },
#endif

        { S("music1.bmp"), -4.94 }
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

    // Ensure the existence of directories
    {
        QDir dir;
        dir.mkdir("test_output");
        dir.mkdir("test_output/skewDetection");
        dir.mkdir("test_output/skewDetection/plots");
        dir.mkdir("test_output/skewDetection/plots/Png");
        dir.mkdir("test_output/skewDetection/plots/histograms");
    }

    const QString uniqId = QString("%1_%2")
                           .arg(QFileInfo(fileName).baseName())
                           .arg(expectedAngle);

    // Generate before skew image
    QScopedPointer<Munip::ProcessStep> rotate(new Munip::ImageRotation(image, rotateBy));
    rotate->process();
    image = rotate->processedImage();
    image.save(QString("test_output/skewDetection/plots/Png/%1before.png").arg(uniqId));


    // Generate after skew image
    QScopedPointer<Munip::SkewCorrection> skew(new Munip::SkewCorrection(image));
    connect(skew.data(), SIGNAL(angleCalculated(qreal)), SLOT(slotCalculatedAngle(qreal)));
    skew->process();

    qreal denominator = 1.0; // Change it aptly for different metric
    qreal accuracy = qAbs((expectedAngle) - (calculatedAngle)) / denominator;
    pushStat(fileName, expectedAngle, accuracy);
    qDebug() << fileName << this->calculatedAngle << expectedAngle << rotateBy;
    skew->processedImage().save(QString("test_output/skewDetection/plots/Png/%1after.png").arg(uniqId));


    // Generate Histogram
    QMap<int, QList<double> > bins;

    QList<double> m_skewList = skew->skewList();
    qSort(m_skewList);
    foreach (double skew, m_skewList) {
        bins[(int)(skew * 100)] << skew;
    }

    const QString angleFileNamePrefix = QString("test_output/skewDetection/plots/histograms/%1").arg(uniqId);
    QFile file(angleFileNamePrefix + QString(".dat"));
    file.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream stream(&file);
    QList<int> keys = bins.keys();
    qSort(keys);

    foreach (int k, keys) {
        stream << ((180.0/M_PI) * std::atan((k/100.0))) << " " << bins[k].count() << endl;
    }
    file.close();

    QStringList args;
    args << "-e";
    args << QString("set terminal png;"
                    "set output '%1.png';"
                    "set xrange [-45:45];"
                    "plot '%2.dat' using 1:2 with impulses;")
            .arg(angleFileNamePrefix)
            .arg(angleFileNamePrefix);

    QProcess::execute(QString("gnuplot"), args);
}

void tst_SkewDetection::cleanupTestCase()
{
    static const QString prefix = "test_output/skewDetection/plots";
    QDir().mkdir(prefix);

    QStringList datFileNames;
    for(Hash::iterator it = stats.begin(); it != stats.end(); ++it) {
        QString fileName = prefix + QChar('/') + QFileInfo(it.key()).baseName() + QString(".dat");
        datFileNames << fileName;
        QFile file(fileName);
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

    QStringList args;
    args << "-e";
    QString secondArg("set terminal png; set output 'test_output/skewDetection/plots/plot.png'; set xrange [-45:45]; set yrange [0:5]; plot ");
    for (int i = 0; i < datFileNames.size(); ++i) {
        secondArg += QChar('\'');
        secondArg += datFileNames[i];
        secondArg += QChar('\'');
        secondArg += " using 1:2 with lines";
        secondArg += (i == datFileNames.size()-1 ? ';' : ',');
   }
   args << secondArg;

   QProcess::execute(QString("gnuplot"), args);
}

QTEST_MAIN(tst_SkewDetection)
#include "main.moc"
