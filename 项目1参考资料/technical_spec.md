# 技术规范

> 版本: 1.0 | 更新日期: 2026-07-13

---

## 1. 系统时钟配置

```
HSE: 8 MHz → PLL ×9 = 72 MHz (SYSCLK)
AHB: 72 MHz (HCLK)
APB1: 36 MHz (PCLK1, ÷2)
APB2: 72 MHz (PCLK2, ÷1)
ADC: PCLK2 ÷ 6 = 12 MHz
```

## 2. FreeRTOS 配置标准

| 参数 | 值 | 说明 |
|------|----|------|
| 内核版本 | FreeRTOS 10.3.1 | CMSIS-RTOS v2 封装 |
| Tick 频率 | 1000 Hz | 1ms 精度 |
| 最大优先级 | 56 | 实际使用 ≤ 4 级 |
| 内存管理 | heap_4 | 16KB 堆 |
| 最小栈 | 128 words | 512 bytes |
| 抢占 | 启用 | |
| 互斥量 | 启用 | 含优先级继承 |
| 软件定时器 | 启用 | 优先级 2, 栈 256 |
| 任务通知 | 启用 | |
| 栈高水位 | 启用 | 运行时监控用 |
| 运行时统计 | 规划启用 | 需 TIM3 做时间基准 |

## 3. 任务设计规范

### 3.1 优先级排布原则

1. **中断 > 任务**: 所有 ISR 优先级 ≥ 5 (高于 kernel 的 15)
2. **控制 > 处理 > 显示**: 控制命令必须优先响应
3. **同级协作**: 生产-消费任务同级，靠队列解耦，避免不必要抢占
4. **最低为显示**: 显示永远不能阻塞数据流

### 3.2 推荐优先级

| 任务 | 优先级 | 理由 |
|------|--------|------|
| ControlTask | AboveNormal (28) | STOP命令需在10ms内响应 |
| ProcessTask | Normal (24) | 信号处理，不可抢占通信 |
| CommTask | Normal (24) | 与处理同级，队列解耦 |
| OLEDTask | BelowNormal (22) | 绝不阻塞采集 |
| HealthTimer | (FreeRTOS Timer) 优先级 2 | 100ms心跳检查 |
| defaultTask | Idle (0) | OS 空闲任务 |

### 3.3 栈空间设计

- ProcessTask: 512B (需要局部变量存储信号处理中间结果)
- CommTask: 512B (协议打包+UART HAL调用栈深)
- ControlTask: 512B (命令解析+状态机)
- OLEDTask: 1024B (OLED 显示缓冲+浮点打印栈深)
- 安全余量: 每任务至少 30% 栈高水位

### 3.4 任务间通信规范

**仅使用以下 IPC 机制:**
1. 二进制信号量: ISR → Task 事件通知 (必须在 ISR 中 `osSemaphoreRelease`)
2. 消息队列: Task → Task 数据传递 (深度 ≥ 4)
3. 任务通知: 轻量事件通知 (替代简单信号量)

**禁止:**
- 全局变量无保护共享 (必须有关键区/互斥量)
- 在 ISR 中做浮点运算
- 在 ISR 中调用阻塞 API

## 4. 协议设计规范

### 4.1 帧格式 (通用)

```
| Offset | Size | Field         | Description              |
|--------|------|---------------|--------------------------|
| 0      | 2B   | Frame Header  | 0x55 0xAA (little-endian)|
| 2      | 2B   | Frame ID      | 自增序号, 小端            |
| 4      | 1B   | Command       | 命令类型                  |
| 5      | 2B   | Payload Len   | 载荷长度, 小端            |
| 7      | N    | Payload       | 变长载荷                  |
| 7+N    | 2B   | CRC16-Modbus  | 从 FrameID 开始计算       |
```

### 4.2 命令集

| CMD | 方向 | 名称 | 载荷 | 说明 |
|-----|------|------|------|------|
| 0x01 | ↑ | ADC_DATA_FULL | 波形+特征 | 完整帧 |
| 0x02 | ↓ | START_ADC | 无 | 启动采集 |
| 0x03 | ↓ | STOP_ADC | 无 | 停止采集 |
| 0x04 | ↓ | SET_FREQ | uint32 Hz | 设置激励频率 |
| 0x05 | ↓ | SET_THRESH | uint16 | 设置检测阈值 |
| 0x06 | ↓ | SET_RATIO | uint8 N | 设置全帧间隔 |
| 0x07 | ↓ | GET_STATUS | 无 | 查询系统状态 |
| 0x08 | ↓ | GET_VERSION | 无 | 查询固件版本 |
| 0xF0 | ↑ | ACK | [CMD, 0x00] | 命令成功 |
| 0xF1 | ↑ | NACK | [CMD, ErrCode] | 命令失败 |

Errcode: 0x01=无效参数, 0x02=状态不允许, 0x03=CRC错误

### 4.3 自适应帧率

- 默认全帧间隔 N=10 (每10帧发1帧完整波形)
- 全帧: ~2017B, 特征帧: ~17B
- 100Hz 下有效吞吐: (1×2017 + 9×17) × 10 = 21.7 KB/s
- 占 921600 波特率实效带宽的 23.6%

## 5. 命名规范

### 5.1 文件命名

```
模块目录结构:
user/
  Excitation/    burst.c/h         激励模块
  Acquisition/   adc_collect.c/h   采集模块
  Communication/ packet.c/h         协议模块
  SignalProcessing/ signal_process.c/h  信号处理
  Monitor/       system_monitor.c/h 系统监控
  Common/        error_handler.c/h  错误处理
                 hal_port.h         HAL抽象接口
```

### 5.2 函数命名

- 公开API: `Module_Action()` 如 `Burst_Init()`, `Monitor_AddAdcSamples()`
- 静态函数: `ActionSubject()` 如 `CRC16_Modbus()`, `Compute_TOF()`
- ISR回调: `HAL_Peripheral_CallbackType()` (HAL 规范)
- 模块前缀: `Burst_`, `ADC_Collect_`, `Packet_`, `Signal_`, `Monitor_`, `Error_`

### 5.3 变量命名

- 全局: `g_` 前缀 如 `g_sysState`
- 静态文件级: `s_` 前缀 如 `s_burstTable`
- HAL 句柄: `h` + 外设名 如 `hdac`, `hadc1`, `huart1`
- FreeRTOS 对象: `Handle` 后缀 如 `ADCDoneSemHandle`

## 6. 错误处理规范

### 6.1 错误分级

| 级别 | 响应 | 示例 |
|------|------|------|
| FATAL | 记录 + 进入FAULT状态 + 等IWDG复位 | 连续DMA错误, 栈溢出 |
| ERROR | 记录 + 外设复位重试 | 单次UART帧错误 |
| WARNING | 仅计数 | 队列瞬时满 |

### 6.2 返回值检查

所有 HAL API 调用必须检查返回值:
```c
if (HAL_ADC_Start_DMA(&hadc1, buf, len) != HAL_OK) {
    Error_Report(ERR_ADC_DMA);
    return; // 或进入恢复流程
}
```

### 6.3 断言

`configASSERT` 已启用。关键代码路径使用断言捕获编程错误:
```c
configASSERT(pxQueue != NULL);
configASSERT(pvBuffer != NULL);
```
