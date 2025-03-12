#include "dialogeditimagelist.h"
#include "qcheckbox.h"
#include "ui_dialogeditimagelist.h"
//#include "qcombobox.h"

DialogEditImageList::DialogEditImageList(QVector<sfile> *pSf, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogEditImageList)
{
    int i;
    ui->setupUi(this);
    QStringList hList;

    QPushButton* enr = new QPushButton(tr("Lancer les opérations"));
    QPushButton* rej = new QPushButton(tr("Abandonner"));
    enr->setDefault(true);
    rej->setCheckable(true);
    rej->setAutoDefault(false);
    ui->buttonBox->addButton(rej, QDialogButtonBox::RejectRole);
    ui->buttonBox->addButton(enr, QDialogButtonBox::AcceptRole);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(selectAll()));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(unselectAll()));

    hList << tr("Actif") << tr("Fichier") << tr("Date");
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setHorizontalHeaderLabels(hList);
    i = ui->tableWidget->width()/16;
    ui->tableWidget->setColumnWidth(0, i);
    ui->tableWidget->setColumnWidth(1, i * 13);
    ui->tableWidget->setColumnWidth(2, i * 2 + 2);

    m_pSfile = pSf;
    m_iCount = pSf->size();
    // fill table view
    ui->tableWidget->setRowCount(m_iCount);
    for(i = 0; i < m_iCount; ++i)
    {
        int j;
        ui->tableWidget->setCellWidget(i,0, new QCheckBox);
        QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
        if(pSf->at(i).iAction != NOTHING)
        {
            j = COPY;
            cb->setCheckState(Qt::Checked);
        }
        else
        {
            j = NOTHING;
            cb->setCheckState(Qt::Unchecked);
        }
        QTableWidgetItem *wi = new QTableWidgetItem((char*)pSf->at(i).full_name);
        ui->tableWidget->setItem(i, 1, wi);

        m_iAction << j;
    }
}

DialogEditImageList::~DialogEditImageList()
{
    delete ui;
}

void DialogEditImageList::selectAll()
{
    for(int i = 0; i < m_iCount; ++i)
    {
        QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
        cb->setCheckState(Qt::Checked);
        m_iAction[i] = COPY;
    }
}

void DialogEditImageList::unselectAll()
{
    for(int i = 0; i < m_iCount; ++i)
    {
        QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
        cb->setCheckState(Qt::Unchecked);
        m_iAction[i] = NOTHING;
    }
}

void DialogEditImageList::validateList()
{
    int i;
    struct sfile sfo;
    for(i = 0; i < m_pSfile->size(); ++i)
    {
        sfo = m_pSfile->at(i);
        sfo.iAction = m_iAction.at(i);
        m_pSfile->replace(i, sfo);
    }

}
