#ifndef ENTETEFICHIER_H
#define ENTETEFICHIER_H

#include "7z.h"
#include "fichier7z.h"
#include "crc32.h"
#include <QString>
#include <QFile>
#include <QVector>

#define NUM_ADDITIONAL_STREAMS_MAX  5

typedef struct{
    Byte *pFileNames;
    size_t *pOffsetNames;
} NAMESPAIR;

typedef struct
{
    UInt32 NumTotalSubStreams;
    UInt32 NumSubDigests;
    CSzData sdNumSubStreams;
    CSzData sdSizes;
    CSzData sdCRCs;
} CSubStreamInfo;


class EnteteFichier
{
public:
    EnteteFichier();
    void setFileName(QString fileName);
    SRes openFile();
    SRes openFile2(QFile *inFile);
    void closeFile() {m_file.close();}
    void menage();
    SRes getHeader();
    SRes decodeStream(UInt32 i);
    SRes readHeader2(CBuf *tempBufs, UInt32 *numTempBufs);
    SRes SzReadStreamsInfo(CSzAr *p,
                           CSzData *sd,
                           UInt32 numFoldersMax, const CBuf *tempBufs, UInt32 numTempBufs,
                           UInt64 *dataOffset,
                           CSubStreamInfo *ssi,
                           ISzAllocPtr alloc);
    SRes ReadSubStreamsInfo(CSzAr *p, CSzData *sd, CSubStreamInfo *ssi);
    SRes ReadPackInfo(CSzAr *p, CSzData *sd, ISzAllocPtr alloc);
    SRes ReadUnpackInfo(CSzAr *p, CSzData *sd2, UInt32 numFoldersMax,
                         const CBuf *tempBufs, UInt32 numTempBufs,
                        ISzAllocPtr alloc);
    SRes ReadFilesInfo(CSzArEx *p, CSzData *sd, CBuf *tempBufs, UInt32 *numTempBfs, CSubStreamInfo *ssi);
    UInt64 SzAr_GetFolderUnpackSize(UInt32 folderIndex)
    {
        return m_arEx.db.CoderUnpackSizes[m_arEx.db.FoToCoderUnpackSizes[folderIndex] + m_arEx.db.FoToMainUnpackSizeIndex[folderIndex]];
    }

    UInt32 numFiles(){ return m_arEx.NumFiles;}
    UInt32 numFolders(){return (UInt32)m_arEx.db.NumFolders;}
    UInt64 numPacks(){return m_arEx.db.NumPackStreams;}
    QString fileName(UInt32 id){
        if(id >=m_fileInfo.size()) return QString(); else return QString(m_fileInfo.at(id).name);}
    UInt32 fileOffset(UInt32 n) {if(n < m_fileInfo.size()) return m_fileInfo[n].dataOffset; return 0;}
    UInt64 fileSize(UInt32 n) {if(n < m_fileInfo.size()) return m_fileInfo[n].size; else return 0;}
    UInt64 filePackedSize(UInt32 n) { if(n < m_fileInfo.size()) return m_fileInfo[n].dataSize; else return 0;}
    UInt32 fileCRC(UInt32 n) {if(n < m_fileInfo.size()) return m_fileInfo[n].CRC; else return 0;}
    bool isDir(int i) {if(i < m_fileInfo.size()) return m_fileInfo[i].isDir; else return true;}
    UInt32 fileAttribs(int i) {if(i < m_fileInfo.size()) return m_fileInfo[i].attributes; else return 0;}
    Byte majorVersion() {return m_entete.majorVersion;}
    Byte minorVersion() {return m_entete.minorVersion;}
    UInt32 startHeaderCRC(){return m_entete.startHeaderCRC;}
    UInt64 nextHeaderOffset(){return m_entete.nextHeaderOffset;}
    UInt64 nextHeaderSize(){return m_entete.nextHeaderSize;}
    UInt32 nextHeaderCRC(){return m_entete.nextHeaderCRC;}
    bool enteteCode(){return m_enteteCode;}
    UInt32 CRCverif(){return m_CRCverif;}
    UInt64 packPos(){return m_arEx.db.PackPositions[1];}
    const Byte *streamAdress(){ return (const Byte*)m_streamDataOut;}
    CNtfsFileTime modifTime(int i){
        CNtfsFileTime ft;
        ft.High = ft.Low = 0;
        if(i<m_fileInfo.size() && SzBitWithVals_Check(&m_arEx.MTime, i))
        {
            ft = m_arEx.MTime.Vals[i];
        }
        return ft;}
    UInt64 modifQTime(int i){
        if(i < m_fileInfo.size()) return m_fileInfo[i].mQTime;
        else return 0;
    }
    UInt64 numPackedStreams(){return m_ssi.NumTotalSubStreams;}
    UInt64 packSize(int i){ if(i >= (int)m_arEx.db.NumPackStreams) return 0; else return m_arEx.db.CoderUnpackSizes[i];}
    Byte* encodage(int i);
    const Byte* signature(){return (const Byte*)m_entete.signature;}
private:
    SRes readFileNames(const Byte *data, size_t size, UInt32 numFiles, size_t *offsets);
    QString m_fileName;
    QFile m_file;
    QFile *m_pFile;
    Byte* m_headerData;
    Byte* m_streamDataIn;
    Byte* m_streamDataOut;
    Byte m_PropertyData[16];
    entete7z m_entete;
    bool m_enteteCode;
    UInt64 m_startPosAfterHeader;
    UInt64 m_dataPos;
    UInt64 m_packPos;
    UInt32 m_CRCverif;
    UInt32 m_numFolders;
    UInt32 m_numFiles;
    UInt32 m_errorCode;
    UInt64 m_lngBufHeader;
    QVector <fileinfoZ> m_fileInfo;
    QVector <UnPackSizePairs> m_uSizePairs;

    char *m_errorString;
    Byte *m_codersData;
    Byte *m_isDir;
    CSzData m_coder;
    CSzData m_unPackedSizes;
    CSzData m_properties;

    CRC32* m_pcrc32;
    CSubStreamInfo m_ssi;
    CSzArEx m_arEx;
};

#endif // ENTETEFICHIER_H
