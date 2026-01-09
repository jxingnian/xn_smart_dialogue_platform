<template>
  <div class="settings-page">
    <h2 class="page-title">系统设置</h2>

    <el-card class="config-card">
      <template #header>
        <span>AI 模型配置</span>
      </template>
      
      <el-form :model="config" label-width="140px" v-loading="loading">
        <!-- 模型来源选择 -->
        <el-form-item label="模型来源">
          <el-radio-group v-model="config.provider">
            <el-radio label="cloud">云端模型</el-radio>
            <el-radio label="local" disabled>本地模型（开发中）</el-radio>
          </el-radio-group>
        </el-form-item>

        <!-- 云端模型配置 -->
        <template v-if="config.provider === 'cloud'">
          <!-- API Key 输入 -->
          <el-form-item label="API Key" required>
            <el-input
              v-model="config.api_key"
              placeholder="请输入通义千问 API Key"
              show-password
              style="width: 400px"
            />
            <el-button 
              type="primary" 
              :loading="fetchingModels"
              @click="fetchModels"
              style="margin-left: 10px"
            >
              获取模型列表
            </el-button>
            <div class="form-tip">
              从 <a href="https://dashscope.console.aliyun.com/apiKey" target="_blank">阿里云 DashScope</a> 获取
            </div>
          </el-form-item>

          <!-- 模型选择 -->
          <el-form-item label="选择模型">
            <el-select 
              v-model="config.cloud_model" 
              style="width: 300px"
              placeholder="请先获取模型列表"
              filterable
            >
              <el-option
                v-for="model in cloudModels"
                :key="model.id"
                :label="model.name"
                :value="model.id"
              />
            </el-select>
            <a 
              v-if="modelDocUrl"
              :href="modelDocUrl" 
              target="_blank" 
              class="doc-link"
            >
              查看模型文档
            </a>
          </el-form-item>
        </template>

        <!-- 保存按钮 -->
        <el-form-item>
          <el-button type="primary" @click="saveConfig" :loading="saving">
            保存配置
          </el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { getAIConfig, updateAIConfig, getAvailableModels, fetchDashScopeModels } from '@/api/config'

// 配置数据
const config = ref({
  provider: 'cloud',
  cloud_model: 'qwen-omni-turbo',
  local_model: '',
  api_key: '',
  local_endpoint: 'http://localhost:11434',
})

// 模型列表
const cloudModels = ref<{id: string, name: string}[]>([])
// 模型文档链接
const modelDocUrl = ref('')

// 加载状态
const loading = ref(false)
const saving = ref(false)
const fetchingModels = ref(false)

// 页面加载时获取配置
onMounted(async () => {
  loading.value = true
  try {
    // 获取已保存的模型列表（包含文档链接）
    const models = await getAvailableModels()
    cloudModels.value = models.cloud
    if (models.doc_url) {
      modelDocUrl.value = models.doc_url
    }
    // 获取当前配置
    const currentConfig = await getAIConfig()
    config.value = currentConfig
  } catch (error) {
    console.error('加载配置失败:', error)
  } finally {
    loading.value = false
  }
})

// 从 DashScope 获取模型列表
async function fetchModels() {
  if (!config.value.api_key) {
    ElMessage.warning('请先输入 API Key')
    return
  }
  
  fetchingModels.value = true
  try {
    const result = await fetchDashScopeModels(config.value.api_key)
    
    if (result.models && result.models.length > 0) {
      cloudModels.value = result.models
      modelDocUrl.value = result.doc_url
      ElMessage.success(`获取到 ${result.models.length} 个模型`)
    } else {
      ElMessage.warning('未获取到可用模型')
    }
  } catch (error: any) {
    ElMessage.error(error.response?.data?.detail || '获取模型列表失败')
  } finally {
    fetchingModels.value = false
  }
}

// 保存配置
async function saveConfig() {
  if (config.value.provider === 'cloud' && !config.value.api_key) {
    ElMessage.warning('请输入 API Key')
    return
  }
  saving.value = true
  try {
    await updateAIConfig(config.value)
    ElMessage.success('配置已保存')
  } catch (error: any) {
    ElMessage.error(error.message || '保存失败')
  } finally {
    saving.value = false
  }
}
</script>

<style scoped lang="scss">
.settings-page {
  .page-title {
    margin: 0 0 20px;
    font-size: 20px;
  }
  .config-card {
    max-width: 900px;
  }
  .form-tip {
    margin-top: 5px;
    font-size: 12px;
    color: #909399;
    a { color: #409eff; }
  }
  .doc-link {
    margin-left: 15px;
    color: #409eff;
    font-size: 14px;
    text-decoration: none;
    &:hover {
      text-decoration: underline;
    }
  }
}
</style>
