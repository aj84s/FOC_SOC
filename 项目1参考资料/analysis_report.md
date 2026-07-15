# 代码审查分析报告

> 审查日期: 2026-07-13
> 项目: 基于 FreeRTOS 的空耦超声激励采集系统
> MCU: STM32F103RC (Cortex-M3, 72MHz, 256KB Flash, 48KB RAM)

---

## 一、项目现状总览

### 1.1 系统架构

```
TIM2 ISR (100Hz/10ms周期)
  ├── Burst_Start()
  │     └── DAC+DMA 输出 100kHz×5周期方波 (100样本 @ 2MSPS, PA4)
  └── ADC_Collect_Start()
        └── ADC1+DMA 采集 1000样本 (连续模式, PA0)
              └── ADC完成中断 → 交换双缓冲 → 释放ADCDoneSem
                    └── ProcessTask 唤醒 → Signal_Process() → 入队PacketQueue
                          └── CommTask 取出 → Packet_Send() → UART1 DMA发送
```

### 1.2 RTOS 任务表

| 任务 | 优先级 | 栈空间 | 实际功能 |
|------|--------|--------|---------|
| ProcessTask | AboveNormal (28) | 512B | 等ADCDoneSem→信号处理→入队 |
| CommTask | Normal (24) | 256B | 出队→打包→UART DMA发送 |
| ControlTask | BelowNormal (22) | 256B | **空循环，无任何逻辑** |
| OLEDTask | BelowNormal (22) | 512B | 每秒刷新监控数据到OLED |
| defaultTask | Normal (24) | 512B | **空循环，无任何逻辑** |

### 1.3 协议帧格式 (当前)

| 偏移 | 大小 | 字段 |
|------|------|------|
| 0 | 2B | 帧头 0x55 0xAA |
| 2 | 2B | 帧序号 (自增) |
| 4 | 1B | 命令 (固定 CMD_ADC_DATA=0x01) |
| 5 | 2B | 载荷长度 (2008) |
| 7 | 2000B | 波形数据 (1000×uint16_t) |
| 2007 | 8B | 信号特征 (maxAmplitude/TOF/RMS/Energy) |
| 2015 | 2B | CRC16-Modbus |

**帧总大小: 2017 字节**

---

## 二、致命缺陷 (必须立即修复)

### 缺陷 #1: 带宽溢出 — 数据必然丢失

**严重程度: 🔴 致命**

**量化分析:**
```
每帧大小:     2017 bytes
发送频率:     100 Hz (每10ms一帧)
所需吞吐量:   2017 × 100 = 201,700 bytes/s = 1,613,600 bits/s
USART1波特率: 921600 baud
实际有效吞吐: 921600 ÷ 10 = 92,160 bytes/s (8N1, 每字节10bit)
带宽缺口:     201,700 - 92,160 = 109,540 bytes/s
带宽利用率:   219% — 超出2.2倍!
```

**后果:** PacketQueue 深度仅2，2帧后 ProcessTask 入队阻塞 → 整个数据管道卡死。

**建议:** 混合自适应协议 — 每N帧只发1帧完整波形，其余帧仅发送特征值(17字节)。

---

### 缺陷 #2: 无看门狗

**严重程度: 🔴 致命**

- `stm32f1xx_hal_conf.h` 中 IWDG 和 WWDG 均被注释禁用
- 代码中没有任何看门狗初始化或喂狗逻辑
- 一旦任一任务死锁或死循环，系统永远无法恢复

**建议:** 启用 IWDG (~1s超时) + 多任务心跳监控。

---

### 缺陷 #3: 故障异常处理器全为空死循环

**严重程度: 🔴 致命**

```c
// HardFault_Handler, MemManage_Handler, BusFault_Handler, UsageFault_Handler
// 全部都是:
void XXX_Handler(void) {
    while (1) {}  // ← 无任何诊断信息
}
```

**后果:** 发生硬件异常时无法知道原因，无法定位 bug。面试官看到直接毙掉。

**建议:** 实现栈帧提取 + 故障寄存器读取 + USART阻塞发送诊断信息。

---

### 缺陷 #4: ADC_Collect_Start() HAL 状态机违规

**严重程度: 🔴 致命**

```c
// adc_collect.c:22-28  — 当前代码(已修复)
void ADC_Collect_Start(void) {
    // 直接调用 Start_DMA，不先 Stop!
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adcWriteBuf, ADC_BUF_LEN);
}
```

**问题:** 每次 TIM2 ISR 调用时，ADC 可能不在 READY 状态。HAL 状态机规定 Start_DMA 前必须 Stop。连续违反状态机会导致静默数据损坏。

**状态:** ✅ 已修复 (2026-07-13)

---

## 三、功能缺失 (影响系统完整度)

### 缺失 #1: USART 无接收能力

**严重程度: 🟡 严重**

- USART1 配置为 TX+RX 模式，但仅实现了 TX DMA
- 无 RX DMA、无 IDLE 检测、无命令解析
- 上位机无法向设备发送任何控制命令
- 已定义的 CMD_START_ADC / CMD_STOP_ADC / CMD_SET_FREQ 从未被使用

---

### 缺失 #2: ControlTask 是空壳

**严重程度: 🟡 严重**

```c
void ControlTask(void *argument) {
    for(;;) {
        osDelay(1);  // 无事可做
    }
}
```

- 无系统状态管理
- 无命令处理
- 无错误响应逻辑

---

### 缺失 #3: 信号处理未完成

**严重程度: 🟡 严重**

```c
// signal_process.c — 仅实现了 maxAmplitude
void Signal_Process(uint16_t *data, uint16_t len, SignalFeature_t *feature) {
    feature->maxAmplitude = Get_Maxvalue(data, len);
    // feature->tof =       ← 空
    // feature->rms =       ← 空
    // feature->energy =    ← 空
}
```

---

## 四、架构隐患 (影响生产可靠性)

### 隐患 #1: 任务优先级排布不合理

- ProcessTask 高于 CommTask → ProcessTask 可抢占 CommTask，但 PacketQueue 只有2深度
- ControlTask 优先级过低 → 无法及时响应停止命令

### 隐患 #2: 无栈水位监控

`INCLUDE_uxTaskGetStackHighWaterMark` 已启用但从未被调用。某个任务栈溢出前无法预警。

### 隐患 #3: 无错误处理框架

- UART 帧错误/溢出 → 未捕获
- ADC 溢出 → 未计数
- DMA 传输错误 → 未处理
- 堆碎片 → 不可见

### 隐患 #4: 默认 defaultTask 占用资源

512B 栈空间 + osPriorityNormal 优先级的任务完全空转，浪费 RAM 和 CPU。

---

## 五、改进优先级排序

| 优先级 | 改进项 | 工时 | 面试影响 |
|--------|--------|------|---------|
| P0 | 带宽自适应协议 | 4h | ⭐⭐⭐⭐⭐ 展示系统瓶颈量化分析 |
| P0 | IWDG 看门狗 | 3h | ⭐⭐⭐⭐⭐ 工业可靠性必备 |
| P0 | 故障异常处理 | 4h | ⭐⭐⭐⭐⭐ Cortex-M 知识深度 |
| P0 | ADC HAL 状态机 (已修复) | 0.5h | ⭐⭐⭐ 展示 HAL 理解深度 |
| P1 | USART 命令接收 | 6h | ⭐⭐⭐⭐ 嵌入式面试经典题 |
| P1 | 信号处理补完 | 3h | ⭐⭐⭐ 基础算法能力 |
| P1 | 系统状态机 | 2h | ⭐⭐⭐⭐ 架构设计能力 |
| P2 | 增强系统监控 | 4h | ⭐⭐⭐⭐ 生产级质量意识 |
| P2 | OLED 多页显示 | 3h | ⭐⭐ 用户体验意识 |
| P2 | 优先级分析文档 | 2h | ⭐⭐⭐⭐ RTOS 理解深度 |
| P2 | 错误恢复框架 | 5h | ⭐⭐⭐⭐⭐ 量产经验 |
| P2 | 增强命令协议 | 4h | ⭐⭐⭐ 协议设计能力 |
| P3 | HAL 可移植层 | 6h | ⭐⭐⭐⭐ 平台抽象思维 |
| P3 | F4/H7 对比分析 | 4h | ⭐⭐⭐⭐⭐ 硬件选型能力 |
| P3 | Doxygen 文档 | 4h | ⭐⭐⭐ 工程素养 |
| P3 | 性能分析报告 | 3h | ⭐⭐⭐⭐ 量化分析习惯 |
| **合计** | | **~57h** | |

---

## 六、面试叙事线建议

**可靠性第一:** "我从看门狗和故障处理开始——没有可靠性的系统不是产品。"

**量化分析:** "带宽问题不是猜的，是逐字节算的——201.7KB/s vs 92KB/s 实效带宽，差了2.2倍。"

**生产级质量:** "栈水位线、堆碎片、错误计数器——这些都是量产嵌入式系统必备，面试官能一眼看出你有没有真正做过产品。"

**平台思维:** "同一套代码跑 F103 和 F407，只换一个 hal_port 实现文件——展示了硬件无关的架构设计能力。"
