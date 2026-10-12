#ifndef I2C_SOFT_H
#define I2C_SOFT_H
#include "main.h"
#include "stdio.h"

#define SCL_H()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)  
#define SCL_L()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)  
#define SDA_H()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET)  
#define SDA_L()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET)
#define SDA_READ()  HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)

void DWT_Delay_Init(void);
void i2c_soft_init(void);
uint8_t I2C_WriteByte(uint8_t data);
uint8_t I2C_ReadByte(uint8_t ack);
void i2c_start(void);
void i2c_stop(void);
void i2c_write_reg(uint8_t reg, uint8_t val);
uint8_t i2c_read_reg(uint8_t reg);

#endif