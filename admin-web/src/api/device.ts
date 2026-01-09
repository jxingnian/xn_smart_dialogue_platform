/**
 * 设备相关 API
 * 
 * 功能说明：
 *   封装设备管理相关的接口调用。
 */

import request from './index'

// 设备类型定义
export interface Device {
  id: number
  device_id: string
  name: string
  type: string
  location?: string
  is_online: boolean
  status?: Record<string, any>
  config?: Record<string, any>
  created_at: string
  updated_at: string
}

// 创建设备参数
export interface CreateDeviceParams {
  device_id: string
  name: string
  type: string
  location?: string
  config?: Record<string, any>
}

// 更新设备参数
export interface UpdateDeviceParams {
  name?: string
  location?: string
  config?: Record<string, any>
}

// 设备列表响应
export interface DeviceListResponse {
  items: Device[]
  total: number
  page: number
  size: number
}

/**
 * 获取设备列表
 */
export function getDevices(params?: {
  page?: number
  size?: number
  type?: string
  is_online?: boolean
}): Promise<DeviceListResponse> {
  return request.get('/devices', { params })
}

/**
 * 获取设备详情
 */
export function getDevice(deviceId: string): Promise<Device> {
  return request.get(`/devices/${deviceId}`)
}

/**
 * 创建设备
 */
export function createDevice(data: CreateDeviceParams): Promise<Device> {
  return request.post('/devices', data)
}

/**
 * 更新设备
 */
export function updateDevice(deviceId: string, data: UpdateDeviceParams): Promise<Device> {
  return request.put(`/devices/${deviceId}`, data)
}

/**
 * 删除设备
 */
export function deleteDevice(deviceId: string): Promise<void> {
  return request.delete(`/devices/${deviceId}`)
}

/**
 * 控制设备
 */
export function controlDevice(deviceId: string, command: string, params?: Record<string, any>): Promise<void> {
  return request.post(`/devices/${deviceId}/control`, { command, params })
}
