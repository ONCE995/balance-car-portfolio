#ifndef ENCODER_H
#define ENCODER_H

/* 编码器转速读取。返回单位：转/min(RPM)，负值表示反向
 * 依赖 TIM2(编码器模式) 与 TIM4(中断采样) */
float Encoder_Getspeed(void);

#endif