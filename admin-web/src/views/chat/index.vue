<template>
  <div class="chat-container">
    <!-- 消息列表 -->
    <div class="message-list" ref="messageListRef">
      <div
        v-for="(msg, index) in messages"
        :key="index"
        :class="['message', msg.role]"
      >
        <div class="avatar">
          <el-icon v-if="msg.role === 'user'"><User /></el-icon>
          <el-icon v-else><Monitor /></el-icon>
        </div>
        <div class="content">{{ msg.content }}</div>
      </div>
      
      <!-- 加载中 -->
      <div v-if="loading" class="message assistant">
        <div class="avatar">
          <el-icon><Monitor /></el-icon>
        </div>
        <div class="content loading">
          <span></span><span></span><span></span>
        </div>
      </div>
    </div>
    
    <!-- 输入区域 -->
    <div class="input-area">
      <el-input
        v-model="inputMessage"
        placeholder="输入消息..."
        @keyup.enter="sendMessage"
        :disabled="loading"
      />
      <el-button
        type="primary"
        @click="sendMessage"
        :loading="loading"
        :disabled="!inputMessage.trim()"
      >
        发送
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, nextTick } from 'vue'
import { User, Monitor } from '@element-plus/icons-vue'
import { sendMessage as sendChatMessage, type ChatMessage } from '@/api/chat'
import { ElMessage } from 'element-plus'

// 消息列表
const messages = ref<ChatMessage[]>([])
// 输入框内容
const inputMessage = ref('')
// 加载状态
const loading = ref(false)
// 消息列表容器引用
const messageListRef = ref<HTMLElement>()

// 滚动到底部
const scrollToBottom = () => {
  nextTick(() => {
    if (messageListRef.value) {
      messageListRef.value.scrollTop = messageListRef.value.scrollHeight
    }
  })
}

// 发送消息
const sendMessage = async () => {
  const message = inputMessage.value.trim()
  if (!message || loading.value) return
  
  // 添加用户消息
  messages.value.push({ role: 'user', content: message })
  inputMessage.value = ''
  scrollToBottom()
  
  // 调用 API
  loading.value = true
  try {
    const response = await sendChatMessage({
      message,
      history: messages.value.slice(0, -1) // 不包含刚添加的消息
    })
    
    // 添加 AI 回复
    messages.value.push({ role: 'assistant', content: response.reply })
    scrollToBottom()
  } catch (error: any) {
    ElMessage.error(error.message || '发送失败')
  } finally {
    loading.value = false
  }
}
</script>

<style scoped lang="scss">
.chat-container {
  display: flex;
  flex-direction: column;
  height: calc(100vh - 120px);
  background: #f5f7fa;
  border-radius: 8px;
  overflow: hidden;
}

.message-list {
  flex: 1;
  overflow-y: auto;
  padding: 20px;
}

.message {
  display: flex;
  margin-bottom: 16px;
  
  &.user {
    flex-direction: row-reverse;
    
    .content {
      background: #409eff;
      color: white;
      margin-right: 12px;
      margin-left: 60px;
    }
  }
  
  &.assistant {
    .content {
      background: white;
      margin-left: 12px;
      margin-right: 60px;
    }
  }
}

.avatar {
  width: 40px;
  height: 40px;
  border-radius: 50%;
  background: #e4e7ed;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  
  .el-icon {
    font-size: 20px;
    color: #909399;
  }
}

.content {
  padding: 12px 16px;
  border-radius: 8px;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-word;
  
  &.loading {
    display: flex;
    gap: 4px;
    
    span {
      width: 8px;
      height: 8px;
      background: #c0c4cc;
      border-radius: 50%;
      animation: bounce 1.4s infinite ease-in-out both;
      
      &:nth-child(1) { animation-delay: -0.32s; }
      &:nth-child(2) { animation-delay: -0.16s; }
    }
  }
}

@keyframes bounce {
  0%, 80%, 100% { transform: scale(0); }
  40% { transform: scale(1); }
}

.input-area {
  display: flex;
  gap: 12px;
  padding: 16px 20px;
  background: white;
  border-top: 1px solid #e4e7ed;
  
  .el-input {
    flex: 1;
  }
}
</style>
