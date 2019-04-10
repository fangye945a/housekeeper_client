#include "housekeeperclient.h"
#include "ui_housekeeperclient.h"

HouseKeeperClient::HouseKeeperClient(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HouseKeeperClient)
{
    ui->setupUi(this);
   // this->setWindowFlags (Qt::Window | Qt::FramelessWindowHint);
    update_time_timer = new QTimer(this);
    connect(update_time_timer, SIGNAL(timeout()), this, SLOT(update_time()));
    update_time_timer->start(1000);
}

HouseKeeperClient::~HouseKeeperClient()
{
    delete ui;
}

void HouseKeeperClient::resizeEvent(QResizeEvent *event)    //背景图
{
    (void)event;    //去除警告
    QPalette palette;   //显示首界面背景图

    if( isFileExist(QString(BGP_PATH)) )
    {
        QPixmap pic(BGP_PATH);
        palette.setBrush(QPalette::Background, QBrush(pic.scaled(this->size())));
        this->setPalette(palette);
    }else
    {
        QPixmap pic(DEBUG_BGP_PATH);
        palette.setBrush(QPalette::Background, QBrush(pic.scaled(this->size())));
        this->setPalette(palette);
    }

}

void HouseKeeperClient::mouseDoubleClickEvent(QMouseEvent *event) //鼠标双击
{
    (void)event;
}

bool HouseKeeperClient::isFileExist(QString filepath)
{
    QFileInfo fileInfo(filepath);
    if(fileInfo.isFile())
    {
        return true; //存在
    }
    return false; //不存在
}

void HouseKeeperClient::onLookupHost(QHostInfo host)
{
    if (host.error() != QHostInfo::NoError)
    {
        ui->net_status->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/网络断开.png);"));
    }
    else
    {
        ui->net_status->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/网络正常.png);"));
    }
}

void HouseKeeperClient::update_time()
{
    QHostInfo::lookupHost("www.baidu.com", this, SLOT(onLookupHost(QHostInfo)));
    QString time_str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->now_time->setText(time_str);
}

void HouseKeeperClient::on_return_home_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}
