idf_component_register(
    SRCS 
        "main.c"
        "app_state_machine.c"
        "managers/wifi_manager.c"
        "managers/mqtt_manager.c"
        "managers/blufi_manager.c"
        "managers/button_manager.c"
        "managers/ota_manager.c"
        "managers/display_manager.c"
    INCLUDE_DIRS 
        "."
        "managers"
        "ui"
    REQUIRES 
        freertos
        esp_wifi
        esp_event
        log
        nvs_flash
        esp_netif
        mqtt
        xn_event_bus
        xn_state_machine
        esp_driver_gpio
        xn_iot_manager_mqtt
        xn_blufi
        xn_wifi
        xn_storage
        xn_ota
        esp_http_client
        esp_https_ota
        json
        xn_display
        lvgl__lvgl
)

# 添加 ui 子目录
add_subdirectory(ui)

# 链接 ui 库
target_link_libraries(${COMPONENT_LIB} PUBLIC ui)
