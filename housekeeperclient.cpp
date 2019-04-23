#include "housekeeperclient.h"
#include "ui_housekeeperclient.h"

HouseKeeperClient::HouseKeeperClient(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HouseKeeperClient)
{
    ui->setupUi(this);

    network_connect_state = 0;
    usb_connect_state = 0;
    tcp_connect_flag = 0;        //TCP连接状态
    memset(&all_params,0,sizeof(all_params));

   // this->setWindowFlags (Qt::Window | Qt::FramelessWindowHint);
    update_time_timer = new QTimer(this);
    connect(update_time_timer, SIGNAL(timeout()), this, SLOT(update_time()));
    update_time_timer->start(1000);

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
            ui->params_tableWidget->setRowCount(2);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("服务器IP地址")));
            ui->params_tableWidget->setItem(0,1,create_item(QString(all_params.tcp_connect_info.tcp_ip)));

            ui->params_tableWidget->setItem(1,0,create_item(QString("服务器端口号")));
            ui->params_tableWidget->setItem(1,1,create_item(QString::number(all_params.tcp_connect_info.tcp_port)));

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
            ui->params_tableWidget->setRowCount(11);
            ui->params_tableWidget->setColumnCount(2);
            ui->params_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    //将表格设置为禁止编辑
            ui->params_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //将表格设置为整行选择
            ui->params_tableWidget->setHorizontalHeaderLabels(QStringList()<<"参数名称"<<"参数值");

            ui->params_tableWidget->setItem(0,0,create_item(QString("ACC状态")));
            ui->params_tableWidget->setItem(0,1,create_item(QString::number(all_params.state_info.acc_state)));
            ui->params_tableWidget->setItem(1,0,create_item(QString("总线状态")));
            ui->params_tableWidget->setItem(1,1,create_item(QString::number(all_params.state_info.bus_connect_state)));
            ui->params_tableWidget->setItem(2,0,create_item(QString("充电状态")));
            ui->params_tableWidget->setItem(2,1,create_item(QString::number(all_params.state_info.charge_state)));
            ui->params_tableWidget->setItem(3,0,create_item(QString("设备连接状态")));
            ui->params_tableWidget->setItem(3,1,create_item(QString::number(all_params.state_info.dev_link_state)));
            ui->params_tableWidget->setItem(4,0,create_item(QString("拨号状态")));
            ui->params_tableWidget->setItem(4,1,create_item(QString::number(all_params.state_info.dial_state)));
            ui->params_tableWidget->setItem(5,0,create_item(QString("GPS天线状态")));
            ui->params_tableWidget->setItem(5,1,create_item(QString::number(all_params.state_info.gps_antena)));
            ui->params_tableWidget->setItem(6,0,create_item(QString("GPS定位状态")));
            ui->params_tableWidget->setItem(6,1,create_item(QString::number(all_params.state_info.gps_state)));
            ui->params_tableWidget->setItem(7,0,create_item(QString("开盖状态")));
            ui->params_tableWidget->setItem(7,1,create_item(QString::number(all_params.state_info.liq_state)));
            ui->params_tableWidget->setItem(8,0,create_item(QString("服务器登录状态")));
            ui->params_tableWidget->setItem(8,1,create_item(QString::number(all_params.state_info.login_state)));
            ui->params_tableWidget->setItem(9,0,create_item(QString("SIM卡状态")));
            ui->params_tableWidget->setItem(9,1,create_item(QString::number(all_params.state_info.simcard_state)));
            ui->params_tableWidget->setItem(10,0,create_item(QString("TCP连接状态")));
            ui->params_tableWidget->setItem(10,1,create_item(QString::number(all_params.state_info.tcp_connect_state)));

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

void HouseKeeperClient::update_time()
{
    QString time_str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->now_time->setText(time_str);
}

void HouseKeeperClient::update_network_connect_state(int state)
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
    on_param_type_currentIndexChanged(ui->param_type->currentIndex());
}

void HouseKeeperClient::on_upgrade_package_clicked()
{
    ui->stackedWidget->setCurrentIndex(UPGRADE_PACKAGE);
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
    }
}

void HouseKeeperClient::on_param_set_clicked()
{
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
    }
}
