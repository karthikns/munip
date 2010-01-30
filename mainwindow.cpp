#include "mainwindow.h"

#include "imagewidget.h"
#include "projection.h"
#include "processstep.h"
#include "sidebar.h"
#include "tools.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

MainWindow* MainWindow::m_instance = 0;

MainWindow::MainWindow()
{
    m_showGridAction = 0;
    m_mdiArea = new QMdiArea(this);
    m_coordinateLabel = new QLabel(this);

    setCentralWidget(m_mdiArea);
    setupActions();

    connect(m_mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(slotOnSubWindowActivate(QMdiSubWindow*)));

    setWindowTitle(QString("Music Notation Information Processing (MuNIP)"));
    // Ensure only one instance of this MainWindow exists.
    Q_ASSERT_X(!m_instance, "MainWindow construction", "MainWindow already exists!");

    m_instance = const_cast<MainWindow*>(this);
}

MainWindow::~MainWindow()
{
}

MainWindow* MainWindow::instance()
{
    return m_instance;
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

    QAction *quitAction = new QAction(QIcon(":/resources/quit.png"), tr("&Quit"), this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    quitAction->setStatusTip(tr("Quit application"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(slotQuit()));

    QMenu *fileMenu = menuBar->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addAction(closeAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);
    QToolBar *fileBar = addToolBar(tr("&File"));
    fileBar->addAction(openAction);
    fileBar->addAction(saveAction);
    fileBar->addAction(saveAsAction);
    fileBar->addAction(closeAction);
    fileBar->addAction(quitAction);


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
    QToolBar *viewBar = addToolBar(tr("&View"));
    viewBar->addAction(zoomInAction);
    viewBar->addAction(zoomOutAction);
    viewBar->addAction(m_showGridAction);

    QAction *toGrayScaleAction = new Munip::ProcessStepAction("GrayScaleConversion", QIcon(),
                                                              tr("&GrayScale conversion"), this);
    toGrayScaleAction->setShortcut(tr("Ctrl+1"));

    QAction *toMonochromeAction = new Munip::ProcessStepAction("MonoChromeConversion", QIcon(),
                                                               tr("Convert to &monochrome"), this);
    toMonochromeAction->setShortcut(tr("Ctrl+2"));
    toMonochromeAction->setStatusTip(tr("Converts the active image to monochrome"));

    QAction *correctSkewAction = new Munip::ProcessStepAction("SkewCorrection", QIcon(),
                                                              tr("Correct &Skew"), this);
    correctSkewAction->setShortcut(tr("Ctrl+3"));

    QAction *staffLineDetect = new Munip::ProcessStepAction("StaffLineDetect", QIcon(),
                                                             tr("&Detect Staff lines"), this);
    staffLineDetect->setShortcut(tr("Ctrl+4"));

    QAction *staffLineRemoval = new Munip::ProcessStepAction("StaffLineRemoval", QIcon(),
                                                             tr("&Remove Staff lines"), this);
    staffLineRemoval->setShortcut(tr("Ctrl+5"));


    QAction *staffParamAction = new Munip::ProcessStepAction("StaffParamExtraction", QIcon(),
                                                             tr("&Staff Parameter extraction"), this);
    staffParamAction->setShortcut(tr("Ctrl+6"));
    staffParamAction->setStatusTip(tr("Computes staff space height and staff line height of a deskewewd image"));;

    QAction *rotation = new Munip::ProcessStepAction("ImageRotation", QIcon(),
                                                     tr("&Rotate image"), this);
    rotation->setShortcut(tr("Ctrl+7"));

    QAction *cluster = new Munip::ProcessStepAction("ImageCluster", QIcon(),tr("Cluster Image"), this);
    cluster->setShortcut(tr("Ctrl+8"));

    QAction *projectionAction = new QAction(tr("&Projection"), this);
    projectionAction->setShortcut(tr("Ctrl+P"));
    projectionAction->setStatusTip(tr("Calculates horizontal projection of the image"));;
    connect(projectionAction, SIGNAL(triggered()), this, SLOT(slotProjection()));

    QMenu *processMenu = menuBar->addMenu(tr("&Process"));
    processMenu->addAction(toGrayScaleAction);
    processMenu->addAction(toMonochromeAction);
    processMenu->addAction(correctSkewAction);
    processMenu->addAction(staffLineDetect);
    processMenu->addAction(staffLineRemoval);
    processMenu->addAction(staffParamAction);
    processMenu->addAction(rotation);
    processMenu->addAction(cluster);
    processMenu->addAction(projectionAction);

    SideBar *processBar = new SideBar();
    processBar->addAction(toGrayScaleAction);
    processBar->addAction(toMonochromeAction);
    processBar->addAction(correctSkewAction);
    processBar->addAction(staffLineDetect);
    processBar->addAction(staffLineRemoval);
    processBar->addAction(staffParamAction);
    processBar->addAction(rotation);
    processBar->addAction(cluster);
    processBar->addAction(projectionAction);

    QDockWidget *dock = new QDockWidget(tr("Process"), this);
    dock->setWidget(processBar);
    addDockWidget(Qt::LeftDockWidgetArea, dock, Qt::Vertical);

    menuBar->addSeparator();

    QAction *aboutAction = new QAction(QIcon(":/resources/about.png"), tr("About MuNIP"), this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(slotAboutMunip()));

    QMenu *helpMenu = menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
    QToolBar *helpBar = addToolBar("&Help");
    helpBar->addAction(aboutAction);

    QStatusBar *s = statusBar(); //create statusbar
    s->addPermanentWidget(m_coordinateLabel);
    m_coordinateLabel->show();
    m_coordinateLabel->setText("hello");
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
                                                    QDir::currentPath() + QDir::separator() + QString("images"),
                                                    tr("Images (*.png *.xpm *.jpg *.bmp)"));
    if (!fileName.isEmpty()) {
        ImageWidget *imgWidget = new ImageWidget(fileName);
        imgWidget->setWidgetID(Munip::IDGenerator::gen());
        QMdiSubWindow *sub = m_mdiArea->addSubWindow(imgWidget);
        sub->widget()->setAttribute(Qt::WA_DeleteOnClose);
        sub->show();
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
