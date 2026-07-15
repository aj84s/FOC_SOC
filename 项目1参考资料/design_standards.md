# 设计规范

> 版本: 1.0 | 更新日期: 2026-07-13

---

## 1. 核心设计原则

### 1.1 数据流优先 (Data Flow First)

在设计任何新功能前，先在纸上画出数据流路径。明确:
- 数据从哪里产生 (ISR / Task / Peripheral)
- 经过哪些缓冲 (Double Buffer / Queue)
- 谁消费 (Task / UART TX)
- 可能拥塞的点 (Queue深度 / 带宽瓶颈)

### 1.2 失效安全 (Fail-Safe)

- 默认状态是安全的 (上电=不采集, 需命令启动)
- 看门狗是最后防线
- 任一组件失效不能导致整系统崩溃
- 错误累积后自动降级 (进入 FAULT 状态)

### 1.3 可观测性 (Observability)

- 关键指标必须可视化 (OLED / 串口可查询)
- 异常必须有诊断输出
- 运行时性能必须是可测量的 (打点+示波器)

### 1.4 平台无关性 (Platform Independence)

- 应用逻辑不直接依赖 HAL API
- 通过 `hal_port.h` 接口间接访问硬件
- 换芯片 = 换一个 `hal_port_xxx.c` 文件

---

## 2. 模块设计模板

每个新模块必须包含以下注释块:

```c
/**
 * @file    module_name.c
 * @brief   模块一句话描述
 * @ingroup ModuleGroup
 *
 * ## 职责 (Responsibility)
 * 本模块负责: ...
 * 本模块不负责: ...
 *
 * ## 数据流 (Data Flow)
 * 输入:  [来源] → 本模块
 * 输出:  本模块 → [去向]
 *
 * ## 并发安全性 (Concurrency)
 * - 哪些函数从 ISR 调用: ...
 * - 哪些函数从任务调用: ...
 * - 共享资源保护方式: ...
 *
 * ## 使用示例 (Usage)
 * ```
 * Module_Init();
 * Module_Start(param);
 * ```
 *
 * ## 设计决策 (Design Decisions)
 * - 为什么用 A 而不用 B: ...
 * - 为什么选这个阈值/周期: ...
 */
```

---

## 3. 中断服务程序 (ISR) 设计规范

### 3.1 ISR 编码铁律

1. **极短**: ISR 执行时间 < 5μs
2. **无阻塞**: 绝不调用阻塞 API (如 osSemaphoreAcquire)
3. **可调用**: osSemaphoreRelease / osMessageQueuePut (fromISR 变体) / TaskNotifyGive
4. **无浮点**: Cortex-M3 无 FPU, ISR 内禁止浮点运算
5. **有清理**: 必须清除中断标志，防止重入

### 3.2 ISR → Task 延迟处理模式

```c
// ISR 中只做最小工作
void ISR_Handler(void) {
    ClearFlag();                    // 清标志
    osSemaphoreRelease(semHandle);  // 通知任务
    // 不做实际处理!
}

// Task 中做实际工作
void Task_Handler(void) {
    for(;;) {
        osSemaphoreAcquire(semHandle, osWaitForever);
        // 这里做实际处理
    }
}
```

---

## 4. DMA 设计规范

### 4.1 DMA 优先级分配

| DMA 通道 | 外设 | 方向 | 优先级 | 理由 |
|----------|------|------|--------|------|
| DMA2_CH3 | DAC | M→P | **HIGH** | 激励波形不能断, 50μs内完成 |
| DMA1_CH1 | ADC | P→M | LOW | 1000样本, ~1ms完成 |
| DMA1_CH4 | USART1 TX | M→P | LOW | 大数据量, 可被其他中断打断 |
| DMA1_CH5 | USART1 RX | P→M | MEDIUM | 命令数据小, 但需及时响应 |

### 4.2 DMA 模式选择

- **Normal 模式**: 简单、可控。完成后手动重启。用于:
  - ADC 采集 (配合双缓冲, 完成中断中切换)
  - DAC 激励 (单次 burst)
  - USART TX (变长帧)

- **Circular 模式**: 连续不间断。用于:
  - (本项目中暂不使用，避免 HAL 状态机复杂化)

---

## 5. 内存管理规范

### 5.1 内存预算 (48KB SRAM)

| 用途 | 大小 | 说明 |
|------|------|------|
| FreeRTOS 堆 | 16 KB | 任务栈+队列+信号量 |
| ADC 双缓冲 | 4 KB | 1000×2×2 bytes |
| 协议缓冲区 | 2 KB | txBuf |
| 系统栈 | 1 KB | startup 0x400 |
| 全局/静态数据 | ~2 KB | .bss + .data |
| **已用** | **~25 KB** | |
| **剩余** | **~23 KB** | 扩展空间充足 |

### 5.2 栈空间预算

| 任务 | 分配 | 实测高水位 | 安全余量 |
|------|------|-----------|---------|
| ProcessTask | 512B | TBD | ≥ 30% |
| CommTask | 512B | TBD | ≥ 30% |
| ControlTask | 512B | TBD | ≥ 30% |
| OLEDTask | 1024B | TBD | ≥ 30% |

> 高水位值需在增强监控第3周实现后填入。

---

## 6. 测试与验证规范

### 6.1 每个Phase完成后的验证清单

- [ ] 编译通过 (0 Error, 0 Warning)
- [ ] 上位机接收帧序号连续
- [ ] OLED显示正常
- [ ] 看门狗: 手动制造死循环 → 系统复位
- [ ] 故障处理: 制造空指针访问 → 串口输出诊断
- [ ] 栈高水位: 运行30分钟后所有任务 > 30%

### 6.2 性能测量方法

1. 在空闲 GPIO (如 PC13) 打点
2. 逻辑分析仪 / 示波器测量脉冲宽度 = 代码段执行时间
3. 关键测量点:
   - ISR 入口 → ISR 出口 (中断总耗时)
   - ISR 出口 → Task 唤醒 (调度延迟)
   - Task 入口 → Task 完成 (处理耗时)
