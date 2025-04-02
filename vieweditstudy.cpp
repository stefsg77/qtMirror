#include "vieweditstudy.h"
#include "ui_vieweditstudy.h"
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include "QTableWidgetItem"
#include "qpushbutton.h"

ViewEditStudy::ViewEditStudy(MainWindow *parent)
    : QDialog(parent)
    , ui(new Ui::ViewEditStudy)
{
    sfileOutput sfo;
    int i;
    qint64 k;
    int count;
    QFile studyFile;

    ui->setupUi(this);
    m_mw = parent;

    QPushButton* enr = new QPushButton(tr("Lancer les opérations"));
    QPushButton* rej = new QPushButton(tr("Abandonner"));
    enr->setDefault(true);
    rej->setCheckable(true);
    rej->setAutoDefault(false);
    ui->buttonBox->addButton(rej, QDialogButtonBox::RejectRole);
    ui->buttonBox->addButton(enr, QDialogButtonBox::AcceptRole);

    m_slHead << tr("Action") << tr("Fichiers") << tr("Date") << tr("Ancien") << tr("V.");
    ui->tableWidget->setColumnCount(5);
    ui->tableWidget->setHorizontalHeaderLabels(m_slHead);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(choiceChanged(int)));
    m_count = 0;
    QString fileName = QDir::homePath();
    fileName.append("/QtMirror.lst");
    studyFile.setFileName(fileName);
    studyFile.open(QIODevice::ReadOnly);
    count = (int)(studyFile.size() / sizeof(sfo));
    m_Sfo.clear();
    for(i = 0; i < count; ++i)
    {
        k = studyFile.read((char*)&sfo, sizeof(sfo));
        if(!k) break;
        if(sfo.iAction != TITLELINE && sfo.iAction != TESTED )
        {
            sfo.iIndex = -1;
            m_Sfo.push_back(sfo);
            m_count++;
        }
    }
    i = ui->tableWidget->width();
    int l = i/50;
    ui->tableWidget->setColumnWidth(0, l * 5);
    ui->tableWidget->setColumnWidth(1, i - (l*16));
    ui->tableWidget->setColumnWidth(2, l * 5);
    ui->tableWidget->setColumnWidth(3, l * 5);
    ui->tableWidget->setColumnWidth(4, l);
    fillTable();
    studyFile.close();
    ui->comboBox->addItem(tr("Les différents"));
    ui->comboBox->addItem(tr("Tout voir"));
    ui->comboBox->setCurrentIndex(0);
    m_bAll = false;
}

ViewEditStudy::~ViewEditStudy()
{
    delete ui;
}

void ViewEditStudy::fillTable()
{
    int i,j;
    QDateTime dt;
//    sfileOutput sfo;
//    QModelIndex mi;
    ui->tableWidget->setRowCount(m_count);
    for(i = j = 0; i < m_count; i++)
    {
        if(m_Sfo[i].iAction != NOTHING || m_bAll == true)
        {
//            ui->tableWidget->setCurrentCell(j,0);
//            mi = ui->tableWidget->currentIndex();
//            ui->tableWidget->setIndexWidget(mi,new QComboBox);
            ui->tableWidget->setCellWidget(j,0, new QComboBox);
//          QComboBox *cb = (QComboBox*)ui->tableWidget->indexWidget(mi);
            QComboBox *cb = (QComboBox*)ui->tableWidget->cellWidget(j,0);
            cb->addItem(tr("Ignorer"));
            cb->addItem(tr("Copier"));
            cb->addItem(tr("Déplacer"));
            cb->addItem(tr("Remplacer"));
            cb->addItem(tr("Effacer"));
            cb->setCurrentIndex(m_Sfo[i].iAction);
            QTableWidgetItem *wi = new QTableWidgetItem((char*)m_Sfo[i].full_name);
            ui->tableWidget->setItem(j, 1, wi);
            dt.setMSecsSinceEpoch(m_Sfo[i].modif_time);
            wi = new QTableWidgetItem(dt.toString("dd/MM/yyyy hh:mm"));
            ui->tableWidget->setItem(j, 2, wi);
            dt.setMSecsSinceEpoch(m_Sfo[i].old_time);
            wi = new QTableWidgetItem(dt.toString("dd/MM/yyyy hh:mm"));
            ui->tableWidget->setItem(j, 3, wi);
            wi = new QTableWidgetItem(QString("%1").arg(m_Sfo[i].iFileNum));
            ui->tableWidget->setItem(j, 4, wi);
            m_Sfo[i].iIndex = j;
            j++;
        }
        else
            m_Sfo[i].iIndex = -1;
    }
}

void ViewEditStudy::choiceChanged(int index)
{
    m_bAll = (bool)index;
    ui->tableWidget->clearContents();
    fillTable();
}

void ViewEditStudy::validate()
{
    int i;
    sfileOutput sfo;
    QFile studyFile;
    QString fileName = QDir::homePath();
    fileName.append("/QtMirror.lst");
    studyFile.setFileName(fileName);
    studyFile.open(QIODevice::ReadWrite);
    for(i = 0; i < m_count; i++)
    {
        sfo = m_Sfo.at(i);
        if(sfo.iIndex != -1)
        {
            QComboBox *cb = (QComboBox*)ui->tableWidget->cellWidget(sfo.iIndex,0);
            sfo.iAction = cb->currentIndex();
        }
        studyFile.write((char*)&sfo, sizeof(sfo));
    }
    studyFile.resize(studyFile.pos());
    studyFile.close();
}
