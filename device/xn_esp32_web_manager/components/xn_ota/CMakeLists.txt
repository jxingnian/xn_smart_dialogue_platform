idf_component_register(
    SRCS "src/xn_ota.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_client esp_https_ota nvs_flash json app_update esp_timer
)
