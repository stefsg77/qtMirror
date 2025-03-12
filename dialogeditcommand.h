#ifndef DIALOGEDITCOMMAND_H
#define DIALOGEDITCOMMAND_H

#include <QDialog>
#include <QString>
#include "QtMirror.h"

namespace Ui {
class DialogEditCommand;
}

class DialogEditCommand : public QDialog
{
    Q_OBJECT
    
public:
    explicit DialogEditCommand(MirwCmde& mc, QWidget *parent = 0);
    ~DialogEditCommand();
    MirwCmde &readForm();
    QString getPath(QString);
public slots:
    void getPathO();
    void getPathI();
    void wasToggled(bool checked);
    
private:
    void fillForm();
    Ui::DialogEditCommand *ui;
    MirwCmde m_mc;
};

#endif // DIALOGEDITCOMMAND_H
