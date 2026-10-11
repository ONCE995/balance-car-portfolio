#include "i2c_soft.h"

#define SCL_H()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)  
#define SCL_L()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)  
#define SDA_H()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET)  
#define SDA_L()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET)
#define SDA_READ()  HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)

static uint32_t cycles_per_us = 0;

void i2c_soft_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET); // Set both pins high
}

void DWT_Delay_Init(void)
{
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
    cycles_per_us = SystemCoreClock / 1000000UL;
}

static void I2C_DelayUs(uint32_t us)
{
    DWT->CYCCNT = 0;
    while (DWT->CYCCNT < us * cycles_per_us);
}

void i2c_start(void){SDA_H();SCL_H();I2C_DelayUs(5);SDA_L();I2C_DelayUs(5);SCL_L();}
void i2c_stop(void){SDA_L();SCL_H();I2C_DelayUs(5);SDA_H();I2C_DelayUs(5);}

/* @brief  向 I2C 总线写入一个字节，并读取从设备的 ACK/NACK 响应。
  * @param  从设备的 ACK/NACK 响应（0 表示 ACK，1 表示 NACK）。
  * @retval 这个函数返回从设备的 ACK/NACK 响应（0 表示 ACK，1 表示 NACK）。
  */
uint8_t I2C_WriteByte(uint8_t data)
{
    for(int i=7;i>=0;i--)
    {
        (data & (1 << i))? SDA_H(): SDA_L();
        I2C_DelayUs(5);
        SCL_H();I2C_DelayUs(5);     /* 上升沿，从机采样 */
        SCL_L();
    }
    SDA_H();I2C_DelayUs(5);         /* 释放 SDA，等待 ACK/NACK */
    SCL_H();I2C_DelayUs(5);
    uint8_t ack = SDA_READ();
    SCL_L();
    return ack;
}

/* @brief  读取 I2C 总线上的一个字节，并发送 ACK/NACK 响应。
  * @param  ack_in: 要发送的 ACK/NACK 响应（0 表示 ACK，1 表示 NACK）。
  * @retval 这个函数返回从 I2C 总线读取的字节。
  */
uint8_t I2C_ReadByte(uint8_t ack_in)
{
    uint8_t data = 0;
    SDA_H();
    for(int i=7;i>=0;i--)
    {
        SCL_H();I2C_DelayUs(5);
        if(SDA_READ()) data |= (1 << i);    /* 读取并设置数据变量中的位 */
        SCL_L();I2C_DelayUs(5);
    }
    (ack_in)? SDA_H(): SDA_L();             /* 发送 ACK/NACK */
    I2C_DelayUs(5);
    SCL_H();I2C_DelayUs(5);
    SCL_L();I2C_DelayUs(5);
    SDA_H();                                /* 释放 SDA */
    return data;
}

uint16_t AS5600_ReadAngle(void)
{
    uint16_t high_byte, low_byte;
    i2c_start();
    I2C_WriteByte(0x36 << 1 | 0);            /* 发送设备地址（写操作）*/
    I2C_WriteByte(0x0C);                    /* 发送寄存器地址（角度高字节寄存器）*/
    i2c_start();
    I2C_WriteByte(0x36 << 1 | 1);         /* 发送设备地址（读操作）*/
    high_byte = I2C_ReadByte(0);         /* 读取高字节并发送 ACK */
    low_byte = I2C_ReadByte(1);         /* 读取低字节并发送 NACK */
    i2c_stop();
    return ((high_byte << 8) | low_byte) & 0x0FFF; /* 返回 12 位角度值，只有12位有效，前4位为0 */
}