#include "housekeeperclient.h"
#include "ui_housekeeperclient.h"

HouseKeeperClient::HouseKeeperClient(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HouseKeeperClient)
{
    ui->setupUi(this);
    memset(&all_params,0,sizeof(all_params));
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
        }
        cJSON_Delete(root);
    }
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
