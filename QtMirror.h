#ifndef QTMIRROR_H
#define QTMIRROR_H

// Les drapeaux
#define QUIETMODE       1
#define NODELETE        2
#define NODATECOMP		4
#define SILENT			8
#define AUTO			16
#define IGNORESEC		32
#define DSTCOMP			64
#define SIMULATE		128
#define INCREMENT		256
#define OVERWRITE		512
#define TOORGPATH		1024
#define ASKTOCOPY		2048
#define NOCOMMDIR       4096
#define STUDY           8192
#define DEFAULTOPTIONS  QUIETMODE+NODELETE+IGNORESEC+DSTCOMP

// Les définitions des actions des lignes de commandes
#define REFLECT         11
#define SYNCHRONE		12
#define RESTORE         13
#define COMPARE         14
#define COMPARE2		15
#define IMAGE			16
#define IMAGERESTORE	17
#define IMAGELIST		18
#define DEFAULTACTION   RESTORE

#define	MAXSPEC         80
#define MAX_PATH        260 // longueur maxi d'un nom et chemin de fichier
#define DATE_TIME       20
#define MAXLENGTH       (MAX_PATH+DATE_TIME)*sizeof(unsigned char)

// Actions to do (Action spécifique pour chaque fichier)
#define NOTHING     0
#define COPY        1
#define MOVE        2
#define REPLACE     3
#define EFFACE      4
#define TITLELINE   16
#define TESTED      -1      // pour indiquer que le fichier a été vu ds la cible

// Type of compression
#define NO_COMPRESSION      0
#define LZH_COMPRESSION     1
#define LZW_COMPRESSION     2
#define LZM_COMPRESSION     3
//-------------------------------------------------------
// Entête des fichiers image
struct imageHeader{
    int size;
    char signature[4];
    char originPath[ MAX_PATH ];
    bool full;
};
struct imageHeaderOld{
    char signature[4];
    char originPath[ MAX_PATH ];
};

//-------------------------------------------------------
// Les structures pour les listes de fichiers
struct sfile{
    unsigned char full_name[ MAX_PATH ];
    unsigned int long long creation_time;
    unsigned int long long modif_time;
    int long long org_size;
    unsigned int offset;
    unsigned int attributes;
    int iRevision;
    unsigned int dwPos;
    int iAction;
};
//-------------------------------------------------------
typedef struct _sfileOutput{
    unsigned char full_name[ MAX_PATH ];
    unsigned char org_path[MAX_PATH];
    unsigned char dest_path[MAX_PATH];
    int long long modif_time;
    int long long old_time;
    unsigned int attributes;
    int iAction;
    int iFileNum;
    int iIndex;
    bool dest_list;
} sfileOutput;
//-------------------------------------------------------
// structure pour lister les fichiers à restaurer
struct sfileR{
    unsigned char full_name[ MAXLENGTH ];
    int long long creation_time;
    int long long modif_time;
    int long long org_size;
    unsigned int attributes;
    unsigned int offset;
    int iRevision;
    unsigned int dwPos;
    int iAction;
    int iIndex;
    bool bSelected;
    int iVersions;
    int volume;
};
//-------------------------------------------------------
// structure de date petit format
typedef struct _DDATE {
    char jour;
    char mois;
    short annee;
    } DDATE, *PDDATE;
//-------------------------------------------------------
// Entête de fichier de commandes
typedef struct _MirwCmdeHead{
    unsigned int	dwSize;
    char            szSignature[32];
    unsigned int	dwCommandSize;
    unsigned int	dwCommandCount;
    char szVolumeName[20];
    char szAttente[200];
}	MirwCmdeHead;
// structure de ligne de commande
typedef struct	_MirwCmde	{
    unsigned int	dwSize;
    char szOrgPath	[MAXLENGTH];
    char szDestPath	[MAXLENGTH];
    char szCommPath	[MAXLENGTH];
    char szDirSpec	[MAXSPEC];
    char szFileSpec	[MAXSPEC];
    char szExclDirSpec	[MAXSPEC];
    char szExclFileSpec	[MAXSPEC];
    char szImageFile[MAXLENGTH];
    unsigned short	wAction;
    unsigned short	wOptions;
    int     inSets; // ancien bool	Auto;
    int     sets; // ancien bool	Quitter;
    int	useVolume; // anciennement bool NoCommDir;
}	MirwCmde, *pMirwCmde;

#endif // QTMIRROR_H
