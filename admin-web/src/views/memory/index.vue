<!--
  记忆管理页面
  
  功能说明：
    管理系统的记忆数据：
    - 文本记忆（对话历史、感知数据）
    - 偏好习惯记忆
    - 图像记忆
    - 支持语义搜索
-->

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { Search, Delete } from '@element-plus/icons-vue'

// 当前选中的记忆类型标签页
const activeTab = ref('text')

// 搜索关键词
const searchKeyword = ref('')

// 文本记忆列表
const textMemories = ref<any[]>([])

// 偏好记忆列表
const preferenceMemories = ref<any[]>([])

// 加载记忆数据
async function loadMemories() {
  // TODO: 调用 API 获取记忆数据
  textMemories.value = [
    { id: 1, content: '用户询问了今天的天气', type: 'conversation', source: 'user', created_at: '2026-01-09 10:30:00' },
    { id: 2, content: '客厅温度 26°C，湿度 45%', type: 'sensor', source: 'device', created_at: '2026-01-09 10:25:00' },
  ]
  
  preferenceMemories.value = [
    { id: 1, action_type: 'device_control', action_detail: { command: 'power_on', device: '净化器' }, frequency: 30, time_pattern: 'weekday_morning' },
    { id: 2, action_type: 'device_control', action_detail: { command: 'set_mode', device: '空调', mode: '26度' }, frequency: 25, time_pattern: 'evening' },
  ]
}

// 语义搜索
async function handleSearch() {
  if (!searchKeyword.value) return
  // TODO: 调用语义搜索 API
  console.log('搜索:', searchKeyword.value)
}

// 删除记忆
function handleDelete(row: any) {
  // TODO: 调用 API 删除记忆
  console.log('删除记忆:', row.id)
}

onMounted(() => {
  loadMemories()
})
</script>

<template>
  <div class="memory-page">
    <!-- 页面标题 -->
    <h2 class="page-title">记忆管理</h2>

    <!-- 搜索栏 -->
    <el-card class="search-card">
      <div class="search-bar">
        <el-input
          v-model="searchKeyword"
          placeholder="输入自然语言搜索记忆，如：上次讨论空调的对话"
          clearable
          style="width: 500px"
          @keyup.enter="handleSearch"
        >
          <template #append>
            <el-button :icon="Search" @click="handleSearch">搜索</el-button>
          </template>
        </el-input>
      </div>
    </el-card>

    <!-- 记忆列表 -->
    <el-card class="mt-md">
      <el-tabs v-model="activeTab">
        <!-- 文本记忆 -->
        <el-tab-pane label="文本记忆" name="text">
          <el-table :data="textMemories" stripe>
            <el-table-column prop="content" label="内容" show-overflow-tooltip />
            <el-table-column prop="type" label="类型" width="100">
              <template #default="{ row }">
                <el-tag size="small">
                  {{ row.type === 'conversation' ? '对话' : '感知' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="source" label="来源" width="80" />
            <el-table-column prop="created_at" label="时间" width="180" />
            <el-table-column label="操作" width="80">
              <template #default="{ row }">
                <el-popconfirm title="确定删除该记忆吗？" @confirm="handleDelete(row)">
                  <template #reference>
                    <el-button type="danger" link :icon="Delete" />
                  </template>
                </el-popconfirm>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>

        <!-- 偏好习惯 -->
        <el-tab-pane label="偏好习惯" name="preference">
          <el-table :data="preferenceMemories" stripe>
            <el-table-column label="习惯描述">
              <template #default="{ row }">
                {{ row.action_detail.command }} - {{ row.action_detail.device }}
                <span v-if="row.action_detail.mode">（{{ row.action_detail.mode }}）</span>
              </template>
            </el-table-column>
            <el-table-column prop="frequency" label="频率" width="80">
              <template #default="{ row }">
                {{ row.frequency }} 次
              </template>
            </el-table-column>
            <el-table-column prop="time_pattern" label="时间规律" width="120">
              <template #default="{ row }">
                <el-tag size="small" type="info">
                  {{ row.time_pattern === 'weekday_morning' ? '工作日早晨' : 
                     row.time_pattern === 'evening' ? '晚间' : row.time_pattern }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="操作" width="80">
              <template #default="{ row }">
                <el-popconfirm title="确定删除该记忆吗？" @confirm="handleDelete(row)">
                  <template #reference>
                    <el-button type="danger" link :icon="Delete" />
                  </template>
                </el-popconfirm>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>

        <!-- 图像记忆 -->
        <el-tab-pane label="图像记忆" name="image">
          <div class="image-placeholder">
            图像记忆管理（待实现）
          </div>
        </el-tab-pane>
      </el-tabs>
    </el-card>
  </div>
</template>

<style scoped lang="scss">
.memory-page {
  .page-title {
    margin: 0 0 var(--spacing-md);
    font-size: 20px;
    font-weight: 500;
  }

  .search-bar {
    display: flex;
    justify-content: center;
  }

  .image-placeholder {
    height: 200px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--text-placeholder);
  }
}
</style>
