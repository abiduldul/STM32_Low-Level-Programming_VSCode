#include "thread_sensor.h"
#include "usart1_rs485.h"
#include "uart1.h"
#include "bgt_w87x.h"
#include "bsp.h"

bgt_sens sensor_data;

void thread_sensor(ULONG thread_input) {
    LPUART1_Init();
    USART1_RS485_Init();
    tx_thread_sleep(MS_TO_TICKS(100));

    uint8_t tx_frame[8];
    char pc_message[200];
    // uint8_t rx_buffer[20]; 

    while (1) {            
        tx_frame[0] = 0x08; 
        tx_frame[1] = 0x03; 
        tx_frame[2] = 0x00; 
        tx_frame[3] = 0x00; 
        tx_frame[4] = 0x00; 
        tx_frame[5] = 0x0D; 

        uint16_t crc = Calculate_Modbus_CRC(tx_frame, 6);
        tx_frame[6] = (uint8_t)(crc & 0xFF);        
        tx_frame[7] = (uint8_t)((crc >> 8) & 0xFF); 

        // HAL_UART_Transmit(&hlpuart1, tx_frame, 8, 100);

        RS485_SendRaw(tx_frame, 8);

        UINT status_sem = tx_semaphore_get(&modbus_rx_sem, MS_TO_TICKS(1000));
        __HAL_UART_DISABLE_IT(&huart1, UART_IT_IDLE);

        if (status_sem != TX_SUCCESS) {
            HAL_UART_DMAStop(&huart1); // Paksa matikan DMA karena sensor tidak merespon
            
            sprintf(pc_message, "Error Modbus: Sensor Timeout / No Reply\r\n");
            HAL_UART_Transmit(&hlpuart1, (uint8_t *)pc_message, strlen(pc_message), 100); // Kirim error ke PC
            
            tx_thread_sleep(MS_TO_TICKS(2000)); // Tunggu 2 detik sebelum mencoba lagi
            continue; // Melompat kembali ke awal baris `while(1)`
        }

        // Proteksi jika terjadi kesalahan pembacaan register CNDTR (0 byte)
        // if (modbus_rx_len == 0) {
        //     tx_thread_sleep(MS_TO_TICKS(5));
        //     continue; 
        // }

        // LANGKAH 9: Terjemahkan data HEX dari sensor menjadi teks yang bisa dibaca di PC
        int cursor = 0;
        cursor += sprintf(&pc_message[cursor], "Sensor Reply (%d bytes): ", modbus_rx_len);

        for (int i = 0; i < modbus_rx_len; i++) {
            cursor += sprintf(&pc_message[cursor], "%02X ", modbus_rx_buffer[i]);
        }
        sprintf(&pc_message[cursor], "\r\n");
        
        // decode_bgt(modbus_rx_buffer, &sensor_data);

        // LANGKAH 10: Kirim laporan hasil pembacaan sensor ke PC monitor via LPUART1
        hlpuart1.gState = HAL_UART_STATE_READY; //
        HAL_UART_Transmit(&hlpuart1, (uint8_t *)pc_message, strlen(pc_message),200); //
        
        sprintf(pc_message,
        "Temp : %d C\r\n"
        "Hum  : %d %%\r\n"
        "Pres : %d hPa\r\n"
        "Wind : %d m/s\r\n"
        "Dir  : %u deg\r\n",
        sensor_data.temp,
        sensor_data.humidity,
        sensor_data.pressure,
        sensor_data.wind_speed,
        sensor_data.wind_direction);

        HAL_UART_Transmit(&hlpuart1, (uint8_t *)pc_message, strlen(pc_message), 200);
        
        tx_queue_send(&bgt_sensor_data_q, &sensor_data, TX_NO_WAIT);
        // Beri jeda 2 milidetik (atau disesuaikan) sebelum melakukan polling berikutnya
        tx_thread_sleep(MS_TO_TICKS(2000)); //
    }
}