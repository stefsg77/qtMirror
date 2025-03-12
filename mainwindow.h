#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QFile>
#include "QtMirror.h"
#include "conversions.h"

#define MAXI_LINES  20

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool maybeSave();
    void fillTable();
    void saveFile();
    QString volumeName(){
        return QString(m_volumeName);
    }
    void convertLine(MirwCmde* pMC);
    void interuptTimer();
    void restartTimer();

public slots:
    void loadList();
    void saveList();
    void saveListAs();
    void selectAll();
    void executeList();
    void executeStudy();
    void simulateList();
    void toggleSimul(int);
    void volumeChanged(QString vol);
    void addLine();
    void editLine();
    void removeLine();
    void moveLineUp();
    void moveLineDown();
    void restoreList();
    void restoreImage();
    void viewLogFile();
    void putText(QString &s);
    void putFileInfo(sfileOutput sfo);
    void timerLoop();
    void resetProgressBar();
    void cellClicked();
    void setProgressBar(int value, int value2, int value3 = 0);
    void setFileName(char* arg) {m_fileName = arg;}
    void about();
signals:
    void progressChanged(int value);
    void progressChanged2(int value);
    void progressChanged3(int value);

private:
    Ui::MainWindow *ui;

    void createActions();
    void createMenus();

    QMenu *m_fileMenu;
    QMenu *m_actionMenu;
    QMenu *m_helpMenu;
    QAction *openCde;
    QAction *saveCde;
    QAction *saveAsCde;
    QAction *exitAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *viewLogAct;
    QAction *restoreFromImageAct;
    QAction *runCommandsAct;

    bool m_listModified;
    bool m_isRunning;
    bool m_simul;
    bool m_study;
    bool m_restore;
    bool m_bSelection;
    bool m_bTimerOn;
    bool m_bTimerInterupt;
    int m_lines;
    int m_index;
    int m_lastProgress;
    int m_lastProgress2;
    int m_lastProgress3;
    unsigned short m_lastAction;
    MirwCmde m_list[MAXI_LINES];
    MirwCmdeHead m_head;
    QString m_volumeName;
    QString m_fileName;
    QStringList m_actions;
    QStringList m_headers;
    int m_tested;
    int m_copied;
    int m_deleted;
    int m_errors;
    int m_indexE;
    qint64 m_copySize;
    QTimer *timer;
    QFile logFile;
    QFile studyFile;
    Conversions convert;
    QVector<bool> m_vState;
};

#endif // MAINWINDOW_H
