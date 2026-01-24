/**
 * @file lv_conf.h
 * Configuration file for LVGL v9.3
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 8 (A8), 16 (RGB565), 24 (RGB888), 32 (XRGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (e.g. SPI) */
#define LV_COLOR_16_SWAP 0

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* Size of the memory available for `lv_malloc()` in bytes (>= 2kB) */
#define LV_MEM_SIZE (48U * 1024U)  /* 48 KB */

/* Set an address for the memory pool instead of allocating it as a normal array. Can be in external SRAM too. */
#define LV_MEM_ADR 0     /* 0: unused */

/* Instead of an address give a memory allocator that will be called to get a memory pool for LVGL. */
#define LV_MEM_POOL_INCLUDE <stdlib.h>
#define LV_MEM_POOL_ALLOC   malloc
#define LV_MEM_POOL_FREE    free

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh, input device read period in milliseconds */
#define LV_DEF_REFR_PERIOD  30      /* [ms] */

/* Default Dot Per Inch. Used to initialize default sizes such as widgets sized, style paddings. */
#define LV_DPI_DEF 130     /* [px/inch] */

/*=================
   FONT USAGE
 *=================*/

/* Montserrat fonts with ASCII range and some symbols */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* Demonstrate special features */
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /* bpp = 3 */
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0  /* Hebrew, Arabic, Persian letters and all their forms */
#define LV_FONT_SIMSUN_16_CJK            0  /* 1000 most common CJK radicals */

/* Pixel perfect monospace fonts */
#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0

/* Set a custom font */
#define LV_FONT_CUSTOM_DECLARE

/* Enable it if you have fonts with a lot of characters. */
#define LV_FONT_FMT_TXT_LARGE 0

/* Enables/disables support for compressed fonts. */
#define LV_USE_FONT_COMPRESSED 0

/* Enable drawing placeholders when glyph dsc is not found */
#define LV_USE_FONT_PLACEHOLDER 1

/*=================
   TEXT SETTINGS
 *=================*/

/* Select a character encoding for strings. */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/* Can break (wrap) texts on these chars */
#define LV_TXT_BREAK_CHARS " ,.;:-_)]}"

/* If a word is at least this long, will break wherever "prettiest" */
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/* Minimum number of characters in a long word to put on a line before a break. */
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/* Minimum number of characters in a long word to put on a line after a break. */
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/* Support bidirectional texts. Allows mixing Left-to-Right and Right-to-Left texts. */
#define LV_USE_BIDI 0

/* Set the default direction. Supported values: `LV_BASE_DIR_LTR`, `LV_BASE_DIR_RTL`, `LV_BASE_DIR_AUTO` */
#define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_LTR

/* Enable Arabic/Persian processing */
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*=================
   WIDGET USAGE
 *=================*/

/* Documentation of the widgets: https://docs.lvgl.io/latest/en/html/widgets/index.html */

#define LV_USE_ANIMIMG    1
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BUTTON     1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR   1
#define LV_USE_CANVAS     1
#define LV_USE_CHART      1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMAGE      1
#define LV_USE_IMAGEBUTTON 1
#define LV_USE_KEYBOARD   1
#define LV_USE_LABEL      1
#define LV_USE_LED        1
#define LV_USE_LINE       1
#define LV_USE_LIST       1
#define LV_USE_MENU       1
#define LV_USE_MSGBOX     1
#define LV_USE_ROLLER     1
#define LV_USE_SCALE      1
#define LV_USE_SLIDER     1
#define LV_USE_SPAN       1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_SWITCH     1
#define LV_USE_TABLE      1
#define LV_USE_TABVIEW    1
#define LV_USE_TEXTAREA   1
#define LV_USE_TILEVIEW   1
#define LV_USE_WIN        1

/*==================
 * THEMES
 *==================*/

/* A simple, impressive and very complete theme */
#define LV_USE_THEME_DEFAULT 1

/* A very simple theme that is a good starting point for a custom theme */
#define LV_USE_THEME_SIMPLE 1

/* A theme designed for monochrome displays */
#define LV_USE_THEME_MONO 1

/*==================
 * LAYOUTS
 *==================*/

/* A layout similar to Flexbox in CSS. */
#define LV_USE_FLEX 1

/* A layout similar to Grid in CSS. */
#define LV_USE_GRID 1

/*==================
 * OTHERS
 *==================*/

/* 1: Enable API to take snapshot for object */
#define LV_USE_SNAPSHOT 1

/* 1: Enable Monkey test */
#define LV_USE_MONKEY 0

/* 1: Enable grid navigation */
#define LV_USE_GRIDNAV 0

/* 1: Enable lv_obj fragment */
#define LV_USE_FRAGMENT 0

/* 1: Support using images as font in label or span widgets */
#define LV_USE_IMGFONT 0

/* 1: Enable a published subscriber based messaging system */
#define LV_USE_MSG 0

/* 1: Enable Pinyin input method */
#define LV_USE_IME_PINYIN 0

/* 1: Enable file explorer */
#define LV_USE_FILE_EXPLORER 0

/*==================
 * EXAMPLES
 *==================*/

/* Enable the examples to be built with the library */
#define LV_BUILD_EXAMPLES 0

/*==================
 * DEMOS
 *==================*/

/* Show some widget. It might be required to increase `LV_MEM_SIZE` */
#define LV_USE_DEMO_WIDGETS 0

/* Demonstrate the usage of encoder and keyboard */
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0

/* Benchmark your system */
#define LV_USE_DEMO_BENCHMARK 0

/* Stress test for LVGL */
#define LV_USE_DEMO_STRESS 0

/* Music player demo */
#define LV_USE_DEMO_MUSIC 0

/* Flex layout demo */
#define LV_USE_DEMO_FLEX_LAYOUT 0

/* Smart-phone like multi-language demo */
#define LV_USE_DEMO_MULTILANG 0

/* Widget transformation demo */
#define LV_USE_DEMO_TRANSFORM 0

/* Demonstrate scroll settings */
#define LV_USE_DEMO_SCROLL 0

/* Demonstrate the usage of encoder and keyboard */
#define LV_USE_DEMO_RENDER 0

/* Vector graphic demo */
#define LV_USE_DEMO_VECTOR_GRAPHIC 0

/*==================
 * LOGGING
 *==================*/

/* Enable the log module */
#define LV_USE_LOG 1

/* How important log should be added: */
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO

/* 1: Print the log with 'printf'; 0: User need to register a callback with `lv_log_register_print_cb()` */
#define LV_LOG_PRINTF 1

/* Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs */
#define LV_LOG_TRACE_MEM        0
#define LV_LOG_TRACE_TIMER      0
#define LV_LOG_TRACE_INDEV      0
#define LV_LOG_TRACE_DISP_REFR  0
#define LV_LOG_TRACE_EVENT      0
#define LV_LOG_TRACE_OBJ_CREATE 0
#define LV_LOG_TRACE_LAYOUT     0
#define LV_LOG_TRACE_ANIM       0
#define LV_LOG_TRACE_CACHE      0

/*==================
 * ASSERTS
 *==================*/

/* Enable asserts if an operation is failed or an invalid data is found. */
#define LV_USE_ASSERT_NULL          1   /* Check if the parameter is NULL. */
#define LV_USE_ASSERT_MALLOC        1   /* Checks is the memory is successfully allocated or no. */
#define LV_USE_ASSERT_STYLE         0   /* Check if the styles are properly initialized. */
#define LV_USE_ASSERT_MEM_INTEGRITY 0   /* Check the integrity of `lv_mem` after critical operations. */
#define LV_USE_ASSERT_OBJ           0   /* Check the object's type and existence */

/* Add a custom handler when assert happens e.g. to restart the MCU */
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);   /* Halt by default */

/*==================
 * OTHERS
 *==================*/

/* 1: Show CPU usage and FPS count */
#define LV_USE_PERF_MONITOR 1
#define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT

/* 1: Show the used memory and the memory fragmentation */
#define LV_USE_MEM_MONITOR 1
#define LV_USE_MEM_MONITOR_POS LV_ALIGN_BOTTOM_LEFT

/* Draw random colored rectangles over the redrawn areas */
#define LV_USE_REFR_DEBUG 0

/* Change the built in (v)snprintf functions */
#define LV_SPRINTF_CUSTOM 0

/* Garbage Collector settings */
#define LV_ENABLE_GC 0

/* For big endian systems set to 1 */
#define LV_BIG_ENDIAN_SYSTEM 0

/* Define a custom attribute to `lv_tick_inc` function */
#define LV_ATTRIBUTE_TICK_INC

/* Define a custom attribute to `lv_timer_handler` function */
#define LV_ATTRIBUTE_TIMER_HANDLER

/* Define a custom attribute to `lv_display_flush_ready` function */
#define LV_ATTRIBUTE_FLUSH_READY

/* Required alignment size for buffers */
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1

/* Will be added where memories needs to be aligned (with -Os data might not be aligned to boundary by default). */
#define LV_ATTRIBUTE_MEM_ALIGN

/* Attribute to mark large constant arrays for example font's bitmaps */
#define LV_ATTRIBUTE_LARGE_CONST

/* Compiler prefix for a big array declaration in RAM */
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/* Place performance critical functions into a faster memory (e.g RAM) */
#define LV_ATTRIBUTE_FAST_MEM

/* Export integer constant to binding. This macro is used with constants in the form of LV_<CONST> that should also appear on LVGL binding API such as Micropython. */
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning

/* Extend the default -32k..32k coordinate range to -4M..4M by using int32_t for coordinates instead of int16_t */
#define LV_USE_LARGE_COORD 0

/* Prefix all global extern data with this */
#define LV_ATTRIBUTE_EXTERN_DATA

/* Use `memcpy` and `memset` instead of for loops. */
#define LV_USE_BUILTIN_MEMCPY 1

/*==================
 * FONT USAGE
 *==================*/

/* Always set a default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Enable handling large font and/or fonts with a lot of characters. */
#define LV_FONT_FMT_TXT_LARGE 0

/* Enables/disables support for compressed fonts. */
#define LV_USE_FONT_COMPRESSED 0

/* Enable drawing placeholders when glyph dsc is not found */
#define LV_USE_FONT_PLACEHOLDER 1

/*==================
 * LV_USE_USER_DATA
 *==================*/

/* 1: Add a `user_data` to drivers and objects */
#define LV_USE_USER_DATA 1

/*========================
 * Image decoder and cache
 *========================*/

/* 1: Enable indexed (palette) images */
#define LV_USE_INDEXED_IMG 1

/* 1: Enable alpha indexed images */
#define LV_USE_ALPHA_INDEXED_IMG 1

/* Default image cache size. Image caching keeps the images opened. */
#define LV_CACHE_DEF_SIZE 4

/* Number of stops allowed per gradient. Increase this to allow more stops. */
#define LV_GRADIENT_MAX_STOPS 2

/* Default gradient buffer size. */
#define LV_GRAD_CACHE_DEF_SIZE 0

/* Allow dithering the gradients (to achieve visual smooth color gradients on a display with limited color depth) */
#define LV_DITHER_GRADIENT 0

/* Add support for error diffusion dithering. */
#define LV_DITHER_ERROR_DIFFUSION 0

/* Maximum buffer size to allocate for rotation. */
#define LV_DISP_ROT_MAX_BUF (10*1024)

/*=================
 * COMPILER SETTINGS
 *=================*/

/* For big endian systems set to 1 */
#define LV_BIG_ENDIAN_SYSTEM 0

/* Define a custom attribute to `lv_tick_inc` function */
#define LV_ATTRIBUTE_TICK_INC

/* Define a custom attribute to `lv_timer_handler` function */
#define LV_ATTRIBUTE_TIMER_HANDLER

/* Define a custom attribute to `lv_display_flush_ready` function */
#define LV_ATTRIBUTE_FLUSH_READY

/* Required alignment size for buffers */
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1

/* Will be added where memories needs to be aligned (with -Os data might not be aligned to boundary by default). */
#define LV_ATTRIBUTE_MEM_ALIGN

/* Attribute to mark large constant arrays for example font's bitmaps */
#define LV_ATTRIBUTE_LARGE_CONST

/* Compiler prefix for a big array declaration in RAM */
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/* Place performance critical functions into a faster memory (e.g RAM) */
#define LV_ATTRIBUTE_FAST_MEM

/* Export integer constant to binding. This macro is used with constants in the form of LV_<CONST> that should also appear on LVGL binding API such as Micropython. */
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning

/* Extend the default -32k..32k coordinate range to -4M..4M by using int32_t for coordinates instead of int16_t */
#define LV_USE_LARGE_COORD 0

#endif /* LV_CONF_H */
