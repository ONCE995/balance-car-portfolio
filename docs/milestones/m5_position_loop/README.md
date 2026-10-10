# M5  位置环（编码器角度累积 + 位置-速度串级）

## 1. 目标
在 M4（速度环）基础上，实现基于编码器反馈的**位置闭环**，并搭建
**位置环（外环） → 速度环（内环） → PWM** 的串级控制，使电机轴稳定停在目标角度。

## 2. 设计

### 2.1 编码器（encoder.c）
- 状态用 `static` 变量封装在模块内，不对外 `extern`：
  - `last_count`：上一次计数值
  - `delta_count`：本周期计数增量
  - `s_pos`：累计脉冲数（记录所有角度变化）
- `Encoder_Update()`：每周期读一次硬件，算增量并累加 `s_pos`
- `Encoder_GetAngle()`：`s_pos × 360 / PULSE_PER_REVOLUTION`（度）
- `Encoder_Getspeed()`：`delta_count × 60 / PULSE_PER_REVOLUTION / ENCODER_DT`（rpm）

> 硬件只在 `Encoder_Update()` 读一次，放到全局变量上。

### 2.2 位置环（main.c，复用 PID_t）
- 复用速度环的 `PID_t` 结构体与 `PID_Calc()`，不另写算法
- **纯 P**：`Kp = 25 / 15 ≈ 1.67`，单位 `rpm/度`
- kp 本身就是"每度误差给多少 rpm"的换算器，度 × kp = rpm
- 输出限幅 ±25 rpm，`|误差| ≤ 15°` 线性、`> 15°` 自动削到 25
PS:想要n度以上，输出限幅除以n，再给到kp即可.
- 外环输出即**目标转速**，直接喂给速度环


### 2.3 注意点
- 主循环必须是 `while(1){ if(flag_pid){...} }`，**不能**用 `while(flag_pid)`（清标志后会退出主循环），main函数会直接过，不再进
- 必须先调 `Encoder_Update()`，否则角度/速度恒为 0
- 速度环限幅需与 TIM3 的 ARR 对齐（本项目 ARR 分辨率为 100，故 ±100 合适）

## 3. 参数

| 参数 | 值 | 说明 |
|---|---|---|
| `PULSE_PER_REVOLUTION` | 11 × 171 × 4 = 7524 | 线数×减速比×4倍频 |
| `ENCODER_DT` / `ENCODER_SAMPLE_TIME` | 0.01 s | 与 TIM4 中断周期一致 |
| `POS_KP` | 1.67 | rpm/度 = 25/15，饱和分界 15° |
| `POS_KI` | 0 | 外环不用积分（见 notes） |
| `POS_KAW` | 0 | 纯 P 无需抗饱和 |
| `POS_OUT_MAX` | 25 rpm | 位置环输出限幅 |

## 4. 实测

| 项目 | 数据 |
|---|---|
| 正转 +90° | 目标 90 / 实测 90.05~90.19，稳态误差约 0.05°~0.19°（纯 P） |
| 反转 −90° | 能反向跟踪到位 √ |
| 纯 P 阶跃 | < 1s 到位，曲线干净无过冲 √ |
| 加积分（已弃用） | 纯 I → 过冲后缓慢回落；I + KAW=0.3 → 卡在 ~77° 极慢上爬 |
| VOFA+ 通道 | `target_angle / real_angle / target_speed / pwm_out` |

## 5. 标准与波形图
------------ 标准 ------------
| # | 标准 | 状态 |
|---|---|---|
| 1 | 编码器角度累积正确（转动线性、可归位） | √ |
| 2 | 位置-速度串级能收敛到目标角 | √ |
| 3 | 稳态误差在设计范围（< 1°） | √（0.05°~0.19°） |
| 4 | 双向跟踪（正反转都能追） | √ |
| 5 | 无积分饱和导致的过冲/震荡 | √（改用纯 P） |
| 6 | 波形文档展示 | √ |

------------ 波形图 ------------
| 截图 | 配置 | 现象 |
|---|---|---|
| [step_plus90_pureP.png](step_plus90_pureP.png) | 纯 P，目标 +90° | <1 s 到位，无过冲，稳态 0.04°~0.19° |
| [step_minus90_pureP.png](step_minus90_pureP.png) | 纯 P，目标 −90° | 与正转相当（<1 s 到位，无过冲，稳态 0.04°~0.19°） |
| [windup_pi_overshoot.png](windup_pi_overshoot.png) | P+I(Ki=0.02,Kaw=0) | 快但过冲，缓慢回落(教训见 notes) |
| [windup_kaw_slowclimb.png](windup_kaw_slowclimb.png) | P+I+KAW=0.3 | 卡 ~75°，极慢上爬(教训见 notes) |


**结论：M5 通过。** 位置环确定为**纯 P**；消除静差的任务交给内环速度环。

## 6. 相关

- 调参教训：`docs/notes/motor/outer_loop_integral_lessons.md`
- 下一步（M6）：无刷电机 + AS5600 磁编码器，软件模拟 I2C 通信读角度。
  本阶段的串级框架（外环→速度环→PWM）与调试方法将整体迁移过去。