#ifndef __AS5600_H
#define __AS5600_H

#include "stdint.h"

uint8_t AS5600_WriteReg(uint8_t reg_addr, uint8_t data);
uint8_t AS5600_ReadReg(uint8_t reg_addr, uint8_t *p_data);
void AS5600_GetMagnetStatus(uint8_t *MD, uint8_t *ML, uint8_t *MH);
uint16_t AS5600_ReadAngle(void);

#endif
