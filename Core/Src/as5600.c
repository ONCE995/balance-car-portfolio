#include "as5600.h"
#include "i2c_soft.h"

#define AS5600_ADDR_W  (0x36 << 1)      /* 0x6C */
#define AS5600_ADDR_R  ((0x36 << 1)|1)  /* 0x6D */

/* 读一个寄存器，返回 0=成功 1=无应答 */
uint8_t AS5600_ReadReg(uint8_t reg, uint8_t *data)
{
    i2c_start();
    if (I2C_WriteByte(AS5600_ADDR_W))  { i2c_stop(); return 1; }
    if (I2C_WriteByte(reg))           { i2c_stop(); return 1; }
    i2c_start();                        /* repeated start */
    if (I2C_WriteByte(AS5600_ADDR_R)) { i2c_stop(); return 1; }
    *data = I2C_ReadByte(1);            /* 唯一字节，NACK 结束 */
    i2c_stop();
    return 0;
}

/* 写一个寄存器，返回 0=成功 1=无应答 */
uint8_t AS5600_WriteReg(uint8_t reg, uint8_t val)
{
    i2c_start();
    if (I2C_WriteByte(AS5600_ADDR_W)) { i2c_stop(); return 1; }
    if (I2C_WriteByte(reg))          { i2c_stop(); return 1; }
    if (I2C_WriteByte(val))          { i2c_stop(); return 1; }
    i2c_stop();
    return 0;
}

/* 读 12 位角度原始值（连续读 0x0C/0x0D，比两次 ReadReg 快） */
uint16_t AS5600_ReadAngle(void)
{
    uint16_t hi, lo;
    i2c_start();
    if (I2C_WriteByte(AS5600_ADDR_W))  { i2c_stop(); return 0; }
    if (I2C_WriteByte(0x0C))          { i2c_stop(); return 0; }
    i2c_start();
    if (I2C_WriteByte(AS5600_ADDR_R)) { i2c_stop(); return 0; }
    hi = I2C_ReadByte(0);              /* ACK：还有下个字节 */
    lo = I2C_ReadByte(1);              /* NACK：读完了 */
    i2c_stop();
    return ((uint16_t)(hi << 8) | lo) & 0x0FFF;     /*保证返回值合并后是两个字节、在有效范围内*/
}

/* 磁铁状态：MD=0 有磁铁(正常) ML/MH=弱/过强 */
void AS5600_GetMagnetStatus(uint8_t *MD, uint8_t *ML, uint8_t *MH)
{
    uint8_t stat;
    if (AS5600_ReadReg(0x0B, &stat)) { /* 通信失败按无磁铁处理 */
        *MD = 1; *ML = 0; *MH = 0; return;
    }
    *MD = (stat >> 5) & 0x01;
    *ML = (stat >> 4) & 0x01;
    *MH = (stat >> 3) & 0x01;
}