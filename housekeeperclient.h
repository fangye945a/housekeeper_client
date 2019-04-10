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
#include <QHostInfo>
#include <QTimer>

#define DEBUG_BGP_PATH  "../housekeeper_client/pictures/BGP.png"
#define BGP_PATH        "./pictures/BGP.png"

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
    void onLookupHost(QHostInfo host);      //网络状态检测
    void update_time();                     //更新时间
private slots:
    void on_return_home_clicked();

private:
    Ui::HouseKeeperClient *ui;
    QTimer *update_time_timer;
};

#endif // HOUSEKEEPERCLIENT_H
