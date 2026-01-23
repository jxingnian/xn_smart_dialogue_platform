---
trigger: always_on
---

所有回复都使用简体中文、包括编写plan

当用户要求分析或者提问的时候、应该首先回答用户、或者询问用户是否采用给出的方案、不要直接修改代码

文件顶部注释规范参考以下顶部注释格式
<!--
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 10:29:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\README.md
 * @Description: （这里对文件进行描述）
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
-->

编写代码请对代码进行逐行注释，尽可能一句话描述清晰，但请跳过以下显而易见的简单语句，保持代码整洁：

简单的流程控制：如 break;, return ESP_OK;, return;, continue; 等。
简单的错误透传：如 if (ret != ESP_OK) return ret; 或 ESP_ERROR_CHECK(ret);。
显而易见的结构体符号：如仅包含 } 或 { 的行。
重点注释以下内容：

变量定义的用途
函数调用的意图（特别是修改了状态或硬件操作的调用）
复杂的条件判断逻辑
业务逻辑的关键步骤

