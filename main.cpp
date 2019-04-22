#include "housekeeperclient.h"
#include <QApplication>
#include <QTextCodec>
#include <QSharedMemory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTextCodec *codec = QTextCodec::codecForName("housekeeperclient");
    QTextCodec::setCodecForLocale(codec);
    QSharedMemory shared_memory;            //共享内存 防止应用多开
    shared_memory.setKey(QString("666666"));
    if(shared_memory.attach())
    {
        return 0;
    }
    if(shared_memory.create(1))
    {
        HouseKeeperClient w;
        w.show();
        return a.exec();
    }
}
