#include "dialogeditcommand.h"
#include "ui_dialogeditcommand.h"
#include <QFileDialog>
#include <QMessageBox>
#include "qpushbutton.h"

DialogEditCommand::DialogEditCommand(MirwCmde &mc, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogEditCommand)
{
    ui->setupUi(this);
    m_mc = mc;

    QPushButton* enr = new QPushButton(tr("Enregistrer"));
    QPushButton* rej = new QPushButton(tr("Abandonner"));
    enr->setDefault(true);
    rej->setCheckable(true);
    rej->setAutoDefault(false);
    ui->buttonBox->addButton(rej, QDialogButtonBox::RejectRole);
    ui->buttonBox->addButton(enr, QDialogButtonBox::AcceptRole);

    ui->pushButton->setText(tr("Parcourir"));
    ui->pushButton_2->setText(tr("Parcourir"));
    ui->label->setText(tr("Origine"));
    ui->label_2->setText(tr("Destination"));
    ui->label_3->setText(tr("Répertoire commun"));
    ui->label_4->setText(tr("Répertoires à inclure"));
    ui->label_5->setText(tr("Répertoires à exclure"));
    ui->label_6->setText(tr("Fichiers à include"));
    ui->label_7->setText(tr("Fichiers à exclure"));
    ui->label_8->setText(tr("Nom du fichier image"));
    ui->radioButton->setText(tr("Copies unidirectionnelles"));
    ui->radioButton_2->setText(tr("Copies bidirectionnelles"));
    ui->radioButton_3->setText(tr("Copies de sauvegarde"));
    ui->radioButton_4->setText(tr("Copies dans fichier image"));
    ui->checkBox->setText(tr("Mode silencieux"));
    ui->checkBox_2->setText(tr("Sans effacements"));
    ui->checkBox_3->setText(tr("Sans répertoire commun"));
    ui->checkBox_4->setText(tr("Sans comparaison de date"));
    ui->checkBox_5->setText(tr("Ignorer 1 seconde d'écart"));
    ui->checkBox_6->setText(tr("Ignorer 1 heure de compensation"));
    ui->checkBox_7->setText(tr("Mode incrémentiel"));
    ui->label_9->setText(tr("Nombre de sauvegardes par séries"));
    ui->label_10->setText(tr("Nombre de séries à conserver"));
    ui->spinBox->setRange(1,10);
    ui->spinBox_2->setRange(1,25);
    connect( ui->pushButton, SIGNAL(clicked()), this, SLOT(getPathI()));
    connect( ui->pushButton_2, SIGNAL(clicked()), this, SLOT(getPathO()));
    connect( ui->radioButton_4, SIGNAL(toggled(bool)), this, SLOT(wasToggled(bool)));
    fillForm();
}

DialogEditCommand::~DialogEditCommand()
{
    delete ui;
}

void DialogEditCommand::wasToggled(bool checked)
{
        ui->checkBox_7->setEnabled(checked);
}

void DialogEditCommand::fillForm()
{
    ui->lineEdit->setText(m_mc.szOrgPath);
    ui->lineEdit_2->setText(m_mc.szDestPath);
    if(m_mc.wOptions & NOCOMMDIR)
    {
        ui->checkBox_3->setChecked(true);
        ui->lineEdit_3->setEnabled(false);
    }
    else
    {
        ui->checkBox_3->setChecked(false);
        ui->lineEdit_3->setEnabled(true);
        ui->lineEdit_3->setText(m_mc.szCommPath);
    }
    ui->lineEdit_4->setText(m_mc.szDirSpec);
    ui->lineEdit_5->setText(m_mc.szExclDirSpec);
    ui->lineEdit_6->setText(m_mc.szFileSpec);
    ui->lineEdit_7->setText(m_mc.szExclFileSpec);
    if(m_mc.wOptions & QUIETMODE) ui->checkBox->setChecked(true);
    if(m_mc.wOptions & NODELETE) ui->checkBox_2->setChecked(true);
    if(m_mc.wOptions & NODATECOMP) ui->checkBox_4->setChecked(true);
    if(m_mc.wOptions & IGNORESEC) ui->checkBox_5->setChecked(true);
    if(m_mc.wOptions & DSTCOMP) ui->checkBox_6->setChecked(true);
    if(m_mc.wOptions & INCREMENT) ui->checkBox_7->setChecked(true);
    if(m_mc.wAction != IMAGE) ui->checkBox_7->setEnabled(false);
    switch (m_mc.wAction)
    {
    case REFLECT:
        ui->radioButton->setChecked(true);
        break;
    case SYNCHRONE:
        ui->radioButton_2->setChecked(true);
        break;
    case RESTORE:
        ui->radioButton_3->setChecked(true);
        break;
    case IMAGE:
        ui->radioButton_4->setChecked(true);
        ui->lineEdit_8->setText(m_mc.szImageFile);
        ui->spinBox->setValue(m_mc.inSets);
        ui->spinBox_2->setValue(m_mc.sets);
        break;
    }
}

MirwCmde &DialogEditCommand::readForm()
{
    QString s;
    m_mc.wOptions = 0;
    s = ui->lineEdit->text();
    strcpy_s(m_mc.szOrgPath, s.toUtf8().data());
    s = ui->lineEdit_2->text();
    strcpy_s(m_mc.szDestPath, s.toUtf8().data());
    if(ui->checkBox_3->isChecked())
    {
        m_mc.wOptions |= NOCOMMDIR;
    }
    else
    {
        s = ui->lineEdit_3->text();
        strcpy_s(m_mc.szCommPath, s.toUtf8().data());
    }
    s = ui->lineEdit_4->text();
    strcpy_s(m_mc.szDirSpec, s.toUtf8().data());
    s = ui->lineEdit_5->text();
    strcpy_s(m_mc.szExclDirSpec, s.toUtf8().data());
    s = ui->lineEdit_6->text();
    strcpy_s(m_mc.szFileSpec, s.toUtf8().data());
    s = ui->lineEdit_7->text();
    strcpy_s(m_mc.szExclFileSpec, s.toUtf8().data());
    if( ui->checkBox->isChecked()) m_mc.wOptions |= QUIETMODE;
    if( ui->checkBox_2->isChecked()) m_mc.wOptions |= NODELETE;
    if( ui->checkBox_4->isChecked()) m_mc.wOptions |= NODATECOMP;
    if( ui->checkBox_5->isChecked()) m_mc.wOptions |= IGNORESEC;
    if( ui->checkBox_6->isChecked()) m_mc.wOptions |= DSTCOMP;
    if( ui->checkBox_7->isChecked()) m_mc.wOptions |= INCREMENT;
    if( ui->radioButton->isChecked()) m_mc.wAction = REFLECT;
    else if(ui->radioButton_2->isChecked()) m_mc.wAction = SYNCHRONE;
    else if(ui->radioButton_3->isChecked()) m_mc.wAction = RESTORE;
    else
    {
        m_mc.wAction = IMAGE;
        s = ui->lineEdit_8->text();
        strcpy_s(m_mc.szImageFile, s.toUtf8().data());
        m_mc.inSets = ui->spinBox->value();
        m_mc.sets = ui->spinBox_2->value();
        if((m_mc.inSets * m_mc.sets) > 100)
        {
            QMessageBox::about(this, tr("info"), tr("Le nombre total de fichiers de sauvegarde ne peux dépasser 100"));
            m_mc.inSets = 5;
            m_mc.sets = 20;
        }
    }
    return m_mc;
}

QString DialogEditCommand::getPath(QString op)
{
    return QFileDialog::getExistingDirectory(this, tr("QtMiroir"), op,
                                                             QFileDialog::ShowDirsOnly
                                                             | QFileDialog::DontResolveSymlinks);
}

void DialogEditCommand::getPathI()
{
    int i;
    char szDir[260];
    QString path = getPath(m_mc.szOrgPath);
    if (!path.isEmpty())
    {
        strcpy_s(szDir, path.toUtf8().data());
        if (szDir[strlen(szDir)-1] != '/')
            strcat_s(szDir, "/");
        i = strlen(szDir);
        if ( i > 4 && !(m_mc.wOptions & NOCOMMDIR))
        {
            for( i -= 2; i > 1; i-- )
            {
                if(szDir[i] == '/')
                    break;
            }
            memcpy(m_mc.szCommPath, &szDir[i], strlen(szDir) - i + 1);
            szDir[i] = 0;
        }
        else
        {
            strcpy(m_mc.szCommPath, "/");
        }
        if (szDir[strlen(szDir)-1] == '/')
            szDir[strlen(szDir)-1] = '\0' ;
        strcpy_s(m_mc.szOrgPath, szDir);
        fillForm();
    }
}

void DialogEditCommand::getPathO()
{
    QString path = getPath(m_mc.szDestPath);
    if(!path.isEmpty())
    {
        strcpy_s(m_mc.szDestPath, path.toUtf8().data());
        fillForm();
    }
}
