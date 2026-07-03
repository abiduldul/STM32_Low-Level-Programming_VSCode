#include "usart1_rs485.h"
#include "bgt_w87x.h"

void decode_bgt(uint8_t *buffer, bgt_sens *data){
    // 1. Dekode Temperature (Index 3 & 4) -> Resolusi 0.1
    int16_t raw_temp = (int16_t)((buffer[3] << 8) | buffer[4]);
    data->temp = raw_temp;

    // 2. Dekode Humidity (Index 5 & 6) -> Resolusi 0.1
    uint16_t raw_hum = (uint16_t)((buffer[5] << 8) | buffer[6]);
    data->humidity = raw_hum;

    // 3. Dekode Pressure (Index 15 & 16) -> Resolusi 0.1
    uint16_t raw_press = (uint16_t)((buffer[15] << 8) | buffer[16]);
    data->pressure = raw_press;

    // 4. Dekode Wind Speed (Index 25 & 26) -> Resolusi 0.01
    uint16_t raw_ws = (uint16_t)((buffer[25] << 8) | buffer[26]);
    data->wind_speed = raw_ws;

    // 5. Dekode Wind Direction (Index 27 & 28) -> Resolusi 1
    data->wind_direction = (uint16_t)((buffer[27] << 8) | buffer[28]);
}