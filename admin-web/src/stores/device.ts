/**
 * 设备状态管理
 * 
 * 功能说明：
 *   使用 Pinia 管理设备相关的全局状态。
 *   包括设备列表、当前选中设备、加载状态等。
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { Device } from '@/api/device'
import * as deviceApi from '@/api/device'

export const useDeviceStore = defineStore('device', () => {
  // ========== 状态 ==========
  
  // 设备列表
  const devices = ref<Device[]>([])
  
  // 当前选中的设备
  const currentDevice = ref<Device | null>(null)
  
  // 加载状态
  const loading = ref(false)
  
  // 分页信息
  const pagination = ref({
    page: 1,
    size: 20,
    total: 0,
  })

  // ========== 计算属性 ==========
  
  // 在线设备列表
  const onlineDevices = computed(() => 
    devices.value.filter(d => d.is_online)
  )
  
  // 在线设备数量
  const onlineCount = computed(() => onlineDevices.value.length)

  // ========== 方法 ==========
  
  /**
   * 加载设备列表
   */
  async function fetchDevices(params?: { page?: number; size?: number; type?: string }) {
    loading.value = true
    try {
      const res = await deviceApi.getDevices(params)
      devices.value = res.items
      pagination.value = {
        page: res.page,
        size: res.size,
        total: res.total,
      }
    } finally {
      loading.value = false
    }
  }

  /**
   * 加载设备详情
   */
  async function fetchDevice(deviceId: string) {
    loading.value = true
    try {
      currentDevice.value = await deviceApi.getDevice(deviceId)
    } finally {
      loading.value = false
    }
  }

  /**
   * 创建设备
   */
  async function createDevice(data: deviceApi.CreateDeviceParams) {
    const device = await deviceApi.createDevice(data)
    devices.value.unshift(device)
    return device
  }

  /**
   * 更新设备
   */
  async function updateDevice(deviceId: string, data: deviceApi.UpdateDeviceParams) {
    const device = await deviceApi.updateDevice(deviceId, data)
    const index = devices.value.findIndex(d => d.device_id === deviceId)
    if (index !== -1) {
      devices.value[index] = device
    }
    return device
  }

  /**
   * 删除设备
   */
  async function deleteDevice(deviceId: string) {
    await deviceApi.deleteDevice(deviceId)
    devices.value = devices.value.filter(d => d.device_id !== deviceId)
  }

  /**
   * 控制设备
   */
  async function controlDevice(deviceId: string, command: string, params?: Record<string, any>) {
    await deviceApi.controlDevice(deviceId, command, params)
  }

  return {
    // 状态
    devices,
    currentDevice,
    loading,
    pagination,
    // 计算属性
    onlineDevices,
    onlineCount,
    // 方法
    fetchDevices,
    fetchDevice,
    createDevice,
    updateDevice,
    deleteDevice,
    controlDevice,
  }
})
