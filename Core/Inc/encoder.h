#ifndef ENCODER_H
#define ENCODER_H

/* 编码器转速读取。返回单位：转/min(RPM)，负值表示反向
 * 依赖 TIM2(编码器模式) 与 TIM4(中断采样) */
float Encoder_Getspeed(void);
float Encoder_GetAngle(void);  /* 编码器角度读取。返回单位：度，负值表示反向 */
void Encoder_Update(void);
#endif