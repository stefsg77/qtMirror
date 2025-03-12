#ifndef VIEWEDITSTUDY_H
#define VIEWEDITSTUDY_H

#include <QDialog>
#include <QString>
#include "mainwindow.h"
#include "QtMirror.h"
#include "qcombobox.h"


namespace Ui {
class ViewEditStudy;
}

class ViewEditStudy : public QDialog
{
    Q_OBJECT

public:
    explicit ViewEditStudy(MainWindow *parent = nullptr);
    ~ViewEditStudy();
    void fillTable();
    void validate();
public slots:
    void choiceChanged(int index);
private:
    Ui::ViewEditStudy *ui;
    QVector<sfileOutput>m_Sfo;
    QString m_text;
    MainWindow *m_mw;
    int m_count;
    QComboBox m_cbox;
    bool m_bAll;
    QStringList m_slHead;
};

#endif // VIEWEDITSTUDY_H
