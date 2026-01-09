/**
 * 对话 API
 * 
 * 功能说明：
 *   封装与 AI 对话相关的 API 调用。
 */

import request from './index'

/** 对话消息 */
export interface ChatMessage {
  role: 'user' | 'assistant'
  content: string
}

/** 对话请求参数 */
export interface ChatRequest {
  message: string
  history?: ChatMessage[]
}

/** 对话响应 */
export interface ChatResponse {
  reply: string
}

/**
 * 发送文本消息
 * @param data 对话请求
 * @returns AI 回复
 */
export function sendMessage(data: ChatRequest): Promise<ChatResponse> {
  return request.post('/chat/text', data)
}
