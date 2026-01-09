/**
 * 系统配置 API
 */

import request from './index'

/** AI 模型配置 */
export interface AIModelConfig {
  provider: 'local' | 'cloud'
  cloud_model: string
  local_model: string
  api_key: string
  local_endpoint: string
}

/** 模型信息 */
export interface ModelInfo {
  id: string
  name: string
  description: string
  multimodal: boolean
}

/** 可用模型列表 */
export interface AvailableModels {
  cloud: ModelInfo[]
  local: ModelInfo[]
}

/** 获取 AI 配置 */
export function getAIConfig(): Promise<AIModelConfig> {
  return request.get('/config/ai')
}

/** 更新 AI 配置 */
export function updateAIConfig(data: AIModelConfig): Promise<{ message: string }> {
  return request.put('/config/ai', data)
}

/** 获取可用模型列表 */
export function getAvailableModels(): Promise<AvailableModels> {
  return request.get('/config/ai/models')
}

/** 获取当前模型信息 */
export function getModelInfo(): Promise<{
  provider: string
  model: string
  multimodal: boolean
  configured: boolean
}> {
  return request.get('/chat/model-info')
}
