#include "entetefichier.h"
#include "7z.h"
#include "LzmaDec.h"


static ISzAlloc zAlloc;
static void *SzAlloc(ISzAllocPtr p, size_t size) { p = p; return malloc(size); }
static void SzFree(ISzAllocPtr p, void *address) { p = p; free(address); }

#define MY_ALLOC(T, p, size, alloc) \
{ if ((p = (T *)ISzAlloc_Alloc(alloc, (size) * sizeof(T))) == NULL) return SZ_ERROR_MEM; }

#define MY_ALLOC_ZE(T, p, size, alloc) \
{ if ((size) == 0) p = NULL; else MY_ALLOC(T, p, size, alloc) }

#define MY_ALLOC_AND_CPY(to, size, from, alloc) \
{ MY_ALLOC(Byte, to, size, alloc); memcpy(to, from, size); }

#define MY_ALLOC_ZE_AND_CPY(to, size, from, alloc) \
{ if ((size) == 0) to = NULL; else { MY_ALLOC_AND_CPY(to, size, from, alloc) } }

#define SzBitWithVals_Check(p, i) ((p)->Defs && ((p)->Defs[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)

#define SzBitArray_Check(p, i) (((p)[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)

#define SzBitWithVals_Check(p, i) ((p)->Defs && ((p)->Defs[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)

#define SzBitUi32s_INIT(p) { (p)->Defs = NULL; (p)->Vals = NULL; }

#define SzData_CLEAR(p) { (p)->Data = NULL; (p)->Size = 0; }

#define SZ_READ_BYTE_SD_NOCHECK(_sd_, dest) \
(_sd_)->Size--; dest = *(_sd_)->Data++;

#define SZ_READ_BYTE_SD(_sd_, dest) \
if ((_sd_)->Size == 0) return SZ_ERROR_ARCHIVE; \
    SZ_READ_BYTE_SD_NOCHECK(_sd_, dest)

#define SZ_READ_BYTE(dest) SZ_READ_BYTE_SD(sd, dest)

#define SZ_READ_BYTE_2(dest) \
    if (sd.Size == 0) return SZ_ERROR_ARCHIVE; \
    sd.Size--; dest = *sd.Data++;

#define SKIP_DATA(sd, size) { sd->Size -= (size_t)(size); sd->Data += (size_t)(size); }
#define SKIP_DATA2(sd, size) { sd.Size -= (size_t)(size); sd.Data += (size_t)(size); }

#define SZ_READ_32(dest) if (sd.Size < 4) return SZ_ERROR_ARCHIVE; \
dest = GetUi32(sd.Data); SKIP_DATA2(sd, 4);

static UInt64 NtfsFileTimeToMsQdateTime(const CNtfsFileTime *t)
{
    UInt64 it = t->High;
    it <<= 32;
    it |= t->Low;
    it /= 10000;
    it -= 11644473600000;
    return it;
}

static void MsQdateTimeToNtfsFileTime(CNtfsFileTime *ft, UInt64 qt)
{
    qt += 11644473600000;
    qt *= 10000;
    ft->Low = (UInt32)qt & 0xffffffff;
    qt >>= 32;
    ft->High = (UInt32)qt;
}

static SRes SzBitUi32s_Alloc(CSzBitUi32s *p, size_t num, ISzAllocPtr alloc)
{
    if (num == 0)
    {
        p->Defs = NULL;
        p->Vals = NULL;
    }
    else
    {
        MY_ALLOC(Byte, p->Defs, (num + 7) >> 3, alloc)
        MY_ALLOC(UInt32, p->Vals, num, alloc)
    }
    return SZ_OK;
}

static void SzBitUi32s_Free(CSzBitUi32s *p, ISzAllocPtr alloc)
{
    ISzAlloc_Free(alloc, p->Defs); p->Defs = NULL;
    ISzAlloc_Free(alloc, p->Vals); p->Vals = NULL;
}

#define SzBitUi64s_INIT(p) { (p)->Defs = NULL; (p)->Vals = NULL; }

static void SzBitUi64s_Free(CSzBitUi64s *p, ISzAllocPtr alloc)
{
    ISzAlloc_Free(alloc, p->Defs); p->Defs = NULL;
    ISzAlloc_Free(alloc, p->Vals); p->Vals = NULL;
}


static void SzAr_Init(CSzAr *p)
{
    p->NumPackStreams = 0;
    p->NumFolders = 0;

    p->PackPositions = NULL;
    SzBitUi32s_INIT(&p->FolderCRCs)

        p->FoCodersOffsets = NULL;
    p->FoStartPackStreamIndex = NULL;
    p->FoToCoderUnpackSizes = NULL;
    p->FoToMainUnpackSizeIndex = NULL;
    p->CoderUnpackSizes = NULL;

    p->CodersData = NULL;

    p->RangeLimit = 0;
}

static void SzAr_Free(CSzAr *p, ISzAllocPtr alloc)
{
    ISzAlloc_Free(alloc, p->PackPositions);
    SzBitUi32s_Free(&p->FolderCRCs, alloc);

    ISzAlloc_Free(alloc, p->FoCodersOffsets);
    ISzAlloc_Free(alloc, p->FoStartPackStreamIndex);
    ISzAlloc_Free(alloc, p->FoToCoderUnpackSizes);
    ISzAlloc_Free(alloc, p->FoToMainUnpackSizeIndex);
    ISzAlloc_Free(alloc, p->CoderUnpackSizes);

    ISzAlloc_Free(alloc, p->CodersData);

    SzAr_Init(p);
}


void SzArEx_Init(CSzArEx *p)
{
    SzAr_Init(&p->db);

    p->NumFiles = 0;
    p->dataPos = 0;

    p->UnpackPositions = NULL;
    p->IsDirs = NULL;

    p->FolderToFile = NULL;
    p->FileToFolder = NULL;

    p->FileNameOffsets = NULL;
    p->FileNames = NULL;

    SzBitUi32s_INIT(&p->CRCs)
        SzBitUi32s_INIT(&p->Attribs)
        // SzBitUi32s_INIT(&p->Parents)
        SzBitUi64s_INIT(&p->MTime)
        SzBitUi64s_INIT(&p->CTime)
}

void SzArEx_Free(CSzArEx *p, ISzAllocPtr alloc)
{
    ISzAlloc_Free(alloc, p->UnpackPositions);
    ISzAlloc_Free(alloc, p->IsDirs);

    ISzAlloc_Free(alloc, p->FolderToFile);
    ISzAlloc_Free(alloc, p->FileToFolder);

    ISzAlloc_Free(alloc, p->FileNameOffsets);
    ISzAlloc_Free(alloc, p->FileNames);

    SzBitUi32s_Free(&p->CRCs, alloc);
    SzBitUi32s_Free(&p->Attribs, alloc);
    // SzBitUi32s_Free(&p->Parents, alloc);
    SzBitUi64s_Free(&p->MTime, alloc);
    SzBitUi64s_Free(&p->CTime, alloc);

    SzAr_Free(&p->db, alloc);
    SzArEx_Init(p);
}

static Z7_NO_INLINE SRes ReadNumber(CSzData *sd, UInt64 *value)
{
    Byte firstByte, mask;
    unsigned i;
    UInt32 v;

    SZ_READ_BYTE(firstByte)
    if ((firstByte & 0x80) == 0)
    {
        *value = firstByte;
        return SZ_OK;
    }
    SZ_READ_BYTE(v)
    if ((firstByte & 0x40) == 0)
    {
        *value = (((UInt32)firstByte & 0x3F) << 8) | v;
        return SZ_OK;
    }
    SZ_READ_BYTE(mask)
    *value = v | ((UInt32)mask << 8);
    mask = 0x20;
    for (i = 2; i < 8; i++)
    {
        Byte b;
        if ((firstByte & mask) == 0)
        {
            const UInt64 highPart = (unsigned)firstByte & (unsigned)(mask - 1);
            *value |= (highPart << (8 * i));
            return SZ_OK;
        }
        SZ_READ_BYTE(b)
        *value |= ((UInt64)b << (8 * i));
        mask >>= 1;
    }
    return SZ_OK;
}


static Z7_NO_INLINE SRes SzReadNumber32(CSzData *sd, UInt32 *value)
{
    Byte firstByte;
    UInt64 value64;
    if (sd->Size == 0)
        return SZ_ERROR_ARCHIVE;
    firstByte = *sd->Data;
    if ((firstByte & 0x80) == 0)
    {
        *value = firstByte;
        sd->Data++;
        sd->Size--;
        return SZ_OK;
    }
    RINOK(ReadNumber(sd, &value64))
    if (value64 >= (UInt32)0x80000000 - 1)
        return SZ_ERROR_UNSUPPORTED;
    if (value64 >= ((UInt64)(1) << ((sizeof(size_t) - 1) * 8 + 4)))
        return SZ_ERROR_UNSUPPORTED;
    *value = (UInt32)value64;
    return SZ_OK;
}

#define ReadID(sd, value) ReadNumber(sd, value)

static SRes SkipData(CSzData *sd)
{
    UInt64 size;
    RINOK(ReadNumber(sd, &size))
    if (size > sd->Size)
        return SZ_ERROR_ARCHIVE;
    SKIP_DATA(sd, size)
    return SZ_OK;
}

static SRes WaitId(CSzData *sd, UInt32 id)
{
    for (;;)
    {
        UInt64 type;
        RINOK(ReadID(sd, &type))
        if (type == id)
            return SZ_OK;
        if (type == kEnd)
            return SZ_ERROR_ARCHIVE;
        RINOK(SkipData(sd))
    }
}

static SRes RememberBitVector(CSzData *sd, UInt32 numItems, const Byte **v)
{
    const UInt32 numBytes = (numItems + 7) >> 3;
    if (numBytes > sd->Size)
        return SZ_ERROR_ARCHIVE;
    *v = sd->Data;
    SKIP_DATA(sd, numBytes)
    return SZ_OK;
}

static UInt32 CountDefinedBits(const Byte *bits, UInt32 numItems)
{
    unsigned b = 0;
    unsigned m = 0;
    UInt32 sum = 0;
    for (; numItems != 0; numItems--)
    {
        if (m == 0)
        {
            b = *bits++;
            m = 8;
        }
        m--;
        sum += (UInt32)((b >> m) & 1);
    }
    return sum;
}

static Z7_NO_INLINE SRes ReadBitVector(CSzData *sd, UInt32 numItems, Byte **v, ISzAllocPtr alloc)
{
    Byte allAreDefined;
    Byte *v2;
    const UInt32 numBytes = (numItems + 7) >> 3;
    *v = NULL;
    SZ_READ_BYTE(allAreDefined)
    if (numBytes == 0)
        return SZ_OK;
    if (allAreDefined == 0)
    {
        if (numBytes > sd->Size)
            return SZ_ERROR_ARCHIVE;
        MY_ALLOC_AND_CPY(*v, numBytes, sd->Data, alloc)
        SKIP_DATA(sd, numBytes)
        return SZ_OK;
    }
    MY_ALLOC(Byte, *v, numBytes, alloc)
    v2 = *v;
    memset(v2, 0xFF, (size_t)numBytes);
    {
        const unsigned numBits = (unsigned)numItems & 7;
        if (numBits != 0)
            v2[(size_t)numBytes - 1] = (Byte)((((UInt32)1 << numBits) - 1) << (8 - numBits));
    }
    return SZ_OK;
}

static Z7_NO_INLINE SRes ReadUi32s(CSzData *sd2, UInt32 numItems, CSzBitUi32s *crcs, ISzAllocPtr alloc)
{
    UInt32 i;
    CSzData sd;
    UInt32 *vals;
    const Byte *defs;
    MY_ALLOC_ZE(UInt32, crcs->Vals, numItems, alloc)
    sd = *sd2;
    defs = crcs->Defs;
    vals = crcs->Vals;
    for (i = 0; i < numItems; i++)
        if (defs && SzBitArray_Check(defs, i))
        {
            SZ_READ_32(vals[i])
        }
        else
            vals[i] = 0;
    *sd2 = sd;
    return SZ_OK;
}

static SRes ReadBitUi32s(CSzData *sd, UInt32 numItems, CSzBitUi32s *crcs, ISzAllocPtr alloc)
{
    SzBitUi32s_Free(crcs, alloc);
    RINOK(ReadBitVector(sd, numItems, &crcs->Defs, alloc))
    return ReadUi32s(sd, numItems, crcs, alloc);
}

static SRes SkipBitUi32s(CSzData *sd, UInt32 numItems)
{
    Byte allAreDefined;
    UInt32 numDefined = numItems;
    SZ_READ_BYTE(allAreDefined)
    if (!allAreDefined)
    {
        const size_t numBytes = (numItems + 7) >> 3;
        if (numBytes > sd->Size)
            return SZ_ERROR_ARCHIVE;
        numDefined = CountDefinedBits(sd->Data, numItems);
        SKIP_DATA(sd, numBytes)
    }
    if (numDefined > (sd->Size >> 2))
        return SZ_ERROR_ARCHIVE;
    SKIP_DATA(sd, (size_t)numDefined * 4)
    return SZ_OK;
}

static Z7_NO_INLINE SRes SkipNumbers(CSzData *sd2, UInt32 num)
{
    CSzData sd;
    sd = *sd2;
    for (; num != 0; num--)
    {
        Byte firstByte, mask;
        unsigned i;
        SZ_READ_BYTE_2(firstByte)
        if ((firstByte & 0x80) == 0)
            continue;
        if ((firstByte & 0x40) == 0)
        {
            if (sd.Size == 0)
                return SZ_ERROR_ARCHIVE;
            sd.Size--;
            sd.Data++;
            continue;
        }
        mask = 0x20;
        for (i = 2; i < 8 && (firstByte & mask) != 0; i++)
            mask >>= 1;
        if (i > sd.Size)
            return SZ_ERROR_ARCHIVE;
        SKIP_DATA2(sd, i)
    }
    *sd2 = sd;
    return SZ_OK;
}


const Byte k7zSignature[6] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};

void Buf_Init(CBuf *p)
{
    p->data = 0;
    p->size = 0;
}


static SRes SzBitUi32s_Alloc(CSzBitUi32s *p, size_t num)
{
    if (num == 0)
    {
        p->Defs = NULL;
        p->Vals = NULL;
    }
    else
    {
        p->Defs = (Byte*)malloc((num+7)>>3);
        p->Vals = (UInt32*)malloc(sizeof(UInt32)*num);
    }
    return SZ_OK;
}

static void SzBitUi32s_Free(CSzBitUi32s *p)
{
    if(p->Defs)
    {
        free (p->Defs);
        p->Defs = NULL;
    }
    if(p->Vals)
    {
        free (p->Vals);
        p->Vals = NULL;
    }
}

static void SzAr_Free(CSzAr *p)
{
    if(p->PackPositions) free (p->PackPositions);
    SzBitUi32s_Free(&p->FolderCRCs);
    if(p->FoCodersOffsets) free (p->FoCodersOffsets);
    if(p->FoStartPackStreamIndex) free (p->FoStartPackStreamIndex);
    if(p->FoToCoderUnpackSizes) free (p->FoToCoderUnpackSizes);
    if(p->FoToMainUnpackSizeIndex) free (p->FoToMainUnpackSizeIndex);
    if(p->CoderUnpackSizes) free (p->CoderUnpackSizes);

    if(p->CodersData) free (p->CodersData);

    SzAr_Init(p);
}

static Z7_NO_INLINE SRes ReadTimeZ(CSzBitUi64s *p, UInt32 num,
                                  CSzData *sd2,
                                  const CBuf *tempBufs, UInt32 numTempBufs,
                                  ISzAllocPtr alloc)
{
    CSzData sd;
    UInt32 i;
    CNtfsFileTime *vals;
    Byte *defs;
    Byte external;

    RINOK(ReadBitVector(sd2, num, &p->Defs, alloc))

    SZ_READ_BYTE_SD(sd2, external)
    if (external == 0)
        sd = *sd2;
    else
    {
        UInt32 index;
        RINOK(SzReadNumber32(sd2, &index))
        if (index >= numTempBufs)
            return SZ_ERROR_ARCHIVE;
        sd.Data = tempBufs[index].data;
        sd.Size = tempBufs[index].size;
    }

    MY_ALLOC_ZE(CNtfsFileTime, p->Vals, num, alloc)
    vals = p->Vals;
    defs = p->Defs;
    for (i = 0; i < num; i++)
        if (defs && SzBitArray_Check(defs, i))
        {
            if (sd.Size < 8)
                return SZ_ERROR_ARCHIVE;
            vals[i].Low = GetUi32(sd.Data);
            vals[i].High = GetUi32(sd.Data + 4);
            SKIP_DATA2(sd, 8)
        }
        else
            vals[i].High = vals[i].Low = 0;

    if (external == 0)
        *sd2 = sd;

    return SZ_OK;
}

EnteteFichier::EnteteFichier() {
    char err[] = "Erreur d'adressage";
    m_pcrc32 = new CRC32;
    zAlloc.Alloc = SzAlloc;
    zAlloc.Free = SzFree;
    m_streamDataIn = nullptr;
    m_streamDataOut = nullptr;
    m_headerData = nullptr;
    m_errorString = (char*)malloc(64);
    strcpy_s(m_errorString, 64, err);
    m_coder.Data = NULL;
    m_coder.Size = 0;
    SzArEx_Init(&m_arEx);
}

Byte* EnteteFichier::encodage(int i)
    {
    if( m_coder.Data == NULL || m_coder.Size == 0 || i != 0)
        return (Byte*)m_errorString;
    else
        return (Byte*)m_coder.Data;
    }

void EnteteFichier::setFileName(QString fileName)
{
    m_fileName = fileName;
    if(m_file.isOpen())
        m_file.close();
    m_file.setFileName(fileName);
}

SRes EnteteFichier::openFile()
{
    int i;
    UInt32 firstCRC;
    if (m_fileName.isEmpty()) return ReadErrorZ;
    if(m_file.isOpen()) m_file.close();
        m_file.setFileName(m_fileName);
        m_file.open(QIODevice::ReadOnly);
        m_pFile = &m_file;
        m_file.read((char*)&m_entete, sizeof(m_entete));
        m_startPosAfterHeader = m_file.pos();
        m_pcrc32->Reset();
        m_pcrc32->Update((Byte*)&m_entete.nextHeaderOffset, 20);
        firstCRC = m_pcrc32->GetValue();
        if(firstCRC != m_entete.startHeaderCRC)
        {
            m_file.close();
            return headCRCerrorZ;
        }
        for(i = 0; i < 6; ++i)
        {
            if(k7zSignature[i] != m_entete.signature[i])
            {
                m_file.close();
                return SignatureErrorZ;
            }
        }
        m_file.seek(m_entete.nextHeaderOffset+sizeof(entete7z));
        if(m_headerData != nullptr) free (m_headerData);
        m_headerData = (Byte*)malloc(m_entete.nextHeaderSize);
        m_lngBufHeader = m_entete.nextHeaderSize;
        m_file.read((char*)m_headerData, m_entete.nextHeaderSize);
        m_pcrc32->Reset();
        m_pcrc32->Update(m_headerData,(size_t) m_entete.nextHeaderSize);
        m_CRCverif = m_pcrc32->GetValue();
        if(m_CRCverif != m_entete.nextHeaderCRC)
        {
            m_file.close();
            return headCRCerrorZ;
        }

        return SZ_OK;
}

SRes EnteteFichier::openFile2(QFile *inFile)
{
    int i;
    UInt32 firstCRC;
    m_pFile = inFile;
    m_pFile->seek(0);
    m_pFile->read((char*)&m_entete, sizeof(m_entete));
    m_startPosAfterHeader = m_pFile->pos();
    m_pcrc32->Reset();
    m_pcrc32->Update((Byte*)&m_entete.nextHeaderOffset, 20);
    firstCRC = m_pcrc32->GetValue();
    if(firstCRC != m_entete.startHeaderCRC)
    {
        return headCRCerrorZ;
    }
    for(i = 0; i < 6; ++i)
    {
        if(k7zSignature[i] != m_entete.signature[i])
        {
            return SignatureErrorZ;
        }
    }
    m_pFile->seek(m_entete.nextHeaderOffset+sizeof(entete7z));
    if(m_headerData != nullptr) free (m_headerData);
    m_headerData = (Byte*)malloc(m_entete.nextHeaderSize);
    m_lngBufHeader = m_entete.nextHeaderSize;
    m_pFile->read((char*)m_headerData, m_entete.nextHeaderSize);
    m_pcrc32->Reset();
    m_pcrc32->Update(m_headerData,(size_t) m_entete.nextHeaderSize);
    m_CRCverif = m_pcrc32->GetValue();
    if(m_CRCverif != m_entete.nextHeaderCRC)
    {
        return headCRCerrorZ;
    }
    return SZ_OK;
}

SRes EnteteFichier::decodeStream(UInt32 index)
{
    SRes res = SZ_OK;
    qint64 result;
    SizeT lngIn;
    SizeT lngOut;
    Byte property[16];
    UInt32 propertyCount = 5;
    ELzmaStatus status;
    //UInt32 crcres;
    UInt64 dataStartPos = 0;

    if(index >= m_arEx.db.NumPackStreams) return UnsupportedFunction;

    if(m_arEx.db.CoderUnpackSizes) lngOut = m_arEx.db.CoderUnpackSizes[index];
    else return BadHeaderErrorZ;
    dataStartPos = m_arEx.db.PackPositions[index];
    lngIn = m_arEx.db.PackPositions[index+1] - dataStartPos;
    for(int i = 0; i < 5; i++)
        property[i] = m_properties.Data[i];
    if(m_streamDataIn != nullptr)
    {
        free (m_streamDataIn);
        m_streamDataIn = nullptr;
    }
    m_streamDataIn = (Byte*)malloc(lngIn + 4);
    if(m_streamDataOut != nullptr)
    {
        free (m_streamDataOut);
        m_streamDataOut = nullptr;
    }
    m_streamDataOut = (Byte*)malloc(lngOut + 4);
    memset(m_streamDataOut, 0, lngOut + 3);
    m_pFile->seek(dataStartPos+sizeof(entete7z));
    result = m_pFile->read((char*)m_streamDataIn, lngIn);
    m_streamDataIn[lngIn] = 0;
    m_streamDataIn[lngIn + 1] = 0;
    if((SizeT)result != lngIn)
    {
        free(m_streamDataIn);
        m_streamDataIn = nullptr;
        free(m_streamDataOut);
        m_streamDataOut = nullptr;
        return ReadErrorZ;
    }
    res = LzmaDecode(m_streamDataOut,&lngOut,(const Byte*)m_streamDataIn,
                      &lngIn,property, propertyCount, LZMA_FINISH_ANY, &status, &zAlloc);
    free(m_streamDataIn);
    m_streamDataIn = nullptr;
    return res;
}

void EnteteFichier::menage()
{
    SzAr_Free(&m_arEx.db);
    if(m_streamDataIn)
    {
        free(m_streamDataIn);
        m_streamDataIn = nullptr;
    }
    if(m_streamDataOut)
    {
        free(m_streamDataOut);
        m_streamDataOut = nullptr;
    }
    if(m_headerData)
    {
        free(m_headerData);
        m_headerData = nullptr;
    }
}

SRes EnteteFichier::getHeader()
{
    //Int64 i;
    //UInt64 temp;
    SRes res;
    m_numFolders = 0;
    Byte type = m_headerData[0];
    SzArEx_Free(&m_arEx, &zAlloc);

    if(type == kEncodedHeader)
    {
        CSzAr tempAr;
        CSzData sd;
        qint64 result;
        SRes sres;
        SizeT lngIn;
        SizeT lngOut;
        Byte property[16];
        UInt32 propertyCount = 5;
        ELzmaStatus status;
        UInt32 crcres;
        UInt64 dataStartPos = 0;

        sd.Data = m_headerData;
        sd.Size = m_lngBufHeader;
        sd.Data++;
        sd.Size--;
        m_enteteCode = true;
        SzAr_Init(&tempAr);
        tempAr.RangeLimit = m_entete.nextHeaderOffset;
        res = SzReadStreamsInfo(&tempAr, &sd, 1, NULL, 0, &dataStartPos, &m_ssi, &zAlloc);
        if(res != SZ_OK) return res;
        if(tempAr.CoderUnpackSizes) lngOut = tempAr.CoderUnpackSizes[0];
        else return BadHeaderErrorZ;
        lngIn = tempAr.PackPositions[1];
        for(int i = 0; i < 5; i++)
            property[i] = m_properties.Data[i];
        if(m_streamDataIn != nullptr)
        {
            free (m_streamDataIn);
            m_streamDataIn = nullptr;
        }
        m_streamDataIn = (Byte*)malloc(lngIn + 4);
        if(m_streamDataOut != nullptr)
        {
            free (m_streamDataOut);
            m_streamDataOut = nullptr;
        }
        m_streamDataOut = (Byte*)malloc(lngOut + 4);
        memset(m_streamDataOut, 0, lngOut + 3);
        m_pFile->seek(dataStartPos+sizeof(entete7z));
        result = m_pFile->read((char*)m_streamDataIn, lngIn);
        m_streamDataIn[lngIn] = 0;
        m_streamDataIn[lngIn + 1] = 0;
        if((SizeT)result != lngIn)
        {
            free(m_streamDataIn);
            m_streamDataIn = nullptr;
            free(m_streamDataOut);
            m_streamDataOut = nullptr;
            return ReadErrorZ;
        }
        sres = LzmaDecode(m_streamDataOut,&lngOut,(const Byte*)m_streamDataIn,
                          &lngIn,property, propertyCount, LZMA_FINISH_ANY, &status, &zAlloc);
        free(m_streamDataIn);
        m_streamDataIn = nullptr;
        if(sres)
        {
            free(m_streamDataOut);
            m_streamDataOut = nullptr;
            return DecompErrorZ;
        }
        free(m_headerData);
        m_headerData = m_streamDataOut;
        m_streamDataOut = nullptr;
        m_pcrc32->Reset();
        m_pcrc32->Update(m_headerData,lngOut);
        crcres = m_pcrc32->GetValue();
        if(crcres != tempAr.FolderCRCs.Vals[0])
        {
            return CRCerrorZ;
        }
        SzAr_Free(&tempAr, &zAlloc);
        m_lngBufHeader = lngOut;
    }
    // entete non codé
    {
        UInt32 i;
        UInt32 numTempBufs = 0;

        m_arEx.startPosAfterHeader = m_startPosAfterHeader;
        m_arEx.db.RangeLimit = m_entete.nextHeaderOffset;
        CBuf tempBufs[NUM_ADDITIONAL_STREAMS_MAX];
        for( i = 0; i < NUM_ADDITIONAL_STREAMS_MAX; i++)
        {
            tempBufs[i].data = NULL;
            tempBufs[i].size = 0;
        }
        res = readHeader2(tempBufs, &numTempBufs);

        for( i = 0; i < numTempBufs; i++)
        {
            if(tempBufs[i].data != NULL && tempBufs[i].size != 0)
                free((void*)tempBufs[i].data);
        }

        return res;
    }
}

SRes EnteteFichier::readHeader2(CBuf *tempBufs, UInt32 *numTempBufs)
{
    //Int64 k = 0;
    //Int64 i;
    UInt64 type;
    SRes res = SZ_OK;
    CSzData sd;

    m_ssi.sdSizes.Data = nullptr;
    m_ssi.sdSizes.Size = 0;
    m_ssi.sdCRCs.Data = nullptr;
    m_ssi.sdCRCs.Size = 0;
    m_ssi.sdNumSubStreams.Data = nullptr;
    m_ssi.sdNumSubStreams.Size = 0;
    m_ssi.NumTotalSubStreams = 0;
    m_ssi.NumSubDigests = 0;

    sd.Data = m_headerData;
    sd.Size = m_lngBufHeader;

    // initialiser m_arEx

    RINOK(ReadID(&sd, &type))
    if(type != kHeader)
    {
        return BadHeaderErrorZ;
    }
    RINOK(ReadID(&sd, &type))
    if(type == kArchiveProperties)
    {
        for(;;)
        {
            UInt64 type2;
            RINOK(ReadNumber(&sd, &type2))
            if(type2 == kEnd)
                break;
        }
        RINOK(ReadID(&sd, &type))
    }
    if(type == kAdditionalStreamsInfo)
    {
        // fonction non implémentée pour mon usage
        m_errorCode = UnsupportedFunction;
        return m_errorCode;
    }

    if (type == kMainStreamsInfo)
    {
        RINOK(SzReadStreamsInfo(&m_arEx.db, &sd, (UInt32)1 << 30, tempBufs, *numTempBufs,
                                &m_arEx.dataPos, &m_ssi, &zAlloc))
        m_arEx.dataPos += m_arEx.startPosAfterHeader;
        RINOK(ReadID(&sd, &type))
    }

    if(type == kEnd)
        return SZ_OK;
    if(type != kFilesInfo)
        return BadHeaderErrorZ;
    // A partir d'ici, on cherche les infos des fichiers

    res = ReadFilesInfo(&m_arEx, &sd, tempBufs, numTempBufs, &m_ssi);

    return res;
}


SRes EnteteFichier::SzReadStreamsInfo(CSzAr *p, CSzData *sd, UInt32 numFoldersMax, const CBuf *tempBufs, UInt32 numTempBufs, UInt64 *dataOffset, CSubStreamInfo *ssi, ISzAllocPtr alloc)
{
    UInt64 type;

    SzData_CLEAR(&ssi->sdSizes)
    SzData_CLEAR(&ssi->sdCRCs)
    SzData_CLEAR(&ssi->sdNumSubStreams)

    *dataOffset = 0;
    RINOK(ReadID(sd, &type))
    if (type == kPackInfo)
    {
        RINOK(ReadNumber(sd, dataOffset))
        if (*dataOffset > p->RangeLimit)
            return SZ_ERROR_ARCHIVE;
        RINOK(ReadPackInfo(p, sd, alloc))
        if (p->PackPositions[p->NumPackStreams] > p->RangeLimit - *dataOffset)
            return SZ_ERROR_ARCHIVE;
        RINOK(ReadID(sd, &type))
    }
    if (type == kUnPackInfo)
    {
        RINOK(ReadUnpackInfo(p, sd, numFoldersMax, tempBufs, numTempBufs, alloc))
        RINOK(ReadID(sd, &type))
    }
    if (type == kSubStreamsInfo)
    {
        RINOK(ReadSubStreamsInfo(p, sd, ssi))
        RINOK(ReadID(sd, &type))
    }
    else
    {
        ssi->NumTotalSubStreams = p->NumFolders;
        // ssi->NumSubDigests = 0;
    }

    return (type == kEnd ? SZ_OK : SZ_ERROR_UNSUPPORTED);

}


SRes EnteteFichier::ReadSubStreamsInfo(CSzAr *p, CSzData *sd, CSubStreamInfo *ssi)
{
    UInt64 type = 0;
    UInt32 numSubDigests = 0;
    const UInt32 numFolders = p->NumFolders;
    UInt32 numUnpackStreams = numFolders;
    UInt32 numUnpackSizesInData = 0;

    for (;;)
    {
        RINOK(ReadID(sd, &type))
        if (type == kNumUnPackStream)
        {
            UInt32 i;
            ssi->sdNumSubStreams.Data = sd->Data;
            numUnpackStreams = 0;
            numSubDigests = 0;
            for (i = 0; i < numFolders; i++)
            {
                UInt32 numStreams;
                RINOK(SzReadNumber32(sd, &numStreams))
                if (numUnpackStreams > numUnpackStreams + numStreams)
                    return SZ_ERROR_UNSUPPORTED;
                numUnpackStreams += numStreams;
                if (numStreams != 0)
                    numUnpackSizesInData += (numStreams - 1);
                if (numStreams != 1 || !SzBitWithVals_Check(&p->FolderCRCs, i))
                    numSubDigests += numStreams;
            }
            ssi->sdNumSubStreams.Size = (size_t)(sd->Data - ssi->sdNumSubStreams.Data);
            continue;
        }
        if (type == kCRC || type == kSize || type == kEnd)
            break;
        RINOK(SkipData(sd))
    }

    if (!ssi->sdNumSubStreams.Data)
    {
        numSubDigests = numFolders;
        if (p->FolderCRCs.Defs)
            numSubDigests = numFolders - CountDefinedBits(p->FolderCRCs.Defs, numFolders);
    }

    ssi->NumTotalSubStreams = numUnpackStreams;
    ssi->NumSubDigests = numSubDigests;

    if (type == kSize)
    {
        ssi->sdSizes.Data = sd->Data;
        RINOK(SkipNumbers(sd, numUnpackSizesInData))
        ssi->sdSizes.Size = (size_t)(sd->Data - ssi->sdSizes.Data);
        m_unPackedSizes = ssi->sdSizes;
        RINOK(ReadID(sd, &type))
    }

    for (;;)
    {
        if (type == kEnd)
            return SZ_OK;
        if (type == kCRC)
        {
            ssi->sdCRCs.Data = sd->Data;
            RINOK(SkipBitUi32s(sd, numSubDigests))
            ssi->sdCRCs.Size = (size_t)(sd->Data - ssi->sdCRCs.Data);
        }
        else
        {
            RINOK(SkipData(sd))
        }
        RINOK(ReadID(sd, &type))
    }
}


SRes EnteteFichier::ReadPackInfo(CSzAr *p, CSzData *sd, ISzAllocPtr alloc)
{
    RINOK(SzReadNumber32(sd, &p->NumPackStreams))

    RINOK(WaitId(sd, kSize))
    MY_ALLOC(UInt64, p->PackPositions, (size_t)p->NumPackStreams + 1, alloc)
    {
        UInt64 sum = 0;
        UInt32 i;
        const UInt32 numPackStreams = p->NumPackStreams;
        for (i = 0; i < numPackStreams; i++)
        {
            UInt64 packSize;
            p->PackPositions[i] = sum;
            RINOK(ReadNumber(sd, &packSize))
            sum += packSize;
            if (sum < packSize)
                return SZ_ERROR_ARCHIVE;
        }
        p->PackPositions[i] = sum;
    }

    for (;;)
    {
        UInt64 type;
        RINOK(ReadID(sd, &type))
        if (type == kEnd)
            return SZ_OK;
        if (type == kCRC)
        {
            /* CRC of packed streams is unused now */
            RINOK(SkipBitUi32s(sd, p->NumPackStreams))
            continue;
        }
        RINOK(SkipData(sd))
    }
}


SRes EnteteFichier::ReadUnpackInfo(CSzAr *p, CSzData *sd2, UInt32 numFoldersMax, const CBuf *tempBufs, UInt32 numTempBufs, ISzAllocPtr alloc)
{
    CSzData sd;

    UInt32 fo, numFolders, numCodersOutStreams, packStreamIndex;
    const Byte *startBufPtr;
    Byte external;

    RINOK(WaitId(sd2, kFolder))

    RINOK(SzReadNumber32(sd2, &numFolders))
    if (numFolders > numFoldersMax)
        return SZ_ERROR_UNSUPPORTED;
    p->NumFolders = numFolders;

    SZ_READ_BYTE_SD(sd2, external)
    if (external == 0)
        sd = *sd2;
    else
    {
        UInt32 index;
        RINOK(SzReadNumber32(sd2, &index))
        if (index >= numTempBufs)
            return SZ_ERROR_ARCHIVE;
        sd.Data = tempBufs[index].data;
        sd.Size = tempBufs[index].size;
    }

    MY_ALLOC(size_t, p->FoCodersOffsets, (size_t)numFolders + 1, alloc)
    MY_ALLOC(UInt32, p->FoStartPackStreamIndex, (size_t)numFolders + 1, alloc)
    MY_ALLOC(UInt32, p->FoToCoderUnpackSizes, (size_t)numFolders + 1, alloc)
    MY_ALLOC_ZE(Byte, p->FoToMainUnpackSizeIndex, (size_t)numFolders, alloc)

    startBufPtr = sd.Data;

    packStreamIndex = 0;
    numCodersOutStreams = 0;

    for (fo = 0; fo < numFolders; fo++)
    {
        UInt32 numCoders, ci, numInStreams = 0;

        p->FoCodersOffsets[fo] = (size_t)(sd.Data - startBufPtr);

        RINOK(SzReadNumber32(&sd, &numCoders))
        if (numCoders == 0 || numCoders > k_Scan_NumCoders_MAX)
            return BadHeaderErrorZ;

        for (ci = 0; ci < numCoders; ci++)
        {
            Byte mainByte;
            unsigned idSize;
            UInt32 coderInStreams;

            SZ_READ_BYTE_2(mainByte)
            if ((mainByte & 0xC0) != 0)
                return SZ_ERROR_UNSUPPORTED;
            idSize = (mainByte & 0xF);
            if (idSize > 8)
                return SZ_ERROR_UNSUPPORTED;
            if (idSize > sd.Size)
                return SZ_ERROR_ARCHIVE;
            m_coder.Data = sd.Data;
            m_coder.Size = idSize;
            SKIP_DATA2(sd, idSize)

            coderInStreams = 1;

            if ((mainByte & 0x10) != 0)
            {
                UInt32 coderOutStreams;
                RINOK(SzReadNumber32(&sd, &coderInStreams))
                RINOK(SzReadNumber32(&sd, &coderOutStreams))
                if (coderInStreams > k_Scan_NumCodersStreams_in_Folder_MAX || coderOutStreams != 1)
                    return SZ_ERROR_UNSUPPORTED;
            }

            numInStreams += coderInStreams;

            if ((mainByte & 0x20) != 0)
            {
                UInt32 propsSize;
                RINOK(SzReadNumber32(&sd, &propsSize))
                if (propsSize > sd.Size)
                    return SZ_ERROR_ARCHIVE;
                m_properties = sd;
                SKIP_DATA2(sd, propsSize)
            }
        }

        {
            UInt32 indexOfMainStream = 0;
            UInt32 numPackStreams = 1;

            if (numCoders != 1 || numInStreams != 1)
            {
                Byte streamUsed[k_Scan_NumCodersStreams_in_Folder_MAX];
                Byte coderUsed[k_Scan_NumCoders_MAX];

                UInt32 i;
                const UInt32 numBonds = numCoders - 1;
                if (numInStreams < numBonds)
                    return SZ_ERROR_ARCHIVE;

                if (numInStreams > k_Scan_NumCodersStreams_in_Folder_MAX)
                    return SZ_ERROR_UNSUPPORTED;

                for (i = 0; i < numInStreams; i++)
                    streamUsed[i] = False;
                for (i = 0; i < numCoders; i++)
                    coderUsed[i] = False;

                for (i = 0; i < numBonds; i++)
                {
                    UInt32 index;

                    RINOK(SzReadNumber32(&sd, &index))
                    if (index >= numInStreams || streamUsed[index])
                        return SZ_ERROR_ARCHIVE;
                    streamUsed[index] = True;

                    RINOK(SzReadNumber32(&sd, &index))
                    if (index >= numCoders || coderUsed[index])
                        return SZ_ERROR_ARCHIVE;
                    coderUsed[index] = True;
                }

                numPackStreams = numInStreams - numBonds;

                if (numPackStreams != 1)
                    for (i = 0; i < numPackStreams; i++)
                    {
                        UInt32 index;
                        RINOK(SzReadNumber32(&sd, &index))
                        if (index >= numInStreams || streamUsed[index])
                            return SZ_ERROR_ARCHIVE;
                        streamUsed[index] = True;
                    }

                for (i = 0; i < numCoders; i++)
                    if (!coderUsed[i])
                    {
                        indexOfMainStream = i;
                        break;
                    }

                if (i == numCoders)
                    return SZ_ERROR_ARCHIVE;
            }

            p->FoStartPackStreamIndex[fo] = packStreamIndex;
            p->FoToCoderUnpackSizes[fo] = numCodersOutStreams;
            p->FoToMainUnpackSizeIndex[fo] = (Byte)indexOfMainStream;
            numCodersOutStreams += numCoders;
            if (numCodersOutStreams < numCoders)
                return SZ_ERROR_UNSUPPORTED;
            if (numPackStreams > p->NumPackStreams - packStreamIndex)
                return SZ_ERROR_ARCHIVE;
            packStreamIndex += numPackStreams;
        }
    }

    p->FoToCoderUnpackSizes[fo] = numCodersOutStreams;

    {
        const size_t dataSize = (size_t)(sd.Data - startBufPtr);
        p->FoStartPackStreamIndex[fo] = packStreamIndex;
        p->FoCodersOffsets[fo] = dataSize;
        MY_ALLOC_ZE_AND_CPY(p->CodersData, dataSize, startBufPtr, alloc)
    }

    if (external != 0)
    {
        if (sd.Size != 0)
            return SZ_ERROR_ARCHIVE;
        sd = *sd2;
    }

    RINOK(WaitId(&sd, kCodersUnPackSize))

    MY_ALLOC_ZE(UInt64, p->CoderUnpackSizes, (size_t)numCodersOutStreams, alloc)
    {
        UInt32 i;
        for (i = 0; i < numCodersOutStreams; i++)
        {
            RINOK(ReadNumber(&sd, p->CoderUnpackSizes + i))
        }
    }

    for (;;)
    {
        UInt64 type;
        RINOK(ReadID(&sd, &type))
        if (type == kEnd)
        {
            *sd2 = sd;
            return SZ_OK;
        }
        if (type == kCRC)
        {
            RINOK(ReadBitUi32s(&sd, numFolders, &p->FolderCRCs, alloc))
            continue;
        }
        RINOK(SkipData(&sd))
    }

}


SRes EnteteFichier::ReadFilesInfo(CSzArEx *p, CSzData *sd, CBuf *tempBufs, UInt32 *numTempBufs, CSubStreamInfo *ssi)
{
//    m_packSizes.clear();
    SRes res = SZ_OK;
//    Int64 l;
//    fileinfoZ file;
    UInt32 numFiles = 0;
    UInt32 numEmptyStreams = 0;
    const Byte *emptyStreams = NULL;
    const Byte *emptyFiles = NULL;
//    memset(&file, 0, sizeof(file));
//    m_fileInfo.clear();

    RINOK(SzReadNumber32(sd, &numFiles))
    p->NumFiles = numFiles;

    for(;;)
    {
        UInt64 type;
        UInt64 size;
        RINOK(ReadID(sd,&type))
        if(type == kEnd)
            break;
        RINOK(ReadNumber(sd, &size))
        if(size > sd->Size)
        {
            return BadHeaderErrorZ;
        }
        if(type >= ((UInt32)1 << 8))
        {
            SKIP_DATA(sd, size)
        }
        else switch((unsigned)type)
        {
            case kName:
            {
                size_t namesSize;
                const Byte *namesData;
                Byte external;

                SZ_READ_BYTE(external)
                if (external == 0)
                {
                    namesSize = (size_t)size - 1;
                    namesData = sd->Data;
                }
                else
                {
                    UInt32 index;
                    RINOK(SzReadNumber32(sd, &index))
                    if (index >= *numTempBufs)
                        return SZ_ERROR_ARCHIVE;
                    namesData = (tempBufs)[index].data;
                    namesSize = (tempBufs)[index].size;
                }

                if ((namesSize & 1) != 0)
                    return BadHeaderErrorZ;
                MY_ALLOC(size_t, p->FileNameOffsets, numFiles + 1, &zAlloc)
                MY_ALLOC_ZE_AND_CPY(p->FileNames, namesSize, namesData, &zAlloc)
                RINOK(readFileNames(p->FileNames, namesSize, numFiles, p->FileNameOffsets))
                if (external == 0)
                {
                    SKIP_DATA(sd, namesSize)
                }
                break;
            }
            case kEmptyStream:
            {
                RINOK(RememberBitVector(sd, numFiles, &emptyStreams))
                numEmptyStreams = CountDefinedBits(emptyStreams, numFiles);
                emptyFiles = NULL;
                break;
            }
            case kEmptyFile:
            {
                RINOK(RememberBitVector(sd, numEmptyStreams, &emptyFiles))
                break;
            }
            case kWinAttributes:
            {
                Byte external;
                CSzData sdSwitch;
                CSzData *sdPtr;
                SzBitUi32s_Free(&p->Attribs, &zAlloc);
                RINOK(ReadBitVector(sd, numFiles, &p->Attribs.Defs, &zAlloc))

                SZ_READ_BYTE(external)
                if (external == 0)
                    sdPtr = sd;
                else
                {
                    UInt32 index;
                    RINOK(SzReadNumber32(sd, &index))
                    if (index >= *numTempBufs)
                        return BadHeaderErrorZ;
                    sdSwitch.Data = (tempBufs)[index].data;
                    sdSwitch.Size = (tempBufs)[index].size;
                    sdPtr = &sdSwitch;
                }
                RINOK(ReadUi32s(sdPtr, numFiles, &p->Attribs, &zAlloc))
                break;
            }
            case kMTime:
            {
                RINOK(ReadTimeZ(&p->MTime, numFiles, sd, tempBufs, *numTempBufs, &zAlloc))
                break;
            }
            case kCTime:
            {
                RINOK(ReadTimeZ( &p->CTime, numFiles, sd, tempBufs, *numTempBufs, &zAlloc))
                break;
            }
            default:
            {
                SKIP_DATA(sd, size)
            }
        }
    }
    if(numFiles - numEmptyStreams != ssi->NumTotalSubStreams)
    {
        return BadHeaderErrorZ;
    }
    for(;;)
    {
        UInt64 type;
        RINOK(ReadID(sd, &type))
        if(type == kEnd)
            break;
        RINOK(SkipData(sd))
    }

    fileinfoZ fiz;
    m_fileInfo.clear();
    m_uSizePairs.clear();

    {
        UInt32 i;
        UInt32 emptyFileIndex = 0;
        UInt32 folderIndex = 0;
        UInt32 remSubStreams = 0;
        UInt32 numSubStreams = 0;
        UInt64 unpackPos = 0;
        const Byte *digestsDefs = NULL;
        const Byte *digestsVals = NULL;
        UInt32 digestsIndex = 0;
        Byte isDirMask = 0;
        Byte crcMask = 0;
        Byte mask = 0x80;

        MY_ALLOC(UInt32, p->FolderToFile, p->db.NumFolders + 1, &zAlloc)
        MY_ALLOC_ZE(UInt32, p->FileToFolder, p->NumFiles, &zAlloc)
        MY_ALLOC(UInt64, p->UnpackPositions, p->NumFiles + 1, &zAlloc)
        MY_ALLOC_ZE(Byte, p->IsDirs, (p->NumFiles + 7) >> 3, &zAlloc)

        RINOK(SzBitUi32s_Alloc(&p->CRCs, p->NumFiles, &zAlloc))

        if(ssi->sdCRCs.Size != 0)
        {
            Byte allDigestsDefined = 0;
            SZ_READ_BYTE_SD_NOCHECK(&ssi->sdCRCs, allDigestsDefined)
            if (allDigestsDefined)
                digestsVals = ssi->sdCRCs.Data;
            else
            {
                const size_t numBytes = (ssi->NumSubDigests + 7) >> 3;
                digestsDefs = ssi->sdCRCs.Data;
                digestsVals = digestsDefs + numBytes;
            }
        }


        for (i = 0; i < numFiles; i++, mask >>= 1)
        {
            memset((Byte*)&fiz, 0, sizeof(fiz));
            if (mask == 0)
            {
                const UInt32 byteIndex = (i - 1) >> 3;
                p->IsDirs[byteIndex] = isDirMask;
                p->CRCs.Defs[byteIndex] = crcMask;
                isDirMask = 0;
                crcMask = 0;
                mask = 0x80;
            }

            p->UnpackPositions[i] = unpackPos;
            fiz.dataOffset = unpackPos;
            p->CRCs.Vals[i] = 0;

            if (emptyStreams && SzBitArray_Check(emptyStreams, i))
            {
                fiz.isDir = true;
                if (emptyFiles)
                {
                    if (!SzBitArray_Check(emptyFiles, emptyFileIndex))
                        isDirMask |= mask;
                    emptyFileIndex++;
                }
                else
                    isDirMask |= mask;
                if (remSubStreams == 0)
                {
                    //p->FileToFolder[i] = (UInt32)-1;
                    fiz.fileToFolder = p->FileToFolder[i] = (UInt32)-1;
                    {
                        m_fileInfo.append(fiz);
                        continue;
                    }
                }
            }

            if (remSubStreams == 0)
            {
                for (;;)
                {
                    if (folderIndex >= p->db.NumFolders)
                        return BadHeaderErrorZ;
                    p->FolderToFile[folderIndex] = i;
                    fiz.folderToFile = i;
                    numSubStreams = 1;
                    if (ssi->sdNumSubStreams.Data)
                    {
                        RINOK(SzReadNumber32(&ssi->sdNumSubStreams, &numSubStreams))
                    }
                    remSubStreams = numSubStreams;
                    if (numSubStreams != 0)
                        break;
                    {
                        const UInt64 folderUnpackSize = SzAr_GetFolderUnpackSize(folderIndex);
                        unpackPos += folderUnpackSize;
                        if (unpackPos < folderUnpackSize)
                            return BadHeaderErrorZ;
                    }
                    folderIndex++;
                }
            }

            p->FileToFolder[i] = folderIndex;
            fiz.folderIndex = folderIndex;

            if (emptyStreams && SzBitArray_Check(emptyStreams, i))
            {
                m_fileInfo.append(fiz);
                continue;
            }
            if (--remSubStreams == 0)
            {
                const UInt64 folderUnpackSize = SzAr_GetFolderUnpackSize(folderIndex);
                const UInt64 startFolderUnpackPos = p->UnpackPositions[p->FolderToFile[folderIndex]];
                if (folderUnpackSize < unpackPos - startFolderUnpackPos)
                {
                    return BadHeaderErrorZ;
                }
                UnPackSizePairs usp;
                usp.FolderIndex = folderIndex;
                usp.UnPackSizeS = folderUnpackSize;
                unpackPos = startFolderUnpackPos + folderUnpackSize;
                m_uSizePairs.append(usp);
                if (unpackPos < folderUnpackSize)
                {
                    return BadHeaderErrorZ;
                }

                if (numSubStreams == 1 && SzBitWithVals_Check(&p->db.FolderCRCs, folderIndex))
                {
                    fiz.CRC = p->CRCs.Vals[i] = p->db.FolderCRCs.Vals[folderIndex];
                    crcMask |= mask;
                }
                folderIndex++;
            }
            else
            {
                UInt64 v;
                RINOK(ReadNumber(&ssi->sdSizes, &v))
                unpackPos += v;
                if (unpackPos < v)
                {
                    return BadHeaderErrorZ;
                }
            }
            if ((crcMask & mask) == 0 && digestsVals)
            {
                if (!digestsDefs || SzBitArray_Check(digestsDefs, digestsIndex))
                {
                    fiz.CRC = p->CRCs.Vals[i] = GetUi32(digestsVals);
                    digestsVals += 4;
                    crcMask |= mask;
                }
                digestsIndex++;
            }
            if(p->MTime.Vals)
            {
                fiz.mTime = p->MTime.Vals[i];
                fiz.mQTime = NtfsFileTimeToMsQdateTime(&fiz.mTime);
            }
            m_fileInfo.append(fiz);
        }
        p->UnpackPositions[i] = unpackPos;


    }
    // remplir un dernier fileinfo fictif avec la position de fin
    fiz.dataOffset = p->db.CoderUnpackSizes[0];
//    fiz.dataOffset = m_unPackSizes[0].UnPackSizeS;
    m_fileInfo.append(fiz);
    // Reste à remplir les FilesInfo
    //CSzData sd2 = m_unPackedSizes;
    {
//        UInt64 lastOffset = 0;
        /*
        UInt64 totalUnpackSize = 0;
        for(UInt32 i = 0; i < p->db.NumPackStreams; i++)
        {
            totalUnpackSize += p->db.CoderUnpackSizes[i];
        }
*/
        for(UInt32 i = 0; i < numFiles; i++)
        {
//            UInt64 size = 0;
            size_t ind;
            short ct;
            UInt32 j = 0;
            UInt32 attributes;
            attributes = SzBitWithVals_Check(&p->Attribs, i) ? p->Attribs.Vals[i] : 0;
            //attributes = p->Attribs.Vals[i];
            // convertir les attributs donnés en attributs pour Qt
            m_fileInfo[i].attributes = attributes;
            Byte *pNames = p->FileNames;
            ind = p->FileNameOffsets[i] * 2;
            do
            {
                ct = (short)pNames[ind + j];
                m_fileInfo[i].name[j/2] = ct;
                j += 2;
            } while(ct != 0);

            m_fileInfo[i].size = SzArEx_GetFileSize(p, i);
//            size = m_fileInfo[i].size = SzArEx_GetFileSize(p, i);
//            lastOffset += size;

            // if(sd2.Size && !m_fileInfo[i].isDir)
            // {
            //     ReadNumber(&sd2, &size);
            //     m_fileInfo[i].size = size;
            //     lastOffset += size;
            // }
        }
    }
    return res;
}


SRes EnteteFichier::readFileNames(const Byte *data, size_t size, UInt32 numFiles, size_t *offsets)
{
    size_t pos = 0;
    *offsets++ = 0;
    if (numFiles == 0)
        return (size == 0) ? SZ_OK : SZ_ERROR_ARCHIVE;
    if (size < 2)
        return SZ_ERROR_ARCHIVE;
    if (data[size - 2] != 0 || data[size - 1] != 0)
        return SZ_ERROR_ARCHIVE;
    do
    {
        const Byte *p;
        if (pos == size)
            return SZ_ERROR_ARCHIVE;
        for (p = data + pos;
#ifdef _WIN32
             *(const UInt16 *)(const void *)p != 0
#else
             p[0] != 0 || p[1] != 0
#endif
             ; p += 2);
        pos = (size_t)(p - data) + 2;
        *offsets++ = (pos >> 1);
    }
    while (--numFiles);
    return (pos == size) ? SZ_OK : SZ_ERROR_ARCHIVE;

}

