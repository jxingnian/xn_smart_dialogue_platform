<!--
  仪表盘页面
  
  功能说明：
    系统首页，展示关键数据概览：
    - 设备总数和在线状态
    - 今日对话次数
    - 记忆数据统计
    - 最近活动日志
-->

<script setup lang="ts">
import { ref, onMounted } from 'vue'

// 统计数据
const stats = ref({
  deviceTotal: 0,
  deviceOnline: 0,
  chatToday: 0,
  memoryTotal: 0,
})

// 加载统计数据
onMounted(async () => {
  // TODO: 调用 API 获取统计数据
  stats.value = {
    deviceTotal: 5,
    deviceOnline: 3,
    chatToday: 128,
    memoryTotal: 1024,
  }
})
</script>

<template>
  <div class="dashboard">
    <!-- 页面标题 -->
    <h2 class="page-title">仪表盘</h2>

    <!-- 统计卡片 -->
    <el-row :gutter="16" class="stat-cards">
      <el-col :span="6">
        <el-card shadow="hover">
          <div class="stat-card">
            <div class="stat-value">{{ stats.deviceTotal }}</div>
            <div class="stat-label">设备总数</div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <div class="stat-card">
            <div class="stat-value online">{{ stats.deviceOnline }}</div>
            <div class="stat-label">在线设备</div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <div class="stat-card">
            <div class="stat-value">{{ stats.chatToday }}</div>
            <div class="stat-label">今日对话</div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <div class="stat-card">
            <div class="stat-value">{{ stats.memoryTotal }}</div>
            <div class="stat-label">记忆总数</div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 占位：后续添加图表和活动日志 -->
    <el-row :gutter="16" class="mt-md">
      <el-col :span="16">
        <el-card>
          <template #header>对话趋势</template>
          <div class="chart-placeholder">图表区域（待实现）</div>
        </el-card>
      </el-col>
      <el-col :span="8">
        <el-card>
          <template #header>最近活动</template>
          <div class="activity-placeholder">活动日志（待实现）</div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<style scoped lang="scss">
.dashboard {
  .page-title {
    margin: 0 0 var(--spacing-md);
    font-size: 20px;
    font-weight: 500;
  }

  .stat-card {
    text-align: center;
    padding: var(--spacing-sm) 0;

    .stat-value {
      font-size: 32px;
      font-weight: bold;
      color: var(--text-primary);

      &.online {
        color: var(--success-color);
      }
    }

    .stat-label {
      margin-top: var(--spacing-xs);
      color: var(--text-secondary);
    }
  }

  .chart-placeholder,
  .activity-placeholder {
    height: 300px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--text-placeholder);
  }
}
</style>
