#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "bsp.h"

void gpio_init(void);
uint32_t HAL_GetTick(void);
void RS485_SendRaw(uint8_t *data, uint16_t length);
void SystemClock_Config(void);
uint16_t Calculate_Modbus_CRC(uint8_t *buffer, uint16_t length);

extern SD_HandleTypeDef hsd_sdmmc1;
void SDMMC1_SD_Init(void);

#endif