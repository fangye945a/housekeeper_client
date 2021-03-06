#include "housekeeperclient.h"
#include "ui_housekeeperclient.h"


const char *products_id[PRODUCT_NUMS] = {  "unknow",        //未设置
                                           "KqSPWIMl3p",    //建起塔机
                                           "1de29pmNvL"     //土方小挖
                                          }; //产品ID定义

HouseKeeperClient::HouseKeeperClient(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HouseKeeperClient)
{
    ui->setupUi(this);
    this->setWindowTitle("二代中联盒子助手v1.5");
    root_permission = isFileExist(QString("root"));
    network_connect_state = 0;
    usb_connect_state = 0;
    tcp_connect_flag = 0;        //TCP连接状态
    memset(&all_params,0,sizeof(all_params));

   // this->setWindowFlags (Qt::Window | Qt::FramelessWindowHint);
    update_time_timer = new QTimer(this);
    connect(update_time_timer, SIGNAL(timeout()), this, SLOT(update_time()));
    update_time_timer->start(1000);

    factory_test_timer = new QTimer(this);
    connect(factory_test_timer, SIGNAL(timeout()), this, SLOT(factory_test_check()));


    detect_thread = new detect_connect(); //创建检测线程
    connect(detect_thread, SIGNAL(send_usb_connect_state(int)), this, SLOT(update_usb_connect_state(int)));
    connect(detect_thread, SIGNAL(send_network_connect_state(int)), this, SLOT(update_network_connect_state(int)));
    connect(detect_thread, SIGNAL(send_adb_driver_state()), this, SLOT(update_adb_driver_state()));
    detect_thread->start(); //开启连接检测线程

    ui->download_hardware->setEnabled(false);

    tcp_client = new QTcpSocket(this);
    connect(tcp_client, SIGNAL(readyRead()), this, SLOT(recv_tcp_data())); //收到TCP服务端数据
    connect(tcp_client, SIGNAL(connected()), this, SLOT(tcp_client_connected())); //TCP连接成功
    connect(tcp_client, SIGNAL(disconnected()), this, SLOT(tcp_client_disconnected())); //TCP连接断开

    if(!root_permission)
    {
        ui->tabWidget->setTabEnabled(1,false);  //禁掉参数设置等其它功能
        ui->tabWidget->setTabEnabled(2,false);
        ui->tabWidget->setTabEnabled(3,false);
        ui->tabWidget->setTabEnabled(4,false);
        ui->tabWidget->setTabEnabled(5,false);
        ui->tabWidget->setTabEnabled(6,false);
        ui->tabWidget->setTabEnabled(7,false);
    }
    init_database_record(); //初始化数据库
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

bool HouseKeeperClient::delete_dir(QString path)
{
    if (path.isEmpty())
    {
        return false;
    }
    QDir dir(path);
    if(!dir.exists()){
        return true;
    }
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot); //设置过滤
    QFileInfoList fileList = dir.entryInfoList(); // 获取所有的文件信息
    foreach (QFileInfo file, fileList)
    { //遍历文件信息
        if (file.isFile())  // 是文件，删除
        {
            file.dir().remove(file.fileName());
        }
        else     // 目录则递归删除
        {
            delete_dir(file.absoluteFilePath());
        }
    }
    return dir.rmpath(dir.absolutePath()); // 删除文件夹
}
QTableWidgetItem *HouseKeeperClient::create_item(QString msg)
{
    QTableWidgetItem *tmp = new QTableWidgetItem(msg);
    tmp->setTextAlignment(Qt::AlignCenter);
    return tmp;
}

void HouseKeeperClient::show_params(int data)
{
    ui->params_tableWidget->clearContents();
    //ui->params_tableWidget->clear();
    switch (data)
    {
        case ENUM_VOLTAGE_INFO:
        {
            ui->params_tableWidget->setRowCount(5);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择

            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");
            ui->params_tableWidget->setItem(0,0,create_item(QString("主电源电压")));
            ui->params_tableWidget->setItem(0,1,create_item(QString::number(all_params.volage_info.power_voltage)));

            ui->params_tableWidget->setItem(1,0,create_item(QString("电池电压")));
            ui->params_tableWidget->setItem(1,1,create_item(QString::number(all_params.volage_info.battery_voltage)));

            ui->params_tableWidget->setItem(2,0,create_item(QString("低功耗阈值")));
            ui->params_tableWidget->setItem(2,1,create_item(QString::number(all_params.volage_info.mvcc_mid)));

            ui->params_tableWidget->setItem(3,0,create_item(QString("休眠阈值")));
            ui->params_tableWidget->setItem(3,1,create_item(QString::number(all_params.volage_info.mvcc_low)));

            ui->params_tableWidget->setItem(4,0,create_item(QString("电池断电电压")));
            ui->params_tableWidget->setItem(4,1,create_item(QString::number(all_params.volage_info.battery_off)));

            //ui->params_tableWidget->resizeColumnsToContents(); //将列的大小设为与内容相匹配
            //ui->params_tableWidget->resizeRowsToContents();    //将行的大小设为与内容相匹配
            ui->params_tableWidget->horizontalHeader()->setStretchLastSection(true);    //填充整行

        }break;
        case ENUM_DEVICE_INFO:
        {
            ui->params_tableWidget->setRowCount(5);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("设备ID")));
            ui->params_tableWidget->setItem(0,1,create_item(QString(all_params.device_info.devid)));

            ui->params_tableWidget->setItem(1,0,create_item(QString("工作模式")));
            ui->params_tableWidget->setItem(1,1,create_item(QString::number(all_params.device_info.working_mode)));

            ui->params_tableWidget->setItem(2,0,create_item(QString("本次通电时长")));
            ui->params_tableWidget->setItem(2,1,create_item(QString::number(all_params.device_info.current_on_time)));

            ui->params_tableWidget->setItem(3,0,create_item(QString("当前温度")));
            ui->params_tableWidget->setItem(3,1,create_item(QString::number(all_params.device_info.current_temp)));

            ui->params_tableWidget->setItem(4,0,create_item(QString("ACC ON总里程数")));
            ui->params_tableWidget->setItem(4,1,create_item(QString::number(all_params.device_info.acc_on_total_mileage)));

            //ui->params_tableWidget->resizeColumnsToContents(); //将列的大小设为与内容相匹配
            //ui->params_tableWidget->resizeRowsToContents();    //将行的大小设为与内容相匹配
            ui->params_tableWidget->horizontalHeader()->setStretchLastSection(true);    //填充整行

        }break;
        case ENUM_TCP_CONNECT_INFO:
        {
            ui->params_tableWidget->setRowCount(3);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("服务器IP地址")));
            ui->params_tableWidget->setItem(0,1,create_item(QString(all_params.tcp_connect_info.tcp_ip)));

            ui->params_tableWidget->setItem(1,0,create_item(QString("服务器端口号")));
            ui->params_tableWidget->setItem(1,1,create_item(QString::number(all_params.tcp_connect_info.tcp_port)));

            ui->params_tableWidget->setItem(2,0,create_item(QString("TCP连接状态")));
            if(all_params.state_info.tcp_connect_state)
                ui->params_tableWidget->setItem(2,1,create_item(QString("已连接")));
            else
                ui->params_tableWidget->setItem(2,1,create_item(QString("未连接")));

            ui->params_tableWidget->horizontalHeader()->setStretchLastSection(true);    //填充整行
        }break;
        case ENUM_REPORT_TIME_INFO:
        {
            ui->params_tableWidget->setRowCount(6);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("AA报警周期")));
            ui->params_tableWidget->setItem(0,1,create_item(QString::number(all_params.report_time_info.aa_alarm_cycle)));

            ui->params_tableWidget->setItem(1,0,create_item(QString("信号弱锁车时长")));
            ui->params_tableWidget->setItem(1,1,create_item(QString::number(all_params.report_time_info.gprs_signal_weak_time)));

            ui->params_tableWidget->setItem(2,0,create_item(QString("低功耗唤醒周期")));
            ui->params_tableWidget->setItem(2,1,create_item(QString::number(all_params.report_time_info.low_consumption_cycle)));

            ui->params_tableWidget->setItem(3,0,create_item(QString("位置信息上报周期")));
            ui->params_tableWidget->setItem(3,1,create_item(QString::number(all_params.report_time_info.position_report_cycle)));

            ui->params_tableWidget->setItem(4,0,create_item(QString("休眠唤醒周期")));
            ui->params_tableWidget->setItem(4,1,create_item(QString::number(all_params.report_time_info.sleep_wakeup_cycle)));

            ui->params_tableWidget->setItem(5,0,create_item(QString("工况数据上报周期")));
            ui->params_tableWidget->setItem(5,1,create_item(QString::number(all_params.report_time_info.workstation_report_cycle)));
            ui->params_tableWidget->horizontalHeader()->setStretchLastSection(true);    //填充整行
        }break;
        case ENUM_STATE_INFO:
        {
            ui->params_tableWidget->setRowCount(10);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("ACC状态")));
            if(all_params.state_info.acc_state)
                ui->params_tableWidget->setItem(0,1,create_item(QString("ON")));
            else
                ui->params_tableWidget->setItem(0,1,create_item(QString("OFF")));

            ui->params_tableWidget->setItem(1,0,create_item(QString("总线状态")));
            if(all_params.state_info.bus_connect_state)
                ui->params_tableWidget->setItem(1,1,create_item(QString("正常")));
            else
                ui->params_tableWidget->setItem(1,1,create_item(QString("故障")));

            ui->params_tableWidget->setItem(2,0,create_item(QString("充电状态")));
            if(all_params.state_info.charge_state)
                ui->params_tableWidget->setItem(2,1,create_item(QString("充电中")));
            else
                ui->params_tableWidget->setItem(2,1,create_item(QString("未充电")));

            ui->params_tableWidget->setItem(3,0,create_item(QString("设备连接状态")));
            if(all_params.state_info.dev_link_state == 0)
                ui->params_tableWidget->setItem(3,1,create_item(QString("初始化检测中")));
            else if(all_params.state_info.dev_link_state == 1)
                ui->params_tableWidget->setItem(3,1,create_item(QString("IOT与QNX连接正常")));
            else if(all_params.state_info.dev_link_state == 2)
                ui->params_tableWidget->setItem(3,1,create_item(QString("IOT与QNX连接断开")));
            else if(all_params.state_info.dev_link_state == 3)
                ui->params_tableWidget->setItem(3,1,create_item(QString("AD板与PLC断开")));
            else if(all_params.state_info.dev_link_state == 4)
                ui->params_tableWidget->setItem(3,1,create_item(QString("AD板与QNX断开")));

            ui->params_tableWidget->setItem(4,0,create_item(QString("拨号状态")));
            if(all_params.state_info.dial_state)
                ui->params_tableWidget->setItem(4,1,create_item(QString("拨号成功")));
            else
                ui->params_tableWidget->setItem(4,1,create_item(QString("拨号失败，重试中")));

            ui->params_tableWidget->setItem(5,0,create_item(QString("GPS天线状态")));
            if(all_params.state_info.gps_antena == 1)
                ui->params_tableWidget->setItem(5,1,create_item(QString("正常")));
            else if(all_params.state_info.gps_antena == 2)
                ui->params_tableWidget->setItem(5,1,create_item(QString("未连接")));
            else if(all_params.state_info.gps_antena == 3)
                ui->params_tableWidget->setItem(5,1,create_item(QString("故障")));
            else
                ui->params_tableWidget->setItem(5,1,create_item(QString("未知状态")));

            ui->params_tableWidget->setItem(6,0,create_item(QString("GPS定位状态")));
            if(all_params.state_info.gps_state)
                ui->params_tableWidget->setItem(6,1,create_item(QString("已定位")));
            else
                ui->params_tableWidget->setItem(6,1,create_item(QString("未定位")));

            ui->params_tableWidget->setItem(7,0,create_item(QString("开盖状态")));
            if(all_params.state_info.liq_state)
                ui->params_tableWidget->setItem(7,1,create_item(QString("开盖")));
            else
                ui->params_tableWidget->setItem(7,1,create_item(QString("关盖")));


            ui->params_tableWidget->setItem(8,0,create_item(QString("服务器登录状态")));
            if(all_params.state_info.login_state)
                ui->params_tableWidget->setItem(8,1,create_item(QString("已登录")));
            else
                ui->params_tableWidget->setItem(8,1,create_item(QString("未登录")));

            ui->params_tableWidget->setItem(9,0,create_item(QString("SIM卡状态")));
            if(all_params.state_info.simcard_state)
                ui->params_tableWidget->setItem(9,1,create_item(QString("正常")));
            else
                ui->params_tableWidget->setItem(9,1,create_item(QString("未检测到或SIM卡故障")));

            ui->params_tableWidget->horizontalHeader()->setStretchLastSection(true);    //填充整行
        }break;
        case ENUM_BAUTRATE_INFO:
        {
            ui->params_tableWidget->setRowCount(3);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("CAN0波特率")));
            ui->params_tableWidget->setItem(0,1,create_item(QString::number(all_params.baudrate_info.can0)));

            ui->params_tableWidget->setItem(1,0,create_item(QString("CAN1波特率")));
            ui->params_tableWidget->setItem(1,1,create_item(QString::number(all_params.baudrate_info.can1)));

            ui->params_tableWidget->setItem(2,0,create_item(QString("RS485波特率")));
            ui->params_tableWidget->setItem(2,1,create_item(QString::number(all_params.baudrate_info.rs485)));

            ui->params_tableWidget->horizontalHeader()->setStretchLastSection(true);    //填充整行
        }break;
        case ENUM_GPS_INFO:
        {
            ui->params_tableWidget->setRowCount(8);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("航向")));
            ui->params_tableWidget->setItem(0,1,create_item(QString::number(all_params.gps_info.gps_course)));
            ui->params_tableWidget->setItem(1,0,create_item(QString("速度")));
            ui->params_tableWidget->setItem(1,1,create_item(QString::number(all_params.gps_info.gps_velocity)));
            ui->params_tableWidget->setItem(2,0,create_item(QString("纬度")));
            ui->params_tableWidget->setItem(2,1,create_item(QString::number(all_params.gps_info.latitude)));
            ui->params_tableWidget->setItem(3,0,create_item(QString("南北纬")));
            ui->params_tableWidget->setItem(3,1,create_item(QString(all_params.gps_info.lattype)));
            ui->params_tableWidget->setItem(4,0,create_item(QString("经度")));
            ui->params_tableWidget->setItem(4,1,create_item(QString::number(all_params.gps_info.longitude)));
            ui->params_tableWidget->setItem(5,0,create_item(QString("东西经")));
            ui->params_tableWidget->setItem(5,1,create_item(QString(all_params.gps_info.lontype)));
            ui->params_tableWidget->setItem(6,0,create_item(QString("卫星颗数")));
            ui->params_tableWidget->setItem(6,1,create_item(QString::number(all_params.gps_info.satallite_num)));
            ui->params_tableWidget->setItem(7,0,create_item(QString("UTC时间")));
            ui->params_tableWidget->setItem(7,1,create_item(QString::number(all_params.gps_info.utc_time)));

            ui->params_tableWidget->horizontalHeader()->setStretchLastSection(true);    //填充整行
        }break;
        case ENUM_GSM_INFO:
        {
            ui->params_tableWidget->setRowCount(5);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("信号强度")));
            ui->params_tableWidget->setItem(0,1,create_item(QString::number(all_params.gsm_info.gsm_signal_strength)));
            ui->params_tableWidget->setItem(1,0,create_item(QString("SIM卡ID")));
            ui->params_tableWidget->setItem(1,1,create_item(QString(all_params.gsm_info.sim_card_id)));
            ui->params_tableWidget->setItem(2,0,create_item(QString("SIM卡IMSI号")));
            ui->params_tableWidget->setItem(2,1,create_item(QString(all_params.gsm_info.sim_card_imsi)));
            ui->params_tableWidget->setItem(3,0,create_item(QString("电话号码")));
            ui->params_tableWidget->setItem(3,1,create_item(QString(all_params.gsm_info.sms_number)));
            ui->params_tableWidget->setItem(4,0,create_item(QString("基站ID")));
            ui->params_tableWidget->setItem(4,1,create_item(QString(all_params.gsm_info.station_id)));
            ui->params_tableWidget->horizontalHeader()->setStretchLastSection(true);    //填充整行
        }break;
        case ENUM_HISTORY_INFO:
        {
            ui->params_tableWidget->setRowCount(10);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("GPRS信号弱时长")));
            ui->params_tableWidget->setItem(0,1,create_item(QString::number(all_params.history_info.gprs_link_lost_time)));
            ui->params_tableWidget->setItem(1,0,create_item(QString("GPS拔天线次数")));
            ui->params_tableWidget->setItem(1,1,create_item(QString::number(all_params.history_info.gps_antenna_stat)));
            ui->params_tableWidget->setItem(2,0,create_item(QString("GPS故障时长")));
            ui->params_tableWidget->setItem(2,1,create_item(QString::number(all_params.history_info.gps_fault_time)));
            ui->params_tableWidget->setItem(3,0,create_item(QString("GPS总里程数")));
            ui->params_tableWidget->setItem(3,1,create_item(QString::number(all_params.history_info.gps_total_mileage)));
            ui->params_tableWidget->setItem(4,0,create_item(QString("开盖次数")));
            ui->params_tableWidget->setItem(4,1,create_item(QString::number(all_params.history_info.lid_no)));
            ui->params_tableWidget->setItem(5,0,create_item(QString("曾锁车标志")));
            ui->params_tableWidget->setItem(5,1,create_item(QString::number(all_params.history_info.once_auto_lock_car_flag)));
            ui->params_tableWidget->setItem(6,0,create_item(QString("SIM卡更换次数")));
            ui->params_tableWidget->setItem(6,1,create_item(QString::number(all_params.history_info.sim_change_card_no)));
            ui->params_tableWidget->setItem(7,0,create_item(QString("SIM卡曾更换标志")));
            ui->params_tableWidget->setItem(7,1,create_item(QString::number(all_params.history_info.sim_change_flag)));
            ui->params_tableWidget->setItem(8,0,create_item(QString("ACC ON总时长")));
            ui->params_tableWidget->setItem(8,1,create_item(QString::number(all_params.history_info.total_acc_on_time)));
            ui->params_tableWidget->setItem(9,0,create_item(QString("通电总时长")));
            ui->params_tableWidget->setItem(9,1,create_item(QString::number(all_params.history_info.total_on_time)));

            ui->params_tableWidget->horizontalHeader()->setStretchLastSection(true);    //填充整行
        }break;
        case ENUM_VERSION_INFO:
        {
            int item_rows = 6;
            for(int i=0;i<5;i++)
            {
                if(all_params.version_info.app_version[i].enable_flag)
                    item_rows++;
            }

            ui->params_tableWidget->setRowCount(item_rows);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("终端固件版本")));
            ui->params_tableWidget->setItem(0,1,create_item(QString(all_params.version_info.fs_version)));
            ui->params_tableWidget->setItem(1,0,create_item(QString("终端硬件版本")));
            ui->params_tableWidget->setItem(1,1,create_item(QString(all_params.version_info.hw_version)));
            ui->params_tableWidget->setItem(2,0,create_item(QString("MCU程序版本")));
            ui->params_tableWidget->setItem(2,1,create_item(QString(all_params.version_info.mcu_version)));
            ui->params_tableWidget->setItem(3,0,create_item(QString("服务程序版本")));
            ui->params_tableWidget->setItem(3,1,create_item(QString(all_params.version_info.service_version)));
            ui->params_tableWidget->setItem(4,0,create_item(QString("应用管理程序版本")));
            ui->params_tableWidget->setItem(4,1,create_item(QString(all_params.version_info.housekeeper_version)));
            ui->params_tableWidget->setItem(5,0,create_item(QString("其他程序版本(如PLC)")));
            ui->params_tableWidget->setItem(5,1,create_item(QString(all_params.version_info.other_version)));

            for(int i=0;i<5;i++)
            {
                if(all_params.version_info.app_version[i].enable_flag)
                {
                    QString param_type = "应用程序[";
                    param_type += QString(all_params.version_info.app_version[i].app_name);
                    param_type += "]版本";
                    ui->params_tableWidget->setItem(6+i,0,create_item(param_type));
                    ui->params_tableWidget->setItem(6+i,1,create_item(QString(all_params.version_info.app_version[i].version)));
                }
            }
            ui->params_tableWidget->horizontalHeader()->setStretchLastSection(true);    //填充整行
        }break;
        default:break;
    }
}

void HouseKeeperClient::parse_all_params_data(cJSON *root)
{
    cJSON *volage_info = cJSON_GetObjectItem(root, "voltage_info");
    if(volage_info)
    {
        cJSON *tmp = cJSON_GetObjectItem(volage_info,"power_voltage");
        if(tmp)
            all_params.volage_info.power_voltage = tmp->valuedouble;
        tmp = cJSON_GetObjectItem(volage_info,"battery_voltage");
        if(tmp)
            all_params.volage_info.battery_voltage = tmp->valuedouble;
        tmp = cJSON_GetObjectItem(volage_info,"mvcc_mid");
        if(tmp)
            all_params.volage_info.mvcc_mid = tmp->valuedouble;
        tmp = cJSON_GetObjectItem(volage_info,"mvcc_low");
        if(tmp)
            all_params.volage_info.mvcc_low = tmp->valuedouble;
        tmp = cJSON_GetObjectItem(volage_info,"battery_off");
        if(tmp)
            all_params.volage_info.battery_off = tmp->valuedouble;
    }

    cJSON *device_info = cJSON_GetObjectItem(root, "device_info");
    if(device_info)
    {
        cJSON *tmp = cJSON_GetObjectItem(device_info,"devid");
        if(tmp)
            strcpy(all_params.device_info.devid, tmp->valuestring);
        tmp = cJSON_GetObjectItem(device_info,"working_mode");
        if(tmp)
            all_params.device_info.working_mode = tmp->valueint;
        tmp = cJSON_GetObjectItem(device_info,"current_on_time");
        if(tmp)
            all_params.device_info.current_on_time = tmp->valueint;
        tmp = cJSON_GetObjectItem(device_info,"acc_on_total_mileage");
        if(tmp)
            all_params.device_info.acc_on_total_mileage = tmp->valueint;
        tmp = cJSON_GetObjectItem(device_info,"current_temp");
        if(tmp)
            all_params.device_info.current_temp = tmp->valuedouble;
    }

    cJSON *tcp_connect_info = cJSON_GetObjectItem(root, "tcp_connect_info");
    if(tcp_connect_info)
    {
        cJSON *tmp = cJSON_GetObjectItem(tcp_connect_info,"tcp_ip");
        if(tmp)
            strcpy(all_params.tcp_connect_info.tcp_ip, tmp->valuestring);
        tmp = cJSON_GetObjectItem(tcp_connect_info,"tcp_port");
        if(tmp)
            all_params.tcp_connect_info.tcp_port = tmp->valueint;
    }

    cJSON *report_time_info = cJSON_GetObjectItem(root, "report_time_info");
    if(report_time_info)
    {
        cJSON *tmp = cJSON_GetObjectItem(report_time_info,"aa_alarm_cycle");
        if(tmp)
            all_params.report_time_info.aa_alarm_cycle = tmp->valueint;
        tmp = cJSON_GetObjectItem(report_time_info,"gprs_signal_weak_time");
        if(tmp)
            all_params.report_time_info.gprs_signal_weak_time = tmp->valueint;
        tmp = cJSON_GetObjectItem(report_time_info,"low_consumption_cycle");
        if(tmp)
            all_params.report_time_info.low_consumption_cycle = tmp->valueint;
        tmp = cJSON_GetObjectItem(report_time_info,"position_report_cycle");
        if(tmp)
            all_params.report_time_info.position_report_cycle = tmp->valueint;
        tmp = cJSON_GetObjectItem(report_time_info,"sleep_wakeup_cycle");
        if(tmp)
            all_params.report_time_info.sleep_wakeup_cycle = tmp->valueint;
        tmp = cJSON_GetObjectItem(report_time_info,"workstation_report_cycle");
        if(tmp)
            all_params.report_time_info.workstation_report_cycle = tmp->valueint;
    }

    cJSON *state_info = cJSON_GetObjectItem(root, "state_info");
    if(state_info)
    {
        cJSON *tmp = cJSON_GetObjectItem(state_info,"acc_state");
        if(tmp)
            all_params.state_info.acc_state = tmp->valueint;
        tmp = cJSON_GetObjectItem(state_info,"bus_connect_state");
        if(tmp)
            all_params.state_info.bus_connect_state = tmp->valueint;
        tmp = cJSON_GetObjectItem(state_info,"charge_state");
        if(tmp)
            all_params.state_info.charge_state = tmp->valueint;
        tmp = cJSON_GetObjectItem(state_info,"dev_link_state");
        if(tmp)
            all_params.state_info.dev_link_state = tmp->valueint;
        tmp = cJSON_GetObjectItem(state_info,"dial_state");
        if(tmp)
            all_params.state_info.dial_state = tmp->valueint;
        tmp = cJSON_GetObjectItem(state_info,"gps_antena");
        if(tmp)
            all_params.state_info.gps_antena = tmp->valueint;
        tmp = cJSON_GetObjectItem(state_info,"gps_state");
        if(tmp)
            all_params.state_info.gps_state = tmp->valueint;
        tmp = cJSON_GetObjectItem(state_info,"liq_state");
        if(tmp)
            all_params.state_info.liq_state = tmp->valueint;
        tmp = cJSON_GetObjectItem(state_info,"login_state");
        if(tmp)
            all_params.state_info.login_state = tmp->valueint;
        tmp = cJSON_GetObjectItem(state_info,"simcard_state");
        if(tmp)
            all_params.state_info.simcard_state = tmp->valueint;
        tmp = cJSON_GetObjectItem(state_info,"tcp_connect_state");
        if(tmp)
            all_params.state_info.tcp_connect_state = tmp->valueint;
    }

    cJSON *baudrate_info = cJSON_GetObjectItem(root, "baudrate_info");
    if(baudrate_info)
    {
        cJSON *tmp = cJSON_GetObjectItem(baudrate_info,"can0_baudrate");
        if(tmp)
            all_params.baudrate_info.can0 = tmp->valueint;
        tmp = cJSON_GetObjectItem(baudrate_info,"can1_baudrate");
        if(tmp)
            all_params.baudrate_info.can1 = tmp->valueint;
        tmp = cJSON_GetObjectItem(baudrate_info,"rs485_baudrate");
        if(tmp)
            all_params.baudrate_info.rs485 = tmp->valueint;
    }

    cJSON *gps_info = cJSON_GetObjectItem(root, "gps_info");
    if(gps_info)
    {
        cJSON *tmp = cJSON_GetObjectItem(gps_info,"gps_course");
        if(tmp)
            all_params.gps_info.gps_course = tmp->valuedouble;
        tmp = cJSON_GetObjectItem(gps_info,"gps_velocity");
        if(tmp)
            all_params.gps_info.gps_velocity = tmp->valuedouble;
        tmp = cJSON_GetObjectItem(gps_info,"latitude");
        if(tmp)
            all_params.gps_info.latitude = tmp->valuedouble;
        tmp = cJSON_GetObjectItem(gps_info,"lattype");
        if(tmp)
            all_params.gps_info.lattype = tmp->valueint;
        tmp = cJSON_GetObjectItem(gps_info,"longitude");
        if(tmp)
            all_params.gps_info.longitude = tmp->valuedouble;
        tmp = cJSON_GetObjectItem(gps_info,"lontype");
        if(tmp)
            all_params.gps_info.lontype = tmp->valueint;
        tmp = cJSON_GetObjectItem(gps_info,"satallite_num");
        if(tmp)
            all_params.gps_info.satallite_num = tmp->valueint;
        tmp = cJSON_GetObjectItem(gps_info,"utc_time");
        if(tmp)
            all_params.gps_info.utc_time = tmp->valueint;
    }

    cJSON *gsm_info = cJSON_GetObjectItem(root, "gsm_info");
    if(gsm_info)
    {
        cJSON *tmp = cJSON_GetObjectItem(gsm_info,"gsm_signal_strength");
        if(tmp)
            all_params.gsm_info.gsm_signal_strength = tmp->valueint;
        tmp = cJSON_GetObjectItem(gsm_info,"sim_card_id");
        if(tmp)
            strcpy(all_params.gsm_info.sim_card_id, tmp->valuestring);
        tmp = cJSON_GetObjectItem(gsm_info,"sim_card_imsi");
        if(tmp)
            strcpy(all_params.gsm_info.sim_card_imsi, tmp->valuestring);
        tmp = cJSON_GetObjectItem(gsm_info,"sms_number");
        if(tmp)
            strcpy(all_params.gsm_info.sms_number, tmp->valuestring);
        tmp = cJSON_GetObjectItem(gsm_info,"station_id");
        if(tmp)
            strcpy(all_params.gsm_info.station_id, tmp->valuestring);
    }

    cJSON *history_info = cJSON_GetObjectItem(root, "history_info");
    if(history_info)
    {
        cJSON *tmp = cJSON_GetObjectItem(history_info,"gprs_link_lost_time");
        if(tmp)
            all_params.history_info.gprs_link_lost_time = tmp->valueint;
        tmp = cJSON_GetObjectItem(history_info,"gps_antenna_stat");
        if(tmp)
            all_params.history_info.gps_antenna_stat = tmp->valueint;
        tmp = cJSON_GetObjectItem(history_info,"gps_fault_time");
        if(tmp)
            all_params.history_info.gps_fault_time = tmp->valueint;
        tmp = cJSON_GetObjectItem(history_info,"gps_total_mileage");
        if(tmp)
            all_params.history_info.gps_total_mileage = tmp->valueint;
        tmp = cJSON_GetObjectItem(history_info,"lid_no");
        if(tmp)
            all_params.history_info.lid_no = tmp->valueint;
        tmp = cJSON_GetObjectItem(history_info,"once_auto_lock_car_flag");
        if(tmp)
            all_params.history_info.once_auto_lock_car_flag = tmp->valueint;
        tmp = cJSON_GetObjectItem(history_info,"sim_change_card_no");
        if(tmp)
            all_params.history_info.sim_change_card_no = tmp->valueint;
        tmp = cJSON_GetObjectItem(history_info,"sim_change_flag");
        if(tmp)
            all_params.history_info.sim_change_flag = tmp->valueint;
        tmp = cJSON_GetObjectItem(history_info,"total_acc_on_time");
        if(tmp)
            all_params.history_info.total_acc_on_time = tmp->valueint;
        tmp = cJSON_GetObjectItem(history_info,"total_on_time");
        if(tmp)
            all_params.history_info.total_on_time = tmp->valueint;
    }
    cJSON *version_info = cJSON_GetObjectItem(root, "version_info");
    if(version_info)
    {
        qDebug()<<"version_info = "<<cJSON_PrintUnformatted(version_info);
        cJSON *tmp = cJSON_GetObjectItem(version_info,"fs_version");
        if(tmp)
            strncpy(all_params.version_info.fs_version, tmp->valuestring, 15);
        tmp = cJSON_GetObjectItem(version_info,"hw_version");
        if(tmp)
            strncpy(all_params.version_info.hw_version, tmp->valuestring, 15);
        tmp = cJSON_GetObjectItem(version_info,"mcu_version");
        if(tmp)
            strncpy(all_params.version_info.mcu_version, tmp->valuestring, 15);
        tmp = cJSON_GetObjectItem(version_info,"service_version");
        if(tmp)
            strncpy(all_params.version_info.service_version, tmp->valuestring, 15);
        tmp = cJSON_GetObjectItem(version_info,"housekeeper_version");
        if(tmp)
            strncpy(all_params.version_info.housekeeper_version, tmp->valuestring, 15);
        tmp = cJSON_GetObjectItem(version_info,"other_version");
        if(tmp)
            strncpy(all_params.version_info.other_version, tmp->valuestring, 63);

        cJSON *apps_name = cJSON_GetObjectItem(version_info,"apps_name");
        cJSON *apps_version = cJSON_GetObjectItem(version_info,"apps_version");
        if(apps_name && apps_name->type == cJSON_Array &&
           apps_version && apps_version->type == cJSON_Array )
        {
            if(cJSON_GetArraySize(apps_name) == cJSON_GetArraySize(apps_version))
            {
                int size = cJSON_GetArraySize(apps_name);
                if(size > 5)
                    size = 5;

                for(int i=0; i<size; i++)
                {
                    all_params.version_info.app_version[i].enable_flag = 1;

                    tmp = cJSON_GetArrayItem(apps_name,i);
                    strncpy(all_params.version_info.app_version[i].app_name, tmp->valuestring, 31);

                    tmp = cJSON_GetArrayItem(apps_version,i);
                    strncpy(all_params.version_info.app_version[i].version, tmp->valuestring, 15);
                }

            }
        }

    }
}

void HouseKeeperClient::parse_can_data(cJSON *root)
{

}

void HouseKeeperClient::parse_json_data(cJSON *root)
{

}

void HouseKeeperClient::parse_tcp_data(cJSON *root)
{

}

void HouseKeeperClient::parse_get_param_reply(cJSON *root)
{
    qDebug()<<"parse_get_param_reply:"<<cJSON_PrintUnformatted(root);
    cJSON *result = cJSON_GetObjectItem(root,"result");
    if(result && result->type == cJSON_Number)
    {
        if(result->valueint == 0)
        {
            ui->param_help->setText("获取参数失败");
        }
        else
        {
            cJSON *value = cJSON_GetObjectItem(root,"value");
            if(value && value->type == cJSON_String)
            {
                ui->param_help->setText("获取参数成功");
                ui->param_value->setText(QString(value->valuestring));
            }
            else
            {
                ui->param_help->setText("缺少关键字段");
            }
        }
    }
    else
        ui->param_help->setText("关键字段信息有误");
}

void HouseKeeperClient::parse_set_param_reply(cJSON *root)
{
    qDebug()<<"parse_set_param_reply:"<<cJSON_PrintUnformatted(root);
    cJSON *result = cJSON_GetObjectItem(root,"result");
    if(result && result->type == cJSON_Number)
    {
        if(result->valueint == 0)
        {
            ui->param_help->setText("设置参数失败");
        }
        else
        {
            ui->param_help->setText("设置参数成功");
            ui->param_value->clear();
        }
    }
    else
        ui->param_help->setText("关键字段信息有误");
}

void HouseKeeperClient::parse_get_factory_param_reply(cJSON *root)
{
    qDebug()<<"parse_get_factory_param_reply:"<<cJSON_PrintUnformatted(root);
    cJSON *result = cJSON_GetObjectItem(root,"result");
    if(result && result->type == cJSON_Number)
    {
        if(result->valueint == 0)
        {
            QMessageBox::information(this,"提示","获取配置信息失败");
        }
        else
        {
            cJSON *factory_params = cJSON_GetObjectItem(root,"factory_params");
            if(factory_params && factory_params->type == cJSON_Array)
            {
                int size = cJSON_GetArraySize(factory_params);
                qDebug()<<"factory_params size = "<<size;
                int exist_flag = 0;
                char *tmp = cJSON_GetArrayItem(factory_params,0)->valuestring;
                for(int i=0; i<ui->product_id_factory->count(); i++)
                {
                    if( !strcmp(products_id[i], tmp) )
                    {
                        exist_flag = 1;
                        ui->product_id_factory->setCurrentIndex(i);
                    }
                }
                if(exist_flag == 0)
                {
                    ui->product_id_factory->setCurrentIndex(0);
                }
                tmp = cJSON_GetArrayItem(factory_params,1)->valuestring;
                ui->sim_number_factory->setText(QString(tmp));
                tmp = cJSON_GetArrayItem(factory_params,2)->valuestring;
                ui->dev_id_factory->setText(QString(tmp));
            }
        }
    }
}

void HouseKeeperClient::parse_set_factory_param_reply(cJSON *root)
{
    qDebug()<<"parse_get_param_reply:"<<cJSON_PrintUnformatted(root);
    cJSON *result = cJSON_GetObjectItem(root,"result");
    if(result && result->type == cJSON_Number)
    {
        if(result->valueint == 0)
        {
            QMessageBox::information(this,"提示","获取配置信息失败");
        }
        else
        {
            insert_database_record(ui->dev_id_factory->text().trimmed());   //设置成功，记录至数据库中
            QMessageBox::information(this,"提示","配置成功");
            ui->product_id_factory->setCurrentIndex(0);
            ui->sim_number_factory->clear();
            ui->dev_id_factory->clear();
        }
    }
}

void HouseKeeperClient::add_time_log_to_test_result(QString str)  //添加时间到测试结果打印
{
    QString time_str = "[";
    time_str += QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    time_str += "] ";
    time_str += str;
    ui->factory_test_result->append(time_str);
}

int HouseKeeperClient::init_test_options()
{
    factory_test_record_result = 0;
    memset(&factory_test_state, 0, sizeof(factory_test_state));    //清空测试检测状态
    remind_time = ui->remind_time_spinBox->value(); //获取设置值

    int ret = 0;
    if(ui->checkBox_acc->isChecked())
    {
        ret = 1;
        factory_test_state.acc_enable = 1;
    }
    if(ui->checkBox_can_bus->isChecked())
    {
        ret = 1;
        factory_test_state.can_bus_enable = 1;
    }
    if(ui->checkBox_battery->isChecked())
    {
        ret = 1;
        factory_test_state.battery_enable = 1;
    }
    if(ui->checkBox_gps->isChecked())
    {
        ret = 1;
        factory_test_state.gps_enable = 1;
    }
    if(ui->checkBox_gps_antena->isChecked())
    {
        ret = 1;
        factory_test_state.gps_antena_enable = 1;
    }
    if(ui->checkBox_lid->isChecked())
    {
        ret = 1;
        factory_test_state.lid_enable = 1;
    }
    if(ui->checkBox_login->isChecked())
    {
        ret = 1;
        factory_test_state.login_enable = 1;
    }
    return ret;
}

void HouseKeeperClient::set_factory_test_state(int type)
{
    switch(type)
    {
    case GPS_DETECT:        //GPS检测
    {
        if(factory_test_state.gps_enable)
        {
            factory_test_state.gps_result = 1;
            QString msg = "GPS定位成功! 经度:";
            msg += QString::number(all_params.gps_info.longitude);
            msg+= " 纬度:";
            msg+= QString::number(all_params.gps_info.latitude);
            msg+= " 耗时:";
            msg+= QString::number(ui->remind_time_spinBox->value() - remind_time);
            msg+= "秒";
            add_time_log_to_test_result(msg);
        }
    }break;
    case GPS_ANTENA_DETECT:
    {
        if(factory_test_state.gps_antena_enable)
        {
            factory_test_state.gps_antena_result = 1;
            QString msg = "GPS天线功能正常！";
            msg+= " 耗时:";
            msg+= QString::number(ui->remind_time_spinBox->value() - remind_time);
            msg+= "秒";
            add_time_log_to_test_result(msg);
        }
    }break;
    case BATTERY_DETECT:
    {
        if(factory_test_state.battery_enable)
        {
            factory_test_state.battery_result = 1;
            QString msg = "电池电压值正常！";
            msg+= " 电池电压:";
            msg+= QString::number(all_params.volage_info.battery_voltage);
            msg+= "V 耗时:";
            msg+= QString::number(ui->remind_time_spinBox->value() - remind_time);
            msg+= "秒";
            add_time_log_to_test_result(msg);
        }
    }break;
    case ACC_TEST:
    {
        if(factory_test_state.acc_enable)
        {
            factory_test_state.acc_result = 1;
            QString msg = "ACC检测功能正常！";
            msg+= " 耗时:";
            msg+= QString::number(ui->remind_time_spinBox->value() - remind_time);
            msg+= "秒";
            add_time_log_to_test_result(msg);
        }
    }break;
    case LID_TEST:
    {
        if(factory_test_state.lid_enable)
        {
            factory_test_state.lid_result = 1;
            QString msg = "开盖检测功能正常！";
            msg+= " 耗时:";
            msg+= QString::number(ui->remind_time_spinBox->value() - remind_time);
            msg+= "秒";
            add_time_log_to_test_result(msg);
        }
    }break;
    case CAN_BUS_DETECT:
    {
        if(factory_test_state.can_bus_enable)
        {
            factory_test_state.can_bus_result = 1;
            QString msg = "CAN总线功能正常！";
            msg+= " 耗时:";
            msg+= QString::number(ui->remind_time_spinBox->value() - remind_time);
            msg+= "秒";
            add_time_log_to_test_result(msg);
        }
    }break;
    case LOGIN_DETECT:
    {
        if(factory_test_state.login_enable)
        {
            factory_test_state.login_result = 1;
            QString msg = "登陆平台成功！";
            msg+= " 耗时:";
            msg+= QString::number(ui->remind_time_spinBox->value() - remind_time);
            msg+= "秒";
            add_time_log_to_test_result(msg);
        }
    }break;
    default:break;
    }
}

void HouseKeeperClient::all_params_proc_for_test()
{
    static int pre_acc_state = 0;
    static int pre_lid_state = 0;
    if(!factory_test_timer->isActive())     //未开始测试不进行处理
    {
        pre_acc_state = 0;
        pre_lid_state = 0;
        return;
    }
    if(all_params.state_info.login_state == 1 && factory_test_state.login_result == 0)  //登陆状态检测
        set_factory_test_state(LOGIN_DETECT);

    if(all_params.state_info.gps_state == 1 && factory_test_state.gps_result == 0)  //GPS定位检测
        set_factory_test_state(GPS_DETECT);

    if(all_params.state_info.gps_antena == 1 && factory_test_state.gps_antena_result == 0)  //GPS天线检测 1正常 2天线未连接 3天线故障
        set_factory_test_state(GPS_ANTENA_DETECT);

    if(all_params.volage_info.battery_voltage >= 3.8 && factory_test_state.battery_result == 0)  //电池电压大于等于3.8V
        set_factory_test_state(BATTERY_DETECT);

    if(pre_acc_state != all_params.state_info.acc_state && pre_acc_state != 0)      //ACC检测测试成功
        set_factory_test_state(ACC_TEST);

    if(pre_lid_state != all_params.state_info.liq_state && pre_lid_state != 0)      //开盖检测测试成功
        set_factory_test_state(LID_TEST);

    pre_acc_state = all_params.state_info.acc_state;
    pre_lid_state = all_params.state_info.liq_state;
}

void HouseKeeperClient::candata_proc_for_test()
{
    if(!factory_test_timer->isActive())     //未开始测试不进行处理
        return;

    if(factory_test_state.can_bus_result == 0)
        set_factory_test_state(CAN_BUS_DETECT);
}

void HouseKeeperClient::init_database_record()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "sqlite3");
    if( isFileExist("./database.db") )  //数据库文件存在
    {
        db.setDatabaseName("./database.db");
        db.open();  //打开数据库文件
        qDebug() <<"打开数据库成功！";
    }
    else
    {
        db.setDatabaseName("./database.db");
        db.open();  //打开数据库文件
        QSqlQuery query(db);
        bool success = query.exec("CREATE TABLE ID_RECORD("
                                  "DEV_ID CHAR(32));");            //创建表
        if(success)
            qDebug() <<"数据库表创建成功！";
        else
            qDebug() <<"数据库表创建失败！";
    }
}

bool HouseKeeperClient::check_database_record(QString devid) //检查数据库中记录
{
    int exist_flag = 0;
    QSqlDatabase db = QSqlDatabase::database("sqlite3"); //建立数据库连接
    QSqlQuery query(db);
    query.exec("select * from ID_RECORD");
    QSqlRecord rec = query.record();
    qDebug() << "ID_RECORD表字段数："<< rec.count();
    while(query.next())
    {
        if(devid == query.value(0).toString())  //如果ID已经烧写过
        {
            qDebug() <<"DEVID:"<<query.value(0).toString();
            exist_flag = 1;
            break;
        }
    }

    if(exist_flag)  //已经烧写过
    {
        int ret = QMessageBox::information(this,"提示","该设备ID已经烧录至其他设备，是否继续烧录？",QMessageBox::Yes|QMessageBox::No,QMessageBox::No); //
        qDebug()<<"Yes = "<<QMessageBox::Yes<<endl<<"NO:"<<QMessageBox::No;
        if(ret == QMessageBox::Yes) //继续烧录
            return true;
        else if(ret == QMessageBox::No)
            return false;

    }
    else        //没有烧写过
        return true;
}

void HouseKeeperClient::insert_database_record(QString devid)
{
    QSqlDatabase db = QSqlDatabase::database("sqlite3"); //建立数据库连接
    QSqlQuery query(db);
    query.exec("select * from ID_RECORD");
    QSqlRecord rec = query.record();
    qDebug() << "ID_RECORD表字段数："<< rec.count();
    while(query.next())
    {
        if(devid == query.value(0).toString())  //如果ID已经烧写过
        {
            qDebug() <<"DEVID:"<<query.value(0).toString();
            return;
        }
    }

    query.prepare("INSERT INTO ID_RECORD VALUES ( ? );");
    query.bindValue(0, devid);
    bool ret=query.exec();
    if(!ret)
    {
        QSqlError lastError = query.lastError();
        qDebug() << lastError.driverText() << QString(QObject::tr("插入失败"));
    }else
        qDebug() <<"插入成功";
}

void HouseKeeperClient::update_time()
{
    QString time_str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->now_time->setText(time_str);
    if(all_params.state_info.dial_state && tcp_connect_flag)
        ui->net_status->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/网络正常.png);"));
    else
        ui->net_status->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/网络断开.png);"));
}

void HouseKeeperClient::factory_test_check()    //参数检测
{
    if(remind_time > 0)
    {
        if(factory_test_state.acc_enable == factory_test_state.acc_result &&
           factory_test_state.battery_enable == factory_test_state.battery_result &&
           factory_test_state.gps_antena_enable == factory_test_state.gps_antena_result &&
           factory_test_state.can_bus_enable == factory_test_state.can_bus_enable &&
           factory_test_state.gps_enable == factory_test_state.gps_result  &&
           factory_test_state.lid_enable == factory_test_state.lid_result &&
           factory_test_state.login_enable == factory_test_state.login_result)  //测试成功
        {
            add_time_log_to_test_result(QString("出厂测试成功"));
            if(ui->factory_test_bt->text()=="结束测试")
                on_factory_test_bt_clicked();    //结束测试
            QMessageBox::information(this,"提示","出厂测试成功");
            factory_test_record_result = 1;
//            if(ui->checkBox_autoclean->isChecked())  //如果开启自动清空保存
//            {
//                on_factory_test_clear_clicked(); //清空并保持测试结果
//            }
            return;
        }
        remind_time--;
        ui->remind_time_label->setText(QString::number(remind_time));
    }
    else            //失败，测试超时
    {
        if(ui->factory_test_bt->text()=="结束测试")
            on_factory_test_bt_clicked();    //结束测试

        QString msg;
        if(factory_test_state.acc_enable && (!factory_test_state.acc_result))
        {
            msg = "ACC测试失败！";
            add_time_log_to_test_result(msg);
        }
        if(factory_test_state.gps_enable && (!factory_test_state.gps_result))
        {
            msg = "GPS定位失败！";
            add_time_log_to_test_result(msg);
        }
        if(factory_test_state.gps_antena_enable && (!factory_test_state.gps_antena_result))
        {
            msg = "GPS天线未连接！";
            add_time_log_to_test_result(msg);
        }
        if(factory_test_state.battery_enable && (!factory_test_state.battery_result) )
        {
            msg = "电池电压检测失败！电压值:";
            msg += QString::number(all_params.volage_info.battery_voltage);
            msg += "V";
            add_time_log_to_test_result(msg);
        }
        if(factory_test_state.lid_enable && (!factory_test_state.lid_result) )
        {
            msg = "开盖测试失败！";
            add_time_log_to_test_result(msg);
        }
        if(factory_test_state.can_bus_enable && (!factory_test_state.can_bus_result) )
        {
            msg = "CAN总线检测失败！";
            add_time_log_to_test_result(msg);
        }
        if(factory_test_state.login_enable && (!factory_test_state.login_result))
        {
            msg = "登录平台超时！";
            add_time_log_to_test_result(msg);
        }

        msg = "出厂测试失败！";
        add_time_log_to_test_result(msg);
        QMessageBox::information(this,"提示","出厂测试失败，时间超时");
        if(ui->checkBox_autoclean->isChecked())  //如果开启自动清空保存
        {
            on_factory_test_clear_clicked(); //清空并保持测试结果
        }
    }
}

void HouseKeeperClient::update_network_connect_state(int state)
{
    if (!state)
    {
        network_connect_state = 0;
    }
    else
    {
        network_connect_state = 1;
    }
}

void HouseKeeperClient::ParseAppData(QByteArray package_data) //解析数据
{
//    qDebug()<<"ParseAppData";
    char *json_str = package_data.data();
    cJSON *root = cJSON_Parse(json_str);
    if(root)
    {
        cJSON *data_type = cJSON_GetObjectItem(root,"data_type");
        if(data_type && data_type->type == cJSON_String)
        {
            if( !strcmp(data_type->valuestring, "PARAMS") )
            {
                parse_all_params_data(root);

                on_param_type_currentIndexChanged(ui->param_type->currentIndex());

                all_params_proc_for_test();     //根据所有参数进行测试
            }
            else if(!strcmp(data_type->valuestring, "CAN"))
            {
                parse_can_data(root);
            }
            else if(!strcmp(data_type->valuestring, "JSON"))
            {
                cJSON *json_data = cJSON_GetObjectItem(root,"json_data");
                if(json_data)
                    parse_json_data(json_data);
            }
            else if(!strcmp(data_type->valuestring, "TCP"))
            {
                parse_tcp_data(root);
            }
            else if( !strcmp(data_type->valuestring, "param_get_reply") )   //读取参数回复
            {
                parse_get_param_reply(root);
            }
            else if( !strcmp(data_type->valuestring, "param_set_reply") )   //设置参数回复
            {
                parse_set_param_reply(root);
            }
            else if( !strcmp(data_type->valuestring, "factory_params_get_reply"))
            {
                parse_get_factory_param_reply(root);
            }
            else if( !strcmp(data_type->valuestring, "factory_params_set_reply"))
            {
                parse_set_factory_param_reply(root);
            }
            else
            {
                qDebug("unknow json:%s",cJSON_PrintUnformatted(root));
            }
        }
        cJSON_Delete(root);
    }
}

void HouseKeeperClient::recv_tcp_data()
{
    qDebug()<<"rcv tcp data...."<<tcp_client->bytesAvailable()<<"bytes";
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
                qDebug()<<"package_data len = "<<package_data.length();
                ParseAppData(package_data); //解析数据
                package_data.clear();
                buff_index = 0;  //下标清零
            }
        }
        else
        {
            if (buff_index < MAX_MESSAGE_LEN-1) //json对象超出长度
            {
                package_data += QByteArray(&ch, 1) ; //读取数据
                buff_index++;
            }
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
    tcp_connect_flag = 1;
}

void HouseKeeperClient::tcp_client_disconnected()
{
    qDebug()<<"tcp_client disconnect!!";
    tcp_connect_flag = 0;
}

void HouseKeeperClient::update_usb_connect_state(int state)
{
    static int cnt_connect = 0;
    static int pre_state = 0;
    if (!state)
    {
        ui->usb_connect_state->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/断开链接.png);"));
        usb_connect_state = 0;
        if(pre_state != state)  //断开连接
        {
            tcp_client->close();
            tcp_connect_flag = 0;

            if(factory_test_timer->isActive())  //测试时连接断开
            {
                QString msg;
                if(factory_test_state.acc_enable && factory_test_state.acc_result == 0)
                {
                    msg = "ACC测试失败！";
                    add_time_log_to_test_result(msg);
                }
                if(factory_test_state.gps_enable && factory_test_state.gps_result  == 0 )
                {
                    msg = "GPS定位失败！";
                    add_time_log_to_test_result(msg);
                }
                if(factory_test_state.gps_antena_enable && factory_test_state.gps_antena_result == 0)
                {
                    if(all_params.state_info.gps_antena == 2)
                        msg = "GPS天线未连接！";
                    else
                        msg = "GPS天线故障！";
                    add_time_log_to_test_result(msg);
                }
                if(factory_test_state.battery_enable && factory_test_state.battery_result == 0 )
                {
                    msg = "电池电压检测失败！电压值:";
                    msg += QString::number(all_params.volage_info.battery_voltage);
                    msg += "V";
                    add_time_log_to_test_result(msg);
                }
                if(factory_test_state.lid_enable && factory_test_state.lid_result == 0 )
                {
                    msg = "开盖测试失败！";
                    add_time_log_to_test_result(msg);
                }
                if(factory_test_state.can_bus_enable && factory_test_state.can_bus_result == 0 )
                {
                    msg = "CAN总线检测失败！";
                    add_time_log_to_test_result(msg);
                }
                if(factory_test_state.login_enable && factory_test_state.login_result == 0 )
                {
                    msg = "登录平台超时！";
                    add_time_log_to_test_result(msg);
                }
                add_time_log_to_test_result(QString("出厂测试失败,USB连接断开"));
                if(ui->factory_test_bt->text()=="结束测试")
                    on_factory_test_bt_clicked();    //结束测试
                QMessageBox::information(this,"提示","出厂测试失败,USB连接断开");
                if(ui->checkBox_autoclean->isChecked())  //如果开启自动清空保存
                {
                    on_factory_test_clear_clicked(); //清空并保持测试结果
                }
            }
        }
    }
    else
    {
        if(tcp_connect_flag)
        {
            ui->usb_connect_state->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/TCP连接.png);"));
            cnt_connect=0;
        }
        else
        {
            ui->usb_connect_state->setStyleSheet(QString("border-image: url(:/new/prefix1/pictures/连接.png);"));
            cnt_connect++;
        }
        usb_connect_state = 1;
        if(pre_state != state)  //建立连接
        {
            qDebug()<<"建立TCP连接...";
            tcp_client->connectToHost("127.0.0.1", 10086, QTcpSocket::ReadWrite);
            pre_state = state;  //记录本次状态
            if(ui->checkBox_autotest->isChecked())  //如果开启自动测试
            {
                if(ui->factory_test_bt->text() == "开始测试")
                    on_factory_test_bt_clicked();    //开始测试
            }
        }

        if(cnt_connect > 5)
        {
            qDebug()<<"再次尝试建立TCP连接...";
            tcp_client->connectToHost("127.0.0.1", 10086, QTcpSocket::ReadWrite);
            cnt_connect = 0;
        }
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
    pre_state = state;  //记录本次状态
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
    if(ui->stackedWidget->currentIndex() == DEV_ID_WRITE) //返回则清空
    {
        ui->product_id_factory->setCurrentIndex(0);
        ui->sim_number_factory->clear();
        ui->dev_id_factory->clear();
    }

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
    ui->param_type->setCurrentIndex(9);
}

void HouseKeeperClient::on_upgrade_package_clicked()
{
    if(root_permission)
        ui->stackedWidget->setCurrentIndex(UPGRADE_PACKAGE);
    else
        QMessageBox::information(this,"提示","需获取管理员权限");

}

void HouseKeeperClient::on_param_type_currentIndexChanged(int index)
{
    int param_type = 0;
    switch(index)
    {
        case 0:
            param_type = ENUM_VOLTAGE_INFO;
            break;
        case 1:
            param_type = ENUM_DEVICE_INFO;
            break;
        case 2:
            param_type = ENUM_TCP_CONNECT_INFO;
            break;
        case 3:
            param_type = ENUM_REPORT_TIME_INFO;
            break;
        case 4:
            param_type = ENUM_STATE_INFO;
            break;
        case 5:
            param_type = ENUM_BAUTRATE_INFO;
            break;
        case 6:
            param_type = ENUM_GPS_INFO;
            break;
        case 7:
            param_type = ENUM_GSM_INFO;
            break;
        case 8:
            param_type = ENUM_HISTORY_INFO;
            break;
        case 9:
            param_type = ENUM_VERSION_INFO;
            break;
    }
    show_params(param_type); //显示参数
}

void HouseKeeperClient::on_ini_filename_currentIndexChanged(int index)
{
    if(index == 0)      //syscfg.ini
    {
        ui->selection_name->clear();
        ui->selection_name->addItem(QString("INIT_PARAM_THEME"));
        ui->selection_name->addItem(QString("VERSION_THEME"));
        ui->selection_name->addItem(QString("POWER_THEME"));
        ui->selection_name->addItem(QString("DEV_BAUDRATE_THEME"));
        ui->selection_name->addItem(QString("REPORT_TIME_THEME"));
    }
    else if(index == 1) //remote_manage.ini
    {
        ui->selection_name->clear();
        ui->selection_name->addItem(QString("MQTT_CFG"));
        ui->selection_name->addItem(QString("OTHER_CFG"));
    }
    else if(index == 2) //zlcfg.ini
    {
        ui->selection_name->clear();
        ui->selection_name->addItem(QString("TCPC_CFG_THEME"));
    }
    else if(index == 3) //infocfg.ini
    {
        ui->selection_name->clear();
        ui->selection_name->addItem(QString("INFO_THEME"));
    }
}

void HouseKeeperClient::on_selection_name_currentIndexChanged(const QString &arg1)
{
    if(arg1 == "INIT_PARAM_THEME")
    {
        ui->key_name->clear();
        ui->key_name->addItem(QString("dev_id"));
        ui->key_name->addItem(QString("terminal_code"));
        ui->key_name->addItem(QString("tcp_svr_port"));
        ui->key_name->addItem(QString("tcp_client_port"));
        ui->key_name->addItem(QString("tcp_remote_svr_ip"));
        ui->key_name->addItem(QString("log_level"));
        ui->key_name->addItem(QString("print_debug_log"));
        ui->key_name->addItem(QString("print_log_place"));
        ui->key_name->addItem(QString("log_file_size"));
        ui->key_name->addItem(QString("log_file_name"));
        ui->key_name->addItem(QString("sqlite_data_max_no"));
        ui->key_name->addItem(QString("sqlite_file_name"));
    }
    else if(arg1 == "VERSION_THEME")
    {
        ui->key_name->clear();
        ui->key_name->addItem(QString("fs_version"));
        ui->key_name->addItem(QString("hw_version"));
    }
    else if(arg1 == "POWER_THEME")
    {
        ui->key_name->clear();
        ui->key_name->addItem(QString("enable_sleep_mode"));
        ui->key_name->addItem(QString("mwake_cycle"));
        ui->key_name->addItem(QString("mvcc_mid_vol"));
        ui->key_name->addItem(QString("mvcc_low_vol"));
        ui->key_name->addItem(QString("battery_off_vol"));
        ui->key_name->addItem(QString("sleep_wakeup_interval"));
    }
    else if(arg1 == "DEV_BAUDRATE_THEME")
    {
        ui->key_name->clear();
        ui->key_name->addItem(QString("can0_bitrate"));
        ui->key_name->addItem(QString("can1_bitrate"));
        ui->key_name->addItem(QString("rs485_baud_rate"));
        ui->key_name->addItem(QString("rs232_baud_rate"));
    }
    else if(arg1 == "REPORT_TIME_THEME")
    {
        ui->key_name->clear();
        ui->key_name->addItem(QString("heartbeat_alarm_interval"));
        ui->key_name->addItem(QString("can_upload_interval"));
        ui->key_name->addItem(QString("rs485_upload_interval"));
        ui->key_name->addItem(QString("location_report_interval"));
    }
    else if(arg1 == "MQTT_CFG")
    {
        ui->key_name->clear();
        ui->key_name->addItem(QString("host"));
        ui->key_name->addItem(QString("port"));
        ui->key_name->addItem(QString("heartbeat"));
        ui->key_name->addItem(QString("username"));
        ui->key_name->addItem(QString("password"));
        ui->key_name->addItem(QString("ssl_enable"));
    }
    else if(arg1 == "OTHER_CFG")
    {
        ui->key_name->clear();
        ui->key_name->addItem(QString("product_id"));
        ui->key_name->addItem(QString("activation_state"));
    }
    else if(arg1 == "TCPC_CFG_THEME")
    {
        ui->key_name->clear();
        ui->key_name->addItem(QString("dev_sn"));
        ui->key_name->addItem(QString("dev_type"));
        ui->key_name->addItem(QString("dev_num"));
        ui->key_name->addItem(QString("gps_fault_days"));
        ui->key_name->addItem(QString("aa_alarm_interval"));
        ui->key_name->addItem(QString("gsm_gprs_weak_hours"));
        ui->key_name->addItem(QString("auto_report_localtion_sec"));
        ui->key_name->addItem(QString("can_sample_interval"));
        ui->key_name->addItem(QString("once_auto_lock_flag"));
        ui->key_name->addItem(QString("server_ip_port"));
        ui->key_name->addItem(QString("apn"));
        ui->key_name->addItem(QString("udp_tcp_flag"));
        ui->key_name->addItem(QString("can_baudrate"));
        ui->key_name->addItem(QString("can_filter_mask"));
        ui->key_name->addItem(QString("can_filter_mask_min"));
        ui->key_name->addItem(QString("can_filter_mask_max"));
        ui->key_name->addItem(QString("can_120res"));
        ui->key_name->addItem(QString("plc_ctr_output"));
        ui->key_name->addItem(QString("plc_power_detect"));
        ui->key_name->addItem(QString("dead_zone_auto_report"));
        ui->key_name->addItem(QString("home_abroad"));
        ui->key_name->addItem(QString("pulse_setable"));
        ui->key_name->addItem(QString("remote_upgrade"));
        ui->key_name->addItem(QString("sim_imis"));
        ui->key_name->addItem(QString("sim_num"));
        ui->key_name->addItem(QString("call_center_num"));
        ui->key_name->addItem(QString("sms_num"));
        ui->key_name->addItem(QString("gsm_signal_strength"));
        ui->key_name->addItem(QString("satellite_num"));
        ui->key_name->addItem(QString("gps_total_odo_initial"));
        ui->key_name->addItem(QString("power_save"));
        ui->key_name->addItem(QString("overspeed_thr"));
        ui->key_name->addItem(QString("keepalive_ctr"));
        ui->key_name->addItem(QString("mainpower_vth"));
        ui->key_name->addItem(QString("backpower_on_interval"));
    }
    else if(arg1 == "INFO_THEME")
    {
        ui->key_name->clear();
        ui->key_name->addItem(QString("sim_id"));
        ui->key_name->addItem(QString("sim_change_card_no"));
        ui->key_name->addItem(QString("acc_on_total_time"));
        ui->key_name->addItem(QString("total_mileage"));
        ui->key_name->addItem(QString("power_on_time"));
        ui->key_name->addItem(QString("lid_no"));
        ui->key_name->addItem(QString("once_lid_no"));
        ui->key_name->addItem(QString("once_sim_change"));
        ui->key_name->addItem(QString("gsm_signal_weak_time"));
        ui->key_name->addItem(QString("gps_antenna"));
        ui->key_name->addItem(QString("once_gps_ant"));
        ui->key_name->addItem(QString("autolock_times"));
        ui->key_name->addItem(QString("gps_fault_time"));
        ui->key_name->addItem(QString("last_longitude"));
        ui->key_name->addItem(QString("last_latitude"));
    }
}

void HouseKeeperClient::on_key_name_currentTextChanged(const QString &arg1)
{
    if(arg1 == QString("dev_id"))
        ui->key_name_help->setText("设备ID");
    else if(arg1 == QString("terminal_code"))
        ui->key_name_help->setText("设备类型编号");
    else if(arg1 == QString("tcp_svr_port"))
        ui->key_name_help->setText("盒子作为TCP服务端监听的端口");
    else if(arg1 == QString("tcp_client_port"))
        ui->key_name_help->setText("盒子作为TCP客户端连接的端口");
    else if(arg1 == QString("tcp_remote_svr_ip"))
        ui->key_name_help->setText("远程服务器IP地址");
    else if(arg1 == QString("log_level"))
        ui->key_name_help->setText("日志打印级别");
    else if(arg1 == QString("print_debug_log"))
        ui->key_name_help->setText("Debug日志打印开关");
    else if(arg1 == QString("print_log_place"))
        ui->key_name_help->setText("日志输出位置，1文件；0终端");
    else if(arg1 == QString("log_file_size"))
        ui->key_name_help->setText("日志文件大小上限");
    else if(arg1 == QString("log_file_name"))
        ui->key_name_help->setText("日志文件默认路径");
    else if(arg1 == QString("sqlite_data_max_no"))
        ui->key_name_help->setText("数据库存储数据项上限");
    else if(arg1 == QString("sqlite_file_name"))
        ui->key_name_help->setText("数据库文件默认路径");
    else if(arg1 == QString("fs_version"))
        ui->key_name_help->setText("固件版本号");
    else if(arg1 == QString("hw_version"))
        ui->key_name_help->setText("硬件版本号");
    else if(arg1 == QString("enable_sleep_mode"))
        ui->key_name_help->setText("休眠使能开关");
    else if(arg1 == QString("mwake_cycle"))
        ui->key_name_help->setText("低功耗唤醒周期，单位:分钟");
    else if(arg1 == QString("mvcc_mid_vol"))
        ui->key_name_help->setText("进入低功耗阈值，24.1->241");
    else if(arg1 == QString("mvcc_low_vol"))
        ui->key_name_help->setText("休眠阈值");
    else if(arg1 == QString("battery_off_vol"))
        ui->key_name_help->setText("电池断电电压阈值");
    else if(arg1 == QString("sleep_wakeup_interval"))
        ui->key_name_help->setText("休眠唤醒周期，单位:分钟");
    else if(arg1 == QString("can0_bitrate"))
        ui->key_name_help->setText("CAN0波特率");
    else if(arg1 == QString("can1_bitrate"))
        ui->key_name_help->setText("CAN1波特率");
    else if(arg1 == QString("rs485_baud_rate"))
        ui->key_name_help->setText("485波特率");
    else if(arg1 == QString("rs232_baud_rate"))
        ui->key_name_help->setText("232波特率");
    else if(arg1 == QString("heartbeat_alarm_interval"))
        ui->key_name_help->setText("tcp连接断开报警周期，类似AA报警，单位:分钟");
    else if(arg1 == QString("can_upload_interval"))
        ui->key_name_help->setText("CAN总线数据上报周期，单位:秒");
    else if(arg1 == QString("rs485_upload_interval"))
        ui->key_name_help->setText("RS485总线数据上报周期，单位:秒");
    else if(arg1 == QString("location_report_interval"))
        ui->key_name_help->setText("位置上报周期，单位:秒");
    else if(arg1 == QString("host"))
        ui->key_name_help->setText("MQTT服务器主机地址");
    else if(arg1 == QString("port"))
        ui->key_name_help->setText("MQTT服务器端口号");
    else if(arg1 == QString("heartbeat"))
        ui->key_name_help->setText("MQTT连接心跳周期");
    else if(arg1 == QString("username"))
        ui->key_name_help->setText("MQTT服务器连接用户名");
    else if(arg1 == QString("password"))
        ui->key_name_help->setText("MQTT服务器连接密码");
    else if(arg1 == QString("ssl_enable"))
        ui->key_name_help->setText("SSL加密使能标志");
    else if(arg1 == QString("product_id"))
        ui->key_name_help->setText("产品ID");
    else if(arg1 == QString("activation_state"))
        ui->key_name_help->setText("设备激活状态");
    else if(arg1 == QString("dev_sn"))
        ui->key_name_help->setText("设备用户码");
    else if(arg1 == QString("dev_type"))
        ui->key_name_help->setText("设备类型");
    else if(arg1 == QString("dev_num"))
        ui->key_name_help->setText("设备编号");
    else if(arg1 == QString("gps_fault_days"))
        ui->key_name_help->setText("GPS故障天数");
    else if(arg1 == QString("aa_alarm_interval"))
        ui->key_name_help->setText("0xAA报警时间设置(min)");
    else if(arg1 == QString("gsm_gprs_weak_hours"))
        ui->key_name_help->setText("GSM/GPRS信号弱报警小时");
    else if(arg1 == QString("auto_report_localtion_sec"))
        ui->key_name_help->setText("自动上报位置信息时长(sec)");
    else if(arg1 == QString("can_sample_interval"))
        ui->key_name_help->setText("CAN总线工况数据上报周期(sec)");
    else if(arg1 == QString("once_auto_lock_flag"))
        ui->key_name_help->setText("曾自动锁车标志");
    else if(arg1 == QString("server_ip_port"))
        ui->key_name_help->setText("服务器IP地址、端口号");
    else if(arg1 == QString("apn"))
        ui->key_name_help->setText("运营商类型");
    else if(arg1 == QString("udp_tcp_flag"))
        ui->key_name_help->setText("0:UDP,1:TCP,只读");
    else if(arg1 == QString("can_baudrate"))
        ui->key_name_help->setText("设置CAN通信波特率,只读");
    else if(arg1 == QString("can_filter_mask"))
        ui->key_name_help->setText("can数据过滤掩码");
    else if(arg1 == QString("can_filter_mask_min"))
        ui->key_name_help->setText("can数据过滤掩码最小值");
    else if(arg1 == QString("can_filter_mask_max"))
        ui->key_name_help->setText("can数据过滤掩码最大值");
    else if(arg1 == QString("can_120res"))
        ui->key_name_help->setText("是否停止can口120欧电阻");
    else if(arg1 == QString("plc_ctr_output"))
        ui->key_name_help->setText("各输出点是否受PLC控制");
    else if(arg1 == QString("plc_power_detect"))
        ui->key_name_help->setText("使用哪个输入口作为PLC上电检查");
    else if(arg1 == QString("dead_zone_auto_report"))
        ui->key_name_help->setText("盲区自动上报");
    else if(arg1 == QString("home_abroad"))
        ui->key_name_help->setText("出口版/国内版");
    else if(arg1 == QString("pulse_setable"))
        ui->key_name_help->setText("脉冲可选");
    else if(arg1 == QString("remote_upgrade"))
        ui->key_name_help->setText("远程升级功能");
    else if(arg1 == QString("sim_imis"))
        ui->key_name_help->setText("SIM卡的唯一识别码");
    else if(arg1 == QString("sim_num"))
        ui->key_name_help->setText("本终端SIM卡号");
    else if(arg1 == QString("call_center_num"))
        ui->key_name_help->setText("呼叫中心号码");
    else if(arg1 == QString("sms_num"))
        ui->key_name_help->setText("短信报警号码");
    else if(arg1 == QString("gsm_signal_strength"))
        ui->key_name_help->setText("GSM信号强度");
    else if(arg1 == QString("satellite_num"))
        ui->key_name_help->setText("GPS卫星颗数");
    else if(arg1 == QString("gps_total_odo_initial"))
        ui->key_name_help->setText("GPS初始里程");
    else if(arg1 == QString("power_save"))
        ui->key_name_help->setText("省电模式的开启、关闭");
    else if(arg1 == QString("overspeed_thr"))
        ui->key_name_help->setText("超速阈值");
    else if(arg1 == QString("keepalive_ctr"))
        ui->key_name_help->setText("心跳控制");
    else if(arg1 == QString("mainpower_vth"))
        ui->key_name_help->setText("主电源欠压值");
    else if(arg1 == QString("backpower_on_interval"))
        ui->key_name_help->setText("启用备用电源后上报周期(hour)");
    else if(arg1 == QString("sim_id"))
        ui->key_name_help->setText("SIM卡ID号");
    else if(arg1 == QString("sim_change_card_no"))
        ui->key_name_help->setText("SIM卡更换次数");
    else if(arg1 == QString("acc_on_total_time"))
        ui->key_name_help->setText("ACC点火总时长");
    else if(arg1 == QString("total_mileage"))
        ui->key_name_help->setText("GPS总里程数");
    else if(arg1 == QString("power_on_time"))
        ui->key_name_help->setText("电源通电总时长");
    else if(arg1 == QString("lid_no"))
        ui->key_name_help->setText("开盖次数");
    else if(arg1 == QString("once_lid_no"))
        ui->key_name_help->setText("曾开盖标志");
    else if(arg1 == QString("once_sim_change"))
        ui->key_name_help->setText("曾拔卡标志");
    else if(arg1 == QString("gsm_signal_weak_time"))
        ui->key_name_help->setText("信号弱时长");
    else if(arg1 == QString("gps_antenna"))
        ui->key_name_help->setText("GPS天线状态");
    else if(arg1 == QString("once_gps_ant"))
        ui->key_name_help->setText("GPS曾拔插标志");
    else if(arg1 == QString("autolock_times"))
        ui->key_name_help->setText("信号弱锁车次数");
    else if(arg1 == QString("gps_fault_time"))
        ui->key_name_help->setText("GPS故障时长");
    else if(arg1 == QString("last_longitude"))
        ui->key_name_help->setText("上次GPS定位经度");
    else if(arg1 == QString("last_latitude"))
        ui->key_name_help->setText("上次GPS定位纬度");
}

void HouseKeeperClient::on_param_read_clicked()
{
    if(tcp_connect_flag)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root,"data_type","get_param");  //获取参数
        QString ini_path = QString("/usrdata/service/etc/") + ui->ini_filename->currentText().trimmed();
        cJSON_AddStringToObject(root,"ini_name",ini_path.toLocal8Bit().data());  //获取参数
        cJSON_AddStringToObject(root,"selection_name",ui->selection_name->currentText().trimmed().toLocal8Bit().data());  //获取参数
        cJSON_AddStringToObject(root,"key_name",ui->key_name->currentText().trimmed().toLocal8Bit().data());  //获取参数
        char *data = cJSON_PrintUnformatted(root);
        qDebug("get_param = %s",data);
        tcp_client->write(data, strlen(data));
        cJSON_Delete(root);
    }else
    {
        QMessageBox::information(this,"提示","未与终端建立通信,请等待建立通信后重试！建立通信后左上角USB连接状态图标将变为浅绿色。");
    }
}

void HouseKeeperClient::on_param_set_clicked()
{
    if(ui->param_value->text().isEmpty())
    {
        QMessageBox::information(this,"提示","参数值不能为空");
        return;
    }
    if(tcp_connect_flag)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root,"data_type","set_param");  //设置参数
        QString ini_path = QString("/usrdata/service/etc/") + ui->ini_filename->currentText().trimmed();
        cJSON_AddStringToObject(root,"ini_name",ini_path.toLocal8Bit().data());
        cJSON_AddStringToObject(root,"selection_name",ui->selection_name->currentText().trimmed().toLocal8Bit().data());
        cJSON_AddStringToObject(root,"key_name",ui->key_name->currentText().trimmed().toLocal8Bit().data());
        cJSON_AddStringToObject(root,"value",ui->param_value->text().trimmed().toLocal8Bit().data());
        char *data = cJSON_PrintUnformatted(root);
        qDebug("set_param = %s",data);
        tcp_client->write(data, strlen(data));
        cJSON_Delete(root);
    }else
    {
        QMessageBox::information(this,"提示","未与终端建立通信,请等待建立通信后重试！建立通信后左上角USB连接状态图标将变为浅绿色。");
    }
}

void HouseKeeperClient::on_tabWidget_tabBarClicked(int index)
{
    if(index == 1)
    {
        on_ini_filename_currentIndexChanged(ui->ini_filename->currentIndex());
    }
}

void HouseKeeperClient::on_get_all_params_clicked()
{
    if(tcp_connect_flag)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root,"data_type","ask_params");  //设置参数
        char *data = cJSON_PrintUnformatted(root);
        qDebug("ask_params = %s",data);
        tcp_client->write(data, strlen(data));
        cJSON_Delete(root);
    }else
    {
        QMessageBox::information(this,"提示","未与终端建立通信,请等待建立通信后重试！建立通信后左上角USB连接状态图标将变为浅绿色。");
    }
}

void HouseKeeperClient::on_get_id_setting_clicked()
{
    if(tcp_connect_flag)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root,"data_type","factory_params_get");  //获取出厂配置参数
        int nums = 3;
        cJSON_AddNumberToObject(root,"param_nums", nums);  //参数个数
        cJSON *ini_array = cJSON_CreateArray();
        cJSON *selection_array = cJSON_CreateArray();
        cJSON *key_array = cJSON_CreateArray();

        cJSON_AddItemToArray(ini_array, cJSON_CreateString("/usrdata/service/etc/remote_manage.ini"));
        cJSON_AddItemToArray(selection_array, cJSON_CreateString("OTHER_CFG"));
        cJSON_AddItemToArray(key_array, cJSON_CreateString("product_id"));

        cJSON_AddItemToArray(ini_array, cJSON_CreateString("/usrdata/service/etc/zlcfg.ini"));
        cJSON_AddItemToArray(selection_array, cJSON_CreateString("TCPC_CFG_THEME"));
        cJSON_AddItemToArray(key_array, cJSON_CreateString("sim_num"));

        cJSON_AddItemToArray(ini_array, cJSON_CreateString("/usrdata/service/etc/syscfg.ini"));
        cJSON_AddItemToArray(selection_array, cJSON_CreateString("INIT_PARAM_THEME"));
        cJSON_AddItemToArray(key_array, cJSON_CreateString("dev_id"));

        cJSON_AddItemToObject(root, "ini_name", ini_array);     //添加数组
        cJSON_AddItemToObject(root, "selection_name", selection_array);
        cJSON_AddItemToObject(root, "key_name", key_array);

        char *data = cJSON_PrintUnformatted(root);
        qDebug("factory_params_get = %s",data);
        tcp_client->write(data, strlen(data));
        cJSON_Delete(root);
    }else
    {
        QMessageBox::information(this,"提示","未与终端建立通信,请等待建立通信后重试！建立通信后左上角USB连接状态图标将变为浅绿色。");
    }
}

void HouseKeeperClient::on_change_id_setting_clicked()
{
    QString tmp = ui->dev_id_factory->text();
    int len = tmp.length();
    for(int i=0; i<len; i++)
    {
        if( (tmp.at(i) >= '0' && tmp.at(i) <= '9') ||
            (tmp.at(i) >= 'A' && tmp.at(i) <= 'F'))
        {
            continue;
        }
        else
        {
            QMessageBox::warning(this,"提示","设备ID必须为16进制字符串(0-9,A-F)");
            return;
        }
    }
    if(len != 16)
    {
        QMessageBox::warning(this,"提示","设备ID长度必须为16位");
        return;
    }

    tmp = ui->sim_number_factory->text();
    len = tmp.length();
    for(int i=0; i<len; i++)
    {
        if( (tmp.at(i) >= '0' && tmp.at(i) <= '9') )
        {
            continue;
        }
        else
        {
            QMessageBox::warning(this,"提示","SIM卡号码必须由0-9之间的数字组成");
            return;
        }
    }
    if(len != 11)
    {
        QMessageBox::warning(this,"提示","SIM卡号码长度必须为11位");
        return;
    }

    if(tcp_connect_flag)
    {
        if( check_database_record(ui->dev_id_factory->text().trimmed()) )
        {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root,"data_type","factory_params_set");  //设置出厂配置参数
            int nums = 3;
            cJSON_AddNumberToObject(root,"param_nums", nums);  //参数个数
            cJSON *ini_array = cJSON_CreateArray();
            cJSON *selection_array = cJSON_CreateArray();
            cJSON *key_array = cJSON_CreateArray();
            cJSON *data_value = cJSON_CreateArray();

            cJSON_AddItemToArray(ini_array, cJSON_CreateString("/usrdata/service/etc/remote_manage.ini"));
            cJSON_AddItemToArray(selection_array, cJSON_CreateString("OTHER_CFG"));
            cJSON_AddItemToArray(key_array, cJSON_CreateString("product_id"));
            cJSON_AddItemToArray(data_value, cJSON_CreateString(products_id[ui->product_id_factory->currentIndex()]));

            cJSON_AddItemToArray(ini_array, cJSON_CreateString("/usrdata/service/etc/zlcfg.ini"));
            cJSON_AddItemToArray(selection_array, cJSON_CreateString("TCPC_CFG_THEME"));
            cJSON_AddItemToArray(key_array, cJSON_CreateString("sim_num"));
            cJSON_AddItemToArray(data_value, cJSON_CreateString(ui->sim_number_factory->text().trimmed().toLocal8Bit().data()));

            cJSON_AddItemToArray(ini_array, cJSON_CreateString("/usrdata/service/etc/syscfg.ini"));
            cJSON_AddItemToArray(selection_array, cJSON_CreateString("INIT_PARAM_THEME"));
            cJSON_AddItemToArray(key_array, cJSON_CreateString("dev_id"));
            cJSON_AddItemToArray(data_value, cJSON_CreateString(ui->dev_id_factory->text().trimmed().toLocal8Bit().data()));

            cJSON_AddItemToObject(root, "ini_name", ini_array);     //添加数组
            cJSON_AddItemToObject(root, "selection_name", selection_array);
            cJSON_AddItemToObject(root, "key_name", key_array);
            cJSON_AddItemToObject(root, "data_value", data_value);

            char *data = cJSON_PrintUnformatted(root);
            qDebug("factory_params_set = %s",data);
            tcp_client->write(data, strlen(data));
            cJSON_Delete(root);
        }
    }
    else
    {
        QMessageBox::information(this,"提示","未与终端建立通信,请等待建立通信后重试！建立通信后左上角USB连接状态图标将变为浅绿色。");
        //QMessageBox::warning(this,"提示","TCP未连接，请等待建立连接后再试");
    }
}

void HouseKeeperClient::on_dev_id_factory_textEdited(const QString &arg1)
{
    QString devid;
    if(arg1.length() > 16)
    {
        devid = arg1.mid(0,16).toUpper();
    }else
        devid = arg1.toUpper();
    ui->dev_id_factory->setText(devid);
}

void HouseKeeperClient::on_sim_number_factory_textEdited(const QString &arg1)
{
    QString phone_number;
    if(arg1.length() > 11)
    {
        phone_number = arg1.mid(0,11);
        ui->sim_number_factory->setText(phone_number);
    }
}

void HouseKeeperClient::on_factory_test_bt_clicked()    //开始测试
{
    if(ui->factory_test_bt->text() == "开始测试")
    {
        if(!init_test_options())    //参数初始化
        {
            QMessageBox::information(this,"提示","至少需要勾选一项测试选项才能开始测试");
            return;
        }
        ui->remind_time_spinBox->setEnabled(false);
        factory_test_timer->start(1000);        //开始计数
        ui->remind_time_label->setText(QString::number(remind_time));
        QString msg = "--------- 出厂测试 -----------\n";
        msg += "超时时间:";
        msg += QString::number(remind_time);
        msg += "秒\n";
        msg += "测试参数:";
        if(factory_test_state.acc_enable)
            msg += "ACC测试 ";
        if(factory_test_state.battery_enable)
            msg += "电池电压检测 ";
        if(factory_test_state.gps_antena_enable)
            msg += "GPS天线检测 ";
        if(factory_test_state.gps_enable)
            msg += "GPS定位检测 ";
        if(factory_test_state.can_bus_enable)
            msg += "CAN总线检测 ";
        if(factory_test_state.login_enable)
            msg += "平台登陆检测 ";
        if(factory_test_state.lid_enable)
            msg += "开盖测试 ";
        ui->factory_test_result->append(msg);
        add_time_log_to_test_result(QString("开始测试"));

        QString my_style = "QPushButton:enabled \
        {                                                               \
            border-radius: 10px;    \
            color: rgb(255, 255, 255);  \
            font: 75 16pt \"Microsoft YaHei UI\"; \
            background-color: rgba(255, 55, 29, 255);  \
        }                                                           \
        QPushButton:hover                           \
        {                                                       \
            border-radius: 10px;                        \
            color: rgb(255, 255, 255);                     \
            font: 75 16pt \"Microsoft YaHei UI\"; \
            background-color: rgba(255, 55, 29, 220);  \
        }";
        ui->factory_test_bt->setStyleSheet(my_style);
        ui->factory_test_bt->setText("结束测试");
    }
    else
    {
        ui->remind_time_spinBox->setEnabled(true);

        if(factory_test_timer->isActive())  //停止定时器
        {
            factory_test_timer->stop();
        }

        remind_time = 0;
        ui->remind_time_label->setText(QString::number(remind_time));


        QString my_style = "QPushButton:enabled \
        {                                                               \
            border-radius: 10px;    \
            color: rgb(255, 255, 255);  \
            font: 75 16pt \"Microsoft YaHei UI\"; \
            background-color: rgba(148, 206, 39, 255);  \
        }                                                           \
        QPushButton:hover                           \
        {                                                       \
            border-radius: 10px;                        \
            color: rgb(255, 255, 255);                     \
            font: 75 16pt \"Microsoft YaHei UI\"; \
            background-color: rgba(148, 206, 39, 220);  \
        }";
        ui->factory_test_bt->setStyleSheet(my_style);
        ui->factory_test_bt->setText("开始测试");
    }
}

void HouseKeeperClient::on_factory_test_clear_clicked() //清空并保持测试结果
{
    QString record = ui->factory_test_result->toPlainText();
    if(!record.isEmpty())   //如果记录不为空
    {
        QString filename = "./log/";
        filename += QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_");
        filename += QString(all_params.device_info.devid);
        if(factory_test_record_result)
            filename += "_成功";
        else
            filename += "_失败";
        filename += ".log";

        QFile outFile(filename);
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream save_text(&outFile);
        save_text << record << endl;        //保存测试记录
        ui->factory_test_result->clear();
    }
}

void HouseKeeperClient::on_app_upgrade_dir_bt_clicked()
{
    QString dirpath = QFileDialog::getExistingDirectory(this,"选择升级程序目录","./",QFileDialog::ShowDirsOnly);
    ui->app_upgrade_dir->setText(dirpath);
}

void HouseKeeperClient::on_app_name_bt_clicked()
{
    QString dirpath = QFileDialog::getOpenFileName(this,"选择边缘应用可执行文件(应用名称默认与可执行文件同名)","./");
    int index = 0;
    for(int i=0; i<dirpath.length();i++)
    {
        if(dirpath.at(i) == '/')
            index = i;
    }
    dirpath = dirpath.mid(index+1,dirpath.length()-1);
    ui->app_name->setText(dirpath);
}

void HouseKeeperClient::on_app_output_path_bt_clicked()
{
    QString dirpath = QFileDialog::getExistingDirectory(this,"选择升级包输出目录","./",QFileDialog::ShowDirsOnly);
    ui->app_output_path->setText(dirpath);
}

void HouseKeeperClient::on_service_upgrade_dir_bt_clicked()
{
    QString dirpath = QFileDialog::getExistingDirectory(this,"选择升级程序目录","./",QFileDialog::ShowDirsOnly);
    ui->service_upgrade_dir->setText(dirpath);
}

void HouseKeeperClient::on_service_output_path_bt_clicked()
{
    QString dirpath = QFileDialog::getExistingDirectory(this,"选择升级包输出目录","./",QFileDialog::ShowDirsOnly);
    ui->service_output_path->setText(dirpath);
}

void HouseKeeperClient::on_package_type_currentIndexChanged(int index)
{
    ui->stackedWidget_upgrade_type->setCurrentIndex(index);
}

void HouseKeeperClient::on_pushButton_clicked()
{
    if(ui->stackedWidget_upgrade_type->currentIndex() == 0 )      //打包应用程序
    {
        if(ui->app_upgrade_dir->text().isEmpty())
        {
            QMessageBox::information(this,"提示","请选择升级文件目录");
            return;
        }
        if(ui->app_name->text().isEmpty())
        {
            QMessageBox::information(this,"提示","请选择边缘应用可执行程序");
            return;
        }
        if(ui->app_output_path->text().isEmpty())
        {
            QMessageBox::information(this,"提示","请选择升级包输出目录");
            return;
        }

        QDir dir;
        QString app_name = ui->app_name->text();
        qDebug()<<"appname = "<<app_name;

        QString app_path =  ui->app_upgrade_dir->text();
        app_path += "\\*";
        dir.mkpath(app_name); //创建目录
        Sleep(1);

        app_path = app_path.replace("/","\\");

        QProcess *process = NULL;
        process = new QProcess(this);
        QString  cmd_format = "xcopy ";
        cmd_format += app_path;
        cmd_format += " ";
        cmd_format += app_name;
        cmd_format += " /E";
        qDebug()<<"copy cmd :"<<cmd_format;

        process->start(cmd_format);
        process->waitForFinished(); //等待执行完成
        qDebug()<<"Result:"<<process->readAll();


        cmd_format = "7za.exe a -ttar app.tar \"";
        cmd_format += app_name;
        qDebug()<<"cmd = "<<cmd_format;

        process->start(cmd_format);
        process->waitForFinished(); //等待执行完成
        qDebug()<<"Result:"<<process->readAll();

        QString now_time_str = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_");
        cmd_format = "7za.exe a -tgzip ";
        cmd_format += ui->app_output_path->text();
        cmd_format += "/";
        cmd_format += now_time_str;
        cmd_format += "app.tar.gz";
        cmd_format += " app.tar";
        qDebug()<<"cmd = "<<cmd_format;

        process->start(cmd_format);
        process->waitForFinished(); //等待执行完成
        qDebug()<<"Result:"<<process->readAll();


        QFile::remove("app.tar"); //删除归档文件

        QString cur_path = QDir::currentPath();
        QString del_dir_path = cur_path + "/" + app_name;
        delete_dir(del_dir_path);

        process->deleteLater();

        QMessageBox::information(this,"提示","边缘应用升级包打包成功");
    }
    else if(ui->stackedWidget_upgrade_type->currentIndex() == 1 ) //打包服务程序
    {
        if(ui->service_upgrade_dir->text().isEmpty())
        {
            QMessageBox::information(this,"提示","请选择升级文件目录");
            return;
        }
        if(ui->service_output_path->text().isEmpty())
        {
            QMessageBox::information(this,"提示","请选择升级包输出目录");
            return;
        }

        QProcess *process = NULL;
    //  QString  cmd_format = "7za.exe --help";
        QString  cmd_format = "7za.exe a -ttar service.tar \"";
        cmd_format += ui->service_upgrade_dir->text();
        cmd_format += "/*";
        qDebug()<<"cmd = "<<cmd_format;
        process = new QProcess(this);

        process->start(cmd_format);
        process->waitForFinished(); //等待执行完成
        qDebug()<<"Result:"<<process->readAll();

        QString now_time_str = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_");
        cmd_format = "7za.exe a -tgzip ";
        cmd_format += ui->service_output_path->text();
        cmd_format += "/";
        cmd_format += now_time_str;
        cmd_format += "service.tar.gz";
        cmd_format += " service.tar";
        qDebug()<<"cmd = "<<cmd_format;

        process->start(cmd_format);
        process->waitForFinished(); //等待执行完成
        qDebug()<<"Result:"<<process->readAll();
        process->deleteLater();

        QFile::remove("service.tar"); //删除归档文件
        QMessageBox::information(this,"提示","终端服务程序升级包打包成功");
    }
}
