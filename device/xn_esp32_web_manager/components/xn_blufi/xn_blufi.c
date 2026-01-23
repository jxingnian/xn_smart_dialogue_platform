/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 15:48:32
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_blufi\xn_blufi.c
 * @Description: BluFi蓝牙配网组件 - 实现 (Refactored)
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#include "xn_blufi.h" // 包含组件头文件
#include "esp_log.h" // 包含日志库
#include "esp_blufi_api.h" // 包含BluFi API
#include "esp_blufi.h" // 包含BluFi定义
#include "esp_bt.h" // 包含BT定义
#include "esp_nimble_hci.h" // 包含NimBLE HCI支持
#include "nimble/nimble_port.h" // 包含NimBLE移植层
#include "nimble/nimble_port_freertos.h" // 包含NimBLE FreeRTOS移植层
#include "host/ble_hs.h" // 包含BLE Host Stack
#include "host/util/util.h" // 包含BLE工具
#include "host/ble_store.h" // 包含BLE存储
#include "services/gap/ble_svc_gap.h" // 包含GAP服务
#include "services/gatt/ble_svc_gatt.h" // 包含GATT服务
#include "esp_wifi.h" // 仅用于 wifi_ap_record_t 结构体定义引用，不调用API
#include <string.h> // 包含字符串库

static const char *TAG = "XN_BLUFI";

// BluFi内部结构体
struct xn_blufi_s {
    char device_name[32]; // 设备名称
    bool ble_connected; // BLE连接状态
    xn_blufi_callbacks_t callbacks; // 回调函数集
    // 缓存待连接的SSID/PWD
    char pending_ssid[33]; 
    char pending_password[65];
};

static xn_blufi_t *g_blufi_instance = NULL; // 全局单例指针

/* ---------------- Internal Helpers ---------------- */

// BluFi事件内部回调 - 处理底层BluFi协议栈的各种事件
static void blufi_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    xn_blufi_t *blufi = g_blufi_instance;
    // 如果实例已被销毁，不再处理事件
    if (blufi == NULL) return;

    switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        // BluFi协议栈初始化完成，开始广播以允许手机搜索连接
        ESP_LOGI(TAG, "BLUFI init finish, start adv");
        esp_blufi_adv_start();
        break;

    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        // BluFi协议栈反初始化完成，资源已释放
        ESP_LOGI(TAG, "BLUFI deinit finish");
        break;

    case ESP_BLUFI_EVENT_BLE_CONNECT:
        // 手机APP已建立BLE连接
        ESP_LOGI(TAG, "BLUFI ble connect");
        // 更新内部连接状态标志
        blufi->ble_connected = true;
        // 连接建立后停止广播，此时不可被其他设备搜索到
        esp_blufi_adv_stop();
        break;

    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        // 手机APP断开BLE连接
        ESP_LOGI(TAG, "BLUFI ble disconnect");
        // 更新内部连接状态标志
        blufi->ble_connected = false;
        
        // 触发应用层回调，通知上层“蓝牙断开”，以便执行如“退出配网模式”等逻辑
        if (blufi->callbacks.on_ble_disconnect) {
            blufi->callbacks.on_ble_disconnect(blufi);
        }
        
        break;

    case ESP_BLUFI_EVENT_RECV_STA_SSID:
        // 从手机接收到WiFi SSID数据包
        // 将数据拷贝到临时缓冲区 pending_ssid 中，等待后续处理
        strncpy(blufi->pending_ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        // 确保字符串手动截断，防止溢出
        blufi->pending_ssid[param->sta_ssid.ssid_len] = '\0';
        ESP_LOGI(TAG, "Recv STA SSID: %s", blufi->pending_ssid);
        break;

    case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        // 从手机接收到WiFi Password数据包
        // 将数据拷贝到临时缓冲区 pending_password 中
        strncpy(blufi->pending_password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
        blufi->pending_password[param->sta_passwd.passwd_len] = '\0';
        ESP_LOGI(TAG, "Recv STA PASSWORD");
        break;

    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
        // 手机发送“连接AP”请求（通常在发送完SSID和密码后）
        ESP_LOGI(TAG, "Req Connect to AP");
        
        // 1. 调用 on_recv_sta_config 回调，将缓存的 SSID/PWD 传递给应用层保存或使用
        if (blufi->callbacks.on_recv_sta_config) {
            blufi->callbacks.on_recv_sta_config(blufi, blufi->pending_ssid, blufi->pending_password);
        }
        
        // 2. 调用 on_connect_request 回调，通知应用层开始执行连接操作
        if (blufi->callbacks.on_connect_request) {
            blufi->callbacks.on_connect_request(blufi);
        }
        break;

    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        // 手机发送“断开WiFi”请求
        ESP_LOGI(TAG, "Req Disconnect from AP");
        // 通知应用层执行断网操作
        if (blufi->callbacks.on_disconnect_request) {
            blufi->callbacks.on_disconnect_request(blufi);
        }
        break;

    case ESP_BLUFI_EVENT_GET_WIFI_LIST:
        // 手机请求获取周围WiFi列表（扫码结果）
        ESP_LOGI(TAG, "Req Get WiFi List");
        // 通知应用层执行WiFi扫描，扫描结果需通过 xn_blufi_send_wifi_list 返回
        if (blufi->callbacks.on_scan_request) {
            blufi->callbacks.on_scan_request(blufi);
        }
        break;

    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
        // 接收到用户自定义数据（非标准Blufi协议数据）
        ESP_LOGI(TAG, "Recv Custom Data len=%d", param->custom_data.data_len);
        // 透传给应用层进行解析处理
        if (blufi->callbacks.on_recv_custom_data) {
            blufi->callbacks.on_recv_custom_data(blufi, param->custom_data.data, param->custom_data.data_len);
        }
        break;

    case ESP_BLUFI_EVENT_GET_WIFI_STATUS:
        // 手机请求查询当前设备WiFi连接状态
        // 此时应用层应检查连接状态并调用 xn_blufi_send_connect_report回复
        if (blufi->callbacks.on_request_wifi_status) {
            blufi->callbacks.on_request_wifi_status(blufi);
        }
        break;

    default:
        break;
    }
}

// ESP BluFi回调结构体初始化
static esp_blufi_callbacks_t s_esp_blufi_callbacks = {
    .event_cb = blufi_event_callback,
    .negotiate_data_handler = NULL,
    .encrypt_func = NULL,
    .decrypt_func = NULL,
    .checksum_func = NULL,
};

// NimBLE reset回调
static void xn_blufi_on_reset(int reason) { ESP_LOGE(TAG, "NimBLE Reset: %d", reason); }
// NimBLE sync回调
static void xn_blufi_on_sync(void) { esp_blufi_profile_init(); }
// NimBLE host task
static void xn_blufi_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* ---------------- Public API ---------------- */

// 创建BluFi实例
xn_blufi_t *xn_blufi_create(const char *device_name)
{
    xn_blufi_t *blufi = malloc(sizeof(xn_blufi_t));
    if (blufi) {
        memset(blufi, 0, sizeof(xn_blufi_t));
        if (device_name) {
            strncpy(blufi->device_name, device_name, sizeof(blufi->device_name) - 1);
        } else {
            strcpy(blufi->device_name, "BLUFI_DEVICE");
        }
    }
    return blufi;
}

// 销毁BluFi实例
void xn_blufi_destroy(xn_blufi_t *blufi)
{
    if (blufi) {
        free(blufi);
    }
}

// 初始化并启动BluFi服务
esp_err_t xn_blufi_init(xn_blufi_t *blufi, xn_blufi_callbacks_t *callbacks)
{
    if (!blufi || !callbacks) return ESP_ERR_INVALID_ARG;

    blufi->callbacks = *callbacks;
    g_blufi_instance = blufi; // 设置全局实例用于回调

    // 释放经典蓝牙内存，节省资源
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret) return ret;

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) return ret;

    ret = esp_nimble_init();
    if (ret) return ret;

    // 配置NimBLE回调
    ble_hs_cfg.reset_cb = xn_blufi_on_reset;
    ble_hs_cfg.sync_cb = xn_blufi_on_sync;
    ble_hs_cfg.gatts_register_cb = esp_blufi_gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ret = esp_blufi_gatt_svr_init();
    if (ret) return ret;

    ret = ble_svc_gap_device_name_set(blufi->device_name);
    if (ret) return ret;

    esp_blufi_btc_init();
    ret = esp_blufi_register_callbacks(&s_esp_blufi_callbacks);
    if (ret) return ret;

    ret = esp_nimble_enable(xn_blufi_host_task);
    if (ret) return ret;

    return ESP_OK;
}

// 反初始化BluFi服务
esp_err_t xn_blufi_deinit(xn_blufi_t *blufi)
{
    esp_blufi_gatt_svr_deinit();
    esp_nimble_deinit();
    esp_blufi_profile_deinit();
    esp_blufi_btc_deinit();
    g_blufi_instance = NULL;
    return ESP_OK;
}

// 发送WiFi列表
esp_err_t xn_blufi_send_wifi_list(uint16_t ap_count, void *ap_list)
{
    // 需要将 wifi_ap_record_t 转换为 esp_blufi_ap_record_t
    // 这里因为解耦，我们假设传入的就是系统标准的 wifi_ap_record_t 数组
    // 但 blufi 需要特殊的结构体，所以需要转换
    if (ap_count > 0 && ap_list) {
        esp_blufi_ap_record_t *blufi_ap_list = malloc(sizeof(esp_blufi_ap_record_t) * ap_count);
        if (!blufi_ap_list) return ESP_ERR_NO_MEM;

        wifi_ap_record_t *src_list = (wifi_ap_record_t *)ap_list;
        for (int i = 0; i < ap_count; i++) {
            memcpy(blufi_ap_list[i].ssid, src_list[i].ssid, sizeof(blufi_ap_list[i].ssid)); // 拷贝SSID
            blufi_ap_list[i].rssi = src_list[i].rssi; // 拷贝RSSI
        }

        esp_blufi_send_wifi_list(ap_count, blufi_ap_list);
        free(blufi_ap_list);
    } else {
        esp_blufi_send_wifi_list(0, NULL);
    }
    return ESP_OK;
}

// 发送连接报告
esp_err_t xn_blufi_send_connect_report(bool connected, const char *ssid, int rssi)
{
    // 构建报告
    // 这里简单封装，详细协议参考乐鑫文档
    esp_blufi_extra_info_t info = {0};
    if (connected && ssid) {
        info.sta_ssid = (uint8_t *)ssid;
        info.sta_ssid_len = strlen(ssid);
        // info.sta_rssi = rssi; // blufi extra info definition check...
        return esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
    } else {
        return esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_FAIL, 0, NULL);
    }
}

// 发送自定义数据
esp_err_t xn_blufi_send_custom_data(uint8_t *data, size_t len)
{
    return esp_blufi_send_custom_data(data, len);
}

// 检查BLE连接状态
bool xn_blufi_is_ble_connected(xn_blufi_t *blufi)
{
    return blufi ? blufi->ble_connected : false;
}
