#include "detect_connect.h"

detect_connect::detect_connect()
{
    flagRunning = true;
    usb_process = NULL;
    network_process = NULL;
}

void detect_connect::run()
{
    qDebug()<<"Create pthread!!";
    QString usb_cmd = "adb devices";
    QString network_cmd = "ping www.baidu.com -n 2 -w 500";
    while(flagRunning)
    {
        if(usb_process == NULL)
            usb_process = new QProcess();

        if(network_process == NULL)
            network_process = new QProcess();


        usb_process->start(usb_cmd);
        usb_process->waitForFinished();
        QString result = usb_process->readAll();
        qDebug()<<"result:"<<result;
        if(result.isEmpty())
        {
            emit send_adb_driver_state();   //未安装adb驱动或未以管理员权限运行
        }
        else
        {
            if(result.contains(QString("device\r\n\r\n")))
            {
                emit send_usb_connect_state(true);  //连接正常
            }
            else
            {
                emit send_usb_connect_state(false);  //连接正常
            }
        }


        network_process->start(network_cmd);
        network_process->waitForFinished();
        result = network_process->readAll();

        if(result.contains(QString("TTL=")))
        {
            emit send_network_connect_state(true);
        }
        else
        {
            emit send_network_connect_state(false);
        }

        msleep(1000);   //每秒检测一次
    }
    qDebug()<<"-------- Exit pthread!!";
}

void detect_connect::stop()
{
    flagRunning = false;   //线程退出标志
}
