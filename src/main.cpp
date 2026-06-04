#include "./gui/mainwindow.h"
#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setWindowIcon(QIcon(":/airplane.png"));

    // ??QtConcurrent
    qDebug() << "QtConcurrent??...";
    QFuture<int> future = QtConcurrent::run([](){ return 42; });
    qDebug() << "QtConcurrent??:" << future.result();

    MainWindow w;
    w.show();
    return a.exec();

    //mainwindow?css???background-color:#081a2a;
                      //color:#e8f4ff;
}
