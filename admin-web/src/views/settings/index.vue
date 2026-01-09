<template>
  <div class="settings-page">
    <h2 class="page-title">系统设置</h2>

    <el-card class="config-card">
      <template #header>
        <span>AI 模型配置</span>
      </template>
      
      <el-form :model="config" label-width="140px" v-loading="loading">
        <el-form-item label="模型来源">
          <el-radio-group v-model="config.provider">
            <el-radio label="cloud">云端模型</el-radio>
            <el-radio label="local" disabled>本地模型（开发中）</el-radio>
          </el-radio-group>
        </el-form-item>

        <template v-if="config.provider === 'cloud'">
          <el-form-item label="选择模型">
            <el-select v-model="config.cloud_model" style="width: 300px">
              <el-option
                v-for="model in cloudModels"
                :key="model.id"
                :label="model.name"
                :value="model.id"
              />
            </el-select>
          </el-form-item>

          <el-form-item label="API Key" required>
            <el-input
              v-model="config.api_key"
              placeholder="请输入通义千问 API Key"
              show-password
              style="width: 400px"
            />
            <div class="form-tip">
              从 <a href="https://dashscope.console.aliyun.com/apiKey" target="_blank">阿里云 DashScope</a> 获取
            </div>
          </el-form-item>
        </template>

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
import { getAIConfig, updateAIConfig, getAvailableModels } from '@/api/config'

const config = ref({
  provider: 'cloud',
  cloud_model: 'qwen-omni-turbo',
  local_model: '',
  api_key: '',
  local_endpoint: 'http://localhost:11434',
})

const cloudModels = ref<{id: string, name: string}[]>([])
const loading = ref(false)
const saving = ref(false)

onMounted(async () => {
  loading.value = true
  try {
    const models = await getAvailableModels()
    cloudModels.value = models.cloud
    const currentConfig = await getAIConfig()
    config.value = currentConfig
  } catch (error) {
    console.error('加载配置失败:', error)
  } finally {
    loading.value = false
  }
})

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
    max-width: 800px;
  }
  .form-tip {
    margin-top: 5px;
    font-size: 12px;
    color: #909399;
    a { color: #409eff; }
  }
}
</style>
