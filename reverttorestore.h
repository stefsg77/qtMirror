#ifndef REVERTTORESTORE_H
#define REVERTTORESTORE_H

#include <string.h>
#include "QtMirror.h"

// Cette classe recoit ine ligne de commande et la modifie pour en faire une ligne de restauration à l'identique
// Inverser Source et Destination
// assurer copie à l'identique
class RevertToRestore
{
public:
    RevertToRestore();
    static unsigned short revert(MirwCmde& cmd)
    {
        unsigned short last = cmd.wAction;
        if( cmd.wAction == IMAGE )
        {
            cmd.wAction = IMAGERESTORE;
        }
        else
        {
            cmd.wAction = RESTORE;
            char buffer[MAX_PATH];
            strcpy_s(buffer, cmd.szOrgPath);
            strcpy_s(cmd.szOrgPath, cmd.szDestPath);
            strcpy_s(cmd.szDestPath, buffer);
        }
        return last;
    }
    static void reRevert(MirwCmde& cmd, unsigned short action)
    {
        if( cmd.wAction == IMAGERESTORE )
        {
            cmd.wAction = IMAGE;
        }
        else
        {
            cmd.wAction = action;
            char buffer[MAX_PATH];
            strcpy_s(buffer, cmd.szOrgPath);
            strcpy_s(cmd.szOrgPath, cmd.szDestPath);
            strcpy_s(cmd.szDestPath, buffer);
        }
        return;
    }
};

#endif // REVERTTORESTORE_H
