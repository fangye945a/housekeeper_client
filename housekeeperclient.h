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
#include <QTimer>
#include <QProcess>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTcpSocket>
#include <QTableWidgetItem>

#include "detect_connect.h"
#include "cJSON.h"

#define DEBUG_BGP_PATH  "../zbox-house-keeper-client/pictures/BGP.png"
#define BGP_PATH        "./pictures/BGP.png"

#define MAX_MESSAGE_LEN 2048 //最大json长度


typedef struct _VOLTAGE_INFO
{
    float power_voltage;
    float battery_voltage;
    float mvcc_mid;
    float mvcc_low;
    float battery_off;
}VOLTAGE_INFO;

typedef struct _DEVICE_INFO
{
    char devid[32];
    unsigned int working_mode;
    unsigned int current_on_time;
    unsigned int acc_on_total_mileage;
    float current_temp;
}DEVICE_INFO;

typedef struct _TCP_CONNECT_INFO
{
    char tcp_ip[16];
    unsigned int tcp_port;
}TCP_CONNECT_INFO;

typedef struct _REPORT_TIME_INFO
{
    unsigned int aa_alarm_cycle;
    unsigned int gprs_signal_weak_time;
    unsigned int low_consumption_cycle;
    unsigned int position_report_cycle;
    unsigned int sleep_wakeup_cycle;
    unsigned int workstation_report_cycle;
}REPORT_TIME_INFO;

typedef struct _STATE_INFO
{
    unsigned char acc_state;
    unsigned char bus_connect_state;
    unsigned char charge_state;
    unsigned char dev_link_state;
    unsigned char dial_state;
    unsigned char gps_antena;
    unsigned char gps_state;
    unsigned char liq_state;
    unsigned char login_state;
    unsigned char simcard_state;
    unsigned char tcp_connect_state;
}STATE_INFO;

typedef struct _BAUTRATE_INFO
{
    unsigned int can0;
    unsigned int can1;
    unsigned int rs485;
}BAUTRATE_INFO;

typedef struct _GPS_INFO
{
    float gps_course;
    float gps_velocity;
    float longitude;    //经度
    float latitude;     //纬度
    unsigned char lattype; //南北纬
    unsigned char lontype; //东西经
    unsigned int satallite_num; //卫星数
    unsigned long int utc_time; //UTC时间
}GPS_INFO;

typedef struct _GSM_INFO
{
    unsigned int gsm_signal_strength;       //信号强度
    char sim_card_imsi[24];		//< SIM卡IMSI号
    char sim_card_id[24];			//< SIM卡ID号
    char sms_number[20];			//< 手机号码
    char station_id[24];           //< 基站ID
}GSM_INFO;

typedef struct _HISTORY_INFO
{
    unsigned char 		once_auto_lock_car_flag;		//< 曾自动锁车标志
    unsigned char   	sim_change_flag;				//< sim卡更换标记   0 未更换, 1 更换
    unsigned int		lid_no;							//< 开盖次数
    unsigned int		sim_change_card_no;				//< sim卡，拔卡次数
    unsigned int		gps_total_mileage;				//< GPS总里程数
    unsigned int		gps_antenna_stat;				//< GPS拔天线次数
    unsigned int		gps_fault_time;					//< GPS故障天数,使用秒存储,使用时需转成天数
    unsigned int		gprs_link_lost_time;			//< GPRS信号弱时长,使用秒存储
    unsigned int 		total_on_time;					//< 总通电时长
    unsigned int 		total_acc_on_time;				//< 总通电ACC ON时长
}HISTORY_INFO;

enum
{
    ENUM_VOLTAGE_INFO,
    ENUM_DEVICE_INFO,
    ENUM_TCP_CONNECT_INFO,
    ENUM_REPORT_TIME_INFO,
    ENUM_STATE_INFO,
    ENUM_BAUTRATE_INFO,
    ENUM_GPS_INFO,
    ENUM_GSM_INFO,
    ENUM_HISTORY_INFO,
    ENUM_VERSION_INFO
};

typedef struct _ALL_PARAMS
{
    VOLTAGE_INFO volage_info;
    DEVICE_INFO device_info;
    TCP_CONNECT_INFO tcp_connect_info;
    REPORT_TIME_INFO report_time_info;
    STATE_INFO state_info;
    BAUTRATE_INFO baudrate_info;
    GPS_INFO gps_info;
    GSM_INFO gsm_info;
    HISTORY_INFO history_info;
}ALL_PARAMS;

enum
{
    FIRST_PAGE,
    DOWNLOAD_HARDWARE,
    DEV_ID_WRITE,
    FACTORY_TEST,
    DEVICE_MANAGE,
    UPGRADE_PACKAGE
};

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
    void show_params(int data);                             //显示参数
    void parse_all_params_data(cJSON *root);               //解析所有参数
    void parse_can_data(cJSON *root);                      //解析CAN数据
    void parse_json_data(cJSON *root);                     //解析JSON数据
    void parse_tcp_data(cJSON *root);                      //解析TCP数据
    QTableWidgetItem *create_item(QString msg);
public slots:
    void update_time();                     //更新时间
    void update_usb_connect_state(bool state);  //更新USB连接状态
    void update_adb_driver_state();  //更新ADB驱动状态
    void update_network_connect_state(bool state);      //更新网络连接状态

    void recv_tcp_data();
    void tcp_client_connected();
    void tcp_client_disconnected();

    void ParseAppData(QByteArray package_data); //json数据解析

private slots:
    void on_return_home_clicked();

    void on_hardware_download_clicked();

    void on_next_help_clicked();

    void on_pre_help_clicked();

    void on_download_hardware_clicked();

    void on_devid_entry_clicked();

    void on_factory_test_clicked();

    void on_dev_manage_clicked();

    void on_upgrade_package_clicked();

    void on_param_type_currentIndexChanged(int index);

private:
    Ui::HouseKeeperClient *ui;
    QTimer *update_time_timer;
    detect_connect *detect_thread;
    QProcess *process;
    QTcpSocket *tcp_client;
    unsigned char network_connect_state;
    unsigned char usb_connect_state;
    ALL_PARAMS all_params;
};

#endif // HOUSEKEEPERCLIENT_H
