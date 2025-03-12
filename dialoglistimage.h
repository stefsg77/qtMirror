#ifndef DIALOGLISTIMAGE_H
#define DIALOGLISTIMAGE_H

#include <QDialog>
#include "QtMirror.h"
//#include "lzhuf2.h"
#include "entetefichier.h"

namespace Ui {
class DialogListImage;
}

class DialogListImage : public QDialog
{
    Q_OBJECT

public:
    explicit DialogListImage(QString& fileName, QWidget *parent = 0);
    ~DialogListImage();
public slots:
    void getImageName();
    void selectAll();
    void restoreSelected();
    void comboChanged(int index);
    void getPath();

private:
    void makeDir(QString fullName);
    void selectFromVolume(int volume);
    int fillTable(int count);
    int readList7z();
    int m_volumes;
    int m_comboLines;
    int m_totalCount;
    int m_selected;
    int m_copied;
    int m_errors;
    const Byte *m_streamAdress;
    Ui::DialogListImage *ui;
    QString m_fileName;
    QString m_destPath;
    QStringList m_slSelect;
    QVector <struct sfileR> m_vSFileD;
    EnteteFichier *m_accesLecture7z;
};

#endif // DIALOGLISTIMAGE_H
