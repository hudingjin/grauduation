#include "./gui/mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 测试QtConcurrent
    qDebug() << "QtConcurrent测试...";
    QFuture<int> future = QtConcurrent::run([](){ return 42; });
    qDebug() << "QtConcurrent结果:" << future.result();

    MainWindow w;
    w.show();
    return a.exec();

    //mainwindow的css样式：background-color:#081a2a;
                      //color:#e8f4ff;
}
