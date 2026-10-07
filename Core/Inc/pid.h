#ifndef PID_H
#define PID_H

typedef struct {
    float Kp, Ki, Kd,Kaw;      /* 比例 / 积分 / 微分 / 抗积分饱和 系数 */
    float integrate;       /* 积分累加量 */
    float last_error;      /* 上次误差，用于微分项 */
    float out_min, out_max;/* 输出限幅范围 */
    float int_max;         /* 积分限幅上限 */
} PID_t;

void PID_Init(PID_t *pid, float Kp, float Ki,float Kaw,
              float out_min, float out_max);

float PID_Calc(PID_t *pid, float target, float actual, float dt);

#endif