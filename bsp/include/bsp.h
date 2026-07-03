#ifndef __BSP_H__
#define __BSP_H__

#include "stm32l4xx_hal.h"
#include "tx_api.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define MS_TO_TICKS(ms) ((ms) * 1000UL / 1000UL)

// extern UART
extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart1;

extern TX_SEMAPHORE pc_rx_sem;
extern TX_SEMAPHORE modbus_rx_sem;
extern TX_QUEUE bgt_sensor_data_q;
extern uint8_t pc_rx_buffer[], modbus_rx_buffer[];
extern uint16_t pc_rx_len, modbus_rx_len;

#endif