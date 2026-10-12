#include "i2c_soft.h"

static uint32_t cycles_per_us = 0;

void i2c_soft_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();                 /* 打开 GPIOB 时钟 */

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 优化点1（对比第一版/哈希8eec04d）：第一版漏设 Speed，
     * 导致 STM32F1 的 MODE[1:0] = 00，引脚实际被配成了“输入”而非“输出”，
     * 整条 I2C 总线被两个上拉电阻钳死在 3.3V 恒定高电平，SCL/SDA 永远拉不低。
     * 这里必须显式指定 Speed（如 LOW=2MHz），引脚才会真正进入输出模式。 */
    GPIO_InitStruct.Pin   = GPIO_PIN_6;           /* PB6 -> SCL，I2C 时钟线 */
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  /* 推挽输出：SCL 由 MCU 强驱动拉高拉低 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = GPIO_PIN_7;           /* PB7 -> SDA，I2C 数据线 */
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;  /* 开漏输出：配合外部上拉电阻实现线与、双向 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* 优化点2：显式把两根线初始化到空闲高电平（I2C 总线空闲态），并
     * 在 OS 内核模型上保证软件模拟 I2C 的起点是“总线释放”状态。 */
    SCL_H();
    SDA_H();
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

void i2c_start(void){SDA_H();SCL_H();I2C_DelayUs(10);SDA_L();I2C_DelayUs(10);SCL_L();}
void i2c_stop(void){SDA_L();SCL_H();I2C_DelayUs(10);SDA_H();I2C_DelayUs(10);}

/* @brief  向 I2C 总线写入一个字节，并读取从设备的 ACK/NACK 响应。
  * @param  从设备的 ACK/NACK 响应（0 表示 ACK，1 表示 NACK）。
  * @retval 这个函数返回从设备的 ACK/NACK 响应（0 表示 ACK，1 表示 NACK）。
  */
uint8_t I2C_WriteByte(uint8_t data)
{
    for(int i=7;i>=0;i--)
    {
        (data & (1 << i))? SDA_H(): SDA_L();
        I2C_DelayUs(10);
        SCL_H();I2C_DelayUs(10);     /* 上升沿，从机采样 */
        SCL_L();
    }
    SDA_H();I2C_DelayUs(10);         /* 释放 SDA，等待 ACK/NACK */
    SCL_H();I2C_DelayUs(10);
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
    SDA_H();                                /* 释放 SDA，准备读取数据 */
    I2C_DelayUs(10);
    for(int i=7;i>=0;i--)
    {
        SCL_H();I2C_DelayUs(10);
        if(SDA_READ()) data |= (1 << i);    /* 读取并设置数据变量中的位 */
        SCL_L();I2C_DelayUs(10);
    }
    (ack_in)? SDA_H(): SDA_L();             /* 发送 ACK/NACK */
    I2C_DelayUs(10);
    SCL_H();I2C_DelayUs(10);
    SCL_L();I2C_DelayUs(10);
    SDA_H();                                /* 释放 SDA */
    return data;
}