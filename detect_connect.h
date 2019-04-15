#ifndef DETECT_CONNECT_H
#define DETECT_CONNECT_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QProcess>
#include <QDebug>

class detect_connect : public QThread
{
    Q_OBJECT
public:
    detect_connect();
    virtual void run();
    void stop();

signals:
    void send_usb_connect_state(bool state);
    void send_network_connect_state(bool state);
private:
    bool flagRunning;   //运行标志
    QProcess *usb_process;
    QProcess *network_process;
};

#endif // DETECT_CONNECT_H
