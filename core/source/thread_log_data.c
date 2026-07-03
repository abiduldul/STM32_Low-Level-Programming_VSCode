#include "bsp.h"
#include "sd.h"
#include "uart1.h"
#include "bgt_w87x.h"

#define SECTOR_SIZE 512
#define LOG_START_SECTOR   1000
#define LOG_END_SECTOR     1020  // Batas akhir lapak logging

uint8_t ram_log_buffer[SECTOR_SIZE];
uint16_t buffer_index = 0;
uint32_t current_sd_sector = 1000;

uint8_t read_buffer[512];
char pc_message[600];

void thread_log_data (ULONG thread_input) {
    SDMMC1_SD_Init();
    tx_thread_sleep(MS_TO_TICKS(1000));

    char sample_log[100];
    bgt_sens received_data_sens;
    while (1)
    {
        UINT status = tx_queue_receive(&bgt_sensor_data_q, &received_data_sens, TX_WAIT_FOREVER);
        
        if (status == TX_SUCCESS) {
            snprintf(pc_message, sizeof(pc_message), "temp : %f \t hum : %f \t press : %f \t wind_speed : %f, wind_dir : %d \r\n",
                        received_data_sens.temp, received_data_sens.humidity, received_data_sens.pressure, received_data_sens.wind_speed, received_data_sens.wind_direction);
            HAL_UART_Transmit(&hlpuart1, (uint8_t *)pc_message, strlen(pc_message), 500);
        }
        // if (status == TX_SUCCESS) {
        //     snprintf(pc_message, sizeof(pc_message), "temp : %f \t hum : %f \t press : %f \t wind_speed : %f, wind_dir : %d \r\n",
        //              received_data_sens.temp, received_data_sens.humidity, received_data_sens.pressure, received_data_sens.wind_speed, received_data_sens.wind_direction);
        //     // HAL_UART_Transmit(&hlpuart1, (uint8_t *)pc_message, strlen(pc_message), 500);
        
        //     int log_len = snprintf(pc_message, sizeof(pc_message), "temp : %f \t hum : %f \t press : %f \t wind_speed : %f, wind_dir : %d \r\n",
        //                        received_data_sens.temp, received_data_sens.humidity, received_data_sens.pressure, received_data_sens.wind_speed, received_data_sens.wind_direction);
            

        //     if ((buffer_index + log_len) >= SECTOR_SIZE) {
        //         SD_write_raw_sector(current_sd_sector, ram_log_buffer);

        //         current_sd_sector++;
        //         if (current_sd_sector > LOG_END_SECTOR) {
        //             current_sd_sector = LOG_START_SECTOR; 
        //         }

        //         buffer_index = 0;
        //         memset(ram_log_buffer, 0, SECTOR_SIZE);

        //         HAL_StatusTypeDef status = SD_read_raw_sector(current_sd_sector, read_buffer);

        //         if (status == HAL_OK) {
        //             snprintf(pc_message, sizeof(pc_message), "Isi sektor %d : \r\n%s\r\n", current_sd_sector, (char*)read_buffer);
        //             HAL_UART_Transmit(&hlpuart1, (uint8_t *)pc_message, strlen(pc_message), 500);
        //         } else {
        //             HAL_UART_Transmit(&hlpuart1, (uint8_t *)"Gagal membaca SD\r\n", 18, 100);
        //         }
        //     }

        //     memcpy(&ram_log_buffer[buffer_index], sample_log, log_len);
        //     buffer_index += log_len;
        // }

        tx_thread_sleep(MS_TO_TICKS(2000));
    }
}