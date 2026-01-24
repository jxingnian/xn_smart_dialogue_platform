SET(SOURCES screens/ui_Screen1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c)

add_library(ui ${SOURCES})

# 链接 LVGL
target_link_libraries(ui PUBLIC idf::lvgl__lvgl)

# 添加包含目录
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
