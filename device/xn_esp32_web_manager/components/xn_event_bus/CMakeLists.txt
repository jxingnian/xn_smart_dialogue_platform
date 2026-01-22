idf_component_register(
    SRCS "src/xn_event_bus.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_timer log
)
