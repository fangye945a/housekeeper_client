#include "housekeeperclient.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    HouseKeeperClient w;
    w.show();

    return a.exec();
}
