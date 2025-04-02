#include "encoder7z.h"
#include "LzmaEnc.h"
#include "QtMirror.h"
#include <QFileInfo>

static ISzAlloc zAlloc;
static void *SzAlloc(ISzAllocPtr p, size_t size) { p = p; return malloc(size); }
static void SzFree(ISzAllocPtr p, void *address) { p = p; free(address); }


#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_HIDDEN 0x00000002
#define FILE_ATTRIBUTE_SYSTEM 0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_ARCHIVE 0x00000020
#define FILE_ATTRIBUTE_DEVICE 0x00000040
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE 0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#define FILE_ATTRIBUTE_COMPRESSED 0x00000800
#define FILE_ATTRIBUTE_OFFLINE 0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED 0x00004000


#define SZ_WRITE_BYTE(org) \
if (sd->Size == 0) return SZ_ERROR_ARCHIVE; \
    sd->Size--; *sd->Data++ = org;

#define SZ_WRITE_BYTE_2(org) \
    if (sd.Size == 0) return SZ_ERROR_ARCHIVE; \
    sd.Size--; *sd.Data++ = org;


static void MsQdateTimeToNtfsFileTime(CNtfsFileTime *ft, UInt64 qt)
{
    qt += 11644473600000;
    qt *= 10000;
    ft->Low = (UInt32)qt & 0xffffffff;
    qt >>= 32;
    ft->High = (UInt32)qt;
}

static SRes Uint64toNumber(CSzDataw *sd, UInt64 var)
{
    SRes res = SZ_OK;
    int i;
    Byte b, v;
    UInt64 temp = 0;
    if(var > 0xffffffffffffff)
    {
        b = 0xff;
        SZ_WRITE_BYTE(b)
        for(i = 7; i >= 0; i--)
        {
            temp = var & 0xff;
            b = temp;
            SZ_WRITE_BYTE(b)
            var >>= 8;
        }
    }
    else if(var > 0xffffffffffff)
    {
        b = 0xfe;
        SZ_WRITE_BYTE(b)
        for(i = 6; i >= 0; i--)
        {
            temp = var & 0xff;
            b = temp;
            SZ_WRITE_BYTE(b)
            var >>= 8;
        }
    }
    else if ( var > 0x1ffffffffff)
    {
        b = 0b11111100;
        temp = var >> 48;
        v = temp & 1;
        b |= v;
        SZ_WRITE_BYTE(b)
        for(i = 5; i >= 0; i--)
        {
            temp = var & 0xff;
            b = temp;
            SZ_WRITE_BYTE(b)
            var >>= 8;
        }
    }
    else if ( var > 0x3ffffffff)
    {
        b = 0b11111000;
        temp = var >> 40;
        v = temp & 3;
        b |= v;
        SZ_WRITE_BYTE(b)
        for(i = 4; i >= 0; i--)
        {
            temp = var & 0xff;
            b = temp;
            SZ_WRITE_BYTE(b)
            var >>= 8;
        }
    }
    else if ( var > 0x7ffffff)
    {
        b = 0b11110000;
        temp = var >> 32;
        v = temp & 7;
        b |= v;
        SZ_WRITE_BYTE(b)
        for(i = 3; i >= 0; i--)
        {
            temp = var & 0xff;
            b = temp;
            SZ_WRITE_BYTE(b)
            var >>= 8;
        }
    }
    else if ( var > 0xfffff)
    {
        b = 0b11100000;
        temp = var >> 24;
        v = temp & 0xf;
        b |= v;
        SZ_WRITE_BYTE(b)
        for(i = 2; i >= 0; i--)
        {
            temp = var & 0xff;
            b = temp;
            SZ_WRITE_BYTE(b)
            var >>= 8;
        }
    }
    else if ( var > 0x3fff)
    {
        b = 0b11000000;
        temp = var >> 16;
        v = temp & 0x1f;
        b |= v;
        SZ_WRITE_BYTE(b)
        for(i =1; i >= 0; i--)
        {
            temp = var & 0xff;
            b = temp;
            SZ_WRITE_BYTE(b)
            var >>= 8;
        }
    }
    else if (var > 0x7f)
    {
        b = 0b10000000;
        temp = var >> 8;
        v = temp & 0x3f;
        b |= v;
        SZ_WRITE_BYTE(b)
        temp = var & 0xff;
        b = temp;
        SZ_WRITE_BYTE(b)
    }
    else {
        b = var & 0x7f;
        SZ_WRITE_BYTE(b)
    }
    return res;
}

Encoder7z::Encoder7z()
{
    zAlloc.Alloc = SzAlloc;
    zAlloc.Free = SzFree;
    m_inBuffer = m_outBuffer = 0;
}

void Encoder7z::init()
{
    if(m_inBuffer) free(m_inBuffer);
    if(m_outBuffer) free(m_outBuffer);
    m_inBuffer = m_outBuffer = 0;
    m_dirNames.clear();
    m_inNames.clear();
    m_sizes.clear();
    m_CRCs.clear();
    m_offsets.clear();
    m_offset = 0;
    m_streamSize = 0;
    m_orgPath.clear();
    m_outName.clear();
    m_mTimes.clear();
}

UInt32 Encoder7z::addFile(QString file)
{
    UInt32 crc = 0;
    UInt32 attribs = 0;
    UInt64 size;
    QFileInfo qfi;
    qfi.setFile(file);
    if(!qfi.isWritable()) attribs |= FILE_ATTRIBUTE_READONLY;
    if(qfi.isHidden()) attribs |= FILE_ATTRIBUTE_HIDDEN;
    if(qfi.isDir()) attribs |= FILE_ATTRIBUTE_DIRECTORY;
    m_attribs.append(attribs);
    size = qfi.size();
    m_sizes.append(size);
    m_offsets.append(m_offset);
    m_offset += size;
    m_CRCs.append(crc);     // cette valeure sera calcuuléee en phane finale
    CNtfsFileTime fileTimeM;
    QDateTime ft = qfi.lastModified();
    MsQdateTimeToNtfsFileTime(&fileTimeM, ft.toMSecsSinceEpoch());
    m_mTimes.append(fileTimeM);
    // si besoin, retirer le path d'origine
    file.remove(m_orgPath);
    m_inNames.append(file);
    return m_inNames.size();
}

SRes Encoder7z::readCompressAndSave()
{
//    UInt64 firstOffset;
//    UInt64 tempOffset;
    SRes res = SZ_OK;
    UInt32 i,j;
    UInt32 numFiles = m_inNames.size();
    // vérifier que toues les données sont présentes
    if(m_outName.size() == 0 || m_orgPath.size() == 0 || numFiles == 0)
        return UnsupportedFunction;
    // commencer par allouer le buffer d'entrée et de sortie, la taille est dans m_offset
    // allouer un buffer de sortie equivalent
    if(m_outBuffer) free(m_outBuffer);
    m_outBuffer = (Byte*)malloc(m_offset);
    if(m_outBuffer == nullptr) return MemoryErrorZ;
    memset(m_outBuffer, 0, m_offset);
    if(m_inBuffer) free(m_inBuffer);
    m_inBuffer = (Byte*)malloc(m_offset);
    if(m_inBuffer == nullptr)
    {
        free(m_outBuffer);
        m_outBuffer = nullptr;
        return MemoryErrorZ;
    }
    // lire tous les fichiers et les ajouter
    for(i = 0; i < numFiles; i++)
    {
        QString fileName;
        QFile inFile;
        UInt64 offset;
        UInt32 crc;
        offset = m_offsets[i];
        fileName = m_orgPath;
        //fileName.append("/");
        fileName.append(m_inNames[i]);
        inFile.setFileName(fileName);
        inFile.open(QIODevice::ReadOnly);
        inFile.read((char*)m_inBuffer+offset, m_sizes[i]);
        inFile.close();
        // calculer le CRC32
        m_crc32.Reset();
        m_crc32.Update((Byte*)m_inBuffer+offset, m_sizes[i]);
        crc = m_crc32.GetValue();
        m_CRCs[i] = crc;
    }
    // appeller la fonction de compression
    SizeT outSize = m_offset;
    CLzmaEncProps enProps;
    LzmaEncProps_Init(&enProps);
    Byte propsEnc[20];
    SizeT propsSize = 20;
    struct imageHeader ih;
    UInt64 done;
    entete7z zHeader;
    /*
SRes LzmaEncode(Byte *dest, SizeT *destLen, const Byte *src, SizeT srcLen,
    const CLzmaEncProps *props, Byte *propsEncoded, SizeT *propsSize, int writeEndMark,
    ICompressProgressPtr progress, ISzAllocPtr alloc, ISzAllocPtr allocBig);
     */
    res = LzmaEncode(m_outBuffer, &outSize, (const Byte *)m_inBuffer, (SizeT)m_offset,
                    (const CLzmaEncProps*)&enProps, propsEnc, &propsSize, 0,
                    NULL, &zAlloc, &zAlloc);

    // libérer le buffer d'entrée
    free(m_inBuffer);
    m_inBuffer = nullptr;
    if( res != SZ_OK)
        return res;


    // créer le fichier de sortie, écrire l'entete 7z, écrire l'entete de qtMirror (qui contient les infos propres à qtMirror
    {
        memset((char*)&zHeader, 0, sizeof(zHeader));
        Byte kSignature[6] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};
        memcpy(zHeader.signature, kSignature, 6);
        zHeader.minorVersion = 4;
        ih.full = m_full;
        ih.size = sizeof(ih);
        ih.signature[0] = 'Q';
        ih.signature[1] = 't';
        ih.signature[2] = '0';
        ih.signature[3] = '2';
        strcpy_s(ih.originPath, MAX_PATH, m_orgPath.toLatin1());
        m_outputFile.setFileName(m_outName);
        m_outputFile.open(QIODevice:: ReadWrite);
        m_outputFile.write((char*)&zHeader, sizeof(zHeader));
        // ecrire les données compressées et se souvenir de l'offset de fin
        done = m_outputFile.write((const char*)m_outBuffer, (UInt64)outSize);
        free(m_outBuffer);
        m_outBuffer = nullptr;
        if(done != outSize){
            m_outputFile.close();
            return WriteErrorZ;
        }
    }
    UInt64 temp64;
//    tempOffset = m_outputFile.pos();

    // créer le header 7z
    Byte mainByte;
    Byte *mainHeader;
    Byte *tempPointer;
    CSzDataw *sd = &m_sd;
    sd->Size = 380 * (m_inNames.size() + 1);
    UInt64 initSize = sd->Size;
    mainHeader = (Byte*)malloc(sd->Size);
    sd->Data = mainHeader;
    // commencer à mettre les données dans le buffer créé
    SZ_WRITE_BYTE(kHeader)
    SZ_WRITE_BYTE(kMainStreamsInfo)
    SZ_WRITE_BYTE(kPackInfo)
    SZ_WRITE_BYTE(0)    // l'offset du streamPrincipal
    SZ_WRITE_BYTE(1)    // nombre de streams
//    Uint64toNumber(sd, (UInt64)m_inNames.size());
    SZ_WRITE_BYTE(kSize)
    Uint64toNumber(sd, outSize);
    SZ_WRITE_BYTE(kEnd)
    SZ_WRITE_BYTE(kUnPackInfo)
    SZ_WRITE_BYTE(kFolder)
    SZ_WRITE_BYTE(1)    //num folders
    SZ_WRITE_BYTE(0)    //external
    SZ_WRITE_BYTE(1)    //num coders
    mainByte = 0x20 | 0x03;
    SZ_WRITE_BYTE(mainByte)
    SZ_WRITE_BYTE(3)
    SZ_WRITE_BYTE(1)
    SZ_WRITE_BYTE(1)
    Uint64toNumber(sd, propsSize);
    for(i = 0; i < (UInt32)propsSize; i++)
    {
        SZ_WRITE_BYTE(propsEnc[i])
    }
    SZ_WRITE_BYTE(kCodersUnPackSize)
    Uint64toNumber(sd, m_offset);
    SZ_WRITE_BYTE(kEnd)
    SZ_WRITE_BYTE(kSubStreamsInfo)
    SZ_WRITE_BYTE(kNumUnPackStream)
    Uint64toNumber(sd, (UInt64) m_inNames.size());
    SZ_WRITE_BYTE(kSize)
    for( i = 0; i < numFiles - 1; i++)
    {
        Uint64toNumber(sd, m_sizes[i]);
    }
    SZ_WRITE_BYTE(kCRC)
    for(i = 0; i < numFiles; i++)
    {
        *(UInt32*)sd->Data = m_CRCs[i];
        sd->Size -= sizeof(UInt32);
        sd->Data += sizeof(UInt32);
    }
    SZ_WRITE_BYTE(kEnd)
    SZ_WRITE_BYTE(kEnd)
    SZ_WRITE_BYTE(kEnd)
    SZ_WRITE_BYTE(kFilesInfo)                       // kFileInfo
    Uint64toNumber(sd, (UInt64)m_inNames.size());   // nombre de fichiers
    // déterminer la taille de la liste des noms et UTF16 et séparés par des int16 à 0
    j = 0;
    for(i = 0; i < numFiles; i++)
    {
        int l;
        l = (m_inNames[i].size() +1) * 2;
        j += l;
    }
    j++;                                            // taille de tous les noms + 1 (pour external), j a aussi été augmenté pour emplacement de number
    temp64 = j;
    if(temp64 > 127)                                // si cette taille est codée sur 2 octets
        j++;
    if(temp64 > 0x3fff)                                // si cette taille est codée sur 3 octets
        j++;
    j++;                                            // et 1 octet de plus pour kName
    // placer éventuellement un kDummy si nécessaire pour avoir n alignement pair pour les noms
    // on vérifie maintenant si sd.data + j est un nombre pair !
    // si non, allors créer un kDummy pour y parvenir: ceci prendra 1 byte kDummy, 1 byte de Size et un nombre impair de 0
    if((UInt64)(sd->Data + j) & 1)
    {
        SZ_WRITE_BYTE(kDummy)
        SZ_WRITE_BYTE(1)
        SZ_WRITE_BYTE(0)
    }
    SZ_WRITE_BYTE(kName)                            // kNames (1byte)
    CSzDataw sd2 = *sd;
    Uint64toNumber(sd, temp64);
    tempPointer = sd->Data;
    SZ_WRITE_BYTE(0)                                // external
    // liste des noms
    for(i = 0; i < numFiles; i++)                   // tous les noms
    {
        int j = m_inNames[i].size();
        Byte b;
        for(int k = 0; k < j; k++)
        {
            b = m_inNames[i][k].toLatin1();
            SZ_WRITE_BYTE(b)
            SZ_WRITE_BYTE(0)
        }
        SZ_WRITE_BYTE(0)
        SZ_WRITE_BYTE(0)
    }
    if(sd->Data < (tempPointer + temp64))
        sd->Data = tempPointer + temp64;
    else if( sd->Data > (tempPointer + temp64))
    {
        temp64 = sd->Data - sd2.Data;
        Uint64toNumber(&sd2, temp64);
    }
    SZ_WRITE_BYTE(kMTime)
    temp64 = numFiles * 8 + 2;
    Uint64toNumber(sd, temp64);
    SZ_WRITE_BYTE(1)    // alldefined
    SZ_WRITE_BYTE(0)    // external no
    for(i = 0; i < numFiles; i++)
    {
        *(UInt32*)sd->Data = m_mTimes[i].Low;
        sd->Size -= sizeof(UInt32);
        sd->Data += sizeof(UInt32);
        *(UInt32*)sd->Data = m_mTimes[i].High;
        sd->Size -= sizeof(UInt32);
        sd->Data += sizeof(UInt32);
    }
    SZ_WRITE_BYTE(kWinAttributes)
    temp64 = numFiles * 4 + 2;
    Uint64toNumber(sd, temp64);
    SZ_WRITE_BYTE(1)    // alldefined
    SZ_WRITE_BYTE(0)    // external no
    for(i = 0; i < numFiles; i++)
    {
        *(UInt32*)sd->Data = m_attribs[i];
        sd->Size -= sizeof(UInt32);
        sd->Data += sizeof(UInt32);
    }
    SZ_WRITE_BYTE(kEnd)
    SZ_WRITE_BYTE(kEnd)
    SZ_WRITE_BYTE(kEnd)

    initSize -= sd->Size;   // régulariser la taille réelle
    m_crc32.Reset();
    m_crc32.Update(mainHeader, initSize);
    m_crcHeader = m_crc32.GetValue();

    // le coder et l'enregistrer
    Byte *codedHeader;
    codedHeader = (Byte*)malloc(initSize);
    UInt64 codedSize;
    res = LzmaEncode(codedHeader, &codedSize, (const Byte *)mainHeader, (SizeT)initSize,
                     (const CLzmaEncProps*)&enProps, propsEnc, &propsSize, 0,
                     NULL, &zAlloc, &zAlloc);
    if(res != SZ_OK){
        free(codedHeader);
        free(mainHeader);
        return res;
    }
    // enregistrer le header codé et enregistrer sa position de départ, puis celle de fin
    m_offsetMainHeader = m_outputFile.pos() - sizeof(entete7z);
    done = m_outputFile.write((char*)codedHeader, codedSize);
    m_offsetEndHeader = m_outputFile.pos() - sizeof(entete7z);
    if(done != codedSize) return WriteErrorZ;
    m_mainHeaderSize = initSize;       // la taille codée est calculée par la différence des 2 offsets
    // créer et enregistrer le header du header codé
    sd->Size = temp64 = 50;
    mainHeader = (Byte*)malloc(sd->Size);
    sd->Data = mainHeader;
    SZ_WRITE_BYTE(kEncodedHeader)
    SZ_WRITE_BYTE(kPackInfo)
    Uint64toNumber(sd, m_offsetMainHeader);
    SZ_WRITE_BYTE(1)        // num pack streams
    SZ_WRITE_BYTE(kSize)
    Uint64toNumber(sd, m_offsetEndHeader - m_offsetMainHeader);
    SZ_WRITE_BYTE(kEnd)
    SZ_WRITE_BYTE(kUnPackInfo)
    SZ_WRITE_BYTE(kFolder)
    SZ_WRITE_BYTE(1)        // 1 folder
    SZ_WRITE_BYTE(0)        // external 0
    SZ_WRITE_BYTE(1)        // num coders
    mainByte = 0x20 | 3;
    SZ_WRITE_BYTE(mainByte)
    SZ_WRITE_BYTE(3)
    SZ_WRITE_BYTE(1)
    SZ_WRITE_BYTE(1)
    Uint64toNumber(sd, propsSize);
    for(i = 0; i < (UInt32)propsSize; i++)
    {
        SZ_WRITE_BYTE(propsEnc[i])
    }
    SZ_WRITE_BYTE(kCodersUnPackSize)
    Uint64toNumber(sd, m_mainHeaderSize);
    SZ_WRITE_BYTE(kCRC)
    SZ_WRITE_BYTE(1)        // alldefined
    *(UInt32*)sd->Data = m_crcHeader;
    sd->Size -= sizeof(UInt32);
    sd->Data += sizeof(UInt32);
    SZ_WRITE_BYTE(kEnd)
    SZ_WRITE_BYTE(kEnd)
    temp64 -= sd->Size;
    m_outputFile.write((char*)mainHeader, temp64);
    // enregistrer le header de QtMirror
    m_outputFile.write((char*)&ih, sizeof(ih));
    m_finalSize = m_outputFile.pos();
    // mettre à jour l'entete
    m_crc32.Reset();
    m_crc32.Update(mainHeader, temp64);
    zHeader.nextHeaderCRC = m_crc32.GetValue();
    zHeader.nextHeaderOffset = m_offsetEndHeader;
    zHeader.nextHeaderSize = temp64;
    m_crc32.Reset();
    m_crc32.Update((const Byte*)&zHeader.nextHeaderOffset, 20);
    zHeader.startHeaderCRC = m_crc32.GetValue();
    m_outputFile.seek(0);
    m_outputFile.write((char*)&zHeader, sizeof(zHeader));
    m_outputFile.close();
    // libérer les buffers, clore le fichier de sortie, et sortir
    free(mainHeader);
    return res;
}
