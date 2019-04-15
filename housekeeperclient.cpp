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

    detect_thread = new detect_connect(); //创建检测线程
    connect(detect_thread, SIGNAL(send_usb_connect_state(bool)), this, SLOT(update_usb_connect_state(bool)));
    connect(detect_thread, SIGNAL(send_network_connect_state(bool)), this, SLOT(update_network_connect_state(bool)));
    detect_thread->start(); //开启连接检测线程
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

void HouseKeeperClient::update_time()
{
    QString time_str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->now_time->setText(time_str);
}

void HouseKeeperClient::update_network_connect_state(bool state)
{
    if (!state)
    {
        ui->net_status->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/网络断开.png);"));
        network_connect_state = 0;
    }
    else
    {
        ui->net_status->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/网络正常.png);"));
        network_connect_state = 1;
    }
}

void HouseKeeperClient::update_usb_connect_state(bool state)
{
    if (!state)
    {
        ui->usb_connect_state->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/断开链接.png);"));
        usb_connect_state = 0;
    }
    else
    {
        ui->usb_connect_state->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/连接.png);"));
        usb_connect_state = 1;
    }
}

void HouseKeeperClient::on_return_home_clicked()
{
    ui->stackedWidget->setCurrentIndex(FIRST_PAGE);
}

void HouseKeeperClient::on_hardware_download_clicked()
{
    ui->stackedWidget->setCurrentIndex(DOWNLOAD_HARDWARE);
}

void HouseKeeperClient::on_next_help_clicked()
{
    int index = ui->download_usage->currentIndex();
    if(index < ui->download_usage->count())
        index++;
    ui->download_usage->setCurrentIndex(index);
}

void HouseKeeperClient::on_pre_help_clicked()
{
    int index = ui->download_usage->currentIndex();
    if(index > 0)
        index--;
    ui->download_usage->setCurrentIndex(index);
}

void HouseKeeperClient::on_download_hardware_clicked()
{
    process = new QProcess(this);
    process->setWorkingDirectory( "./Release_Multi_Upgrade" );
    QString path = "./Release_Multi_Upgrade/Quectel_Customer_FW_Download_Tool_V4.32.exe";
    qDebug()<<"path = "<<path;
    process->start(path);
}

void HouseKeeperClient::on_devid_entry_clicked()
{
    ui->stackedWidget->setCurrentIndex(DEV_ID_WRITE);
}

void HouseKeeperClient::on_factory_test_clicked()
{
    ui->stackedWidget->setCurrentIndex(FACTORY_TEST);
}

void HouseKeeperClient::on_dev_manage_clicked()
{
    ui->stackedWidget->setCurrentIndex(DEVICE_MANAGE);
}

void HouseKeeperClient::on_upgrade_package_clicked()
{
    ui->stackedWidget->setCurrentIndex(UPGRADE_PACKAGE);
}
