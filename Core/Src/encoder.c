#include "encoder.h"
#include "tim.h"

/* 每转总脉冲数 = 编码器线数 * 减速比 * 4倍频(正交编码 A/B 双沿计数) */
#define PULSE_PER_REVOLUTION  (11.0f * 171.0f * 4.0f)

/* 采样周期 = TIM4 中断周期 = 10ms */
#define ENCODER_DT            (0.01f)

static uint16_t last_count = 0;  // 上一次计数值
static int16_t delta_count = 0; // 计数增量
static int32_t s_pos = 0; // 全局角度变量

void Encoder_Update(void)
{
    /* 读取 TIM2 的计数值 */
    uint16_t count = (uint16_t)TIM2->CNT;

    /* 计算计数增量 */
    delta_count = count - last_count;
    last_count = count;

    /* 累加角度 */
    s_pos += delta_count; 
}

float Encoder_GetAngle(void)
{
    float real_angle = (float)s_pos * 360.0f / PULSE_PER_REVOLUTION;
    return real_angle;
}

float Encoder_Getspeed(void)
{
    /* 速度 = 一个采样周期内的计数增量 / 每转脉冲数 / 采样时间
     * 结果单位为 转/min(RPM)，带回向符号。 */
    float speed = ((float)delta_count) * 60.0f / PULSE_PER_REVOLUTION / ENCODER_DT;
    return speed;
}