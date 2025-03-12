#ifndef DIALOGVIEWLOG_H
#define DIALOGVIEWLOG_H

#include <QDialog>

namespace Ui {
class DialogViewLog;
}

class DialogViewLog : public QDialog
{
    Q_OBJECT

public:
    explicit DialogViewLog(QWidget *parent = 0);
    ~DialogViewLog();

private:
    Ui::DialogViewLog *ui;
};

#endif // DIALOGVIEWLOG_H
