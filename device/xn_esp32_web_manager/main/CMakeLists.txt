idf_component_register(SRCS "main.c" "app_blufi.c" 
                       PRIV_REQUIRES xn_blufi xn_iot_manager_mqtt
                       INCLUDE_DIRS "")
