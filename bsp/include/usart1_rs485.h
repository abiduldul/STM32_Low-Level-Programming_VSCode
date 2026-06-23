#ifndef __USART1_RS485_H__
#define __USART1_RS485_H__

#include "system.h"

void USART1_RS485_Init(void);
void RS485_SendRaw(uint8_t *data, uint16_t length);

#endif