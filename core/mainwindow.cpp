#include "mainwindow.h"

#include "imagewidget.h"
#include "projection.h"
#include "processstep.h"
#include "sidebar.h"
#include "symbol.h"
#include "tools.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QDomDocument>
#include <QFileDialog>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QScopedPointer>
#include <QSplitter>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWebView>

MainWindow* MainWindow::m_instance = 0;

MainWindow::MainWindow()
{
    m_showGridAction = 0;
    m_tabWidget = new QTabWidget;
    setCentralWidget(m_tabWidget);

    m_mdiArea = new QMdiArea(this);

    m_tabWidget->addTab(m_mdiArea, QIcon(), "Images");

    m_coordinateLabel = new QLabel(this);

    setIconSize(QSize(16, 16));

    setup2ndTab();
    setupActions();
    applyStyle();

    connect(m_mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(slotOnSubWindowActivate(QMdiSubWindow*)));

    setWindowTitle(QString("Music Notation Information Processing (MuNIP)"));
    // Ensure only one instance of this MainWindow exists.
    Q_ASSERT_X(!m_instance, "MainWindow construction", "MainWindow already exists!");

    m_instance = const_cast<MainWindow*>(this);
}

MainWindow::~MainWindow()
{
    delete m_brailleTranscriptionProcess;
}

MainWindow* MainWindow::instance()
{
    return m_instance;
}

void MainWindow::setup2ndTab()
{
    QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, true);
    m_webView = new QWebView;

    m_brailleView = new QTextEdit;
    m_brailleView->setReadOnly(true);
    m_brailleView->setFontPointSize(20);

    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_webView);
    splitter->addWidget(m_brailleView);

    m_tabWidget->addTab(splitter, QIcon(), "Player/Braille");
    m_brailleTranscriptionProcess = new QProcess;
    connect(m_brailleTranscriptionProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(slotOnTranscriptionComplete(int,QProcess::ExitStatus)));
}

void MainWindow::setupActions()
{
    QMenuBar *menuBar = QMainWindow::menuBar();

    QAction *openAction = new QAction(QIcon(":/resources/open.png"), tr("&Open"), this);
    openAction->setShortcuts(QKeySequence::Open);
    openAction->setStatusTip(tr("Opens an image for viewing or processing"));
    connect(openAction, SIGNAL(triggered()), this, SLOT(slotOpen()));

    QAction *saveAction = new QAction(QIcon(":/resources/save.png"), tr("&Save"), this);
    saveAction->setShortcuts(QKeySequence::Save);
    saveAction->setStatusTip(tr("Saves an image"));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(slotSave()));

    QAction *saveAsAction = new QAction(QIcon(":/resources/save-as.png"), tr("Save &As"), this);
    saveAsAction->setStatusTip(tr("Prompts for a new filename for the image to be saved."));
    connect(saveAsAction, SIGNAL(triggered()), this, SLOT(slotSaveAs()));

    QAction *closeAction = new QAction(QIcon(":/resources/close.png"), tr("&Close window"), this);
    closeAction->setShortcuts(QKeySequence::Close);
    closeAction->setStatusTip(tr("Closes the image"));
    connect(closeAction, SIGNAL(triggered()), this, SLOT(slotClose()));

    QAction *closeAllAction = new QAction(QIcon(":/resources/close.png"), tr("&Close all windows"), this);
    closeAllAction->setStatusTip(tr("Closes all open windows"));
    connect(closeAllAction, SIGNAL(triggered()), this, SLOT(slotCloseAll()));

    QAction *quitAction = new QAction(QIcon(":/resources/quit.png"), tr("&Quit"), this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    quitAction->setStatusTip(tr("Quit application"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(slotQuit()));

    QToolBar *toolBar = addToolBar("ToolBar");
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addAction(closeAction);
    fileMenu->addAction(closeAllAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);
    toolBar->addAction(openAction);
    toolBar->addAction(saveAction);
    toolBar->addAction(saveAsAction);
    toolBar->addAction(closeAction);
    toolBar->addAction(closeAllAction);
    toolBar->addAction(quitAction);
    toolBar->addSeparator();


    QAction *zoomInAction = new QAction(QIcon(":/resources/zoom-in.png"), tr("Zoom &in"), this);
    zoomInAction->setShortcuts(QKeySequence::ZoomIn);
    zoomInAction->setStatusTip(tr("Zooms in the image"));
    connect(zoomInAction, SIGNAL(triggered()), this, SLOT(slotZoomIn()));

    QAction *zoomOutAction = new QAction(QIcon(":/resources/zoom-out.png"), tr("Zoom &out"), this);
    zoomOutAction->setShortcuts(QKeySequence::ZoomOut);
    zoomOutAction->setStatusTip(tr("Zooms out the image"));
    connect(zoomOutAction, SIGNAL(triggered()), this, SLOT(slotZoomOut()));

    m_showGridAction = new QAction(QIcon(":/resources/show-grid.png"), tr("Show grid"), this);
    m_showGridAction->setCheckable(true);
    m_showGridAction->setShortcut(tr("Ctrl+G"));
    m_showGridAction->setStatusTip(tr("Hide/Shows the grid"));
    m_showGridAction->setEnabled(false);
    connect(m_showGridAction, SIGNAL(toggled(bool)), this, SLOT(slotToggleShowGrid(bool)));

    QMenu *viewMenu = menuBar->addMenu(tr("&View"));
    viewMenu->addAction(zoomInAction);
    viewMenu->addAction(zoomOutAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_showGridAction);
    toolBar->addAction(zoomInAction);
    toolBar->addAction(zoomOutAction);
    toolBar->addAction(m_showGridAction);
    toolBar->addSeparator();

    QList<Munip::ProcessStepAction*> psActions = Munip::ProcessStepFactory::actions(this);

    QAction *projectionAction = new QAction(tr("&Projection"), this);
    projectionAction->setShortcut(tr("Ctrl+P"));
    projectionAction->setStatusTip(tr("Calculates horizontal projection of the image"));;
    connect(projectionAction, SIGNAL(triggered()), this, SLOT(slotProjection()));

    QAction *playAction = new QAction(tr("&Play"), this);
    playAction->setShortcut(tr("Ctrl+P"));
    playAction->setStatusTip(tr("Plays the result of latest symbol detection"));
    connect(playAction, SIGNAL(triggered()), this, SLOT(slotPlay()));

    QMenu *processMenu = menuBar->addMenu(tr("&Process"));
    SideBar *processBar = new SideBar();
    int i = 1;
    foreach (Munip::ProcessStepAction *action, psActions) {
        action->setShortcut(QString("Ctrl+%1").arg(i));
        processMenu->addAction(action);
        processBar->addAction(action);

        if(i == 5) {
            ++i;
            playAction->setShortcut(QString("Ctrl+%1").arg(i));
            processMenu->addAction(playAction);
            processBar->addAction(playAction);
       }

       ++i;
    }

    processMenu->addAction(projectionAction);
    processBar->addAction(projectionAction);

    QDockWidget *dock = new QDockWidget(tr("Process"), this);
    dock->setWidget(processBar);
    addDockWidget(Qt::LeftDockWidgetArea, dock, Qt::Vertical);

    menuBar->addSeparator();

    QAction *aboutAction = new QAction(QIcon(":/resources/about.png"), tr("About MuNIP"), this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(slotAboutMunip()));

    QMenu *helpMenu = menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
    toolBar->addAction(aboutAction);

    QStatusBar *s = statusBar(); //create statusbar
    s->addPermanentWidget(m_coordinateLabel);
    m_coordinateLabel->show();
    m_coordinateLabel->setText("hello");
}

void MainWindow::applyStyle()
{
    QFile styleFile(":/resources/style.css");
    if (styleFile.open(QIODevice::ReadOnly)) {
        QByteArray styleText = styleFile.readAll();
        setStyleSheet(styleText);
    }
}

ImageWidget* MainWindow::activeImageWidget() const
{
    QMdiSubWindow *sub = m_mdiArea->activeSubWindow();
    if (!sub) {
        return 0;
    }

    return dynamic_cast<ImageWidget*>(sub->widget());

}

void MainWindow::addSubWindow(QWidget *widget)
{
    QMdiSubWindow *sub = m_mdiArea->addSubWindow(widget);
    widget->setAttribute(Qt::WA_DeleteOnClose);
    sub->show();
}

void MainWindow::slotOpen()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"),
                                                    QDir::currentPath() + QDir::separator() + QString("images")
                                                    + QDir::separator() + QString("Test Images"),
                                                    tr("Images (*.png *.xpm *.jpg *.bmp)"));
    if (!fileName.isEmpty()) {
        ImageWidget *imgWidget = new ImageWidget(fileName);
        imgWidget->setWidgetID(Munip::IDGenerator::gen());
        QMdiSubWindow *sub = m_mdiArea->addSubWindow(imgWidget);
        sub->widget()->setAttribute(Qt::WA_DeleteOnClose);
        sub->show();
        m_tabWidget->setCurrentIndex(0);
    }
}

void MainWindow::slotSave()
{
    ImageWidget* img = activeImageWidget();
    if (img) {
        img->slotSave();
    }
}

void MainWindow::slotSaveAs()
{
    ImageWidget *img = activeImageWidget();
    if (img) {
        img->slotSaveAs();
    }
}

void MainWindow::slotClose()
{
    m_mdiArea->closeActiveSubWindow();
}

void MainWindow::slotCloseAll()
{
    m_mdiArea->closeAllSubWindows();
}

void MainWindow::slotQuit()
{
    qApp->quit();
}

void MainWindow::slotZoomIn()
{
    ImageWidget *img = activeImageWidget();
    if (img) {
        img->slotZoomIn();
    }
}

void MainWindow::slotZoomOut()
{
    ImageWidget *img = activeImageWidget();
    if (img) {
        img->slotZoomOut();
    }
}

void MainWindow::slotToggleShowGrid(bool b)
{
    ImageWidget *img = activeImageWidget();
    if (img) {
        img->slotSetShowGrid(b);
    }
}

void MainWindow::slotProjection()
{
    ImageWidget *imgWidget = activeImageWidget();
    if (!imgWidget) {
        return;
    }

    Munip::ProjectionData data = Munip::horizontalProjection(imgWidget->image());
    Munip::ProjectionWidget *wid = new Munip::ProjectionWidget(data);
    QMdiSubWindow *sub = m_mdiArea->addSubWindow(wid);
    sub->widget()->setAttribute(Qt::WA_DeleteOnClose);
    sub->show();
}

void MainWindow::slotPlay()
{
    int tempo = 120;
    int numerator = 4;
    int denominator = 4;

    {
        QScopedPointer<QDialog> dialog(new QDialog);
        QString data[3] = { "Tempo", "Numerator", "Denominator" };
        int defaultValues[3] = { tempo, numerator, denominator };
        int mins[3] = { 80, 1, 1 };
        int maxs[3] = { 160, 64, 64 };

        QVBoxLayout *layout = new QVBoxLayout(dialog.data());
        QGridLayout *grid = new QGridLayout;
        layout->addLayout(grid);
        QDialogButtonBox *box = new QDialogButtonBox(dialog.data());
        box->setOrientation(Qt::Horizontal);
        box->setStandardButtons(QDialogButtonBox::Ok);

        dialog.data()->connect(box, SIGNAL(accepted()), SLOT(accept()));

        for (int i = 0; i < 3; ++i) {
            QString caption = data[i];
            caption.prepend('&');
            QLabel *label = new QLabel(caption);
            label->setAlignment(Qt::AlignRight);

            QSpinBox *spinBox = new QSpinBox;
            spinBox->setObjectName(data[i]);
            spinBox->setRange(mins[i], maxs[i]);
            spinBox->setValue(defaultValues[i]);
            label->setBuddy(spinBox);

            grid->addWidget(label, i, 0, Qt::AlignRight);
            grid->addWidget(spinBox, i, 1, Qt::AlignRight);
        }


        layout->addWidget(box);

        QSpinBox * temp = qFindChild<QSpinBox*>(dialog.data(), data[1]);
        if (temp) temp->setFocus();

        int code = dialog->exec();

        if (code == QDialog::Accepted) {
            temp = qFindChild<QSpinBox*>(dialog.data(), data[0]);
            if (temp) tempo = temp->value();

            temp = qFindChild<QSpinBox*>(dialog.data(), data[1]);
            if (temp) numerator = temp->value();

            temp = qFindChild<QSpinBox*>(dialog.data(), data[2]);
            if (temp) denominator = temp->value();
        }
    }

    qDebug() << Q_FUNC_INFO << tempo << numerator << denominator;
    Munip::StaffData::generateMusicXML(tempo, numerator, denominator);

    QFile file(":/resources/play.html");
    file.open(QIODevice::ReadOnly);
    QByteArray content = file.readAll();

    const QString currentDir = QDir().currentPath();
    m_webView->setHtml(content, QUrl::fromLocalFile(QDir::currentPath() + "/"));
    QDir().setCurrent(currentDir);

    QProcessEnvironment sysEnvironment = QProcessEnvironment::systemEnvironment();
    QString processString = QString("java -jar \"%1\" -nw \"%2/play.xml\"")
                            .arg(sysEnvironment.value("FREEDOTS", "freedots.jar"))
                            .arg(QDir().currentPath());
    m_brailleTranscriptionProcess->start(processString);
}

void MainWindow::slotAboutMunip()
{
    QString aboutText =
        tr(
            "MuNIP is an optical musical notation recognition and processing software\n"
            "\n"
            "Authors (all from PESIT): \n"
            "    Vignesh C <vig.chan@gmail.com>\n"
            "    Karthik N S <kknskk@gmail.com>\n"
            "    Gopala Krishna A <krishna.ggk@gmail.com>\n"
            "\n"
            "Under the guidance of:\n"
            "    Prof Shailaja S S - HOD of IS dept in PESIT"
            );
    QMessageBox::about(this, tr("About MuNIP"), aboutText);
}

void MainWindow::slotStatusMessage(const QString& msg)
{
    m_coordinateLabel->setText(msg);
}

void MainWindow::slotStatusErrorMessage(const QString& msg)
{
    statusBar()->showMessage(msg, 2000);
}

void MainWindow::slotOnSubWindowActivate(QMdiSubWindow *)
{
    ImageWidget *img = activeImageWidget();
    if (!img) {
        m_showGridAction->setEnabled(false);
    }
    else {
        m_showGridAction->setEnabled(true);
        m_showGridAction->blockSignals(true);
        m_showGridAction->setChecked(img->showGrid());
        m_showGridAction->blockSignals(false);
    }
}

void MainWindow::slotOnTranscriptionComplete(int exitCode, QProcess::ExitStatus status)
{
    qDebug() << exitCode << status;
    QString out = QString::fromUtf8(m_brailleTranscriptionProcess->readAllStandardOutput());
    QString err = QString::fromUtf8(m_brailleTranscriptionProcess->readAllStandardError());
    if (exitCode != 0 || status != QProcess::NormalExit) {
        m_brailleView->setText(tr("Transcription failed.. Retry.\n%1\n%2")
                    .arg(out).arg(err));
    } else {
        m_brailleView->setText(out);
    }
    m_tabWidget->setCurrentIndex(1);
}
