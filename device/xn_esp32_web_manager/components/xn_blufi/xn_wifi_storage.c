/*
 * @Author: 星年 jixingnian@gmail.com
 * @Date: 2025-01-15
 * @Description: WiFi配置存储层 - 实现文件
 */

#include "xn_wifi_storage.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "XN_WIFI_STORAGE";
// NVS命名空间
#define NVS_NAMESPACE "wifi_cfg"
// 最多存储10个WiFi配置
#define MAX_WIFI_CONFIGS 10

/* 初始化WiFi存储层 */
esp_err_t xn_wifi_storage_init(void)
{
    // 初始化NVS Flash
    esp_err_t ret = nvs_flash_init();
    
    // 如果NVS分区被截断或版本不匹配，则需要擦除
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS需要擦除，正在擦除...");
        // 擦除NVS
        ESP_ERROR_CHECK(nvs_flash_erase());
        // 重新初始化
        ret = nvs_flash_init();
    }
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi存储层初始化成功");
    } else {
        ESP_LOGE(TAG, "WiFi存储层初始化失败: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/* 保存WiFi配置到NVS（添加到列表） */
esp_err_t xn_wifi_storage_save(const char *ssid, const char *password)
{
    if (ssid == NULL) {
        ESP_LOGE(TAG, "SSID不能为空");
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    // 打开NVS，读写模式
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "打开NVS失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 读取当前已存储的配置数量，默认为0
    uint8_t count = 0;
    nvs_get_u8(nvs_handle, "count", &count);
    
    // 检查是否已存在相同SSID
    int existing_index = -1;
    for (int i = 0; i < count; i++) {
        char key[16];
        // 构建SSID键名: ssid_0, ssid_1, ...
        snprintf(key, sizeof(key), "ssid_%d", i);
        
        size_t len = 64;
        char stored_ssid[64];
        // 读取存储的SSID
        if (nvs_get_str(nvs_handle, key, stored_ssid, &len) == ESP_OK) {
            // 如果SSID匹配，记录索引
            if (strcmp(stored_ssid, ssid) == 0) {
                existing_index = i;
                break;
            }
        }
    }
    
    // 如果已存在，则更新该位置；否则在末尾添加
    int index = (existing_index >= 0) ? existing_index : count;
    
    // 如果是新配置且存储已满，则需要腾出空间
    if (existing_index < 0 && count >= MAX_WIFI_CONFIGS) {
        ESP_LOGW(TAG, "WiFi配置已满，删除最旧的配置");
        // 删除第一个配置（最旧的），所有后续配置前移
        for (int i = 0; i < count - 1; i++) {
            char old_ssid_key[16], old_pwd_key[16];
            char new_ssid_key[16], new_pwd_key[16];
            
            // 生成旧索引和新索引的键名
            snprintf(old_ssid_key, sizeof(old_ssid_key), "ssid_%d", i + 1);
            snprintf(old_pwd_key, sizeof(old_pwd_key), "pwd_%d", i + 1);
            snprintf(new_ssid_key, sizeof(new_ssid_key), "ssid_%d", i);
            snprintf(new_pwd_key, sizeof(new_pwd_key), "pwd_%d", i);
            
            size_t len = 64;
            char temp[64];
            
            // 移动SSID
            if (nvs_get_str(nvs_handle, old_ssid_key, temp, &len) == ESP_OK) {
                nvs_set_str(nvs_handle, new_ssid_key, temp);
            }
            
            // 移动密码
            len = 64;
            if (nvs_get_str(nvs_handle, old_pwd_key, temp, &len) == ESP_OK) {
                nvs_set_str(nvs_handle, new_pwd_key, temp);
            }
        }
        // 新配置放在最后
        index = count - 1;
    } else if (existing_index < 0) {
        // 如果是新配置且未满，计数增加
        count++;
    }
    
    // 构建要保存的键名
    char ssid_key[16], pwd_key[16];
    snprintf(ssid_key, sizeof(ssid_key), "ssid_%d", index);
    snprintf(pwd_key, sizeof(pwd_key), "pwd_%d", index);
    
    // 保存SSID
    ret = nvs_set_str(nvs_handle, ssid_key, ssid);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "保存SSID失败: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }
    
    // 保存密码，如果密码为空则删除该键
    if (password) {
        ret = nvs_set_str(nvs_handle, pwd_key, password);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "保存密码失败: %s", esp_err_to_name(ret));
            nvs_close(nvs_handle);
            return ret;
        }
    } else {
        nvs_erase_key(nvs_handle, pwd_key);
    }
    
    // 保存新的配置数量
    nvs_set_u8(nvs_handle, "count", count);
    
    // 提交更改到Flash
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi配置已保存 [%d/%d]: %s", index + 1, count, ssid);
    } else {
        ESP_LOGE(TAG, "提交NVS失败: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/* 从NVS加载第一个WiFi配置（兼容旧接口） */
esp_err_t xn_wifi_storage_load(xn_wifi_config_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "配置指针不能为空");
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    // 清空输出结构体
    memset(config, 0, sizeof(xn_wifi_config_t));
    
    // 打开NVS，只读模式
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 尝试读取第一个简单的配置（ssid_0）
    size_t len = sizeof(config->ssid);
    ret = nvs_get_str(nvs_handle, "ssid_0", config->ssid, &len);
    if (ret != ESP_OK) {
        nvs_close(nvs_handle);
        return ret;
    }
    
    // 读取对应的密码
    len = sizeof(config->password);
    nvs_get_str(nvs_handle, "pwd_0", config->password, &len);
    
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "WiFi配置已加载: %s", config->ssid);
    return ESP_OK;
}

/* 加载所有WiFi配置 */
esp_err_t xn_wifi_storage_load_all(xn_wifi_config_t *configs, uint8_t *count, uint8_t max_count)
{
    if (configs == NULL || count == NULL) {
        ESP_LOGE(TAG, "参数不能为空");
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    // 初始化计数
    *count = 0;
    
    // 打开NVS
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 读取存储的总数量
    uint8_t stored_count = 0;
    nvs_get_u8(nvs_handle, "count", &stored_count);
    
    // 遍历读取每一个配置
    for (int i = 0; i < stored_count && i < max_count; i++) {
        char ssid_key[16], pwd_key[16];
        snprintf(ssid_key, sizeof(ssid_key), "ssid_%d", i);
        snprintf(pwd_key, sizeof(pwd_key), "pwd_%d", i);
        
        size_t len = sizeof(configs[i].ssid);
        // 读取SSID
        if (nvs_get_str(nvs_handle, ssid_key, configs[i].ssid, &len) == ESP_OK) {
            // 读取密码
            len = sizeof(configs[i].password);
            nvs_get_str(nvs_handle, pwd_key, configs[i].password, &len);
            (*count)++; // 成功读取则增加计数
        }
    }
    
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "已加载%d个WiFi配置", *count);
    return ESP_OK;
}

/* 删除指定索引的WiFi配置 */
esp_err_t xn_wifi_storage_delete_by_index(uint8_t index)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    // 打开NVS
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "打开NVS失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 读取当前数量
    uint8_t count = 0;
    nvs_get_u8(nvs_handle, "count", &count);
    
    // 检查索引有效性
    if (index >= count) {
        ESP_LOGW(TAG, "索引超出范围: %d >= %d", index, count);
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_ARG;
    }
    
    // 删除指定配置后，将后面的配置向前移动填补空缺
    for (int i = index; i < count - 1; i++) {
        char old_ssid_key[16], old_pwd_key[16];
        char new_ssid_key[16], new_pwd_key[16];
        
        // i+1 移动到 i
        snprintf(old_ssid_key, sizeof(old_ssid_key), "ssid_%d", i + 1);
        snprintf(old_pwd_key, sizeof(old_pwd_key), "pwd_%d", i + 1);
        snprintf(new_ssid_key, sizeof(new_ssid_key), "ssid_%d", i);
        snprintf(new_pwd_key, sizeof(new_pwd_key), "pwd_%d", i);
        
        size_t len = 64;
        char temp[64];
        
        // 移动SSID
        if (nvs_get_str(nvs_handle, old_ssid_key, temp, &len) == ESP_OK) {
            nvs_set_str(nvs_handle, new_ssid_key, temp);
        }
        
        // 移动密码
        len = 64;
        if (nvs_get_str(nvs_handle, old_pwd_key, temp, &len) == ESP_OK) {
            nvs_set_str(nvs_handle, new_pwd_key, temp);
        }
    }
    
    // 删除最后一个配置的键（因为已经移上去了）
    char last_ssid_key[16], last_pwd_key[16];
    snprintf(last_ssid_key, sizeof(last_ssid_key), "ssid_%d", count - 1);
    snprintf(last_pwd_key, sizeof(last_pwd_key), "pwd_%d", count - 1);
    nvs_erase_key(nvs_handle, last_ssid_key);
    nvs_erase_key(nvs_handle, last_pwd_key);
    
    // 更新总数量
    count--;
    nvs_set_u8(nvs_handle, "count", count);
    
    // 提交更改
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi配置已删除，索引: %d", index);
    } else {
        ESP_LOGE(TAG, "删除WiFi配置失败: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/* 从NVS删除所有WiFi配置 */
esp_err_t xn_wifi_storage_delete(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    // 打开NVS
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "打开NVS失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 读取数量
    uint8_t count = 0;
    nvs_get_u8(nvs_handle, "count", &count);
    
    // 遍历删除所有配置键
    for (int i = 0; i < count; i++) {
        char ssid_key[16], pwd_key[16];
        snprintf(ssid_key, sizeof(ssid_key), "ssid_%d", i);
        snprintf(pwd_key, sizeof(pwd_key), "pwd_%d", i);
        nvs_erase_key(nvs_handle, ssid_key);
        nvs_erase_key(nvs_handle, pwd_key);
    }
    
    // 删除计数键
    nvs_erase_key(nvs_handle, "count");
    
    // 提交更改
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "所有WiFi配置已删除");
    } else {
        ESP_LOGE(TAG, "删除WiFi配置失败: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/* 检查是否存在WiFi配置 */
bool xn_wifi_storage_exists(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    // 打开NVS，只读模式
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return false;
    }
    
    // 检查计数是否大于0
    uint8_t count = 0;
    ret = nvs_get_u8(nvs_handle, "count", &count);
    nvs_close(nvs_handle);
    
    return (ret == ESP_OK && count > 0);
}
