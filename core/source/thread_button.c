#include "bsp.h"

void thread_button(ULONG thread_input) {
    gpio_init();
    HAL_Delay(100);
    // char hay[100];
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