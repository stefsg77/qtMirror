#include <QFileDialog>
#include <QMessageBox>
#include<QDateTime>
#include <QTableWidgetItem>
#include <utime.h>
#include "dialoglistimage.h"
#include "entetefichier.h"
#include "ui_dialoglistimage.h"

bool myCompareR(const sfileR &sf1, const sfileR &sf2)
{
    int i = 0;
    int a,b;
    bool bCmp = false;
    while(i < MAX_PATH)
    {
        a = toupper(sf1.full_name[i]);
        b = toupper(sf2.full_name[i]);
        if( a == 0 && b == 0 )
        {
            bCmp = false;
            break;
        }
        else if (a < b)
        {
            return true;
        }
        else if (a > b)
        {
            return false;
        }
        i++;
    }
    // si égalité de nom, comparer la version
    if(sf1.iIndex < sf2.iIndex) bCmp = true;
    return bCmp;
}

DialogListImage::DialogListImage(QString &fileName, MainWindow *parent) :
    QDialog(parent),
    ui(new Ui::DialogListImage)
{
    ui->setupUi(this);
    QStringList hList;
    m_mw = parent;
    m_accesLecture7z = new EnteteFichier();
    m_slSelect << tr("Aucune sélection") << tr("Tout sélectionner") << tr("Dernière versions");
    hList << tr("Action") << tr("Fichier") << tr("Date") << tr("V.");
    ui->tableWidget->setColumnCount(4);
    ui->tableWidget->setHorizontalHeaderLabels(hList);

    QPushButton* enr = new QPushButton(tr("OK"));
    QPushButton* rej = new QPushButton(tr("Abandonner"));
    enr->setDefault(true);
    rej->setCheckable(true);
    rej->setAutoDefault(false);
    ui->buttonBox->addButton(rej, QDialogButtonBox::RejectRole);
    ui->buttonBox->addButton(enr, QDialogButtonBox::AcceptRole);

    ui->pushButton->setText(tr("Rechercher"));
    ui->pushButton_2->setText(tr("Parcourir"));
    ui->pushButton_3->setText(tr("Restaurer les fichiers sélectionnés"));
    ui->label->setText(tr("Chemin et nom du fichier"));
    ui->label_2->setText(tr("Restaurer vers un autre emplacement"));
    ui->checkBox->setText(tr("Ne restaurer que les fichiers plus récents"));
    ui->comboBox->addItems(m_slSelect);

    m_fileName = fileName;
    connect( ui->pushButton, SIGNAL(clicked()), this, SLOT(getImageName()));
    connect( ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(comboChanged(int)));
    connect( ui->pushButton_2, SIGNAL(clicked()), this, SLOT(getPath()));
    connect( ui->pushButton_3, SIGNAL(clicked()), this, SLOT(restoreSelected()));
    int w = ui->tableWidget->width();
    int x = w/120;
    ui->tableWidget->setColumnWidth(0,x*10);
    ui->tableWidget->setColumnWidth(1,x*85);
    ui->tableWidget->setColumnWidth(2,x*20);
    ui->tableWidget->setColumnWidth(3,x*5);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    m_comboLines = 2;
    ui->checkBox->setChecked(true);
    if(!m_fileName.isEmpty())
    {
        ui->lineEdit->setText(m_fileName);
        int numFiles = readList7z();
        fillTable(numFiles);
        m_totalCount = numFiles;
    }
}

DialogListImage::~DialogListImage()
{
    m_accesLecture7z->menage();
    delete m_accesLecture7z;
    delete ui;
}

void DialogListImage::selectLatest()
{
    //pour chaque fichier, sélestionner le plus récent
    int i;
    int count = m_vSFileD.size();
    QString fileName;
    QString temp;
    for(i = 0; i < count; i++)
    {
        temp = QString((char*)m_vSFileD.at(i).full_name);
        if(temp != fileName)
        {
            if(i > 0)
            {
                QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i-1,0);
                cb->setCheckState(Qt::Checked);
            }
            fileName = temp;
        }
    }
    QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(count-1,0);
    cb->setCheckState(Qt::Checked);
}

void DialogListImage::getPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("QtMiroir"), QDir::homePath(),
                                                             QFileDialog::ShowDirsOnly
                                                             | QFileDialog::DontResolveSymlinks);
    if(!path.isEmpty())
    {
        m_destPath = path;
        ui->lineEdit_2->setText(m_destPath);
    }
}

void DialogListImage::comboChanged(int index)
{
    int i, count;
    count = m_vSFileD.size();
    switch (index) {
    case 0:
        for(i = 0; i < count; i++)
        {
            QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
            cb->setCheckState(Qt::Unchecked);
        }
        break;
    case 1:
        for(i = 0; i < count; i++)
        {
            QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
            cb->setCheckState(Qt::Checked);
        }
        break;
    case 2:
        selectLatest();
        break;
    default:
        for(i = 0; i < count; i++)
        {
            QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
            cb->setCheckState(Qt::Unchecked);
        }
        selectFromVolume(index - 2);
        break;
    }
}

void DialogListImage::selectAll()
{
    int i, count;
    count = m_vSFileD.size();
    for(i = 0; i < count; i++)
    {
        QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
        cb->setCheckState(Qt::Checked);
    }
}

void DialogListImage::selectFromVolume(int volume)
{
    for( int i = 0; i < m_vSFileD.size(); i++)
    {
        if( m_vSFileD.at(i).iRevision >= volume)
        {
            QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
            cb->setCheckState(Qt::Checked);
        }
    }
}

void DialogListImage::makeDir(QString fullName)
{
    QDir dir;
    int i = fullName.lastIndexOf('/');
    fullName.truncate(i);              // remove file name
    dir.mkpath(fullName);
}

void DialogListImage::restoreSelected()
{
    int volume = 0;
    bool overWrite;
    bool res;
    SRes decodeRes;
    bool fileIsRead = false;
    int lengthToRemove = 0;
    struct sfileR sfr;
    QString fileName;
    QString destName;
    QFile image;
    QFile file;
    const Byte *dataPtr;
    m_errors = m_copied = 0;
    if( ui->checkBox->isChecked() ) overWrite = false; else overWrite = true;
    if( !m_destPath.isEmpty() )
    {
        if( m_destPath.at(m_destPath.size()-1) != '/')
            m_destPath.append("/");
        sfr = m_vSFileD.at(0);
        destName = QString((const char *)sfr.full_name);
        if( destName.startsWith("//"))
            lengthToRemove = 2;
        else if( destName.at(1) == ':')
            lengthToRemove = 3;
        else
            lengthToRemove = 0;
    }
    // commencer par tous les fichiers à récupérer par numéro de volume si m_volumes > 1
    while( volume < m_volumes)
    {
        for( int i = 0; i < m_totalCount; ++i )
        {
            QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
            if( cb->isChecked() )
            {
                // la ligne est sélectionnée
                sfr = m_vSFileD.at(i);
                // vérifier si c'est le bon volume
                if(sfr.volume != volume) continue;
                if( !fileIsRead)
                {
                    fileName = m_fileName;
                    fileName.append(QString("(%1).7z").arg(sfr.iRevision, 2, 10, QChar('0')));
                    image.setFileName(fileName);
                    // ouvrir le fichier
                    res = image.open(QIODevice::ReadOnly);
                    if( !res ) return;
                    m_accesLecture7z->menage();
                    m_accesLecture7z->openFile2(&image);
                    decodeRes = m_accesLecture7z->getHeader();
                    if(decodeRes != SZ_OK)
                    {
                        image.close();
                        return;
                    }
                    decodeRes = m_accesLecture7z->decodeStream(0);
                    if(decodeRes != SZ_OK)
                    {
                        image.close();
                        return;
                    }
                    dataPtr = m_accesLecture7z->streamAdress();
                    image.close();
                    fileIsRead = true;
                }
                const Byte *filePtr = dataPtr + sfr.offset;
                if((UInt64)(sfr.offset + sfr.org_size) <= m_accesLecture7z->packSize(0))
                {
                    m_copied++;
                    destName = m_destPath;
                    destName.append(QString((const char*)sfr.full_name + lengthToRemove));
                    // si chemin alternatif de destination présent, alors changer
                    file.setFileName(destName);

                    if( file.exists() )
                    {
                        QFileInfo qfi(file);
                        if( overWrite || (UInt64)qfi.lastModified().toMSecsSinceEpoch() < sfr.modif_time)
                            QFile::remove(destName);
                        else
                            break;
                    }
                    if(m_mw)
                    {
                        QString s = "Copie de ";
                        s.append( (const char *)sfr.full_name);
                        m_mw->putText(s);

                    }
                    res = file.open(QIODevice::ReadWrite);
                    //si erreure, tenter de créer le chemin
                    // vérifier que la position existe
                    if ( !res )
                    {
                        makeDir(destName);
                        res = file.open(QIODevice::ReadWrite);
                        if( !res ) break;
                    }
                    file.write((char*)filePtr, sfr.org_size);
                    QDateTime dt = QDateTime::fromMSecsSinceEpoch(sfr.modif_time);
                    file.setFileTime(dt, QFileDevice::FileModificationTime);

                    file.close();
                }
            }
        }
        volume++;
        fileIsRead = false;
    }
    QList<QTableWidgetItem *> selectedList = ui->tableWidget->selectedItems();
    m_selected = selectedList.size();
    ui->label_3->setText(tr("Opération terminée !"));
}

void DialogListImage::getImageName()
{
    qint64 fileSize;
    struct imageHeader ih;
    QString fileName;
    QFile file;
    fileName = QFileDialog::getOpenFileName(this,tr("QtMiroir"), QDir::homePath(), tr("Fichiers images(*.7z)"));
    if(fileName.isEmpty()) return;
    file.setFileName(fileName);
    file.open(QIODevice::ReadOnly);
    fileSize = file.size();
    file.seek(fileSize-sizeof(ih));
    file.read((char*)&ih, sizeof(ih));
    file.close();
    if( ih.signature[0] != 'Q' || ih.signature[1] != 't' )
    {
        imageHeaderOld *piho = (imageHeaderOld *)&ih;
        if( piho->signature[0] != 'Q' || piho->signature[1] != 't')
        {
            //pas un format compatible.
            QMessageBox::warning(this, tr("QtMirror"), tr("Le fichier sélectionné n'est pas dans un format reconnu."),QMessageBox::Abort);
            m_totalCount = 0;
            return;
        }
    }
    // copier le nom puis remplir le formulaire
    // retirer "(00).7z"
    m_fileName = fileName.left(fileName.size()-7);
    int numFiles = readList7z();
    fillTable(numFiles);
    m_totalCount = numFiles;
    return;
}

int DialogListImage::fillTable(int count)
{
    int i;
    struct sfileR sf;
    QString s;
    QDateTime dt;
    ui->lineEdit_2->setText(m_destPath);
    ui->tableWidget->setRowCount(count);
    for( i = 0; i < count; ++i)
    {
        sf = m_vSFileD.at(i);
        ui->tableWidget->setCellWidget(i,0, new QCheckBox);
        QCheckBox *cb = (QCheckBox*)ui->tableWidget->cellWidget(i,0);
        cb->setCheckState(Qt::Unchecked);
        s = (char*)sf.full_name;
        s.append(QString("(%1)").arg(sf.iIndex , 2, 10, QChar('0')));
        QTableWidgetItem *wi = new QTableWidgetItem(s);
        ui->tableWidget->setItem(i, 1, wi);
//        wi = new QTableWidgetItem(QString("%1").arg(sf.iRevision+1));
//        ui->tableWidget->setItem(i, 2, wi);
        dt.setMSecsSinceEpoch(sf.modif_time);
        wi = new QTableWidgetItem(dt.toString("dd/MM/yyyy hh:mm:ss"));
        ui->tableWidget->setItem(i, 2, wi);
        wi = new QTableWidgetItem(QString("%1").arg(sf.iRevision));
        ui->tableWidget->setItem(i, 3, wi);
    }
    ui->comboBox->clear();
    ui->comboBox->addItems(m_slSelect);
    if( m_volumes > 0)
    {
        QString s;
        for( i = 0; i < m_volumes; i++)
        {
            s = QString(tr("Sélectionner depuis le volume %1")).arg(i);
            ui->comboBox->addItem(s);
        }
        m_comboLines = i+2;
    }
    return count;
}

bool myCompare(const sfileR &sf1, const sfileR &sf2)
{
    int i = 0;
    int a,b;
    while(i < MAX_PATH)
    {
        a = toupper(sf1.full_name[i]);
        b = toupper(sf2.full_name[i]);
        if( a == 0 && b == 0 ) break;
        if (a < b) return true;
        else if (a > b) return false;
        i++;
    }
    if( sf1.modif_time < sf2.modif_time ) return true;
    return false;
}

int DialogListImage::readList7z()
{
    imageHeader ih;
    m_volumes = 0;
    int totalNumFiles;
    int numFiles;
    qint64 fileSize;
    qint64 pos;
    qint64 done;
    //struct sfile sf;
    struct sfileR sfr;
    QFile imageFile;
    QString fileName;
    bool found;
    m_vSFileD.clear();
    fileName = m_fileName;
    fileName.append(QString("(00).7z"));
    imageFile.setFileName(fileName);
    while(imageFile.exists())
    {
        imageFile.open(QIODevice::ReadOnly);
        fileSize = imageFile.size();
        pos = fileSize - sizeof(imageHeader);
        imageFile.seek(pos);
        done = imageFile.read((char*)&ih, sizeof(imageHeader));
        //déterminer si valide
        if(done != sizeof(imageHeader))
        {
            imageFile.close();
            QMessageBox::warning(this, tr("QtMirror"), tr("Le fichier sélectionné ne correspond pas à cette version."),QMessageBox::Abort);
            m_totalCount = 0;
            return 0;
        }
        if( ih.signature[0] != 'Q' || ih.signature[1] != 't' || ih.signature[3] < '1')
        {
            imageFile.close();
            QMessageBox::warning(this, tr("QtMirror"), tr("Le fichier sélectionné ne correspond pas à cette version."),QMessageBox::Abort);
            m_totalCount = 0;
            return 0;
        }
        m_destPath = QString(ih.originPath);
        imageFile.seek(0);
        m_accesLecture7z->menage();
        m_accesLecture7z->openFile2(&imageFile);
        m_accesLecture7z->getHeader();

        numFiles = m_accesLecture7z->numFiles();
        for(int j = 0; j < numFiles; j++)
        {
            if( m_accesLecture7z->isDir(j))
                continue;

            memset((char*)&sfr, 0, sizeof(sfr));
            sfr.volume = m_volumes;
            sfr.modif_time = m_accesLecture7z->modifQTime(j);
            sfr.attributes = m_accesLecture7z->fileAttribs(j);
            sfr.org_size = m_accesLecture7z->fileSize(j);
            sfr.offset = m_accesLecture7z->fileOffset(j);
            QString name = m_accesLecture7z->fileName(j);
            for(int k = 0; k < name.size(); k++)
            {
                sfr.full_name[k] = (char)name.data()[k].toLatin1();
            }
            // Remplir les autres champs de sf
            found = false;
            if(m_volumes > 0)
            {
                //trouver si est déjà dans la liste
                for(int i = 0; i < m_vSFileD.size(); ++i)
                {
                    int comp = strcmp((const char*)sfr.full_name, (const char*)m_vSFileD[i].full_name);
                    //if( comp > 0) break;
                    //else
                    if(comp == 0)
                    {
                        found = true;
                        sfr.iIndex = ++m_vSFileD[i].iVersions;
                        break;
                    }
                }
            }
            if( !found )
            {
                sfr.iIndex = 0;
                sfr.iVersions = 0;
            }
            sfr.iRevision = m_volumes;
            m_vSFileD.append(sfr);
        }

        imageFile.close();
        ++m_volumes;
        fileName = m_fileName;
        fileName.append(QString("(%1).7z").arg(m_volumes, 2, 10, QChar('0')));
        imageFile.setFileName(fileName);
    }
    //    m_volumes--;
    std::sort(m_vSFileD.begin(), m_vSFileD.end(), myCompareR);

    totalNumFiles = m_vSFileD.size();

    return totalNumFiles;
}
