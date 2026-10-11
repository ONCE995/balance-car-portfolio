#ifndef I2C_SOFT_H
#define I2C_SOFT_H
#include "main.h"

void DWT_Delay_Init(void);
void i2c_soft_init(void);
uint16_t AS5600_ReadAngle(void);

#endif