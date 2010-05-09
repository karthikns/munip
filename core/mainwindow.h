#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ImageWidget;
class QLabel;
class QMdiArea;
class QMdiSubWindow;
class QTabWiget;
class QWebView;

class MainWindow : public QMainWindow
{
    Q_OBJECT;
public:
    MainWindow();
    ~MainWindow();

    ImageWidget* activeImageWidget() const;
    void addSubWindow(QWidget *widget);

    static MainWindow* instance();

public slots:
    void slotOpen();
    void slotSave();
    void slotSaveAs();
    void slotClose();
    void slotCloseAll();
    void slotQuit();

    void slotZoomIn();
    void slotZoomOut();
    void slotToggleShowGrid(bool b);

    void slotProjection();
    void slotPlay();

    void slotAboutMunip();

    void slotStatusMessage(const QString& status);
    void slotStatusErrorMessage(const QString& status);

private slots:
    void slotOnSubWindowActivate(QMdiSubWindow *);

private:
    void setupWebView();
    void setupActions();
    void applyStyle();

    QAction *m_showGridAction;
    QTabWidget *m_tabWidget;
    QWebView *m_webView;
    QMdiArea *m_mdiArea;

    QLabel *m_coordinateLabel;
    static MainWindow* m_instance;
};

#endif //MAINWINDOW_H
