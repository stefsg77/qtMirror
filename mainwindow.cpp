#include <QtCore>
#include <QFileDialog>
#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialogeditcommand.h"
#include "dialoglistimage.h"
#include "dialogviewlog.h"
#include "scanner.h"
#include "reverttorestore.h"
#include "vieweditstudy.h"
#include "QCheckBox"

scanner *m_scan;
char crlf[] = { 13, 10, 0};
unsigned char version[] = {0,9,0};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QStringList args = QCoreApplication::arguments();
    createActions();
    createMenus();
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(loadList()));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(saveList()));
    connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(executeList()));
    connect(ui->pushButton_5, SIGNAL(clicked()), this, SLOT(editLine()));
    connect(ui->pushButton_4,SIGNAL(clicked()), this, SLOT(addLine()));
    connect(ui->pushButton_6, SIGNAL(clicked()), this, SLOT(moveLineUp()));
    connect(ui->pushButton_7, SIGNAL(clicked()), this, SLOT(moveLineDown()));
    connect(ui->pushButton_8,SIGNAL(clicked()),this, SLOT(removeLine()));
    connect(ui->pushButton_9, SIGNAL(clicked()), this, SLOT(executeStudy()));
    connect(ui->pushButton_10, SIGNAL(clicked()), this, SLOT(restoreList()));
    connect(ui->pushButton_11, SIGNAL(clicked()), this, SLOT(selectAll()));
    connect(ui->checkBox, SIGNAL(stateChanged(int)), this, SLOT(toggleSimul(int)));
    connect(ui->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(volumeChanged(QString)));
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerLoop()));
    m_simul = false;
    m_index = 0;
    m_indexE = 0;
    m_lines = 0;
    m_lastAction = 0;
    m_listModified = false;
    m_isRunning = false;
    m_bSelection = false;
    m_bTimerOn = false;
    m_scan = 0;
    m_actions << tr("REFLETER") << tr("SYNCHRONISER") << tr("SAUVEGARDER") << tr("COMPARAISON") << tr("COMPARAISON BIN") << tr("FICHIER IMAGE");
    ui->tableWidget->setColumnCount(5);
    int w = ui->tableWidget->width()/16;
    ui->tableWidget->setColumnWidth(0,w);
    ui->tableWidget->setColumnWidth(1,w*4);
    ui->tableWidget->setColumnWidth(2,w*4);
    ui->tableWidget->setColumnWidth(3,w*4);
    ui->tableWidget->setColumnWidth(4,w*3);
    m_headers << tr("Actif") << tr("Origine") << tr("Destination") << tr("Répertoire") << tr("Action");
    ui->tableWidget->setHorizontalHeaderLabels(m_headers);
    ui->pushButton->setText(tr("Charger un fichier de commandes"));
    ui->pushButton_2->setText(tr("Sauvegarder le fichier de commandes"));
    ui->pushButton_3->setText(tr("Exécuter les lignes sélectionnées"));
    ui->pushButton_9->setText(tr("Exécuter pas a pas la ligne sélectionnée"));
    ui->pushButton_10->setText(tr("Restauration des lignes sélectionnées"));
    ui->pushButton_11->setText(tr("Inverser la sélection"));
    ui->pushButton_4->setText(tr("Ajouter une ligne"));
    ui->pushButton_5->setText(tr("Editer la ligne"));
    ui->pushButton_6->setText(tr("Déplacer vers le haut"));
    ui->pushButton_7->setText(tr("Déplacer vers le bas"));
    ui->pushButton_8->setText(tr("Effacer la ligne"));
    ui->checkBox->setText(tr("En simulation seulement"));
    ui->label->setText(tr("Nom du Volume"));
    ui->label_2->setText(tr("Fichier"));
    ui->label_3->setText(tr("Ligne"));
    ui->label_4->setText(tr("Total"));
//    ui->progressBar->setRange(0, 100);
//    ui->progressBar->setValue(0);
    connect( this, SIGNAL(progressChanged(int)), ui->progressBar, SLOT(setValue(int)));
    connect( this, SIGNAL(progressChanged2(int)), ui->progressBar_2, SLOT(setValue(int)));
    connect( this, SIGNAL(progressChanged3(int)), ui->progressBar_3, SLOT(setValue(int)));
    m_lastProgress = m_lastProgress2 = m_lastProgress3 = 0;
    int nbArgs = args.count();
    if(nbArgs > 1) m_fileName = args.at(1);
    if(!m_fileName.isNull())
    {
        qint64 size;
        MirwCmdeHead mh;
        QFile file;
        file.setFileName(m_fileName);
        ui->label_5->setText(m_fileName);
        mh.dwCommandCount = 0;
        if (!file.open(QIODevice::ReadOnly)) return;
        size = file.read((char*)&mh, sizeof(mh));
        if( !size )
        {
            file.close();
            m_lines = 0;
            m_listModified = false;
            return;
        }
        m_head = mh;
        m_lines = mh.dwCommandCount;
        if( mh.dwSize >= sizeof(mh)) m_volumeName = mh.szVolumeName;
        else m_volumeName.clear();
        convert.setVolume(mh.szVolumeName);
        m_index = 0;
        int i;
        m_vState.clear();
        file.seek(mh.dwSize);
        for( i = 0; i < m_lines && i < MAXI_LINES; ++i)
        {
            MirwCmde mc;
            size = file.read((char*)&mc, mh.dwCommandSize);
            if(size != mh.dwCommandSize) break;
            convert.convertLine(&mc);
            m_list[i] = mc;
            m_vState.append(true);
        }
        file.close();
        m_listModified = false;
        fillTable();
        m_bSelection = false;
        if(nbArgs > 2 && args.at(2) == "-go")
            executeList();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createActions()
{
    openCde = new QAction(tr("&Charger un fichier de commandes..."), this);
    openCde->setShortcuts(QKeySequence::New);
    openCde->setStatusTip(tr("Rechercher un fichier de commandes existant"));
    connect(openCde, SIGNAL(triggered()), this, SLOT(loadList()));

    saveCde = new QAction(tr("&Sauver le fichier de commande"), this);
    saveCde->setShortcuts(QKeySequence::Save);
    saveCde->setStatusTip(tr("Sauvegarder le dans un fichier la liste des commandes"));
    connect(saveCde, SIGNAL(triggered()), this, SLOT(saveList()));

    saveAsCde = new QAction(tr("Sauver sous..."), this);
    saveAsCde->setShortcuts(QKeySequence::SaveAs);
    saveAsCde->setStatusTip(tr("Sauvegarder la liste sous un autre nom"));
    connect(saveAsCde, SIGNAL(triggered()), this, SLOT(saveListAs()));

    exitAct = new QAction(tr("&Quitter"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Quitter QtMirror"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    aboutAct = new QAction(tr("&A Propos"), this);
    aboutAct->setStatusTip(tr("Affiche la fenêtre A propos"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("A propos de Qt"), this);
    aboutQtAct->setStatusTip(tr("Donner les informations sur la librairieQt utilisée"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    viewLogAct = new QAction(tr("Voir l'historique"), this);
    viewLogAct->setStatusTip(tr("Ouvrir le fichier log pour voir l'historique des opérations"));
    connect(viewLogAct, SIGNAL(triggered()), this, SLOT(viewLogFile()));

    runCommandsAct = new QAction(tr("Lancer les opérations"), this);
    runCommandsAct->setStatusTip(tr("Lancer les opérations de toutes les lignes sélectionnées"));
    connect(runCommandsAct, SIGNAL(triggered()), this, SLOT(executeList()));

    restoreFromImageAct = new QAction(tr("Restaurer depuis des fichiers image"), this);
    restoreFromImageAct->setStatusTip(tr("Permet de voir et sélectionner des fichiers à restaurer depuis des sauvegardes image"));
    connect(restoreFromImageAct, SIGNAL(triggered()), this, SLOT(restoreImage()));
}

void MainWindow::createMenus()
{
    m_fileMenu = ui->menuBar->addMenu(tr("&Fichier"));
    m_fileMenu->addAction(openCde);
    m_fileMenu->addAction(saveCde);
    m_fileMenu->addAction(saveAsCde);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(viewLogAct);
    m_fileMenu->addAction(exitAct);

    ui->menuBar->addSeparator();
    m_actionMenu = menuBar()->addMenu(tr("&Actions"));
    m_actionMenu->addAction(runCommandsAct);
    m_actionMenu->addAction(restoreFromImageAct);

    ui->menuBar->addSeparator();
    m_helpMenu = menuBar()->addMenu(tr("&Aide"));
    m_helpMenu->addAction(aboutAct);
    m_helpMenu->addAction(aboutQtAct);
}

void MainWindow::about()
{
    QString s = tr("<b>QtMirror</b> est un logiciel open source\n"
                   "de copies de sauvegardes multiplateforme\n"
                   "développé par Stéphane Gaspar  - (2024)\n "
                   "basé sur la bibliothèque Qt et sur 7zip de Igor Pavlov.\n "
                   "contact : stephane.gaspar@gmail.com -");
    QString v = tr(" version : %1.%2%3");
    v = v.arg(version[0]).arg(version[1]).arg(version[2]);
    s.append(v);

    QMessageBox::about(this, "QtMirror", s );
}

void MainWindow::toggleSimul(int s)
{
    m_simul = (bool)s;
}

void MainWindow::volumeChanged(QString vol)
{
    m_volumeName = vol;
}

void MainWindow::cellClicked()
{
    int row;
    QCheckBox* cb;
    for(row = 0; row < m_lines; row++)
    {
        cb = (QCheckBox*)ui->tableWidget->cellWidget(row, 0);
        m_vState[row] = cb->checkState();
    }
}

void MainWindow::resetProgressBar()
{
//    ui->progressBar->setValue(0);
    if(m_lastProgress > 0)
    {
        emit progressChanged(110);
        m_lastProgress = 0;
    }
}

void MainWindow::setProgressBar(int value, int value2, int value3)
{
    if( value != m_lastProgress)
    {
        m_lastProgress = value;
        emit progressChanged(value);
    }
    if( value2 != m_lastProgress2)
    {
        m_lastProgress2 = value2;
        emit progressChanged2(value2);
    }
    if( value3 && (value3 != m_lastProgress3))
    {
        m_lastProgress3 = value3;
        emit progressChanged3(value3);
    }
}

void MainWindow::convertLine(MirwCmde *pMC)
{
    // convertit la ligne pour assurer compatibilité
    // 1: remplacer contre-slash par slash
    int i;
    int l;
    l = strlen(pMC->szOrgPath);
    for( i = 0 ; i < l; ++i)
    {
        if( pMC->szOrgPath[i] == '\\' ) pMC->szOrgPath[i] = '/';
    }
    l = strlen(pMC->szDestPath);
    for( i = 0 ; i < l; ++i)
    {
        if( pMC->szDestPath[i] == '\\' ) pMC->szDestPath[i] = '/';
    }
    l = strlen(pMC->szCommPath);
    for( i = 0 ; i < l; ++i)
    {
        if( pMC->szCommPath[i] == '\\' ) pMC->szCommPath[i] = '/';
    }
    // 2: vérifier que pour windows les chemins commencent par unité:
    // et pour MAC, si commence par unité: remplacer par volume ou root (c: devient '/' et au delà devient "/Volumes/+VOLUMENAME")
}

void MainWindow::loadList()
{
    qint64 size;
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Ouvrir une liste"), QDir::homePath(), tr("Liste de commandes (*.mrw)"));
        if (!fileName.isEmpty())
        {
            m_fileName = fileName;
            ui->label_5->setText(m_fileName);
            MirwCmdeHead mh;
            QFile file;
            file.setFileName(fileName);
            mh.dwCommandCount = 0;
            file.open(QIODevice::ReadOnly);
            size = file.read((char*)&mh, sizeof(mh));
            if( !size )
            {
                file.close();
                m_lines = 0;
                m_listModified = false;
                return;
            }
            m_head = mh;
            m_lines = mh.dwCommandCount;
            if( mh.dwSize >= sizeof(mh)) m_volumeName = mh.szVolumeName;
            else m_volumeName.clear();
            convert.setVolume(mh.szVolumeName);
            m_index = 0;
            int i;
            m_vState.clear();
            file.seek(mh.dwSize);
            for( i = 0; i < m_lines && i < MAXI_LINES; ++i)
            {
                MirwCmde mc;
                size = file.read((char*)&mc, mh.dwCommandSize);
                if(size != mh.dwCommandSize) break;
                convert.convertLine(&mc);
                m_list[i] = mc;
                m_vState.append(true);
//                file.read((char*)&m_list[i], sizeof(uint));
//                file.read((char*)&m_list[i].szOrgPath, m_list[i].dwSize);
            }
            file.close();
            m_listModified = false;
            fillTable();
            m_bSelection = false;
//            selectAll();
        }
    }
}

bool MainWindow::maybeSave()
{
    if (m_listModified)
    {
        QMessageBox message;
        QPushButton *saveButton = message.addButton(tr("Sauvegarder"), QMessageBox::ActionRole);
        QPushButton *cancelButton = message.addButton(tr("Abandonner"), QMessageBox::RejectRole);
    //    QPushButton *discardButton = message.addButton(tr("Ignorer modifications"), QMessageBox::DestructiveRole);
        message.setWindowTitle("QtMirror");
        message.setText(tr("La liste a été créée ou modifiée.\nVoulez-vous l'enregistrer ?"));
        message.exec();
        if( message.clickedButton() == saveButton )
        {
            saveList();
            return true;
        }
        else if(message.clickedButton() == cancelButton ) return false;
    }
    return true;
}

void MainWindow::saveList()
{
    if( m_listModified && m_lines)
    {
        if(m_fileName.isEmpty())
        {
            saveListAs();
        }
        else
        {
            saveFile();
            m_listModified = false;
        }
    }

}

void MainWindow::saveFile()
{
    MirwCmdeHead mh;
    memset(&mh,0,sizeof(mh));
    mh.dwCommandCount = m_lines;
    mh.dwCommandSize = sizeof(MirwCmde);
    mh.dwSize = sizeof(MirwCmdeHead);
    strcpy_s(mh.szSignature,"Miroir");
    strcpy_s(mh.szVolumeName, m_volumeName.toUtf8().data());
    if (!m_fileName.isEmpty())
    {
        QFile file;
        if( !m_fileName.endsWith(".mrw") ) m_fileName.append(".mrw");
        file.setFileName(m_fileName);
        file.open(QIODevice::ReadWrite);
        file.write((char*)&mh, sizeof(mh));
        int i;
        for( i = 0; i < m_lines; ++i)
        {
            MirwCmde mc;
            mc = m_list[i];
            mc.dwSize = sizeof(mc);
#ifdef Q_OS_WIN
            if( (mc.useVolume & 1) && (mc.szOrgPath[0] != '/') ) mc.szOrgPath[0] = 'v';
            if( (mc.useVolume & 2) && (mc.szDestPath[0] != '/') ) mc.szDestPath[0] = 'v';
#endif
            mc.useVolume = 0;
            file.write((char*)&mc, sizeof(mc));
        }
        file.close();
    }

}

void MainWindow::saveListAs()
{
    if( m_listModified && m_lines)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Sauver liste"),
                                                        QDir::homePath(),
                                                        tr("Liste de commande (*.mrw)"));
        if (!fileName.isEmpty())
        {
            m_fileName = fileName;
            saveFile();
        }
    }

}

void MainWindow::selectAll()
{
    int i;
    QCheckBox *cb;
    if(m_bSelection)  // all selected activé
    {
        // unselect all
        for(i = 0; i < m_lines; ++i)
        {
            cb = (QCheckBox *)ui->tableWidget->cellWidget(i,0);
            cb->setChecked(false);
        }
        m_bSelection = false;
//        ui->tableWidget->clearSelection();
;    }
    else
    {
        // select all
        for(i = 0; i < m_lines; ++i)
        {
            cb = (QCheckBox *)ui->tableWidget->cellWidget(i,0);
            cb->setChecked(true);
        }
        m_bSelection = true;
//        ui->tableWidget->selectAll();
;   }
}

void MainWindow::fillTable()
{
    int i;
    QCheckBox* cb;
    ui->lineEdit->setText(m_volumeName);
    ui->tableWidget->setRowCount(m_lines);
    for( i = 0; i < m_lines; ++i)
    {
        ui->tableWidget->setCellWidget(i, 0, new QCheckBox);
        cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
        cb->setChecked(m_vState[i]);
        connect(cb, SIGNAL(clicked()), this, SLOT(cellClicked()));
        QTableWidgetItem *wi = new QTableWidgetItem(m_list[i].szOrgPath);
        ui->tableWidget->setItem(i, 1, wi);
        wi = new QTableWidgetItem(m_list[i].szDestPath);
        ui->tableWidget->setItem(i, 2, wi);
        wi = new QTableWidgetItem(m_list[i].szCommPath);
        ui->tableWidget->setItem(i, 3, wi);
        wi = new QTableWidgetItem(m_actions[m_list[i].wAction-REFLECT]);
        ui->tableWidget->setItem(i, 4, wi);
    }
    ui->tableWidget->selectRow(0);
}

void MainWindow::editLine()
{
    int i = ui->tableWidget->currentRow();
    if( i < 0 || i > m_lines) {
        i = 0;
        ui->tableWidget->selectRow(0);
    }
    DialogEditCommand *dec = new DialogEditCommand(m_list[i], this);
    if(dec->exec())
    {
        MirwCmde mc = dec->readForm();
        convert.setVolume(m_volumeName);
        convert.convertLine(&mc);
        m_list[i] = mc;
        fillTable();
        m_listModified = true;
    }
}

void MainWindow::addLine()
{
    MirwCmde mc;
    memset((void*)&mc, 0, sizeof(mc));
    mc.wAction = DEFAULTACTION;
    mc.wOptions = DEFAULTOPTIONS;
    strcpy(mc.szFileSpec,"*.*");
    strcpy(mc.szDirSpec, "*.*");
    strcpy_s(mc.szOrgPath, QDir::homePath().toLatin1().data());
    int i = m_lines;
    DialogEditCommand *dec = new DialogEditCommand(mc, this);
    if(dec->exec())
    {
        MirwCmde mc = dec->readForm();
        convert.setVolume(m_volumeName);
        convert.convertLine(&mc);
        m_list[i] = mc;
        m_list[i].dwSize = sizeof(MirwCmde);
        m_lines++;
        m_vState.append(true);
        fillTable();
        m_listModified = true;
    }

}

void MainWindow::removeLine()
{
    int i = ui->tableWidget->currentRow();
    if( i < m_lines)
    {
        for(int j = i+1; j < m_lines;++j)
        {
            m_list[j-1] = m_list[j];
        }
        --m_lines;
        fillTable();
    }
}

void MainWindow::moveLineUp()
{
    MirwCmde mc;
    int i = ui->tableWidget->currentRow();
    if( i > 0)
    {
        mc = m_list[i];
        m_list[i] = m_list[i-1];
        m_list[i-1] = mc;
        fillTable();
        ui->tableWidget->selectRow(i-1);
    }
}

void MainWindow::moveLineDown()
{
    MirwCmde mc;
    int i = ui->tableWidget->currentRow();
    if( i < (m_lines-1))
    {
        mc = m_list[i];
        m_list[i] = m_list[i+1];
        m_list[i+1] = mc;
        fillTable();
        ui->tableWidget->selectRow(i+1);
    }
}

void MainWindow::simulateList()
{
    if( m_isRunning || !m_lines ) return;
    m_simul = true;
    m_restore = false;
    // créer logFile
    QString fileName = QDir::homePath();
    fileName.append("/QtMirror.log");
    logFile.setFileName(fileName);
    logFile.open(QIODevice::ReadWrite);
    timer->start(500);
    m_bTimerOn = true;
    m_tested = m_copied = m_deleted = m_errors = 0;
    m_copySize = 0;
    m_indexE = 0;
    m_isRunning = true;
}

void MainWindow::restoreList()
{
    if( m_isRunning || !m_lines ) return;
    QMessageBox message;
    QPushButton *doButton = message.addButton(tr("Continuer"), QMessageBox::ActionRole);
    QPushButton *cancelButton = message.addButton(tr("Abandonner"), QMessageBox::RejectRole);
    message.setWindowTitle("QtMirror");
    message.setText(tr("Etes vous sûr de vouloir restaurer depuis la dernière sauvegarde\n(Ecraser les données actuelles) ?\n"
                       "Pour le fichiers Image, utiliser la fonction dédiée"));
    message.exec();
    if( message.clickedButton() == doButton )
    {
        m_restore = true;
        // créer logFile
        QString fileName = QDir::homePath();
        fileName.append("/QtMirror.log");
        logFile.setFileName(fileName);
        logFile.open(QIODevice::ReadWrite);
        timer->start(500);
        m_bTimerOn = true;
        m_tested = m_copied = m_deleted = m_errors = 0;
        m_copySize = 0;
        m_indexE = 0;
        m_isRunning = true;
        return;
    }
    else if(message.clickedButton() == cancelButton ) return;
}

void MainWindow::viewLogFile()
{
    DialogViewLog *dvl = new DialogViewLog(this);
    dvl->exec();
    return;
}

void MainWindow::timerLoop()
{
    QCheckBox* cb;
    MirwCmde mc;
    if( !m_isRunning ) return;
    if(m_indexE > m_lines)
    {
        m_isRunning = false;
        m_indexE = 0;
        return;
    }
    if( !m_scan && m_indexE < m_lines)
    {
        // première passe
        cb = (QCheckBox*)ui->tableWidget->cellWidget(m_indexE, 0);
        if( cb->isChecked() )
        {
            ui->tableWidget->selectRow(m_indexE);
            ui->tableWidget->update();
            mc = m_list[m_indexE];
            if( m_simul ) mc.wOptions |= SIMULATE;
            if( m_restore )
            {
                if(mc.wAction != IMAGE)
                {
                    m_lastAction = RevertToRestore::revert(mc);
                }
                else
                {
                    m_indexE++;
                    return;
                }
            }
            emit progressChanged2(110);
            m_lastProgress2 = 0;
            emit progressChanged3(110);
            m_lastProgress3 = 0;
            m_scan = new scanner(mc, this);
            m_scan->start();
        }
        m_indexE++;
        return;
    }
    // vérifier si le travail est fini
    if(m_scan)
    {
        if(m_scan->isRunning() ) return; // non, alors sortir
        // a partir d'ici, scanner a fini son travail, récupérer les totaux, le détruire et voir si il y a un suivant
        m_copied += m_scan->copied();
        m_tested += m_scan->scanned();
        m_deleted += m_scan->deleted();
        m_errors += m_scan->errors();
        m_copySize += m_scan->copySize();
        delete m_scan;
        m_scan = nullptr;
    }
    if( m_indexE < m_lines )
    {
        // suivant
        ui->tableWidget->selectRow(m_indexE);
        ui->tableWidget->update();
        mc = m_list[m_indexE];
        if( m_simul ) mc.wOptions |= SIMULATE;
        if( m_restore && m_lastAction)RevertToRestore::reRevert(mc, m_lastAction);
        emit progressChanged2(110);
        m_lastProgress2 = 0;
        m_lastProgress3 = (m_indexE * 100) / m_lines;
        emit progressChanged3(m_lastProgress3);
        m_scan = new scanner(mc, this);
        m_scan->start();
        m_indexE++;
    }
    else
    {
        // c'est fini
        timer->stop();
        m_bTimerOn = false;
        m_lastAction = 0;
        emit progressChanged3(100);
        m_isRunning = false;
        m_scan = 0;
        // afficher le résultat
        QString s;
        s = QString(tr("%1 fichiers testés, %2 copiés, %3 effacés et %4 erreurs")).arg(m_tested).arg(m_copied).arg(m_deleted).arg(m_errors);
        putText(s);
        logFile.resize(logFile.pos());
        logFile.close();
    }
    return;
}

void MainWindow::interuptTimer()
{
    if(!m_bTimerOn || m_bTimerInterupt) return;
    timer->stop();
    m_bTimerInterupt = true;
}

void MainWindow::restartTimer()
{
    if(!m_bTimerOn || !m_bTimerInterupt) return;
    timer->start(500);
    m_bTimerInterupt = false;
}


void MainWindow::executeList()
{
    if(m_simul)
    {
        simulateList();
        return;
    }
    if( m_isRunning || !m_lines ) return;
    m_restore = false;
    // créer logFile
    QString fileName = QDir::homePath();
    fileName.append("/QtMirror.log");
    logFile.setFileName(fileName);
    logFile.open(QIODevice::ReadWrite);
    timer->start(500);
    m_bTimerOn = true;
    m_tested = m_copied = m_deleted = m_errors = 0;
    m_copySize = 0;
    m_indexE = 0;
    m_isRunning = true;
}

void MainWindow::executeStudy()
{
    int cnt1,cnt2;
    if( m_isRunning || !m_lines ) return;
//    m_simul = false;
    m_restore = false;
    // créer ListFile
    QString fileName = QDir::homePath();
    fileName.append("/QtMirror.lst");
    studyFile.setFileName(fileName);
    studyFile.open(QIODevice::ReadWrite);
    m_tested = m_copied = m_deleted = m_errors = 0;
    m_copySize = 0;
    cnt1 = cnt2 = 0;
    m_study = true;
    m_isRunning = true;
    MirwCmde mc;
    QModelIndex mi = ui->tableWidget->currentIndex();
    m_indexE = mi.row();
    if( m_indexE < 0 || (m_indexE > m_lines)) return;

    //ui->tableWidget->selectRow(m_indexE);
    //ui->tableWidget->update();
    mc = m_list[m_indexE];
    mc.wOptions |= STUDY;
    emit progressChanged2(110);
    m_lastProgress2 = 0;
    emit progressChanged3(110);
    m_lastProgress3 = 0;
    m_scan = new scanner(mc, this);
    m_scan->start();

    while(m_scan->isRunning())
    {
        cnt1++;
        if (cnt1 == 100000)
        {
            cnt2++;
            emit progressChanged2(cnt2/10);
            if(cnt2 == 1000) cnt2 = 0;
            cnt1 = 0;
        }
    }
    // a partir d'ici, scanner a fini son travail, récupérer les totaux, le détruire et voir si il y a un suivant
    emit progressChanged2(100);
    studyFile.resize(studyFile.pos());
    studyFile.close();
    ViewEditStudy* ves = new ViewEditStudy(this);
    if(ves->exec() && !m_simul)
    {
        ves->validate();
        // créer logFile
        QString fileName = QDir::homePath();
        fileName.append("/QtMirror.log");
        logFile.setFileName(fileName);
        logFile.open(QIODevice::ReadWrite);
        m_scan->execOpFile();
        m_copied += m_scan->copied();
        m_tested += m_scan->scanned();
        m_deleted += m_scan->deleted();
        m_errors += m_scan->errors();
        m_copySize += m_scan->copySize();
        // afficher le résultat
        QString s;
        s = QString(tr("%1 fichiers testés, %2 copiés, %3 effacés et %4 erreurs")).arg(m_tested).arg(m_copied).arg(m_deleted).arg(m_errors);
        putText(s);
        logFile.resize(logFile.pos());
        logFile.close();
        emit progressChanged3(100);
    }
    delete m_scan;
    m_scan = nullptr;
    m_isRunning = false;
}

void MainWindow::restoreImage()
{
    QString fullImageName;
    MirwCmde mc;
    QModelIndex mi = ui->tableWidget->currentIndex();
    m_indexE = mi.row();
    fullImageName.clear();
    if( m_indexE >= 0 || (m_indexE < m_lines))
    {
        mc = m_list[m_indexE];
        if(mc.wAction == IMAGE || mc.wAction == IMAGERESTORE)
        {
            fullImageName = mc.szDestPath;
            if( !fullImageName.endsWith('/') ) fullImageName.append("/");
            fullImageName.append(mc.szImageFile);
            if( fullImageName.endsWith(".img") || fullImageName.endsWith(".imq") )
            {
                // retirer l'extension
                fullImageName.truncate(fullImageName.size()-4);
            }
        }
    }
    DialogListImage *dli = new DialogListImage(fullImageName, this);
    dli->exec();
    return;
}

void MainWindow::putFileInfo(sfileOutput sfo)
{
    studyFile.write((const char *)&sfo, sizeof(sfileOutput));
}

void MainWindow::putText(QString &s)
{
    ui->info->setText(s);
    update();
    logFile.write(s.toUtf8());
    logFile.write(crlf, 2);
}
