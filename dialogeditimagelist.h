#ifndef DIALOGEDITIMAGELIST_H
#define DIALOGEDITIMAGELIST_H

#include <QDialog>
#include <QString>
#include "QTableWidgetItem"
#include "qpushbutton.h"
#include "QtMirror.h"

namespace Ui {
class DialogEditImageList;
}

class DialogEditImageList : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEditImageList(QVector<struct sfile>* pSf, QWidget *parent = nullptr);
    ~DialogEditImageList();
    void validateList();
public slots:
    void selectAll();
    void unselectAll();

private:
    Ui::DialogEditImageList *ui;
    QVector <int> m_iAction;
    QVector<struct sfile>* m_pSfile;
    int m_iCount;
};

#endif // DIALOGEDITIMAGELIST_H
