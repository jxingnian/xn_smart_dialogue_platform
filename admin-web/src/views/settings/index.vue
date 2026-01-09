<!--
  系统设置页面
  
  功能说明：
    系统配置管理：
    - AI 服务配置（选择本地/云端）
    - 系统参数配置
    - 用户管理（待实现）
-->

<script setup lang="ts">
import { ref } from 'vue'

// AI 服务配置
const aiConfig = ref({
  llm: {
    provider: 'cloud',
    cloudModel: 'qwen-max',
    localModel: 'qwen2:7b',
  },
  asr: {
    provider: 'cloud',
  },
  tts: {
    provider: 'cloud',
  },
  embedding: {
    provider: 'local',
  },
  faceRecognition: {
    provider: 'local',
  },
})

// 保存配置
function handleSave() {
  // TODO: 调用 API 保存配置
  console.log('保存配置:', aiConfig.value)
}
</script>

<template>
  <div class="settings-page">
    <!-- 页面标题 -->
    <h2 class="page-title">系统设置</h2>

    <!-- AI 服务配置 -->
    <el-card>
      <template #header>
        <span>AI 服务配置</span>
      </template>
      
      <el-form :model="aiConfig" label-width="120px">
        <!-- LLM 配置 -->
        <el-form-item label="对话模型">
          <el-radio-group v-model="aiConfig.llm.provider">
            <el-radio value="local">本地部署</el-radio>
            <el-radio value="cloud">云端 API</el-radio>
          </el-radio-group>
          <div class="config-detail mt-sm">
            <el-input
              v-if="aiConfig.llm.provider === 'local'"
              v-model="aiConfig.llm.localModel"
              placeholder="本地模型名称"
              style="width: 200px"
            />
            <el-select
              v-else
              v-model="aiConfig.llm.cloudModel"
              placeholder="选择云端模型"
              style="width: 200px"
            >
              <el-option label="通义千问 Max" value="qwen-max" />
              <el-option label="通义千问 Plus" value="qwen-plus" />
              <el-option label="通义千问 Turbo" value="qwen-turbo" />
            </el-select>
          </div>
        </el-form-item>

        <!-- ASR 配置 -->
        <el-form-item label="语音识别">
          <el-radio-group v-model="aiConfig.asr.provider">
            <el-radio value="local">本地部署 (Whisper)</el-radio>
            <el-radio value="cloud">云端 API (通义听悟)</el-radio>
          </el-radio-group>
        </el-form-item>

        <!-- TTS 配置 -->
        <el-form-item label="语音合成">
          <el-radio-group v-model="aiConfig.tts.provider">
            <el-radio value="local">本地部署 (Edge-TTS)</el-radio>
            <el-radio value="cloud">云端 API (通义)</el-radio>
          </el-radio-group>
        </el-form-item>

        <!-- Embedding 配置 -->
        <el-form-item label="文本向量化">
          <el-radio-group v-model="aiConfig.embedding.provider">
            <el-radio value="local">本地部署 (BGE)</el-radio>
            <el-radio value="cloud">云端 API</el-radio>
          </el-radio-group>
        </el-form-item>

        <!-- 人脸识别配置 -->
        <el-form-item label="人脸识别">
          <el-radio-group v-model="aiConfig.faceRecognition.provider">
            <el-radio value="local">本地部署 (InsightFace)</el-radio>
            <el-radio value="cloud">云端 API (阿里云)</el-radio>
          </el-radio-group>
        </el-form-item>

        <el-form-item>
          <el-button type="primary" @click="handleSave">保存配置</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<style scoped lang="scss">
.settings-page {
  .page-title {
    margin: 0 0 var(--spacing-md);
    font-size: 20px;
    font-weight: 500;
  }

  .config-detail {
    margin-left: var(--spacing-md);
  }
}
</style>
