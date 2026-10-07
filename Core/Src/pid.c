#include "pid.h"

void PID_Init(PID_t *pid, float Kp, float Ki,float Kaw,
              float out_min, float out_max)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kaw = Kaw;
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

    float out_unsat = pid->Kp * error + pid->Ki * pid->integrate;

    float out = out_unsat;
    if (out > pid->out_max) out = pid->out_max;
    if (out < pid->out_min) out = pid->out_min;

    if (pid->Ki > 0.0f )
    {   /*反算式抗饱和：饱和时把"被砍掉的部分"折算回积分抽走；
         * 未饱和时 out==out_unsat，修正项自动为 0，就是纯积分 */
        pid->integrate += error * dt+ pid->Kaw * (out - out_unsat) / pid->Ki;
        if (pid->integrate >  pid->int_max) pid->integrate =  pid->int_max;
        if (pid->integrate < -pid->int_max) pid->integrate = -pid->int_max;
    }
    return out;
}