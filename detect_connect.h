#ifndef DETECT_CONNECT_H
#define DETECT_CONNECT_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QProcess>
#include <QDebug>
#include <QSerialPortInfo>
#include <QSerialPort>

class detect_connect : public QThread
{
    Q_OBJECT
public:
    detect_connect();
    virtual void run();
    void stop();

signals:
    void send_adb_driver_state();
    void send_usb_connect_state(int state);
    void send_network_connect_state(int state);
private:
    bool flagRunning;   //运行标志
    QProcess *usb_process;
    QProcess *network_process;
    int usb_state;
    int net_state;
};

#endif // DETECT_CONNECT_H
