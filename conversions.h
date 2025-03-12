#ifndef CONVERSIONS_H
#define CONVERSIONS_H

#include "QtMirror.h"
#include <QString>

class Conversions
{
public:
    Conversions();
    bool convertLine(MirwCmde* pMC);
    void setVolume(const char* volume);
    void setVolume(const QString &volume);
    bool setDrive(char *path);
static bool strcmpm( const char *s1, const char *s2, int max = 0, bool caseSensitive = false);

protected:
    void shiftRight(char *s, int count = 1);
private:
    char m_szVolumeName[20];
    char m_userPath[MAX_PATH];
};

#endif // CONVERSIONS_H
