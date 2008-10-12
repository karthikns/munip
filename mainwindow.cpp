#include "mainwindow.h"

#include "imagewidget.h"
#include "scanner/staff.h"
#include "tools.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>

struct IDGenerator
{
    static int lastID;

    static int gen()
    {
        return ++lastID;
    }
};

int IDGenerator::lastID = -1;

MainWindow::MainWindow()
{
    m_showGridAction = 0;
    m_mdiArea = new QMdiArea(this);

    setCentralWidget(m_mdiArea);
    setupActions();

    connect(m_mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(slotOnSubWindowActivate(QMdiSubWindow*)));

    setWindowTitle(QString("Music Notation Information Processing (MuNIP)"));
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupActions()
{
    QMenuBar *menuBar = QMainWindow::menuBar();

    QAction *openAction = new QAction(tr("&Open"), this);
    openAction->setShortcuts(QKeySequence::Open);
    openAction->setStatusTip(tr("Opens an image for viewing or processing"));
    connect(openAction, SIGNAL(triggered()), this, SLOT(slotOpen()));

    QAction *saveAction = new QAction(tr("&Save"), this);
    saveAction->setShortcuts(QKeySequence::Save);
    saveAction->setStatusTip(tr("Saves an image"));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(slotSave()));

    QAction *saveAsAction = new QAction(tr("Save &As"), this);
    saveAsAction->setStatusTip(tr("Prompts for a new filename for the image to be saved."));
    connect(saveAsAction, SIGNAL(triggered()), this, SLOT(slotSaveAs()));

    QAction *closeAction = new QAction(tr("&Close window"), this);
    closeAction->setShortcuts(QKeySequence::Close);
    closeAction->setStatusTip(tr("Closes the image"));
    connect(closeAction, SIGNAL(triggered()), this, SLOT(slotClose()));

    QAction *quitAction = new QAction(tr("&Quit"), this);
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


    QAction *zoomInAction = new QAction(tr("Zoom &in"), this);
    zoomInAction->setShortcuts(QKeySequence::ZoomIn);
    zoomInAction->setStatusTip(tr("Zooms in the image"));
    connect(zoomInAction, SIGNAL(triggered()), this, SLOT(slotZoomIn()));

    QAction *zoomOutAction = new QAction(tr("Zoom &out"), this);
    zoomOutAction->setShortcuts(QKeySequence::ZoomOut);
    zoomOutAction->setStatusTip(tr("Zooms out the image"));
    connect(zoomOutAction, SIGNAL(triggered()), this, SLOT(slotZoomOut()));

    m_showGridAction = new QAction(tr("Show grid"), this);
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

    QAction *toMonochromeAction = new QAction(tr("Convert to &monochrome"), this);
    toMonochromeAction->setShortcut(tr("Ctrl+M"));
    toMonochromeAction->setStatusTip(tr("Converts the active image to monochrome"));
    connect(toMonochromeAction, SIGNAL(triggered()), this, SLOT(slotConvertToMonochrome()));

    QAction *removeLinesAction = new QAction(tr("&Remove lines"), this);
    removeLinesAction->setShortcut(tr("Ctrl+R"));
    removeLinesAction->setStatusTip(tr("Removes the horizontal staff lines from the image"));
    connect(removeLinesAction, SIGNAL(triggered()), this, SLOT(slotRemoveLines()));

    QMenu *processMenu = menuBar->addMenu(tr("&Process"));
    processMenu->addAction(toMonochromeAction);
    processMenu->addAction(removeLinesAction);

    menuBar->addSeparator();

    QAction *aboutAction = new QAction(tr("About MuNIP"), this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(slotAboutMunip()));

    QMenu *helpMenu = menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
}

ImageWidget* MainWindow::activeImageWidget() const
{
    QMdiSubWindow *sub = m_mdiArea->activeSubWindow();
    if (!sub) {
        return 0;
    }

    return dynamic_cast<ImageWidget*>(sub->widget());

}

void MainWindow::slotOpen()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"), QString(),
                                                    tr("Images (*.png *.xpm *.jpg *.bmp)"));
    if (!fileName.isEmpty()) {
        ImageWidget *imgWidget = new ImageWidget(fileName);
        imgWidget->setWidgetID(IDGenerator::gen());
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

void MainWindow::slotConvertToMonochrome()
{
    ImageWidget *imgWidget = activeImageWidget();
    if (!imgWidget) {
        return;
    }

    QPixmap mono = QPixmap::fromImage(Munip::convertToMonochrome(imgWidget->image()));
    ImageWidget *monoWidget = new ImageWidget(mono);
    monoWidget->setWidgetID(IDGenerator::gen());
    monoWidget->setProcessorWidget(imgWidget);

    QMdiSubWindow *sub = m_mdiArea->addSubWindow(monoWidget);
    sub->widget()->setAttribute(Qt::WA_DeleteOnClose);
    sub->show();
}

void MainWindow::slotRemoveLines()
{
    ImageWidget *imgWidget = activeImageWidget();
    if (!imgWidget) {
        return;
    }

    Munip::StaffLineRemover remover(imgWidget->image());
    remover.removeLines2();

    ImageWidget *processedImageWidget = new ImageWidget(QPixmap::fromImage(remover.processedImage()));
    processedImageWidget->setWidgetID(IDGenerator::gen());
    processedImageWidget->setProcessorWidget(imgWidget);

    QMdiSubWindow *sub = m_mdiArea->addSubWindow(processedImageWidget);
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
            "Vignesh C <vig.chan@gmail.com>\n"
            "Karthik N S <kknskk@gmail.com>\n"
            "Gopala Krishna A <krishna.ggk@gmail.com>\n"
            "\n"
            "Under the guidance of:\n"
            "Prof Shailaja S S - HOD of IS dept in PESIT"
            );
    QMessageBox::about(this, tr("About MuNIP"), aboutText);
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
