<!--
  主布局组件
  
  功能说明：
    管理端的主要布局结构，包括：
    - 顶部导航栏：显示系统名称和用户信息
    - 左侧菜单：显示功能导航
    - 主内容区：显示当前页面内容
-->

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import {
  Monitor,
  SetUp,
  Document,
  Setting,
  Fold,
  Expand,
} from '@element-plus/icons-vue'

const route = useRoute()
const router = useRouter()

// 菜单是否折叠
const isCollapse = ref(false)

// 当前激活的菜单
const activeMenu = computed(() => route.path)

// 菜单配置
const menuItems = [
  { path: '/dashboard', title: '仪表盘', icon: Monitor },
  { path: '/devices', title: '设备管理', icon: SetUp },
  { path: '/memories', title: '记忆管理', icon: Document },
  { path: '/settings', title: '系统设置', icon: Setting },
]

// 切换菜单折叠状态
function toggleCollapse() {
  isCollapse.value = !isCollapse.value
}

// 菜单点击跳转
function handleMenuSelect(path: string) {
  router.push(path)
}
</script>

<template>
  <el-container class="layout-container">
    <!-- 左侧菜单 -->
    <el-aside :width="isCollapse ? '64px' : '200px'" class="layout-aside">
      <!-- Logo -->
      <div class="logo">
        <span v-if="!isCollapse">知境中枢</span>
        <span v-else>知</span>
      </div>
      
      <!-- 导航菜单 -->
      <el-menu
        :default-active="activeMenu"
        :collapse="isCollapse"
        background-color="#304156"
        text-color="#bfcbd9"
        active-text-color="#409eff"
        @select="handleMenuSelect"
      >
        <el-menu-item
          v-for="item in menuItems"
          :key="item.path"
          :index="item.path"
        >
          <el-icon><component :is="item.icon" /></el-icon>
          <template #title>{{ item.title }}</template>
        </el-menu-item>
      </el-menu>
    </el-aside>

    <el-container>
      <!-- 顶部导航 -->
      <el-header class="layout-header">
        <div class="header-left">
          <!-- 折叠按钮 -->
          <el-icon class="collapse-btn" @click="toggleCollapse">
            <Fold v-if="!isCollapse" />
            <Expand v-else />
          </el-icon>
        </div>
        <div class="header-right">
          <!-- 用户信息 -->
          <el-dropdown>
            <span class="user-info">
              管理员
              <el-icon><arrow-down /></el-icon>
            </span>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item>个人设置</el-dropdown-item>
                <el-dropdown-item divided>退出登录</el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </el-header>

      <!-- 主内容区 -->
      <el-main class="layout-main">
        <router-view />
      </el-main>
    </el-container>
  </el-container>
</template>

<style scoped lang="scss">
.layout-container {
  height: 100vh;
}

.layout-aside {
  background-color: #304156;
  transition: width 0.3s;
  overflow: hidden;

  .logo {
    height: 60px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: #fff;
    font-size: 18px;
    font-weight: bold;
    background-color: #263445;
  }

  .el-menu {
    border-right: none;
  }
}

.layout-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background-color: #fff;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.08);

  .collapse-btn {
    font-size: 20px;
    cursor: pointer;
    &:hover {
      color: var(--primary-color);
    }
  }

  .user-info {
    display: flex;
    align-items: center;
    cursor: pointer;
  }
}

.layout-main {
  background-color: var(--bg-color-page);
  padding: var(--spacing-md);
}
</style>
