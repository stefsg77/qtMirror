#ifndef COPYFILE_H
#define COPYFILE_H

#include <QString>
#include <QFile>
#include <QFileInfo>


class QCopyFile : public QObject
{
    Q_OBJECT
public:
    explicit QCopyFile(QObject *parent = 0);
    explicit QCopyFile(const QString &fileName, QObject *parent = 0);
    ~QCopyFile();
    void setFileName(QString &fileName);
    bool copy(const QString &newName);

signals:
    void copyChunkSignal(int percent, int chunk);

private:
    QFileInfo inInfo;
    QFile inFile;
    QFile outFile;
};

#endif // COPYFILE_H
