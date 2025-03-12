#include "copyfile.h"
#include <QDateTime>
#include <utime.h>

QCopyFile::QCopyFile(QObject *parent) :
    QObject(parent)
{
}

QCopyFile::QCopyFile(const QString &fileName, QObject *parent) :
    QObject(parent)
{
    inFile.setFileName(fileName);
    inInfo = QFileInfo(fileName);
}

QCopyFile::~QCopyFile()
{
}

void QCopyFile::setFileName(QString &fileName)
{
    inFile.setFileName(fileName);
    inInfo = QFileInfo(fileName);
}

/*!
    Copies the file currently specified by fileName() to a file called
    \a newName.  Returns true if successful; otherwise returns false.

    Note that if a file with the name \a newName already exists,
    copy() returns false (i.e. QFile will not overwrite it).

    The source file is closed before it is copied.

    \sa setFileName()
*/

bool
QCopyFile::copy(const QString &newName)
{
    bool error;
    qint64 totalSize;
    if (inFile.fileName().isEmpty()) {
        return false;
    }
    outFile.setFileName(newName);
    if (QFile(newName).exists()) {
        return false;
    }
            error = false;
            if(!inFile.open(QFile::ReadOnly)) {
                error = true;
            } else {
                totalSize = inFile.size();
                QString fileTemplate = QLatin1String("%1/qt_temp.XXXXXX");
                outFile.setFileName(fileTemplate.arg(QFileInfo(newName).path()));
                if (!outFile.open(QIODevice::ReadWrite))
                    error = true;
                if (error) {
                    outFile.close();
                    inFile.close();
                } else {
                    char block[4096];
                    qint64 totalRead = 0;
                    while(!inFile.atEnd()) {
                        qint64 in = inFile.read(block, sizeof(block));
                        if (in <= 0)
                            break;
                        totalRead += in;
                        if(in != outFile.write(block, in)) {
                            inFile.close();
                            error = true;
                            break;
                        }
                        emit copyChunkSignal((int)((totalRead*100)/totalSize),(int)in);
                    }

                    if (totalRead != totalSize) {
                        // Unable to read from the source. The error string is
                        // already set from read().
                        error = true;
                    }
                    if (!error && !outFile.rename(newName)) {
                        error = true;
                        inFile.close();
                    }
                    if (error)
                        outFile.remove();
                }
            }
            if(!error) {
                QFile::setPermissions(newName, inFile.permissions());
                inFile.close();
                struct utimbuf utb;
                utb.actime = (time_t)(inInfo.lastModified().toSecsSinceEpoch());
                utb.modtime = utb.actime;
                utime(newName.toUtf8().data(), &utb);
                return true;
            }
    return false;
}
