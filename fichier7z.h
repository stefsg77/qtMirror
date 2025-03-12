#ifndef FICHIER7Z_H

#include "7z.h"
#include "7zTypes.h"
#include <QVector>
#define MY_CPU_LE_UNALIGN
#ifndef RINOK
#define RINOK(x) { const int _result_ = (x); if (_result_ != 0) return _result_; }
#endif


#define kEnd    0x00
#define kHeader 0x01
#define kArchiveProperties  0x02
#define kAdditionalStreamsInfo  0X03
#define kMainStreamsInfo    0x04
#define kFilesInfo  0x05
#define kPackInfo   0x06
#define kUnPackInfo 0X07
#define kSubStreamsInfo    0x08
#define kSize   0x09
#define kCRC    0x0A
#define kFolder 0x0B
#define kCodersUnPackSize 0X0C
#define kNumUnPackStream    0x0D
#define kEmptyStream    0x0E
#define kEmptyFile  0x0F
#define kAnti   0x10
#define kName   0x11
#define kCTime  0x12
#define kATime  0x13
#define kMTime  0x14
#define kWinAttributes  0x15
#define kComment    0x16
#define kEncodedHeader 0x17
#define kStartPos   0x18
#define kDummy  0x19

#define IsComplexCoder  0b0010000
#define HasAttributes   0b0100000
#define CodecIdSizeMask     0b0001111

#define OpenErrorZ   1
#define SignatureErrorZ 2
#define headCRCerrorZ   3
#define ReadErrorZ   4
#define WriteErrorZ  5
#define CRCerrorZ    6
#define DecompErrorZ 7
#define BadHeaderErrorZ 8
#define UnsupportedFunction 9
#define MemoryErrorZ 10

#define Error1MsgZ   "Erreur à l'ouverture du fichier"
#define Error2MsgZ   "Erreur de signature de fichier"
#define Error3MsgZ   "Erreur de CRC entête"
#define Error4MsgZ   "Erreur à la lecture du fichier"
#define Error5MsgZ   "Erreur à l'ecriture du fichier"
#define Error6MsgZ   "Erreur de CRC32"
#define Error7MsgZ   "Erreur à la décompression"
#define Error8MsgZ   "Erreur de format de l'entete"
#define Error9MsgZ   "A rencontré une fonction non implémentée"
#define Error10MsgZ  "Erreur mémoire"

#define k_Scan_NumCoders_MAX 64
#define k_Scan_NumCodersStreams_in_Folder_MAX 64

#ifdef MY_CPU_LE_UNALIGN

#define GetUi16(p) (*(const UInt16 *)(const void *)(p))
#define GetUi32(p) (*(const UInt32 *)(const void *)(p))
#ifdef MY_CPU_LE_UNALIGN_64
#define GetUi64(p) (*(const UInt64 *)(const void *)(p))
#define SetUi64(p, v) { *(UInt64 *)(void *)(p) = (v); }
#endif

#define SetUi16(p, v) { *(UInt16 *)(void *)(p) = (v); }
#define SetUi32(p, v) { *(UInt32 *)(void *)(p) = (v); }

#else

#define GetUi16(p) ( (UInt16) ( \
             ((const Byte *)(p))[0] | \
    ((UInt16)((const Byte *)(p))[1] << 8) ))

#define GetUi32(p) ( \
             ((const Byte *)(p))[0]        | \
    ((UInt32)((const Byte *)(p))[1] <<  8) | \
    ((UInt32)((const Byte *)(p))[2] << 16) | \
    ((UInt32)((const Byte *)(p))[3] << 24))

#define SetUi16(p, v) { Byte *_ppp_ = (Byte *)(p); UInt32 _vvv_ = (v); \
    _ppp_[0] = (Byte)_vvv_; \
    _ppp_[1] = (Byte)(_vvv_ >> 8); }

#define SetUi32(p, v) { Byte *_ppp_ = (Byte *)(p); UInt32 _vvv_ = (v); \
    _ppp_[0] = (Byte)_vvv_; \
    _ppp_[1] = (Byte)(_vvv_ >> 8); \
    _ppp_[2] = (Byte)(_vvv_ >> 16); \
    _ppp_[3] = (Byte)(_vvv_ >> 24); }

#endif

#pragma pack(1)
typedef struct _entete7z{
    Byte signature[6];
    Byte majorVersion;
    Byte minorVersion;
    UInt32 startHeaderCRC;
    UInt64 nextHeaderOffset;
    UInt64 nextHeaderSize;
    UInt32 nextHeaderCRC;
} entete7z;
typedef struct _folder{
    Byte CodecIdSize;
    Byte CodecId[16];
    UInt64 NumInStreams;
    UInt64 NumOutStreams;
    UInt64 NumUStreams;
    UInt64 StreamsSize;
    UInt64 PropertiesSize;
    Byte Properties[16];
    size_t CoderOffset;
    UInt32 StartIndex;
    UInt32 UnpackSize;
    UInt32 CRC32id;
    Byte UnpackSizeIndex;
} folder;
typedef struct _bindpairs
{
    UInt64 InIndex;
    UInt64 OutIndex;
} BindPairs;

typedef struct
{
    Byte *data;
    size_t size;
} CBuf;

typedef struct _unpacksizespair{
    UInt32 FolderIndex;
    UInt64 UnPackSizeS;
} UnPackSizePairs;

typedef struct
{
    UInt64 offset;
    UInt64 size;
} UnPackBlocks;

typedef struct _fileInfoZ{
    Byte propType;
    Byte timeDefined;
    CNtfsFileTime cTime;
    CNtfsFileTime aTime;
    CNtfsFileTime mTime;
    UInt64 mQTime;
    wchar_t name[260];
    wchar_t limit;
    UInt64 dataOffset;
    UInt64 dataSize;
    UInt64 size;
    UInt32 attributes;
    UInt32 CRC;
    UInt32 folderIndex;
    UInt32 fileToFolder;
    UInt32 folderToFile;
    bool isDir;
} fileinfoZ;
#pragma pack()

#define FICHIER7Z_H

#endif // FICHIER7Z_H
