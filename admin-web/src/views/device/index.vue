<!--
  设备管理页面
  
  功能说明：
    管理所有接入系统的智能设备：
    - 设备列表展示（支持筛选和搜索）
    - 添加新设备
    - 编辑设备信息
    - 删除设备
    - 查看设备状态
-->

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { Plus, Refresh } from '@element-plus/icons-vue'

// 设备类型选项
const deviceTypes = [
  { value: 'purifier', label: '净化器' },
  { value: 'fish_feeder', label: '喂鱼器' },
]

// 筛选条件
const filters = ref({
  type: '',
  status: '',
  keyword: '',
})

// 设备列表
const deviceList = ref<any[]>([])
const loading = ref(false)

// 加载设备列表
async function loadDevices() {
  loading.value = true
  try {
    // TODO: 调用 API 获取设备列表
    deviceList.value = [
      { id: 1, device_id: 'purifier_001', name: '客厅净化器', type: 'purifier', location: '客厅', is_online: true },
      { id: 2, device_id: 'purifier_002', name: '卧室净化器', type: 'purifier', location: '卧室', is_online: false },
      { id: 3, device_id: 'fish_feeder_001', name: '鱼缸喂食器', type: 'fish_feeder', location: '书房', is_online: true },
    ]
  } finally {
    loading.value = false
  }
}

// 添加设备对话框
const dialogVisible = ref(false)
const dialogTitle = ref('添加设备')
const deviceForm = ref({
  device_id: '',
  name: '',
  type: '',
  location: '',
})

// 打开添加对话框
function handleAdd() {
  dialogTitle.value = '添加设备'
  deviceForm.value = { device_id: '', name: '', type: '', location: '' }
  dialogVisible.value = true
}

// 打开编辑对话框
function handleEdit(row: any) {
  dialogTitle.value = '编辑设备'
  deviceForm.value = { ...row }
  dialogVisible.value = true
}

// 删除设备
function handleDelete(row: any) {
  // TODO: 调用 API 删除设备
  console.log('删除设备:', row.device_id)
}

// 提交表单
function handleSubmit() {
  // TODO: 调用 API 保存设备
  console.log('保存设备:', deviceForm.value)
  dialogVisible.value = false
  loadDevices()
}

onMounted(() => {
  loadDevices()
})
</script>

<template>
  <div class="device-page">
    <!-- 页面标题 -->
    <h2 class="page-title">设备管理</h2>

    <!-- 工具栏 -->
    <el-card class="toolbar-card">
      <div class="toolbar">
        <div class="toolbar-left">
          <el-select v-model="filters.type" placeholder="设备类型" clearable style="width: 120px">
            <el-option
              v-for="item in deviceTypes"
              :key="item.value"
              :label="item.label"
              :value="item.value"
            />
          </el-select>
          <el-select v-model="filters.status" placeholder="在线状态" clearable style="width: 120px">
            <el-option label="在线" value="online" />
            <el-option label="离线" value="offline" />
          </el-select>
          <el-input
            v-model="filters.keyword"
            placeholder="搜索设备名称"
            clearable
            style="width: 200px"
          />
        </div>
        <div class="toolbar-right">
          <el-button :icon="Refresh" @click="loadDevices">刷新</el-button>
          <el-button type="primary" :icon="Plus" @click="handleAdd">添加设备</el-button>
        </div>
      </div>
    </el-card>

    <!-- 设备列表 -->
    <el-card class="mt-md">
      <el-table :data="deviceList" v-loading="loading" stripe>
        <el-table-column prop="device_id" label="设备ID" width="150" />
        <el-table-column prop="name" label="设备名称" />
        <el-table-column prop="type" label="类型" width="100">
          <template #default="{ row }">
            {{ deviceTypes.find(t => t.value === row.type)?.label || row.type }}
          </template>
        </el-table-column>
        <el-table-column prop="location" label="位置" width="100" />
        <el-table-column prop="is_online" label="状态" width="80">
          <template #default="{ row }">
            <el-tag :type="row.is_online ? 'success' : 'info'" size="small">
              {{ row.is_online ? '在线' : '离线' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="180" fixed="right">
          <template #default="{ row }">
            <el-button type="primary" link @click="handleEdit(row)">编辑</el-button>
            <el-popconfirm title="确定删除该设备吗？" @confirm="handleDelete(row)">
              <template #reference>
                <el-button type="danger" link>删除</el-button>
              </template>
            </el-popconfirm>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- 添加/编辑对话框 -->
    <el-dialog v-model="dialogVisible" :title="dialogTitle" width="500px">
      <el-form :model="deviceForm" label-width="80px">
        <el-form-item label="设备ID" required>
          <el-input v-model="deviceForm.device_id" placeholder="请输入设备唯一标识" />
        </el-form-item>
        <el-form-item label="设备名称" required>
          <el-input v-model="deviceForm.name" placeholder="请输入设备名称" />
        </el-form-item>
        <el-form-item label="设备类型" required>
          <el-select v-model="deviceForm.type" placeholder="请选择设备类型" style="width: 100%">
            <el-option
              v-for="item in deviceTypes"
              :key="item.value"
              :label="item.label"
              :value="item.value"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="位置">
          <el-input v-model="deviceForm.location" placeholder="请输入设备位置" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSubmit">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<style scoped lang="scss">
.device-page {
  .page-title {
    margin: 0 0 var(--spacing-md);
    font-size: 20px;
    font-weight: 500;
  }

  .toolbar {
    display: flex;
    justify-content: space-between;
    align-items: center;

    .toolbar-left {
      display: flex;
      gap: var(--spacing-sm);
    }
  }
}
</style>
