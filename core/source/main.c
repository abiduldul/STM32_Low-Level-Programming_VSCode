#include <stdio.h>
#include "system.h"
#include "thread_button.h"
#include "thread_log_data.h"
#include "thread_sensor.h"

#define STACK_SIZE_DEFAULT 2048
#define STACK_SIZE_LOG 2048

TX_SEMAPHORE pc_rx_sem;
TX_SEMAPHORE modbus_rx_sem;

TX_THREAD core1;
TX_THREAD core2;
TX_THREAD core3;

char stack_area0[STACK_SIZE_DEFAULT];
char stack_area1[STACK_SIZE_DEFAULT];
char stack_area2[STACK_SIZE_LOG];

int main(void) {
    HAL_Init();
    SystemClock_Config(); // 80 MHz
    HAL_Delay(500);

    gpio_init();
    HAL_Delay(100);
    
    tx_kernel_enter();
    return 0;
}

void tx_application_define(void *first_unused_memory) {
    tx_semaphore_create(&pc_rx_sem, "PC RX Semaphore", 0);
    tx_semaphore_create(&modbus_rx_sem, "Modbus RX Semaphore", 0);

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