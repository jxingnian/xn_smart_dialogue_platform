/**
 * @file xn_event_types.h
 * @brief 事件类型定义 - 通用可移植
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

typedef enum {
    XN_EVT_CAT_SYSTEM   = 0x0000,   ///< 系统事件
    XN_EVT_CAT_WIFI     = 0x0100,   ///< WiFi事件
    XN_EVT_CAT_BLUFI    = 0x0200,   ///< BluFi事件
    XN_EVT_CAT_MQTT     = 0x0300,   ///< MQTT事件
    XN_EVT_CAT_BUTTON   = 0x0400,   ///< 按键事件
    XN_EVT_CAT_SENSOR   = 0x0500,   ///< 传感器事件
    XN_EVT_CAT_AUDIO    = 0x0600,   ///< 音频事件
    XN_EVT_CAT_CMD      = 0x0800,   ///< 命令事件
    XN_EVT_CAT_USER     = 0x1000,   ///< 用户自定义
} xn_event_category_t;

/*===========================================================================
 *                          系统事件 (0x0000 - 0x00FF)
 *===========================================================================*/

typedef enum {
    XN_EVT_SYSTEM_INIT_DONE     = 0x0001,
    XN_EVT_SYSTEM_READY         = 0x0002,
    XN_EVT_SYSTEM_ERROR         = 0x0003,
    XN_EVT_SYSTEM_LOW_MEMORY    = 0x0004,
    XN_EVT_SYSTEM_REBOOT        = 0x0005,
} xn_event_system_t;

/*===========================================================================
 *                          WiFi事件 (0x0100 - 0x01FF)
 *===========================================================================*/

typedef enum {
    XN_EVT_WIFI_STA_START       = 0x0101,
    XN_EVT_WIFI_STA_STOP        = 0x0102,
    XN_EVT_WIFI_CONNECTED       = 0x0103,
    XN_EVT_WIFI_DISCONNECTED    = 0x0104,
    XN_EVT_WIFI_GOT_IP          = 0x0105,
    XN_EVT_WIFI_LOST_IP         = 0x0106,
    XN_EVT_WIFI_SCAN_DONE       = 0x0110,
} xn_event_wifi_t;

typedef struct {
    uint8_t ssid[33];
    uint8_t bssid[6];
    int8_t rssi;
    uint8_t channel;
} xn_evt_wifi_connected_t;

typedef struct {
    uint16_t reason;
} xn_evt_wifi_disconnected_t;

typedef struct {
    uint32_t ip;
    uint32_t netmask;
    uint32_t gateway;
} xn_evt_wifi_got_ip_t;

/*===========================================================================
 *                          BluFi事件 (0x0200 - 0x02FF)
 *===========================================================================*/

typedef enum {
    XN_EVT_BLUFI_INIT_DONE      = 0x0201,
    XN_EVT_BLUFI_DEINIT_DONE    = 0x0202,
    XN_EVT_BLUFI_CONNECTED      = 0x0203,
    XN_EVT_BLUFI_DISCONNECTED   = 0x0204,
    XN_EVT_BLUFI_RECV_CONFIG    = 0x0210,
    XN_EVT_BLUFI_CONFIG_DONE    = 0x0211,
} xn_event_blufi_t;

typedef struct {
    uint8_t ssid[33];
    uint8_t password[65];
} xn_evt_blufi_config_t;

/*===========================================================================
 *                          MQTT事件 (0x0300 - 0x03FF)
 *===========================================================================*/

typedef enum {
    XN_EVT_MQTT_CONNECTING      = 0x0301,
    XN_EVT_MQTT_CONNECTED       = 0x0302,
    XN_EVT_MQTT_DISCONNECTED    = 0x0303,
    XN_EVT_MQTT_SUBSCRIBED      = 0x0310,
    XN_EVT_MQTT_PUBLISHED       = 0x0311,
    XN_EVT_MQTT_DATA            = 0x0320,
    XN_EVT_MQTT_ERROR           = 0x03FF,
} xn_event_mqtt_t;

typedef struct {
    char *topic;
    uint16_t topic_len;
    char *data;
    uint32_t data_len;
    int msg_id;
} xn_evt_mqtt_data_t;

/*===========================================================================
 *                          命令事件 (0x0800 - 0x08FF)
 *===========================================================================*/

typedef enum {
    XN_CMD_WIFI_CONNECT         = 0x0801,
    XN_CMD_WIFI_DISCONNECT      = 0x0802,
    XN_CMD_MQTT_CONNECT         = 0x0810,
    XN_CMD_MQTT_DISCONNECT      = 0x0811,
    XN_CMD_MQTT_PUBLISH         = 0x0812,
    XN_CMD_BLUFI_START          = 0x0820,
    XN_CMD_BLUFI_STOP           = 0x0821,
} xn_event_cmd_t;

/*===========================================================================
 *                          按键事件 (0x0400 - 0x04FF)
 *===========================================================================*/

typedef enum {
    XN_EVT_BUTTON_PRESSED       = 0x0401,
    XN_EVT_BUTTON_RELEASED      = 0x0402,
    XN_EVT_BUTTON_CLICK         = 0x0403,
    XN_EVT_BUTTON_DOUBLE_CLICK  = 0x0404,
    XN_EVT_BUTTON_LONG_PRESS    = 0x0405,
} xn_event_button_t;

typedef struct {
    uint8_t button_id;
    uint32_t duration_ms;
} xn_evt_button_t;

/*===========================================================================
 *                          特殊标识
 *===========================================================================*/

#define XN_EVT_ANY              0xFFFF      ///< 匹配所有事件

typedef enum {
    XN_EVT_SRC_UNKNOWN  = 0,
    XN_EVT_SRC_SYSTEM   = 1,
    XN_EVT_SRC_WIFI     = 2,
    XN_EVT_SRC_BLUFI    = 3,
    XN_EVT_SRC_MQTT     = 4,
    XN_EVT_SRC_BUTTON   = 5,
    XN_EVT_SRC_USER     = 100,
} xn_event_source_t;

#ifdef __cplusplus
}
#endif

#endif /* XN_EVENT_TYPES_H */
