#ifndef __BGT_W87X_H__
#define __BGT_W87X_H__

#include "system.h"

typedef struct 
{
    uint16_t temp;
    uint16_t humidity;
    uint16_t pressure;
    uint16_t wind_speed;
    uint16_t wind_direction; 
} bgt_sens;

void decode_bgt(uint8_t *buffer, bgt_sens *data);
#endif