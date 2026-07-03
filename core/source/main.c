#include <stdio.h>
#include "system.h"
#include "thread_button.h"
#include "thread_log_data.h"
#include "thread_sensor.h"
#include "bgt_w87x.h"

#define STACK_SIZE_DEFAULT 2048
#define STACK_SIZE_LOG 2048
#define SENSOR_MSG_WORDS   ((sizeof(bgt_sens) + 3) / 4)
#define SENSOR_MSG_COUNT   10                               // Keep a backlog of 10 readings
#define QUEUE_TOTAL_BYTES  (SENSOR_MSG_WORDS * 4 * SENSOR_MSG_COUNT)

TX_SEMAPHORE pc_rx_sem;
TX_SEMAPHORE modbus_rx_sem;

TX_QUEUE bgt_sensor_data_q;

TX_THREAD core1;
TX_THREAD core2;
TX_THREAD core3;

char stack_area0[STACK_SIZE_DEFAULT];
char stack_area1[STACK_SIZE_DEFAULT];
char stack_area2[STACK_SIZE_LOG];

uint32_t sensor_queue_memory[QUEUE_TOTAL_BYTES / 4];

int main(void) {
    // 1. Enable the Hardware FPU (CP10 and CP11 Coprocessors)
    // This gives full access to the Floating Point Unit (FPU)
    SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));

    HAL_Init();
    SystemClock_Config(); // 80 MHz
    HAL_Delay(500);
    
    tx_kernel_enter();
    return 0;
}

void tx_application_define(void *first_unused_memory) {
    tx_semaphore_create(&pc_rx_sem, "PC RX Semaphore", 0);
    tx_semaphore_create(&modbus_rx_sem, "Modbus RX Semaphore", 0);

    tx_queue_create(&bgt_sensor_data_q, "Sensor Queue", SENSOR_MSG_WORDS,
                    sensor_queue_memory, QUEUE_TOTAL_BYTES);
    tx_thread_create(&core1, "core1", 
                     thread_button, 0, 
                     stack_area0, STACK_SIZE_DEFAULT, 
                     2, 2, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&core2, "core2", 
                     thread_sensor, 0, 
                     stack_area1, STACK_SIZE_DEFAULT,
                     1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&core3, "core3",
                     thread_log_data, 0, 
                     stack_area2, STACK_SIZE_LOG,
                     3, 3, TX_NO_TIME_SLICE, TX_AUTO_START);
}