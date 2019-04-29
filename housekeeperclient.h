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
#include <QMessageBox>

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

typedef struct _APP_VERSION
{
    char enable_flag;	//使能标志
    char version[16];	//版本号
    char app_name[32];	//应用名称
}APP_VERSION;

typedef struct _VERSION_INFO
{
    char hw_version[16];			//硬件版本
    char fs_version[16];			//固件版本(文件系统版本)
    char service_version[16];      //服务程序版本
    char housekeeper_version[16];  //应用管理程序版本
    char mcu_version[16];			//单片机程序版本
    char other_version[64];		//其他程序版本(如PLC)
    APP_VERSION app_version[5];			//应用程序版本
}VERSION_INFO;

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
    VERSION_INFO version_info;
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


typedef struct _FACTORY_TEST
{
   int gps_enable;
   int gps_result;

   int login_enable;
   int login_result;

   int can_bus_enable;  //CAN总线
   int can_bus_result;

   int lid_enable;
   int lid_result;

   int acc_enable;
   int acc_result;

   int battery_enable;
   int battery_result;

   int gps_antena_enable;
   int gps_antena_result;
}FACTORY_TEST_ST;

enum
{
    GPS_DETECT,
    GPS_ANTENA_DETECT,
    BATTERY_DETECT,
    ACC_TEST,
    LID_TEST,
    CAN_BUS_DETECT,
    LOGIN_DETECT
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
    void parse_get_param_reply(cJSON *root);
    void parse_set_param_reply(cJSON *root);
    void parse_get_factory_param_reply(cJSON *root);
    void parse_set_factory_param_reply(cJSON *root);
    void add_time_log_to_test_result(QString str);
    int init_test_options();        //初始化检测选项
    void set_factory_test_state(int type);   //设置测试结果
    void all_params_proc_for_test();   //出厂测试参数处理
    void candata_proc_for_test();   //出厂测试can数据处理

    QTableWidgetItem *create_item(QString msg);
public slots:
    void update_time();                     //更新时间
    void factory_test_check();                //出厂测试检测
    void update_usb_connect_state(int state);  //更新USB连接状态
    void update_adb_driver_state();  //更新ADB驱动状态
    void update_network_connect_state(int state);      //更新网络连接状态

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

    void on_ini_filename_currentIndexChanged(int index);

    void on_get_id_setting_clicked();

    void on_selection_name_currentIndexChanged(const QString &arg1);

    void on_key_name_currentTextChanged(const QString &arg1);

    void on_param_read_clicked();

    void on_param_set_clicked();

    void on_tabWidget_tabBarClicked(int index);

    void on_get_all_params_clicked();

    void on_change_id_setting_clicked();

    void on_dev_id_factory_textEdited(const QString &arg1);

    void on_sim_number_factory_textEdited(const QString &arg1);

    void on_factory_test_bt_clicked();

    void on_factory_test_clear_clicked();

private:
    Ui::HouseKeeperClient *ui;
    int remind_time;                //剩余时间
    QTimer *update_time_timer;
    QTimer *factory_test_timer;     //出厂测试定时器
    detect_connect *detect_thread;
    QProcess *process;
    QTcpSocket *tcp_client;
    unsigned char network_connect_state;
    unsigned char usb_connect_state;
    unsigned char tcp_connect_flag;        //TCP连接状态
    ALL_PARAMS all_params;
    FACTORY_TEST_ST factory_test_state;       //出厂测试状态
    int factory_test_record_result;            //出厂测试结果记录状态
};

#endif // HOUSEKEEPERCLIENT_H
