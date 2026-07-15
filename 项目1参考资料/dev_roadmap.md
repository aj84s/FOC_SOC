# 开发路线图

> 版本: 1.0 | 更新日期: 2026-07-13

---

## 总体策略: 分阶段、可验证、可回退

每完成一个 Phase 就是一个稳定的里程碑，功能完整、可以独立运行。不在一个 Phase 内混入太多改动。

---

## Phase 1: 可靠性地基 (预计 11.5h)

**目标:** 消除所有致命缺陷，系统能安全地失败(fail safely)而非静默损坏。

### Step 1.1 — ADC HAL 状态机修复 ✅
- [x] `adc_collect.c`: `ADC_Collect_Start()` 先 Stop 再 Start
- [x] 加入 ADC 异常恢复逻辑 (DeInit→Init→Start)
- **验证:** TIM2 ISR 连续触发 1000 次后 ADC 数据正常

### Step 1.2 — 自适应协议
- [ ] `packet.h`: 新增 `FRAME_MODE_FULL` / `FRAME_MODE_FEATURE` 枚举
- [ ] `packet.h`: 新增 `CMD_FEATURE_DATA` 命令码
- [ ] `packet.c`: 实现 `Packet_SendFeatureOnly()` 函数 (仅发特征值)
- [ ] `packet.c`: 修改 `Packet_Send()`, 加入 `burstCounter % fullFrameRatio` 逻辑
- [ ] `packet.c`: 新增 `Packet_SetFullFrameRatio()` 配置函数
- **验证:** 上位机查看帧类型分布 (10%全帧 + 90%特征帧), 帧序号连续

### Step 1.3 — IWDG 看门狗
- [ ] `stm32f1xx_hal_conf.h`: 启用 `HAL_IWDG_MODULE_ENABLED`
- [ ] `main.c`: `IWDG_Init()` 函数, LSI/64/625 = ~1s超时
- [ ] `system_monitor.h`: 新增 `Monitor_ReportTaskAlive(taskId)`
- [ ] `system_monitor.c`: 多任务心跳数组 + 100ms 检查定时器
- [ ] `freertos.c`: 启动 `HealthTimer` FreeRTOS 软件定时器
- [ ] 各关键任务: 主循环调用 `Monitor_ReportTaskAlive()`
- **验证:** 在 ProcessTask 加 `while(1);` → 确认 ~1s 后系统复位

### Step 1.4 — 故障异常处理
- [ ] `stm32f1xx_it.c`: 重写 `HardFault_Handler`
  - [ ] 汇编代码判断 MSP/PSP → 提取栈帧
  - [ ] C 函数读取 CFSR/HFSR
  - [ ] 解码故障类型 (UndefinedInstr/InvState/BusFault/MemManage)
  - [ ] USART1 阻塞发送诊断信息
- [ ] 同样处理 `BusFault_Handler`, `UsageFault_Handler`, `MemManage_Handler`
- **验证:** 故意 `*(uint32_t*)0 = 0;` → 确认串口收到诊断信息

---

## Phase 2: 功能完整 (预计 11h)

**目标:** 补完信号处理、实现双向通信、加入系统状态机。

### Step 2.1 — 信号处理补完
- [ ] `signal_process.h`: `SignalFeature_t` 新增 `threshold` 字段
- [ ] `signal_process.c`: 实现 `Compute_TOF()` — 阈值交叉法
- [ ] `signal_process.c`: 实现 `Compute_RMS()` — uint64累加防溢出
- [ ] `signal_process.c`: 实现 `Compute_Energy()` — 原始能量
- [ ] `signal_process.c`: `Signal_Process()` 调用所有计算函数
- **验证:** 上位机查看特征值(TOF/RMS/Energy非零)

### Step 2.2 — USART 命令接收
- [ ] `usart.c`: `HAL_UART_MspInit` 新增 RX DMA (DMA1_CH5)
- [ ] `usart.h`: 导出 `hdma_usart1_rx`
- [ ] `stm32f1xx_it.c`: USART1_IRQHandler 增加 IDLE 检测
- [ ] `dma.c`: 注册 DMA1_CH5 NVIC
- [ ] `packet.h`: 新增 `CommandMsg_t` 结构体, `Packet_ParseCommand()` 声明
- [ ] `packet.c`: 实现命令解析 + ACK/NACK 响应函数
- [ ] `freertos.c`: `ControlTask` 实现 — 等待 TaskNotify → 解析命令 → 执行
- **验证:** 上位机发送 START_ADC/STOP_ADC → 设备响应 ACK

### Step 2.3 — 系统状态机
- [ ] `main.h`: 新增 `SystemState_t` 枚举 (IDLE/RUNNING/FAULT)
- [ ] `main.c`: 声明 `g_sysState`
- [ ] `burst.c`: `Burst_Start()` 加状态守卫 (非 RUNNING 则忽略)
- [ ] `adc_collect.c`: `ADC_Collect_Start()` 加状态守卫
- [ ] `freertos.c`: ControlTask 管理状态转换
- **验证:** IDLE 状态下上位机发数据请求 → 无响应; START 后正常

---

## Phase 3: 生产级质量 (预计 18h)

**目标:** 加入生产嵌入式系统所需的监控、错误处理、架构优化。

### Step 3.1 — 增强系统监控
- [ ] `system_monitor.h`: 扩展 `SystemMonitor_t` (栈水位/堆/错误计数/运行时间)
- [ ] `system_monitor.c`: 实现 `Monitor_UpdateTaskStacks()`
- [ ] `system_monitor.c`: 堆监控 (xPortGetFreeHeapSize)
- [ ] `FreeRTOSConfig.h`: 启用 `configGENERATE_RUN_TIME_STATS`
- [ ] `tim.c`: 配置 TIM3 为 32-bit 自由运行计数器

### Step 3.2 — OLED 多页显示
- [ ] `freertos.c`: OLEDTask 实现 3 页切换
- [ ] 页0: 实时指标, 页1: 系统健康, 页2: 信号特征

### Step 3.3 — 优先级文档 + 分析
- [ ] `freertos.c`: 在每个任务定义上方加入优先级选择理由注释
- [ ] 调整: ControlTask → AboveNormal, ProcessTask/CommTask → Normal

### Step 3.4 — 错误恢复框架
- [ ] 新建 `user/Common/error_handler.h`: 错误码枚举 + API 声明
- [ ] 新建 `user/Common/error_handler.c`: 错误记录 + 恢复策略
- [ ] `usart.c`: 实现 `HAL_UART_ErrorCallback()`
- [ ] `adc_collect.c`: ADC overrun 检测

### Step 3.5 — 增强命令协议
- [ ] `packet.c`: 实现 ACK/NACK 响应
- [ ] `packet.c`: 实现 GET_STATUS 响应
- [ ] `packet.c`: 实现 GET_VERSION 响应

---

## Phase 4: 可移植性与文档 (预计 17h)

**目标:** 为面试准备可展示的架构文档和跨平台分析。

### Step 4.1 — HAL 抽象层
- [ ] 新建 `user/Common/hal_port.h`: 统一接口声明
- [ ] 新建 `user/Common/hal_port_f103.c`: F103 实现
- [ ] `burst.c` / `adc_collect.c` / `packet.c`: 改用 hal_port 接口

### Step 4.2 — F103 vs F4/H7 对比分析
- [ ] 新建 `docs/f103_vs_f4_comparison.md`

### Step 4.3 — Doxygen 文档
- [ ] 所有 .c/.h 文件: 加入 Doxygen 标准注释

### Step 4.4 — 性能分析报告
- [ ] 新建 `docs/performance_analysis.md`
- [ ] 实测: CPU 利用率 / ISR 延迟 / Task 唤醒延迟 / RAM 占用

---

## 验证门禁 (每 Phase 结束必须通过)

```
□ 编译: 0 Error, 0 Warning
□ 运行: 上位机接收帧序号连续 (运行 5 分钟)
□ 异常: 看门狗复位正常
□ 监控: OLED 所有页面正常显示
□ 栈安全: 运行 30 分钟后检查栈高水位
```
