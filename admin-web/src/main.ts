/**
 * 应用入口文件
 * 
 * 功能说明：
 *   创建 Vue 应用实例，注册全局插件和组件。
 *   这是整个前端应用的启动入口。
 */

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import ElementPlus from 'element-plus'
import zhCn from 'element-plus/es/locale/lang/zh-cn'
import 'element-plus/dist/index.css'

import App from './App.vue'
import router from './router'

import './styles/index.scss'

// 创建 Vue 应用实例
const app = createApp(App)

// 注册 Pinia 状态管理
app.use(createPinia())

// 注册路由
app.use(router)

// 注册 Element Plus，使用中文语言包
app.use(ElementPlus, {
  locale: zhCn,
})

// 挂载应用到 #app 元素
app.mount('#app')
