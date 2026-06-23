#include "bsp.h"
#include "sd.h"
#include "uart1.h"

void thread_log_data (ULONG thread_input) {
    SDMMC1_SD_Init();
    tx_thread_sleep(MS_TO_TICKS(1000));

    // SDCard_SimpleTest();
    while (1)
    {
        tx_thread_sleep(MS_TO_TICKS(1000));
        
    }
}