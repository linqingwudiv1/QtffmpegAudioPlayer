#include "mainwindow.h"
#include <QApplication>
#include <QDebug>

#include "audiodecodemgr.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile qssFile(":/qss/qss.txt");
    if(qssFile.open(QIODevice::ReadOnly)){
        QString qss = qssFile.readAll();
        //qDebug()<<qss;
        qApp->setStyleSheet(qss);
        qssFile.close();
    }
    a.setWindowIcon(QIcon(":/img/img/contacts.png"));
    MainWindow *w = new MainWindow();
    w->show();
    return a.exec();

}
