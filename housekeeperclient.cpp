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
    connect(detect_thread, SIGNAL(send_adb_driver_state()), this, SLOT(update_adb_driver_state()));
    detect_thread->start(); //开启连接检测线程

    ui->download_hardware->setEnabled(false);

    tcp_client = new QTcpSocket(this);
    connect(tcp_client, SIGNAL(readyRead()), this, SLOT(recv_tcp_data())); //收到TCP服务端数据
    connect(tcp_client, SIGNAL(connected()), this, SLOT(tcp_client_connected())); //TCP连接成功
    connect(tcp_client, SIGNAL(disconnected()), this, SLOT(tcp_client_disconnected())); //TCP连接断开
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

void HouseKeeperClient::ParseAppData(QByteArray package_data) //解析数据
{

}

void HouseKeeperClient::recv_tcp_data()
{
    QByteArray package_data;   //一包数据
    int buff_index = 0;
    int flag = 0;  //记录{}标志
    char ch = 0;    //记录临时字符
    while(tcp_client->bytesAvailable() > 0 ) //如果有数据
    {
        tcp_client->read(&ch, 1);   //读一个字节
        if (ch == '{') //遇到json对象
        {
            flag++;
        }
        else if (ch == '}' && flag > 0)
        {
            flag--;
        }

        if (flag == 0) //若没有遇到json对象或者已读完一个json对象
        {
            if (buff_index > 8 && buff_index < MAX_MESSAGE_LEN-1) //判断是否有有效数据,json对象长度至少为9bytes才做处理 {"a":"b"}
            {
                package_data += QByteArray(&ch, 1) ; //读取数据
                ParseAppData(package_data); //解析数据
                package_data.clear();
                buff_index = 0;  //下标清零
            }
        }
        else
        {
            if (buff_index < MAX_MESSAGE_LEN-1) //json对象超出长度
                package_data += QByteArray(&ch, 1) ; //读取数据
            else  //直接丢掉该包数据
            {
                buff_index = 0;
                flag = 0;       //重新接受数据
                package_data.clear();
            }
        }
    }
}

void HouseKeeperClient::tcp_client_connected()
{
    qDebug()<<"connect success!!";
}

void HouseKeeperClient::tcp_client_disconnected()
{
    qDebug()<<"tcp_client disconnect!!";
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

    QString msg = "请选择端口 ";
    int i = 0;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QSerialPort serial;
        serial.setPort(info);
        if(serial.open(QIODevice::ReadWrite))
        {

            if(   info.description().contains(QString("Quectel USB DM Port"))
               || info.description().contains(QString("QDLoader"))     )
            {
                if(i != 0)
                    msg+="、";
                 msg += info.portName();
                 i++;
            }
            serial.close();
        }
    }
    msg += " 进行固件烧写";
    if(i == 0)
    {
        msg = "设备未连接！！";
        ui->download_com_help->setText(msg);
        ui->download_hardware->setEnabled(false);
        ui->download_com_help->setStyleSheet(QString("color: rgb(255, 0, 0);font: 25 16pt \"Microsoft YaHei UI Light\";"));
    }
    else
    {
        ui->download_hardware->setEnabled(true);
        ui->download_com_help->setStyleSheet(QString("color: rgb(70, 171, 15);font: 25 16pt \"Microsoft YaHei UI Light\";"));
    }

    ui->download_com_help->setText(msg);
}

void HouseKeeperClient::update_adb_driver_state()
{
    QString msg = "请确认ADB驱动已安装，并以管理员权限运行本程序！";
    ui->download_hardware->setEnabled(false);
    ui->download_com_help->setText(msg);
    ui->download_com_help->setStyleSheet(QString("color: rgb(255, 0, 0);font: 25 16pt \"Microsoft YaHei UI Light\";"));
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
