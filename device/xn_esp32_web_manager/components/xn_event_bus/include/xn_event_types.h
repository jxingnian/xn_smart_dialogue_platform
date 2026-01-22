/**
 * @file xn_event_types.h
 * @brief 事件类型定义 - 通用可移植
 * 
 * 此文件定义了系统中的所有事件类别、事件ID以及相关的数据结构。
 * 事件ID采用分段管理，高字节表示类别，低字节表示具体事件。
 */

#ifndef XN_EVENT_TYPES_H
#define XN_EVENT_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 *                          事件类别定义
 *===========================================================================*/

/**
 * @brief 事件类别枚举
 * 
 * 用于对事件进行逻辑分组，方便管理和过滤。
 * 每个类别占用 0x0100 (256) 个ID空间。
 */
typedef enum {
    XN_EVT_CAT_SYSTEM   = 0x0000,   ///< 系统基础事件（启动、重启、错误等）
    XN_EVT_CAT_WIFI     = 0x0100,   ///< WiFi相关事件（连接、断开、扫描等）
    XN_EVT_CAT_BLUFI    = 0x0200,   ///< BluFi配网事件
    XN_EVT_CAT_MQTT     = 0x0300,   ///< MQTT协议相关事件
    XN_EVT_CAT_BUTTON   = 0x0400,   ///< 物理按键事件
    XN_EVT_CAT_SENSOR   = 0x0500,   ///< 传感器数据事件
    XN_EVT_CAT_AUDIO    = 0x0600,   ///< 音频处理事件
    XN_EVT_CAT_CMD      = 0x0800,   ///< 控制命令事件（用于模块间控制）
    XN_EVT_CAT_USER     = 0x1000,   ///< 用户自定义事件保留区
} xn_event_category_t;

/*===========================================================================
 *                          系统事件 (0x0000 - 0x00FF)
 *===========================================================================*/

/**
 * @brief 系统事件ID定义
 */
typedef enum {
    XN_EVT_SYSTEM_INIT_DONE     = 0x0001,   ///< 系统初始化完成
    XN_EVT_SYSTEM_READY         = 0x0002,   ///< 系统准备就绪（所有服务已启动）
    XN_EVT_SYSTEM_ERROR         = 0x0003,   ///< 系统级错误
    XN_EVT_SYSTEM_LOW_MEMORY    = 0x0004,   ///< 内存不足警告
    XN_EVT_SYSTEM_REBOOT        = 0x0005,   ///< 系统即将重启
} xn_event_system_t;

/*===========================================================================
 *                          WiFi事件 (0x0100 - 0x01FF)
 *===========================================================================*/

/**
 * @brief WiFi事件ID定义
 */
typedef enum {
    XN_EVT_WIFI_STA_START       = 0x0101,   ///< WiFi Station模式启动
    XN_EVT_WIFI_STA_STOP        = 0x0102,   ///< WiFi Station模式停止
    XN_EVT_WIFI_CONNECTED       = 0x0103,   ///< WiFi已连接到AP（携带 xn_evt_wifi_connected_t）
    XN_EVT_WIFI_DISCONNECTED    = 0x0104,   ///< WiFi已断开连接（携带 xn_evt_wifi_disconnected_t）
    XN_EVT_WIFI_GOT_IP          = 0x0105,   ///< 已获取IP地址（携带 xn_evt_wifi_got_ip_t）
    XN_EVT_WIFI_LOST_IP         = 0x0106,   ///< 丢失IP地址
    XN_EVT_WIFI_SCAN_DONE       = 0x0110,   ///< WiFi扫描完成
} xn_event_wifi_t;

/**
 * @brief WiFi连接成功事件数据
 */
typedef struct {
    uint8_t ssid[33];       ///< 连接的SSID
    uint8_t bssid[6];       ///< AP的MAC地址
    int8_t rssi;            ///< 信号强度
    uint8_t channel;        ///< 信道
} xn_evt_wifi_connected_t;

/**
 * @brief WiFi断开连接事件数据
 */
typedef struct {
    uint16_t reason;        ///< 断开原因代码
} xn_evt_wifi_disconnected_t;

/**
 * @brief 获取IP事件数据
 */
typedef struct {
    uint32_t ip;            ///< IP地址
    uint32_t netmask;       ///< 子网掩码
    uint32_t gateway;       ///< 网关地址
} xn_evt_wifi_got_ip_t;

/*===========================================================================
 *                          BluFi事件 (0x0200 - 0x02FF)
 *===========================================================================*/

/**
 * @brief BluFi事件ID定义
 */
typedef enum {
    XN_EVT_BLUFI_INIT_DONE      = 0x0201,   ///< BluFi初始化完成
    XN_EVT_BLUFI_DEINIT_DONE    = 0x0202,   ///< BluFi反初始化完成
    XN_EVT_BLUFI_CONNECTED      = 0x0203,   ///< 蓝牙已连接
    XN_EVT_BLUFI_DISCONNECTED   = 0x0204,   ///< 蓝牙已断开
    XN_EVT_BLUFI_RECV_CONFIG    = 0x0210,   ///< 收到配网配置（携带 xn_evt_blufi_config_t）
    XN_EVT_BLUFI_CONFIG_DONE    = 0x0211,   ///< 配网完成
} xn_event_blufi_t;

/**
 * @brief BluFi配网数据
 */
typedef struct {
    uint8_t ssid[33];       ///< 配置的SSID
    uint8_t password[65];   ///< 配置的密码
} xn_evt_blufi_config_t;

/*===========================================================================
 *                          MQTT事件 (0x0300 - 0x03FF)
 *===========================================================================*/

/**
 * @brief MQTT事件ID定义
 */
typedef enum {
    XN_EVT_MQTT_CONNECTING      = 0x0301,   ///< 正在连接MQTT服务器
    XN_EVT_MQTT_CONNECTED       = 0x0302,   ///< MQTT已连接
    XN_EVT_MQTT_DISCONNECTED    = 0x0303,   ///< MQTT已断开
    XN_EVT_MQTT_SUBSCRIBED      = 0x0310,   ///< 订阅主题成功
    XN_EVT_MQTT_PUBLISHED       = 0x0311,   ///< 消息发布成功
    XN_EVT_MQTT_DATA            = 0x0320,   ///< 收到MQTT消息（携带 xn_evt_mqtt_data_t）
    XN_EVT_MQTT_ERROR           = 0x03FF,   ///< MQTT错误
} xn_event_mqtt_t;

/**
 * @brief MQTT消息数据
 */
typedef struct {
    char *topic;            ///< 消息主题
    uint16_t topic_len;     ///< 主题长度
    char *data;             ///< 消息内容
    uint32_t data_len;      ///< 内容长度
    int msg_id;             ///< 消息ID
} xn_evt_mqtt_data_t;

/*===========================================================================
 *                          命令事件 (0x0800 - 0x08FF)
 *===========================================================================*/

/**
 * @brief 控制命令事件ID定义
 * 用于模块间发送控制指令
 */
typedef enum {
    XN_CMD_WIFI_CONNECT         = 0x0801,   ///< 请求连接WiFi
    XN_CMD_WIFI_DISCONNECT      = 0x0802,   ///< 请求断开WiFi
    XN_CMD_MQTT_CONNECT         = 0x0810,   ///< 请求连接MQTT
    XN_CMD_MQTT_DISCONNECT      = 0x0811,   ///< 请求断开MQTT
    XN_CMD_MQTT_PUBLISH         = 0x0812,   ///< 请求发布MQTT消息
    XN_CMD_BLUFI_START          = 0x0820,   ///< 请求启动BluFi
    XN_CMD_BLUFI_STOP           = 0x0821,   ///< 请求停止BluFi
} xn_event_cmd_t;

/*===========================================================================
 *                          按键事件 (0x0400 - 0x04FF)
 *===========================================================================*/

/**
 * @brief 按键事件ID定义
 */
typedef enum {
    XN_EVT_BUTTON_PRESSED       = 0x0401,   ///< 按键按下
    XN_EVT_BUTTON_RELEASED      = 0x0402,   ///< 按键释放
    XN_EVT_BUTTON_CLICK         = 0x0403,   ///< 单击
    XN_EVT_BUTTON_DOUBLE_CLICK  = 0x0404,   ///< 双击
    XN_EVT_BUTTON_LONG_PRESS    = 0x0405,   ///< 长按
} xn_event_button_t;

/**
 * @brief 按键事件数据
 */
typedef struct {
    uint8_t button_id;      ///< 按键ID
    uint32_t duration_ms;   ///< 按下持续时间(ms)
} xn_evt_button_t;

/*===========================================================================
 *                          特殊标识
 *===========================================================================*/

#define XN_EVT_ANY              0xFFFF      ///< 特殊ID，用于订阅所有事件

/**
 * @brief 事件源定义
 */
typedef enum {
    XN_EVT_SRC_UNKNOWN  = 0,    ///< 未知源
    XN_EVT_SRC_SYSTEM   = 1,    ///< 系统内核
    XN_EVT_SRC_WIFI     = 2,    ///< WiFi驱动/管理器
    XN_EVT_SRC_BLUFI    = 3,    ///< BluFi组件
    XN_EVT_SRC_MQTT     = 4,    ///< MQTT客户端
    XN_EVT_SRC_BUTTON   = 5,    ///< 按键驱动
    XN_EVT_SRC_USER     = 100,  ///< 用户应用起始源ID
} xn_event_source_t;

#ifdef __cplusplus
}
#endif

#endif /* XN_EVENT_TYPES_H */
