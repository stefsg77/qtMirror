#ifndef ENCODER7Z_H
#define ENCODER7Z_H

#include "7z.h"
#include "fichier7z.h"
#include "crc32.h"
#include <QFile>
#include <QString>
#include <QVector>

class Encoder7z
{
public:
    Encoder7z();
    void init();
    void setOutName(QString name){ m_outName = name;}
    void setOrgPath(QString path){m_orgPath = path;}
    void setFull(bool full){m_full = full;}
    UInt32 addFile(QString file);
    SRes readCompressAndSave();
    UInt64 finalSize(){return m_finalSize;}
private:
    QFile m_outputFile;
    QString m_outName;
    QString m_orgPath;
    QVector <QString> m_inNames;
    QVector <QString> m_dirNames;
    entete7z m_entete;
    bool m_full;
    Byte *m_inBuffer;
    Byte *m_outBuffer;
    CRC32 m_crc32;
    UInt64 m_inSize;
    UInt64 m_outSize;
    UInt64 m_offset;
    UInt64 m_streamSize;
    UInt64 m_offsetMainHeader;
    UInt64 m_offsetEndHeader;
    UInt64 m_mainHeaderSize;
    UInt64 m_endHeaderSize;
    UInt64 m_finalSize;
    UInt32 m_crcHeader;
    CSzDataw m_sd;
    QVector <UInt64> m_offsets;
    QVector <UInt64> m_sizes;
    QVector <UInt32> m_CRCs;
    QVector <UInt32> m_attribs;
    QVector <CNtfsFileTime> m_mTimes;
    QVector <fileinfoZ> m_filesInfo;
};

#endif // ENCODER7Z_H
