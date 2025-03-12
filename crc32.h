#ifndef CRC32_H
#define CRC32_H
#include "defs.h"
#include <cstddef>
//#include <cstddef>

typedef union {
    UInt32 Value;          	// Valeur sur 32 Bit
    struct {
        UInt8  Tail;        	//  8 bits Entrée du registre
        UInt16 Remainder;   	// 16 bits Reste de la division
        UInt8  Head;        	//  8 bits Sortie du registre
    } Part;
}TCRC_Part;

struct CRC32_s
{
    void generate_table(UInt32(&table)[256])
    {
        UInt32 polynomial = 0xEDB88320;
        for (UInt32 i = 0; i < 256; i++)
        {
            UInt32 c = i;
            for (size_t j = 0; j < 8; j++)
            {
                if (c & 1) {
                    c = polynomial ^ (c >> 1);
                }
                else {
                    c >>= 1;
                }
            }
            table[i] = c;
        }
    }

    UInt32 update(UInt32(&table)[256], UInt32 initial, const void* buf, size_t len)
    {
        UInt32 c = initial ^ 0xFFFFFFFF;
        const UInt8* u = static_cast<const UInt8*>(buf);
        for (size_t i = 0; i < len; ++i)
        {
            c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
        }
        return c ^ 0xFFFFFFFF;
    }
};

class CRC32
{
private:
    UInt32 table[256];
    CRC32_s crc32_s;
    UInt32 initial;
public:
    CRC32()
        : initial(0)
    {
        crc32_s.generate_table(table);
    }

    void Reset()
    {
        initial = 0;
        crc32_s.generate_table(table);
    }

    void Update(const Byte * buf, size_t len)
    {
        initial = crc32_s.update(table, initial, (const void *)buf, len);
    }

    UInt32 GetValue() const
    {
        return initial;
    }
};

#endif // CRC32_H
