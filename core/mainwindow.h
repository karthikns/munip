#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>

class ImageWidget;
class QLabel;
class QMdiArea;
class QMdiSubWindow;
class QProcess;
class QTabWiget;
class QTextEdit;
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
    void slotOnTranscriptionComplete(int exitCode, QProcess::ExitStatus status);

private:
    void setup2ndTab();
    void setupActions();
    void applyStyle();

    QAction *m_showGridAction;
    QTabWidget *m_tabWidget;
    QWebView *m_webView;
    QTextEdit *m_brailleView;
    QMdiArea *m_mdiArea;
    QProcess *m_brailleTranscriptionProcess;

    QLabel *m_coordinateLabel;
    static MainWindow* m_instance;
};

#endif //MAINWINDOW_H
