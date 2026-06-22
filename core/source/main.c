#include <stdio.h>
#include "system.h"
#include "uart1_init.h"
#include "usart1_rs485_init.h"
#include "sd_init.h"

#define STACK_SIZE_DEFAULT 2048
#define STACK_SIZE_LOG 2048

extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart1;
extern uint8_t pc_rx_buffer[], modbus_rx_buffer[];
extern uint16_t pc_rx_len, modbus_rx_len;

TX_SEMAPHORE pc_rx_sem;
TX_SEMAPHORE modbus_rx_sem;

TX_THREAD numero_uno;
TX_THREAD numero_duo;
TX_THREAD numero_tres;

void thread_button(ULONG thread_input);
void thread_read_sensor(ULONG thread_input);
void thread_log_data(ULONG thread_input);

char stack_area0[STACK_SIZE_DEFAULT];
char stack_area1[STACK_SIZE_DEFAULT];
char stack_area2[STACK_SIZE_LOG];

int main(void) {
    HAL_Init();
    SystemClock_Config(); // 80 MHz
    HAL_Delay(500);

    gpio_init();
    HAL_Delay(100);

    LPUART1_Init();
    USART1_RS485_Init();
    HAL_Delay(100);
    
    tx_kernel_enter();
    return 0;
}

void tx_application_define(void *first_unused_memory) {
    tx_semaphore_create(&pc_rx_sem, "PC RX Semaphore", 0);
    tx_semaphore_create(&modbus_rx_sem, "Modbus RX Semaphore", 0);

    tx_thread_create(&numero_uno, "Numero Uno", 
                     thread_button, 0, 
                     stack_area0, STACK_SIZE_DEFAULT, 
                     2, 2, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&numero_duo, "Numero duo", 
                     thread_read_sensor, 0, 
                     stack_area1, STACK_SIZE_DEFAULT,
                     1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&numero_tres, "Numero tres",
                     thread_log_data, 0, 
                     stack_area2, STACK_SIZE_LOG,
                     3, 3, TX_NO_TIME_SLICE, TX_AUTO_START);
}

void thread_button(ULONG thread_input) {
    char hay[100];
    while (1)
    {
        // if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
        //     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
        // }else {
        //     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
        //     sprintf(hay, "Menyala abangku...\r\n");
        //     HAL_UART_Transmit(&hlpuart1, (uint8_t *)hay, strlen(hay), 200);
        // }
        tx_thread_sleep(1);
    }
}

void thread_read_sensor(ULONG thread_input) {
    uint8_t tx_frame[8];
    char pc_message[200];
    // uint8_t rx_buffer[20]; 

    while (1) {            
        tx_frame[0] = 0x08; 
        tx_frame[1] = 0x03; 
        tx_frame[2] = 0x00; 
        tx_frame[3] = 0x00; 
        tx_frame[4] = 0x00; 
        tx_frame[5] = 0x01; 

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

        // LANGKAH 10: Kirim laporan hasil pembacaan sensor ke PC monitor via LPUART1
        hlpuart1.gState = HAL_UART_STATE_READY; //
        HAL_UART_Transmit(&hlpuart1, (uint8_t *)pc_message, strlen(pc_message),200); //

        // Beri jeda 2 milidetik (atau disesuaikan) sebelum melakukan polling berikutnya
        tx_thread_sleep(MS_TO_TICKS(2000)); //
    }
}

void thread_log_data (ULONG thread_input) {
    SDMMC1_SD_Init();
    tx_thread_sleep(MS_TO_TICKS(1000));

    // SDCard_SimpleTest();
    while (1)
    {
        tx_thread_sleep(MS_TO_TICKS(1000));
        
    }
}