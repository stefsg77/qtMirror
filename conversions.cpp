#include <QDir>
#include "conversions.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

Conversions::Conversions()
{
    QString home = QDir::homePath();
    strcpy_s(m_userPath, home.toUtf8());
}

bool Conversions::convertLine(MirwCmde *pMC)
{
    // convertit la ligne pour assurer compatibilité
    // 1: remplacer contre-slash par slash
    int i;
    int l;
    bool error = false;
    char szBuffer[MAX_PATH];
    l = strlen(pMC->szOrgPath);
    for( i = 0 ; i < l; ++i)
    {
        if( pMC->szOrgPath[i] == '\\' ) pMC->szOrgPath[i] = '/';
    }
    l = strlen(pMC->szDestPath);
    for( i = 0 ; i < l; ++i)
    {
        if( pMC->szDestPath[i] == '\\' ) pMC->szDestPath[i] = '/';
    }
    l = strlen(pMC->szCommPath);
    for( i = 0 ; i < l; ++i)
    {
        if( pMC->szCommPath[i] == '\\' ) pMC->szCommPath[i] = '/';
    }
    // 2: vérifier que pour windows les chemins commencent par unité:
#ifdef Q_OS_WIN
    if( pMC->szOrgPath[0] == '/')
    {
        // c'est une ligne MAC, voir si commence par /Users ou par /Volumes
        // si commence par "/Users/UTILISATEUR/" remplacer par QDir::homePath()
        if( strcmpm(pMC->szOrgPath,"/Users/", 7, true))
        {
            strcpy_s(szBuffer, pMC->szOrgPath);
            strcpy_s(pMC->szOrgPath, QDir::homePath().toUtf8());
            // trouver le 3eme slash (apres le nom d'utilisateur)
            i = l = 0;
            while(szBuffer[l] && i < 3){
                if( szBuffer[l] == '/' ) ++i;
                l++;
            }
            if( i == 3 && l > 0) strcat_s( pMC->szOrgPath, &szBuffer[l-1]);
        }
        // si commence par "/Volumes/VOLUME/" remplacer par "DRIVE:/"
        if( strcmpm(pMC->szOrgPath,"/Volumes/", 9, true))
        {
            strcpy_s(szBuffer, pMC->szOrgPath);
            l = 9;
            i = 0;
            while( szBuffer[l] && szBuffer[l] != '/' )
            {
                m_szVolumeName[i++] = szBuffer[l++];
            };
            strcpy_s(pMC->szOrgPath, "v:");
            strcat_s(pMC->szOrgPath, &szBuffer[l]);
        }
    }
    // remplacer le v: par le lecteur approprié
    pMC->useVolume = 0;
    if( strcmpm(pMC->szOrgPath, "v:", 2))
    {
        pMC->useVolume = 1;
        if(!setDrive(pMC->szOrgPath)) error = true;
    }
    if( pMC->szDestPath[0] == '/')
    {
        // c'est une ligne MAC, voir si commence par /Users ou par /Volumes
        // si commence par "/Users/UTILISATEUR/" remplacer par QDir::homePath()
        if( strcmpm(pMC->szDestPath,"/Users/", 7, true))
        {
            strcpy_s(szBuffer, pMC->szDestPath);
            strcpy_s(pMC->szDestPath, QDir::homePath().toUtf8());
            // trouver le 3eme slash (apres le nom d'utilisateur)
            i = l = 0;
            while(szBuffer[l] && i < 3){
                if( szBuffer[l] == '/' ) ++i;
                l++;
            }
            if( i == 3 && l > 0) strcat_s( pMC->szDestPath, &szBuffer[l-1]);
        }
        // si commence par "/Volumes/VOLUME/" remplacer par "DRIVE:/"
        if( strcmpm(pMC->szDestPath,"/Volumes/", 9, true))
        {
            strcpy_s(szBuffer, pMC->szDestPath);
            l = 9;
            i = 0;
            while( szBuffer[l] && szBuffer[l] != '/' )
            {
                m_szVolumeName[i++] = szBuffer[l++];
            };
            strcpy_s(pMC->szDestPath, "v:");
            strcat_s(pMC->szDestPath, &szBuffer[l]);
        }
    }
    // remplacer le v: par le lecteur approprié
    if( strcmpm(pMC->szDestPath, "v:", 2))
    {
        pMC->useVolume |= 2;
        if(!setDrive(pMC->szDestPath)) error = true;
    }
#endif
    // et pour MAC, si commence par unité:
#ifdef Q_OS_MAC
    if( pMC->szOrgPath[1] == ':' )
    {
        if(pMC->szOrgPath[0] == 'v' || pMC->szOrgPath[0] == 'V')
        {
            //cas particulier pout 'v', remplacer "v:" par "/Volumes/VOLUME"
            strcpy_s(szBuffer, pMC->szOrgPath);
            strcpy_s(pMC->szOrgPath,"/Volumes/");
            strcat_s(pMC->szOrgPath, m_szVolumeName);
            strcat_s(pMC->szOrgPath, &szBuffer[2]);
        }
        else
        {
            // pour les autres lettre, remplacer "c:" par /c"
            pMC->szOrgPath[1] = pMC->szOrgPath[0];
            pMC->szOrgPath[0] = '/';
        }
    }
    if( pMC->szDestPath[1] == ':' )
    {
        if(pMC->szDestPath[0] == 'v' || pMC->szDestPath[0] == 'V')
        {
            //cas particulier pout 'v', remplacer "v:" par "/Volumes/VOLUME"
            strcpy_s(szBuffer, pMC->szDestPath);
            strcpy_s(pMC->szDestPath,"/Volumes/");
            strcat_s(pMC->szDestPath, m_szVolumeName);
            strcat_s(pMC->szDestPath, &szBuffer[2]);
        }
        else
        {
            // pour les autres lettre, remplacer "c:" par /c"
            pMC->szDestPath[1] = pMC->szDestPath[0];
            pMC->szDestPath[0] = '/';
        }
    }
#endif
    return error;
}

bool Conversions::setDrive(char *path)
{
#ifdef Q_OS_WIN
    DWORD dwMask;
    DWORD dwListDrives;
    char c;
    char RootPathName[4];
    char VolumeName[MAX_PATH];
    char FileSysName[20];
    DWORD dwVSN, dwMCL, dwFSF, dwFSN;

    dwListDrives = GetLogicalDrives();
    if (dwListDrives == 0) return false;
    // tester les disques logiques (en passant a et b)
    for( c = 2; c < 26; c++)
    {
        dwMask = 1 << c;
        if( dwMask & dwListDrives)
        {
            // si le lecteur existe, tester le nom
            strcpy(RootPathName, "A:\\");
            RootPathName[0] += c;
            dwFSN = 20;
            GetVolumeInformationA( RootPathName, VolumeName, MAX_PATH, &dwVSN,
                &dwMCL, &dwFSF, FileSysName, dwFSN);
            if( strcmpm((const char*)VolumeName, (const char*)m_szVolumeName, strlen(m_szVolumeName)))
            {
                // concordance trouvée
                path[0] = 'A'+c;
                return true;
            }
        }
    }
#endif
    return false;
}

void Conversions::setVolume(const char *volume)
{
    strcpy_s(m_szVolumeName, volume);
}

void Conversions::setVolume(const QString &volume)
{
    strcpy_s(m_szVolumeName, volume.toUtf8().data());
}

bool Conversions::strcmpm(const char *s1, const char *s2, int max, bool caseSensitive)
{
    char c1, c2;
    if (!max)
    {
        max = (int)strlen(s1);
        if(max > (int)strlen(s2))
            max = (int)strlen(s2);
    }
    while(max && *s1 && *s2) {
        if( caseSensitive )
        {
            c1 = *s1;
            c2 = *s2;
        }
        else
        {
            c1 = toupper(*s1);
            c2 = toupper(*s2);
        }
        if( c1 == '-' || c1 == '_' ) c1 = ' ';
        if( c2 == '-' || c2 == '_' ) c2 = ' ';
        if(c1 != c2) return false;
        s1++;
        s2++;
        --max;
    }
    if( max ) return false;
    return true;
}

void Conversions::shiftRight(char *s, int count)
{
    if(count < 1) return;
    int l = (int)strlen(s);
    if(count >= l) return;
    l--;
    do{
        s[l+count] = s[l];
        l--;
    }while(l >=0);
}
