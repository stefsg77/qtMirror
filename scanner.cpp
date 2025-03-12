#include "scanner.h"
#include <QDateTime>
#include <QDir>
#include <QtAlgorithms>
#include <utime.h>
#include "dialogeditimagelist.h"

bool myCompare(const sfile &sf1, const sfile &sf2)
{
    int i = 0;
    int a,b;
    while(i < MAX_PATH)
    {
        a = toupper(sf1.full_name[i]);
        b = toupper(sf2.full_name[i]);
        if( a == 0 && b == 0 ) return false;
        else if (a < b) return true;
        else if (a > b) return false;
        i++;
    }
    return false;
}

scanner::scanner(MirwCmde mc, MainWindow *parent) :
    QThread(parent)
{
    m_accesLecture7z = new EnteteFichier();
    m_accesEcriture7z = new Encoder7z();
    m_cf = new QCopyFile(this);
//    m_lzh = new LZHuf2();
    m_mc = mc;
    m_mw = parent;
    m_scanned = m_copied = m_deleted = m_errors = 0;
    m_copySize = 0;
    m_volumes = 0;
    retcode = 0;
    m_bRunning = false;
    m_textExt << "TXT" << "C" << "H" << "CPP" << "PRO" << "ASM" << "1" << "2";
    m_compExt << " ARC" << "ZIP" << "LHA" << "LZH" << "ARJ" << "7Z" << "JPG" << "ZOO" << "RAR" << "AVI" << "ICE" << "LIF" << "CAB"
              << "MP3" << "M4P" << "MPG" << "WMA" << "WMV" << "MKV";
    if(mc.wAction == IMAGE || mc.wAction == IMAGERESTORE)
    {
        m_fullImageName = mc.szDestPath;
        if( !m_fullImageName.endsWith('/') ) m_fullImageName.append("/");
        m_fullImageName.append(mc.szImageFile);
        if( m_fullImageName.endsWith(".img") || m_fullImageName.endsWith(".imq") )
        {
            // retirer l'extension
            m_fullImageName.truncate(m_fullImageName.size()-4);
        }
    }
    connect( m_cf, SIGNAL(copyChunkSignal(int,int)), this, SLOT(getProgress(int,int)));
//    connect( m_lzh, SIGNAL(chunk(int,int)), this, SLOT(getProgress(int,int)));
}

scanner::~scanner()
{
    delete m_cf;
//    delete m_lzh;
}

void scanner::getProgress(int percent, int chunk)
{
    m_totalJobDone += chunk;
    int globalPercent = (int)((m_totalJobDone*100)/m_totalJobSize);
    if( percent < 0 ) percent =0;
    if( percent > 100 ) percent = 100;
    if( globalPercent < 0 ) globalPercent =0;
    if( globalPercent > 100 ) globalPercent = 100;
    m_mw->setProgressBar(percent, globalPercent);
}

bool scanner::isCompressed(QFileInfo *pfi)
{
    QString ext = pfi->suffix();
    if( m_compExt.contains(ext, Qt::CaseInsensitive)) return true;
    if( pfi->baseName().startsWith("BASEIM",Qt::CaseInsensitive)) return true;
    return false;
}

bool scanner::isCompressed(sfile *file)
{
    QString s = QString((char*)file->full_name);
    if( s.contains("BASEIM", Qt::CaseInsensitive)) return true;
    QString ext = s.right(3);
    if( m_compExt.contains(ext, Qt::CaseInsensitive)) return true; else return false;
}

bool scanner::isText(sfile *file)
{
    QString ext = QString((char*)file->full_name).right(3);
    if( m_textExt.contains(ext, Qt::CaseInsensitive)) return true; else return false;
    return false;
}

bool scanner::directoryCheck(QFileInfo *pfi)
{
    char *pnt = m_mc.szDirSpec;
    if(!(*pnt)) return false;
    if( !strcmp(pnt,"*.*")) return true;
    QString s;
    QString dir = pfi->fileName();
    dir = dir.toUpper();
    while( *pnt )
    {
        s.clear();
        while(*pnt && (*pnt != ';'))
        {
            s.append(QChar(*pnt));
            pnt++;
        }
        s = s.toUpper();
        if( s == dir ) return true;
        if( *pnt == ';' ) pnt++;
    }
    return false;
}

bool scanner::directoryExclCheck(QFileInfo *pfi)
{
    bool bMatch1 = false;
    bool bMatch2 = false;
    int i = 0;
    if(pfi->fileName().at(0) == '.' ) return false;
    QString  sBase = pfi->baseName();
    QString sExt = pfi->suffix();
 //   sBase = sBase.toUpper();
 //   sExt = sExt.toUpper();
    char *pnt = m_mc.szExclDirSpec;
    if(!(*pnt)) return true;
    if( !strcmp(pnt,"*.*")) return false;
    QString s;
    // tester le la base du nom
    while( *pnt )
    {
        bool bIgnoreBeg = false;
        bool bIgnoreEnd = false;
        s.clear();
        if(*pnt == '*')
        {
            bIgnoreBeg = true;
            pnt++;
        }
        i = 0;
        while(*pnt && (*pnt != ';') && (*pnt!= '.'))
        {
            s.append(QChar(*pnt));
            pnt++;
            i++;
        }
        if(i>2 && s.at(i-1) == '*')
        {
            s.truncate(i-1);
            bIgnoreEnd = true;
        }
        if(bIgnoreBeg && bIgnoreEnd)
        {
            // ignorer l'avant et l'apres
            if(sBase.contains(s, Qt::CaseInsensitive))
                bMatch1 = true;
        }
        else if(bIgnoreEnd)
        {
            // ignorer la fin
            sBase.truncate(s.size());
            if(!sBase.compare(s, Qt::CaseInsensitive))
                bMatch1 = true;
        }
        else
        {
            // ignorer le début, sBase ne doit garder que les derniers caractères
            sBase = sBase.right(s.size());
            if(!sBase.compare(s, Qt::CaseInsensitive))
                bMatch1 = true;
        }
        // tester l'extentsion
        s.clear();
        i = 0;
        while(*pnt && (*pnt != ';'))
        {
            if(*pnt != '.')
            {
                s.append(QChar(*pnt));
                i++;
            }
            pnt++;
        }
        if(s.length() == 0 || s.at(0) == '*' || !sExt.compare(s, Qt::CaseInsensitive)) bMatch2 = true;
        if(bMatch1 && bMatch2) return false;
        if( *pnt == ';' ) pnt++;
        bMatch1 = bMatch2 = false;
    }
    return true;
}

bool scanner::fileCheck(QFileInfo *pfi)
{
    // on analyse seuleument les extensions
    char *pnt = m_mc.szFileSpec;
    if(!(*pnt)) return false;
    if( !strcmp(pnt,"*.*")) return true;
    QString s;
    QString ext = pfi->suffix();
    ext = ext.toUpper();
    while( *pnt )
    {
        if( *pnt == ';' ) pnt++;
        if( *pnt == '*' ) pnt++;
        if( *pnt == '.' ) pnt++;
        s.clear();
        while(*pnt && (*pnt != ';') && (*pnt != '.') )
        {
            s.append(QChar(*pnt));
            pnt++;
        }
        s = s.toUpper();
        if( s == ext ) return true;
    }
    return false;
}

bool scanner::fileExclCheck(QFileInfo *pfi)
{
    // on analyse seuleument les extensions
    char *pnt = m_mc.szExclFileSpec;
    if(!(*pnt)) return true;
//    if( !strcmp(pnt,"*.*")) return false;
    QString base = pfi->baseName();
    QString s;
    QString ext = pfi->suffix();
    ext = ext.toUpper();
    base = base.toUpper();
    // tester d'abord les extension
    while( *pnt )
    {
        if( *pnt == ';' ) pnt++;
        if( *pnt == '*' ) pnt++;
        if( *pnt == '.' ) pnt++;
        s.clear();
        while(*pnt && (*pnt != ';') && (*pnt != '.') )
        {
            s.append(QChar(*pnt));
            pnt++;
        }
        s = s.toUpper();
        if( s == ext ) return false;
    }
    // puis tester la base
    pnt = m_mc.szExclFileSpec;
    while( *pnt )
    {
        if( *pnt == ';' ) pnt++;
        if( *pnt == '*' ) pnt++;
        if( *pnt == '.' ) pnt++;
        s.clear();
        while(*pnt && (*pnt != ';') && (*pnt != '.') && (*pnt != '*') )
        {
            s.append(QChar(*pnt));
            pnt++;
        }
        s = s.toUpper();
        if( base.startsWith(s) ) return false;
        if( *pnt == '*' ) pnt++;
        if( *pnt == '.' ) pnt++;
        if( *pnt == '*' ) pnt++;
    }
    return true;
}

int scanner::compareFileTime(QFileInfo *pfi1, QFileInfo *pfi2)
{
    int s1, s2, dh;
    QDateTime dt1 = pfi1->lastModified();
    QDateTime dt2 = pfi2->lastModified();
    if(dt1.date().year() > dt2.date().year()) return 1;
    if(dt1.date().year() < dt2.date().year()) return -1;
    if(dt1.date().month() > dt2.date().month()) return 1;
    if(dt1.date().month() < dt2.date().month()) return -1;
    if(dt1.date().day() > dt2.date().day()) return 1;
    if(dt1.date().day() < dt2.date().day()) return -1;

    if( m_mc.wOptions & DSTCOMP )
    {
        dh = abs(dt1.time().hour() - dt2.time().hour());
        if( dh > 1 )
        {
            if( dt1.time().hour() > dt2.time().hour() ) return 1;
            else return -1;
        }
    }
    else
    {
        if( dt1.time().hour() > dt2.time().hour() ) return 1;
        else if( dt1.time().hour() < dt2.time().hour() ) return -1;
    }

    s1 = dt1.time().minute() * 60 + dt1.time().second();
    s2 = dt2.time().minute() * 60 + dt2.time().second();
    if ( m_mc.wOptions & IGNORESEC )
    {
        if( dt1.time().minute() == dt2.time().minute() ) return 0;
        dh = abs(s1 - s2);
        if( dh > 64)
        {
            if( s1 > s2 ) return 1;
            else if( s1 < s2 ) return -1;
        }
    }
    else
    {
        if( s1 > s2 ) return 1;
        else if( s1 < s2 ) return -1;
    }
    return 0;
}

int scanner::compareFileTime(sfile *psf1, sfile *psf2)
{
    unsigned long long int dif64;
    int dif = 0;
    // les dates sont données en millisecondes
    if(psf1->modif_time > psf2->modif_time)
    {
        dif64 = psf1->modif_time - psf2->modif_time;
        if ( dif64 > 0x7fffffff) dif = 0x7fffffff;
        else dif = (int)dif64;
    }
    else
    {
        dif64 = psf2->modif_time - psf1->modif_time;
        if ( dif64 > 0x7fffffff) dif = 0x80000000;
        else dif -= (int)dif64;
    }
//    dif = psf1->modif_time - psf2->modif_time;
    if(abs(dif) < 500 || ((m_mc.wOptions & IGNORESEC) && abs(dif) < 1500)) return 0;
    if( m_mc.wOptions & DSTCOMP )
    {
        int adif = abs(abs(dif) - 3600000);
        if((m_mc.wOptions & IGNORESEC) && adif < 1500) return 0;
    }
    return dif;
}

void scanner:: listOfFiles(bool org, QString orgPath)
{
    int j;
    QFileInfoList list = QDir(orgPath).entryInfoList();
    j = list.size();
    for( int i = 0; i < j; ++i)
    {
        QFileInfo fi = list.at(i);
        if( fi.isDir() )
        {
            if( directoryCheck(&fi) && directoryExclCheck(&fi))
                listOfFiles(org, fi.absoluteFilePath());
        }
        else if( fi.isFile())
        {
            if( fileCheck(&fi) && fileExclCheck(&fi))
            {
                    struct sfile sf;
                    memset((void*)&sf, 0, sizeof(sf));
                    sf.creation_time = fi.birthTime().toMSecsSinceEpoch();
                    sf.modif_time = fi.lastModified().toMSecsSinceEpoch();
                    sf.org_size = fi.size();
                    strcpy_s((char*)sf.full_name, MAX_PATH, fi.absoluteFilePath().toLatin1().data());
                    if(org) m_vSFileO.append(sf); else m_vSFileD.append(sf);
                    ++m_scanned;
            }
        }
    }
}

int scanner::listOf7zImageFiles()
{
    struct imageHeader ih;
//    entete7z head7z;
//    Byte attribs[8];
    qint64 fileSize;
    int fullCount = 1;
    int incCount = 0;
    int numFiles;
    //    int count = 0;
    m_volumes = 0;
    //int sfileSize;
    struct sfile sf;
    m_vSFileD.clear();
    QFile file;
    QString fileName;
    m_destDir[0] = 0;
    fileName = m_fullImageName;
    fileName.append(QString("(00).7z"));
    file.setFileName(fileName);
    while( file.exists() )
    {
        //        struct sfileR sfr;
        qint64 pos;
        qint64 done;
        bool found;
        file.open(QIODevice::ReadOnly);
        // trouver le header de qtMirror
        fileSize = file.size();
        pos = fileSize - sizeof(imageHeader);
        file.seek(pos);
        done = file.read((char*)&ih, sizeof(imageHeader));
        //déterminer si valide
        if(done != sizeof(imageHeader))
        {
            file.close();
            return 0;
        }
        if( ih.signature[0] != 'Q' || ih.signature[1] != 't' || ih.signature[3] < '1')
        {
            file.close();
            return 0;
        }

        if( m_volumes)
        {
            if( ih.full )
            {
                fullCount++;
                incCount = 0;
            }
            else incCount++;
        }
        if(m_destDir[0] == 0)
            strcpy_s(m_destDir, ih.originPath);

        file.seek(0);
        m_accesLecture7z->openFile2(&file);
        m_accesLecture7z->getHeader();

        numFiles = m_accesLecture7z->numFiles();
        for(int j = 0; j < numFiles; j++)
        {
            if( m_accesLecture7z->isDir(j))
                continue;

            memset((char*)&sf, 0, sizeof(sf));
            sf.modif_time = m_accesLecture7z->modifQTime(j);
            sf.attributes = m_accesLecture7z->fileAttribs(j);
            sf.org_size = m_accesLecture7z->fileSize(j);
            sf.offset = m_accesLecture7z->fileOffset(j);
            QString name = m_accesLecture7z->fileName(j);
            for(int k = 0; k < name.size(); k++)
            {
                sf.full_name[k] = (char)name.data()[k].toLatin1();
            }
            // Remplir les autres champs de sf
            found = false;
            if(m_volumes > 0 && m_unique)
            {
                //trouver si est déjà dans la liste
                for(int i = 0; i < m_vSFileD.size(); ++i)
                {
                    int comp = strcmp((const char*)sf.full_name, (const char*)m_vSFileD[i].full_name);
                    //if( comp > 0) break;
                    if(comp == 0)
                    {
                        found = true;
                        if(compareFileTime(&sf, &m_vSFileD[i]) > 0)
                        //                            if(sf.modif_time > m_vSFileD[i].modif_time)
                        {
                            //                                m_vSFileD[i] = sf;
                            m_vSFileD[i].dwPos = j;
                            m_vSFileD[i].modif_time = sf.modif_time;
                            m_vSFileD[i].creation_time = sf.creation_time;
                            m_vSFileD[i].org_size = sf.org_size;
                            m_vSFileD[i].iRevision = m_volumes;
                        }
                        // la recherche doit aller jusqu'a la fin de liste
                        //break;
                    }
                }
            }
            if(!found)
            {
                // not found, so add entry
                sf.iRevision = m_volumes;
                sf.dwPos = j;
                m_vSFileD.append(sf);
            }
        }
        file.close();
        ++m_volumes;
        fileName = m_fullImageName;
        fileName.append(QString("(%1).7z").arg(m_volumes, 2, 10, QChar('0')));
        file.setFileName(fileName);
        if( m_mc.inSets  && m_mc.inSets == (incCount+1))
            m_vSFileD.clear();
    }

    if( ( m_mc.sets ) && (fullCount >= m_mc.sets) && (m_vSFileD.size() == 0) )
    {
        // faire le ménage car il y a trop de sauvegardes présentes
        // en effacant une image complete et images successives partielles
        //        jusqu'à la complete suivante
        int i, j;
        for( i = 0; i < m_volumes; i++)
        {
            fileName = m_fullImageName;
            fileName.append(QString("(%1).7z").arg(i, 2, 10, QChar('0')));
            file.setFileName(fileName);
            file.open(QIODevice::ReadOnly);
            file.seek(file.size() - sizeof(ih));
            file.read((char *)&ih, sizeof(ih));
            file.close();
            if(ih.full && i) break;
            else file.remove();
        }
        // renommer les fichiers suivants
        for( j = 0; i < m_volumes; i++)
        {
            // renommer
            fileName = m_fullImageName;
            fileName.append(QString("(%1).7z").arg(j, 2, 10, QChar('0')));
            file.rename(fileName);
            fileName = m_fullImageName;
            fileName.append(QString("(%1).7z").arg(i+1, 2, 10, QChar('0')));
            file.setFileName(fileName);
            j++;
        }
        // enfin ajuster m_volumes au nouveau numéro de dernier fichier
        m_volumes = j;
    }

    return m_vSFileD.size();
    //return count;
}

void scanner::compareListOfFiles()
{
    int i,j,res;
    // prendre la liste d'origine pour la comparer à celle de destination
    // pour trouver le fichier correspondant dans destination, il faut enlever du nom complet le chemin d'origine
    // et ajouter celui de destination
    char szFileToFind[MAX_PATH];
    char szFileToCompare[MAX_PATH];
    for( i = 0; i < m_vSFileO.size(); ++i)
    {
        bool found;
        if( m_mc.wAction == IMAGE )
        {
            strcpy_s( szFileToFind,(const char*)m_vSFileO.at(i).full_name);
        }
        else
        {
            strcpy_s(szFileToFind, m_mc.szDestPath);
            if(szFileToFind[strlen(szFileToFind)-1] == '/')
                szFileToFind[strlen(szFileToFind)-1] = 0;
            strcat_s(szFileToFind, (const char*)&m_vSFileO.at(i).full_name[strlen(m_mc.szOrgPath)]);
        }
        found = false;
        for( j = 0; j < m_vSFileD.size() ; ++j)
        {
            if( m_mc.wAction == IMAGE )
            {
                strcpy_s(szFileToCompare, m_destDir);
                if(szFileToCompare[strlen(szFileToCompare)-1] != '/')
                    strcat_s(szFileToCompare, "/");
                strcat_s(szFileToCompare, (const char*)&m_vSFileD.at(j).full_name);
            }
            else
            {
                strcpy_s( szFileToCompare, (const char*)m_vSFileD.at(j).full_name);
            }
            bool comp = Conversions::strcmpm(szFileToFind, szFileToCompare);
//            int comp = strcmp(szFileToFind, szFileToCompare);
//            if( comp > 0) break;
//            else if( comp == 0)
//            if( comp == 0 )
            if( comp)
            {
                // trouvé
                m_vSFileD[j].iAction = TESTED;
                found = true;
                break;
            }
        }
        if(found)
        {
            // correspondance trouvée, tester si plus récent ou plus ancien
            res = compareFileTime(&m_vSFileO[i], &m_vSFileD[j]);
            // si diff de 0, l'action à entreprendre dépend de wAction de m_mc
            // si RESTORE, copier quoi qu'il en soit
            // si SYNCHRONE, copier ou etre remplacé par plus récent
            // si REFLECT, copier si plus récent
            if( res > 0)
            {
                m_totalJobSize += m_vSFileO[i].org_size;
                m_vSFileO[i].iAction = COPY;
            }
            else if (res < 0)
            {
                if (m_mc.wAction == RESTORE)
                {
                    m_vSFileO[i].iAction = COPY;
                    m_totalJobSize += m_vSFileO[i].org_size;
                }
                else if( m_mc.wAction == SYNCHRONE)
                {
                    m_vSFileO[i].iAction = REPLACE;
                    m_totalJobSize += m_vSFileD[j].org_size;
                }
                else
                {
                    m_vSFileO[i].iAction = NOTHING;
                    m_vSFileD[j].iAction = TESTED;
                }
            }
            else
            {
                m_vSFileO[i].iAction = NOTHING;
                m_vSFileD[j].iAction = TESTED;
            }
        }
        else
        {
            // le destinataire n'existe pas
            m_vSFileO[i].iAction = COPY;
            m_totalJobSize += m_vSFileO[i].org_size;
        }
        if(m_study)
        {
            sfileOutput sfo;
            memset((char*)&sfo, 0, sizeof(sfo));
            strcpy_s((char*)sfo.full_name, MAX_PATH, (const char*)m_vSFileO.at(i).full_name);
            strcpy_s((char*)sfo.org_path, MAX_PATH, (const char*)m_mc.szOrgPath);
            strcpy_s((char*)sfo.dest_path, MAX_PATH, (const char*)m_mc.szDestPath);
            sfo.iAction = m_vSFileO[i].iAction;
            sfo.modif_time = m_vSFileO[i].modif_time;
            sfo.iFileNum = m_vSFileD[j].iRevision;
            sfo.old_time = m_vSFileD[j].modif_time;
            m_mw->putFileInfo(sfo);
        }
    }
}

void scanner::makeDir(QString fullName)
{
    QDir dir;
    int i = fullName.lastIndexOf('/');
    fullName.truncate(i);              // remove file name
    dir.mkpath(fullName);
}

void scanner::execOpFile()
{
    int i, cnt;
    bool done;
    char szDestFile[MAX_PATH];
    sfileOutput sfo;
    QString s;
    QString orgName;
    QString destName;

    QFile studyFile;
    QString fileName = QDir::homePath();

    m_bRunning = true;
    fileName.append("/QtMirror.lst");
    studyFile.setFileName(fileName);
    if(!studyFile.open(QIODevice::ReadOnly)) return;
    cnt = studyFile.size() / sizeof(sfo);
    for (i = 0; i < cnt; ++i)
    {
        studyFile.read((char*)&sfo, sizeof(sfo));
        strcpy_s(szDestFile, (const char*)sfo.dest_path);
        strcat_s(szDestFile, (const char*)&sfo.full_name[strlen((const char*)sfo.org_path)]);
        orgName = (const char *)sfo.full_name;
        destName = szDestFile;
        switch(sfo.iAction)
        {
        case NOTHING:
            break;
        case COPY:
            m_copied++;
            s = tr("Copie de ");
            s.append( orgName );
            m_mw->putText(s);
                if(QFile::exists(destName)) QFile::remove(destName);
                //done = QFile::copy((const char*)sf.full_name, szDestFile);
                m_cf->setFileName(orgName);
                m_mw->resetProgressBar();
                done = m_cf->copy(destName);
                if( !done )
                {
                    makeDir(destName);
                    //done = QFile::copy((const char*)sf.full_name, szDestFile);
                    done = m_cf->copy(destName);
                    if( !done ){
                        m_errors++;
                        s = tr("Erreur de copie de ");
                        s.append(orgName);
                        m_mw->putText(s);
                    }
                }
            break;
        case MOVE:
            m_copied++;
            s = tr("Déplacement de ");
            s.append( orgName);
            m_mw->putText(s);
                if(QFile::exists(destName)) QFile::remove(destName);
                //done = QFile::copy((const char*)sf.full_name, szDestFile);
                m_cf->setFileName(orgName);
                m_mw->resetProgressBar();
                done = m_cf->copy(destName);
                if( !done )
                {
                    makeDir(QString(szDestFile));
                    //done = QFile::copy((const char*)sf.full_name, szDestFile);
                    m_cf->copy(destName);
                    if( !done )
                    {
                        m_errors++;
                        s = tr("Erreur de déplacement de ");
                        s.append(orgName);
                        m_mw->putText(s);
                        break;      //do not remove if could not make the copy
                    }
                }
                QFile::remove(orgName);
            break;
        case REPLACE:
            m_copied++;
            s = tr("Remplacement de ");
            s.append( orgName);
            m_mw->putText(s);
                if(QFile::exists(orgName)) QFile::remove(orgName);
                m_cf->setFileName(destName);
                m_mw->resetProgressBar();
                done = m_cf->copy(orgName);
                //done = QFile::copy(szDestFile, (const char*)sf.full_name);
                if( !done ){
                    m_errors++;
                    s = tr("Erreur de remplacement de ");
                    s.append(orgName);
                    m_mw->putText(s);
                }
            break;
        case EFFACE:
            m_deleted++;
            s = tr("Effacement de ");
            s.append( orgName);
            m_mw->putText(s);
                QFile::remove(orgName);
            break;
        }
    }
    studyFile.close();
    m_bRunning = false;
}
void scanner::execOp()
{
    int i;
    bool done;
    char szDestFile[MAX_PATH];
    struct sfile sf;
    QString s;
    QString orgName;
    QString destName;

    if (m_study) return;

    for (i = 0; i < m_vSFileO.size(); ++i)
    {
        sf = m_vSFileO.at(i);
        strcpy_s(szDestFile, m_mc.szDestPath);
        strcat_s(szDestFile, (const char*)&sf.full_name[strlen(m_mc.szOrgPath)]);
        orgName = (const char *)sf.full_name;
        destName = szDestFile;
        switch(sf.iAction)
        {
        case COPY:
            m_copied++;
            s = tr("Copie de ");
            s.append( orgName );
            m_mw->putText(s);
            if( !m_simul )
            {
                if(QFile::exists(destName)) QFile::remove(destName);
                //done = QFile::copy((const char*)sf.full_name, szDestFile);
                m_cf->setFileName(orgName);
                m_mw->resetProgressBar();
                done = m_cf->copy(destName);
                if( !done )
                {
                    makeDir(destName);
                    //done = QFile::copy((const char*)sf.full_name, szDestFile);
                    done = m_cf->copy(destName);
                    if( !done ){
                        m_errors++;
                        s = tr("Erreur de copie de ");
                        s.append(orgName);
                        m_mw->putText(s);
                    }
                }
            }
            break;
        case MOVE:
            m_copied++;
            s = tr("Déplacement de ");
            s.append( orgName);
            m_mw->putText(s);
            if( !m_simul )
            {
                if(QFile::exists(destName)) QFile::remove(destName);
                //done = QFile::copy((const char*)sf.full_name, szDestFile);
                m_cf->setFileName(orgName);
                m_mw->resetProgressBar();
                done = m_cf->copy(destName);
                if( !done )
                {
                    makeDir(QString(szDestFile));
                    //done = QFile::copy((const char*)sf.full_name, szDestFile);
                    m_cf->copy(destName);
                    if( !done )
                    {
                        m_errors++;
                        s = tr("Erreur de déplacement de ");
                        s.append(orgName);
                        m_mw->putText(s);
                        break;      //do not remove if could not make the copy
                    }
                }
                QFile::remove(orgName);
            }
            break;
        case REPLACE:
            m_copied++;
            s = tr("Remplacement de ");
            s.append( orgName);
            m_mw->putText(s);
            if( !m_simul )
            {
                if(QFile::exists(orgName)) QFile::remove(orgName);
                m_cf->setFileName(destName);
                m_mw->resetProgressBar();
                done = m_cf->copy(orgName);
                //done = QFile::copy(szDestFile, (const char*)sf.full_name);
                if( !done ){
                    m_errors++;
                    s = tr("Erreur de remplacement de ");
                    s.append(orgName);
                    m_mw->putText(s);
                }
            }
            break;
        case EFFACE:
            m_deleted++;
            s = tr("Effacement de ");
            s.append( orgName);
            m_mw->putText(s);
            if( !m_simul )
            {
                QFile::remove(orgName);
            }
            break;
        }
    }
}

void scanner::execOpR()
{
    int i;
    char szDestFile[MAX_PATH];
    struct sfile sf;
    QString mess;
    QString orgName;
    QString destName;
    bool done;

    for (i = 0; i < m_vSFileD.size(); ++i)
    {
        sf = m_vSFileD.at(i);
        if(sf.iAction != TESTED)
        {
            // si non testé, alors c'est qu'il n'existe pas dans le rép. d'origine
            orgName = (const char*)sf.full_name;
            mess = tr("Copie de ");
            mess.append(orgName);
            m_mw->putText(mess);
            if(m_study)
            {
                sfileOutput sfo;
                memset((char*)&sfo, 0, sizeof(sfo));
                strcpy_s((char*)sfo.full_name, MAX_PATH, (const char*)m_vSFileD.at(i).full_name);
                strcpy_s((char*)sfo.org_path, MAX_PATH, (const char*)m_mc.szOrgPath);
                strcpy_s((char*)sfo.dest_path, MAX_PATH, (const char*)m_mc.szDestPath);
                sfo.iAction = m_vSFileO[i].iAction;
                sfo.modif_time = m_vSFileO[i].modif_time;
                sfo.dest_list = true;
                m_mw->putFileInfo(sfo);
            }
            if( !m_simul )
            {
                destName = m_mc.szOrgPath;
//                destName.append(orgName.right(orgName.size()-strlen(m_mc.szDestPath)));
                destName.append(orgName.remove(0, strlen(m_mc.szDestPath)));
                QFile::remove(destName);
                m_cf->setFileName(orgName);
                m_mw->resetProgressBar();
                done = m_cf->copy(destName);
                if (done) done = QFile::copy((const char*)sf.full_name, szDestFile);
                if( !done )
                {
                    makeDir(szDestFile);
                    done = m_cf->copy(destName);
                }
                if( done ) ++m_copied;
                else
                {
                    ++m_errors;
                    mess = tr("Erreur de copie de ");
                    mess.append(orgName);
                    m_mw->putText(mess);
                }
            }
            else m_copied++;
        }
    }
}

void scanner::execOpI7z()
{
    //enregistrer dans un fichier image
    int i;
    qint64 fileSize = 0;
    qint64 totalSize = 0;
    //qint64 result;
    QString fileName;
    QFile file;
    QString mess;
    QString orgPath;
    QString name;
    //struct imageHeader ih;
    struct sfile sf;
    fileName = m_fullImageName;
    m_accesEcriture7z->init();
    fileName.append(QString("(%1).7z").arg(m_volumes, 2, 10, QChar('0')));
    if( !m_simul )
    {
        m_accesEcriture7z->setOutName(fileName);
        orgPath = m_mc.szOrgPath;
        orgPath.append(m_mc.szCommPath);
        m_accesEcriture7z->setOrgPath(orgPath);
        m_accesEcriture7z->setFull(true);
        for (i = 0; i < m_vSFileO.size(); ++i)
        {
            sf = m_vSFileO.at(i);
            if (sf.iAction != COPY)
            {
                m_accesEcriture7z->setFull(false);
                break;
            }
        }
    }
    for (i = 0; i < m_vSFileO.size(); ++i)
    {
        sf = m_vSFileO.at(i);
        if (sf.iAction == COPY)
        {
            // vérifier si la taille originale de fichier ne dépasse pas la capacité FAT32
            if( sf.org_size > 0xffffffff )
            {
                mess = (const char *)sf.full_name;
                mess.append(tr(" ne peut etre copié car trop volumineux."));
                m_mw->putText(mess);
            }
            else
            {
                m_copied++;
                mess = "Copie de ";
                mess.append( (const char *)sf.full_name);
                m_mw->putText(mess);
                if( !m_simul )
                {
                    // commencer par vérifier si ne risque pas de déborder en taille pour un disque en FAT32
                    if( (totalSize+sf.org_size) >= 0xfffffffe )
                    {
                        //oui, alors clôtuter ce fichier et changer de volume
                        m_accesEcriture7z->readCompressAndSave();
                        fileSize += m_accesEcriture7z->finalSize();
                        m_volumes++;
                        fileName = m_fullImageName;
                        fileName.append(QString("(%1).7z").arg(m_volumes, 2, 10, QChar('0')));
                        m_accesEcriture7z->setOutName(fileName);
                        fileSize = 0;
                    }
                    // ajouter le fichier à  l'image
                    name = QString((char*)sf.full_name);
                    m_accesEcriture7z->addFile(name);
                    totalSize += sf.org_size;
                } // end if !m_simul
            } // end else
        }
    }
    m_accesEcriture7z->readCompressAndSave();
    fileSize += m_accesEcriture7z->finalSize();
    m_totalSize = totalSize;
    m_copySize = fileSize;
    mess = QString("Taille image:%1 octets pour un total de %2").arg(fileSize).arg(totalSize);
    m_mw->putText(mess);
}

void scanner::restoreFromImage7z()
{
    SRes sres;
    int read = -1;
    UInt64 size64;
    const Byte *adrData;
    QString fileName;
    QFile file;
    QFile outFile;
    int i;
    struct sfile sf;
    QString s;

    for (i = 0; i < m_vSFileD.size(); ++i)
    {
        sf = m_vSFileD.at(i);
        if(sf.iAction == NOTHING) break;
        if(sf.iRevision != read)
        {
            fileName = m_fullImageName;
            fileName.append(QString("(%1).7z").arg(sf.iRevision, 2, 10, QChar('0')));
            file.setFileName(fileName);
            // ouvrir le fichier
            file.open(QIODevice::ReadOnly);
            m_accesLecture7z->openFile2(&file);
            sres = m_accesLecture7z->getHeader();
            if(sres != SZ_OK)
            {
                file.close();
                break;
            }
            sres = m_accesLecture7z->decodeStream(0);
            file.close();
            if(sres != SZ_OK)
            {
                break;
            }
            read = sf.iRevision;
        }
        // calcul de l'offset de fichier à récupérer
        adrData = m_accesLecture7z->streamAdress();
        adrData += sf.offset;
        size64 = sf.offset + sf.org_size;
        // vérifier que la position existe
        if( m_accesLecture7z->packSize(0) > size64 )
        {
            m_copied++;
            s = "Copie de ";
            s.append( (const char *)sf.full_name);
            m_mw->putText(s);
            if( !m_simul && sf.iAction != NOTHING)
            {
                struct utimbuf utb;
                outFile.setFileName(QString((const char*)sf.full_name));
                if( outFile.exists() ) QFile::remove((const char*)sf.full_name);
                outFile.open(QIODevice::ReadWrite);
                outFile.write((char*)adrData, sf.org_size);
                outFile.close();
                // ICI ETUDIER LA QUESTION DE RETABLIR LE TIMESTAMP DU FICHIER----------------------------------
                utb.actime = (time_t)sf.modif_time;
                utb.modtime = (time_t)sf.modif_time;
                utime((const char*)sf.full_name, &utb);
            }
        }
    }
    return;
}

void scanner::run()
{
    if(m_bRunning) return;
    m_bRunning = true;
    m_totalJobSize = 0;
    m_totalJobDone = 0;
    m_scanned = m_copied = m_deleted = m_errors = 0;
    QString orgPath;
    QString destPath;
    QString mess;
    sfileOutput sfo;

    memset(&sfo, 0, sizeof(sfo));
    orgPath = m_mc.szOrgPath;
    destPath = m_mc.szDestPath;
    orgPath.append(m_mc.szCommPath);
    destPath.append(m_mc.szCommPath);
    if( m_mc.wOptions & SIMULATE )
        m_simul = true;
    else
        m_simul = false;
    if( m_mc.wOptions & STUDY )
    {
        m_study = true;
        m_simul = true;
    }
    else
        m_study = false;
    switch(m_mc.wAction)
    {
    case REFLECT:
        m_vSFileO.clear();
        mess = "Analyse du répertoire source: ";
        mess.append(orgPath);
        m_mw->putText(mess);
        if(m_study)
        {
            strcpy_s((char*)sfo.full_name, MAX_PATH, m_mc.szOrgPath);
            sfo.iAction = TITLELINE;
            m_mw->putFileInfo(sfo);
        }
        listOfFiles(true, orgPath);
        std::sort(m_vSFileO.begin(), m_vSFileO.end(), myCompare);
        m_vSFileD.clear();
        mess = "Analyse du répertoire destination: ";
        mess.append(destPath);
        m_mw->putText(mess);
        if(m_study)
        {
            strcpy_s((char*)sfo.full_name, MAX_PATH, m_mc.szDestPath);
            sfo.iAction = TITLELINE;
            m_mw->putFileInfo(sfo);
        }
        listOfFiles(false, destPath);
        std::sort(m_vSFileD.begin(), m_vSFileD.end(), myCompare);
        compareListOfFiles();
        execOp();
        break;
    case SYNCHRONE:
        m_vSFileO.clear();
        mess = "Analyse du répertoire source: ";
        mess.append(orgPath);
        m_mw->putText(mess);
        if(m_study)
        {
            strcpy_s((char*)sfo.full_name, MAX_PATH, m_mc.szOrgPath);
            sfo.iAction = TITLELINE;
            m_mw->putFileInfo(sfo);
        }
        listOfFiles(true, orgPath);
        std::sort(m_vSFileO.begin(), m_vSFileO.end(), myCompare);
        m_vSFileD.clear();
        mess = "Analyse du répertoire destination: ";
        mess.append(destPath);
        m_mw->putText(mess);
        if(m_study)
        {
            strcpy_s((char*)sfo.full_name, MAX_PATH, m_mc.szDestPath);
            sfo.iAction = TITLELINE;
            m_mw->putFileInfo(sfo);
        }
        listOfFiles(false, destPath);
        std::sort(m_vSFileD.begin(), m_vSFileD.end(), myCompare);
        compareListOfFiles();
        execOp();
        execOpR();
        break;
    case RESTORE:
        m_vSFileO.clear();
        mess = "Analyse du répertoire source: ";
        mess.append(orgPath);
        m_mw->putText(mess);
        if(m_study)
        {
            strcpy_s((char*)sfo.full_name, MAX_PATH, m_mc.szOrgPath);
            sfo.iAction = TITLELINE;
            m_mw->putFileInfo(sfo);
        }
        listOfFiles(true, orgPath);
        std::sort(m_vSFileO.begin(), m_vSFileO.end(), myCompare);
        m_vSFileD.clear();
        mess = "Analyse du répertoire destination: ";
        mess.append(destPath);
        m_mw->putText(mess);
        if(m_study)
        {
            strcpy_s((char*)sfo.full_name, MAX_PATH, m_mc.szDestPath);
            sfo.iAction = TITLELINE;
            m_mw->putFileInfo(sfo);
        }
        listOfFiles(false, destPath);
        std::sort(m_vSFileD.begin(), m_vSFileD.end(), myCompare);
        compareListOfFiles();
        execOp();
        break;
    case IMAGE:
        m_vSFileO.clear();
        mess = "Analyse du répertoire source: ";
        mess.append(orgPath);
        if(!m_study)
        {
            m_mw->putText(mess);
        }
        listOfFiles(true, orgPath);
        std::sort(m_vSFileO.begin(), m_vSFileO.end(), myCompare);
        m_vSFileD.clear();
        if( m_mc.wOptions & INCREMENT)
        {
            // pour une sauvegarde incrémentielle, on lit les fichiers des anciennes
            // sauvegardes
            mess = "Analyse des fichiers de sauvegarde: ";
            mess.append(m_mc.szImageFile);
            if(m_study)
            {
                // en cas de réalisation d'un fichier d'étude de travail à faire
                // indiquer en titre la recherche
                strcpy_s((char*)sfo.full_name, MAX_PATH, mess.toLatin1().data());
                sfo.iAction = TITLELINE;
                m_mw->putFileInfo(sfo);
            }
            else
            {
                m_mw->putText(mess);
            }
            m_unique = true;
            listOf7zImageFiles();
            std::sort(m_vSFileD.begin(), m_vSFileD.end(), myCompare);
            compareListOfFiles();
        }
        else
        {
            // pour une sauvegarde totale, on calcule la taille totale
            for( int i = 0; i < m_vSFileO.size(); ++i )
            {
                m_vSFileO[i].iAction = COPY;
                m_totalJobSize += m_vSFileO[i].org_size;
            }
        }
        execOpI7z();
        break;
    case IMAGELIST:
        m_vSFileD.clear();
        mess = "Analyse des fichiers de sauvegarde: ";
        mess.append(m_mc.szImageFile);
        m_mw->putText(mess);
        m_unique = false;
        listOf7zImageFiles();
        std::sort(m_vSFileD.begin(), m_vSFileD.end(), myCompare);
        break;
    case IMAGERESTORE:
        m_vSFileD.clear();
        mess = "Analyse des fichiers de sauvegarde: ";
        mess.append(m_mc.szImageFile);
        m_mw->putText(mess);
        m_unique = true;
        listOf7zImageFiles();
        std::sort(m_vSFileD.begin(), m_vSFileD.end(), myCompare);
        // inserrer ici l'envoi de la liste des fichiers pour validation de restauration
        DialogEditImageList* mEil;
        m_mw->interuptTimer();
        mEil = new DialogEditImageList(&m_vSFileD);
        if(mEil->exec())
        {
            mEil->validateList();
            restoreFromImage7z();
        }
        m_mw->restartTimer();
        break;
    }
    m_bRunning = false;
}

void scanner::end()
{
    exit(retcode);
}
