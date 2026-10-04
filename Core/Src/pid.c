#include "pid.h"

void PID_Init(PID_t *pid, float Kp, float Ki,
              float out_min, float out_max)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->integrate = 0.0f;
    pid->last_error = 0.0f;
    pid->out_max = out_max;
    pid->out_min = out_min;
    /*积分限幅上限 = 输出上限 / Ki*/
    pid->int_max = (Ki > 0.0f) ? (out_max / Ki) : 0.0f;
}

float PID_Calc(PID_t *pid, float target, float actual, float dt)
{
    float error = target - actual;

    float out;
/* 抗积分饱和：先算 PI 项，若已越界则不累加积分，防止 windup */
    float pwm_out_pi = pid->Kp * error + pid->Ki * pid->integrate;
    if (pwm_out_pi < pid->out_max && pwm_out_pi > pid->out_min)
    {
        pid->integrate += error * dt;
    }
    /* 积分限幅：修正后的对称限幅。*/
    if (pid->integrate >  pid->int_max) pid->integrate =  pid->int_max;
    if (pid->integrate < -pid->int_max) pid->integrate = -pid->int_max;
    

    out = pid->Kp * error
        + pid->Ki * pid->integrate;

    /* 输出限幅（PWM 占空比 0~100） */
    if (out > pid->out_max) out = pid->out_max;
    if (out < pid->out_min) out = pid->out_min;

    return out;
}