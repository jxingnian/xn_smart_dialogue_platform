<!--
  根组件
  
  功能说明：
    应用的根组件，包含全局布局和路由出口。
    所有页面都会渲染在 router-view 中。
-->

<script setup lang="ts">
// 根组件不需要额外逻辑，只负责渲染路由
</script>

<template>
  <el-config-provider>
    <router-view />
  </el-config-provider>
</template>

<style>
/* 全局样式重置 */
html, body, #app {
  height: 100%;
  margin: 0;
  padding: 0;
}
</style>
