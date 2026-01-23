idf_component_register(SRCS "xn_wifi.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_wifi esp_event nvs_flash lwip)
