#ifndef SCANNER_H
#define SCANNER_H

#include <QThread>
#include <QString>
#include <QFileInfo>
#include "QtMirror.h"
#include "mainwindow.h"
#include "copyfile.h"
#include "entetefichier.h"
#include "encoder7z.h"
#include "crc32.h"

class scanner : public QThread
{
    Q_OBJECT
public:
    explicit scanner(MirwCmde mc, MainWindow *parent = 0);
    ~scanner();
    void run();
    void end();
    int scanned(){ return m_scanned;}
    int copied(){return m_copied;}
    int deleted(){return m_deleted;}
    int errors(){return m_errors;}
    qint64 copySize(){ return m_copySize;}
    qint64 totalSize(){ return m_totalSize;}
    //bool myCompare(const struct sfile &sf1, const struct sfile &sf2);
    void execOpFile();
public slots:
    void getProgress(int percent, int chunk);
    
protected:
    bool directoryCheck(QFileInfo *pfi);
    bool directoryExclCheck(QFileInfo *pfi);
    bool fileCheck(QFileInfo *pfi);
    bool fileExclCheck(QFileInfo *pfi);
    bool isCompressed(QFileInfo *pfi);
    bool isCompressed(sfile *file);
    bool isText(sfile *file);
    int compareFileTime(QFileInfo *pfi1, QFileInfo *pfi2);
    int compareFileTime(struct sfile *psf1, struct sfile *psf2);
    void listOfFiles(bool org, QString orgPath);
    int listOf7zImageFiles();
    void compareListOfFiles();
    void makeDir(QString fullName);
    void restoreFromImage7z();
    void execOp();
    void execOpR();
    void execOpI7z();

private:
    bool m_simul;               // travail en simulation seulement
    bool m_study;               // recherche des opérations à faire pour affichage
    bool m_unique;              // dans les fichiers image, ne retenir que la version
                                // la plus récente de chaque fichier
    bool m_bRunning;            // en cours de travail
    int retcode;
    MainWindow *m_mw;
    MirwCmde m_mc;
    QString m_fullImageName;
    int m_volumes;              // nombre de volumes d'unr savegarde image
    int m_scanned;              // nombre de fichiers testés dans la source
    int m_scannedR;             // puis dans la destination
    int m_copied;
    int m_deleted;
    int m_errors;
    qint64 m_copySize;
    qint64 m_totalSize;
    qint64 m_totalJobSize;
    qint64 m_totalJobDone;
    QStringList m_compExt;      // les extensions pour les fichiers compressés
    QStringList m_textExt;      // les extensions pour les fichiers textes
    QVector <struct sfile> m_vSFileO;   // liste des fichiers lus en entrée
    QVector <struct sfile> m_vSFileD;   // et ceux dans la destination
    QVector <struct sfileR> m_vRFile;   // les fichiers à restaurer
    char m_destDir[MAX_PATH];
    QCopyFile *m_cf;
    CRC32 m_crc32;
    EnteteFichier *m_accesLecture7z;
    Encoder7z *m_accesEcriture7z;
};

#endif // SCANNER_H
