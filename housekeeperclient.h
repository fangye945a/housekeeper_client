#ifndef HOUSEKEEPERCLIENT_H
#define HOUSEKEEPERCLIENT_H

#include <QWidget>
#include <QDebug>
#include <QPalette>
#include <QPixmap>
#include <QMouseEvent>
#include <QFileInfo>
#include <QFile>
#include <QDateTime>
#include <QtNetwork/QTcpSocket>
#include <QTimer>
#include <QProcess>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTcpSocket>

#include "detect_connect.h"

#define DEBUG_BGP_PATH  "../zbox-house-keeper-client/pictures/BGP.png"
#define BGP_PATH        "./pictures/BGP.png"

#define MAX_MESSAGE_LEN 2048 //最大json长度

enum
{
    FIRST_PAGE,
    DOWNLOAD_HARDWARE,
    DEV_ID_WRITE,
    FACTORY_TEST,
    DEVICE_MANAGE,
    UPGRADE_PACKAGE
};

namespace Ui {
class HouseKeeperClient;
}

class HouseKeeperClient : public QWidget
{
    Q_OBJECT

public:
    explicit HouseKeeperClient(QWidget *parent = 0);
    ~HouseKeeperClient();

    void resizeEvent(QResizeEvent *event);          //背景图片跟随界面大小缩放
    void mouseDoubleClickEvent(QMouseEvent *event); //鼠标双击事件

    bool isFileExist(QString filepath);             //判断文件是否存在

public slots:
    void update_time();                     //更新时间
    void update_usb_connect_state(bool state);  //更新USB连接状态
    void update_adb_driver_state();  //更新ADB驱动状态
    void update_network_connect_state(bool state);      //更新网络连接状态

    void recv_tcp_data();
    void tcp_client_connected();
    void tcp_client_disconnected();

    void ParseAppData(QByteArray package_data); //json数据解析

private slots:
    void on_return_home_clicked();

    void on_hardware_download_clicked();

    void on_next_help_clicked();

    void on_pre_help_clicked();

    void on_download_hardware_clicked();

    void on_devid_entry_clicked();

    void on_factory_test_clicked();

    void on_dev_manage_clicked();

    void on_upgrade_package_clicked();

private:
    Ui::HouseKeeperClient *ui;
    QTimer *update_time_timer;
    detect_connect *detect_thread;
    QProcess *process;
    QTcpSocket *tcp_client;
    unsigned char network_connect_state;
    unsigned char usb_connect_state;
};

#endif // HOUSEKEEPERCLIENT_H
