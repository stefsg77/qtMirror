#include "dialogviewlog.h"
#include "ui_dialogviewlog.h"
#include <QFile>
#include <QString>
#include <QDir>

DialogViewLog::DialogViewLog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogViewLog)
{
    ui->setupUi(this);
    connect( ui->pushButton, SIGNAL(clicked()), this, SLOT(accept()));
    QString fileName = QDir::homePath();
    fileName.append("/QtMirror.log");
    QFile log;
    log.setFileName(fileName);
    log.open(QIODevice::ReadOnly);
    char *buffer = new char[log.size()+1];
    log.read(buffer, log.size());
    buffer[log.size()] = 0;
    QString texte = buffer;
    log.close();
    ui->textBrowser->setText(texte);
    delete [] buffer;
}

DialogViewLog::~DialogViewLog()
{
    delete ui;
}
