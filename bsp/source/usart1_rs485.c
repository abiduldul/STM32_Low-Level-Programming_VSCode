#include "usart1_rs485.h"
#include "system.h"

UART_HandleTypeDef huart1;

DMA_HandleTypeDef hdma_usart1_tx; //rs485
DMA_HandleTypeDef hdma_usart1_rx;

void USART1_RS485_Init(void) {
    // 1. Enable Peripheral Clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();   // For PA9 (TX) and PA10 (RX)
    __HAL_RCC_GPIOB_CLK_ENABLE();   // For PB3 (DE/RE)
    __HAL_RCC_USART1_CLK_ENABLE();  // For USART1 Core

    // Configure PA9 (TX) and PA10 (RX)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;        
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;   // AF7 is the routing for USART1
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure PB3 as the RS-485 DE/RE Control Pin
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;    // Standard Digital Output
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

    // Demux RS485/RS232 (RS485 = HIGH, RS232 = LOW)
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);

    // konfigurasi untuk TX DMA
    hdma_usart1_tx.Instance = DMA1_Channel3;                 // Channel for USART1_TX
    hdma_usart1_tx.Init.Request = DMA_REQUEST_USART1_TX;    // Route request to USART1
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;    // RAM to UART
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;        // UART register address stays the same
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;            // Move to next character in RAM string
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;                   // Send once, then stop
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&hdma_usart1_tx);
    __HAL_LINKDMA(&huart1, hdmatx, hdma_usart1_tx);

    // konfigurasi untuk RX DMA
    hdma_usart1_rx.Instance = DMA1_Channel4;
    hdma_usart1_rx.Init.Request = DMA_REQUEST_USART1_RX;    // Route request to USART1
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;    // From UART to RAM
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;        // UART register address stays the same
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;            // Increment RAM address for each received byte
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_NORMAL;                   // Receive once, then stop
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_HIGH;        // High priority to avoid missing bytes
    HAL_DMA_Init(&hdma_usart1_rx);
    __HAL_LINKDMA(&huart1, hdmarx, hdma_usart1_rx);

    // Configure USART1 Parameters
    huart1.Instance = USART1;                  
    huart1.Init.BaudRate = 9600;              
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;         
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);

    // Interupt di level CPU
    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 6, 0); // Untuk DMA RX
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

    HAL_NVIC_SetPriority(USART1_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

// ==========================================
// RS-485 RAW HEX TRANSMIT
// ==========================================
#define MODBUS_RX_BUFF_SIZE 128
uint8_t modbus_rx_buffer[MODBUS_RX_BUFF_SIZE];
uint16_t modbus_rx_len = 0;

void RS485_SendRaw(uint8_t *data, uint16_t length) {
    // 1. DE/RE HIGH -> Transmit Mode
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
    
    HAL_UART_DMAStop(&huart1); // Pastikan tidak ada DMA yang sedang berjalan

    // 2. Send the exact number of bytes
    HAL_UART_Transmit_DMA(&huart1, data, length);
    // tx_thread_sleep(MS_TO_TICKS(2));
    
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);
    
    // 3. DE/RE LOW -> Listen Mode (Do this immediately so we don't miss the reply!)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

    ////////// Reset DMA untuk siap menerima balasan //////////
    __HAL_UART_CLEAR_FLAG(&huart1, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF  | UART_CLEAR_PEF);
    __HAL_UART_CLEAR_IDLEFLAG(&huart1);              // drop any stale IDLE
    HAL_UART_Receive_DMA(&huart1, modbus_rx_buffer, MODBUS_RX_BUFF_SIZE);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
}

void DMA1_Channel3_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

void DMA1_Channel4_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

extern TX_SEMAPHORE modbus_rx_sem;

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
    // 1. Cek apakah interupsi berasal dari IDLE Line milik USART1
    if ((USART1->ISR & USART_ISR_IDLE) != RESET) {
        
        // 2. BERSIHKAN FLAG IDLE di register ICR agar tidak terjadi badai interupsi
        USART1->ICR = USART_ICR_IDLECF;

        // 3. MATEMATIKA LOW-LEVEL CNDTR: Amptip sisa slot kosong di DMA1 Channel 4
        // uint32_t sisa_slot = DMA1_Channel4->CNDTR;
        uint32_t sisa_slot = huart1.hdmarx->Instance->CNDTR;
        modbus_rx_len = MODBUS_RX_BUFF_SIZE - sisa_slot;

        // 4. Hentikan DMA penerimaan agar data terkunci aman di RAM
        HAL_UART_DMAStop(&huart1);

        // 5. Bangunkan thread1 (Modbus Thread) melalui Semaphore
        tx_semaphore_put(&modbus_rx_sem);
        
        return;
    }

    // Biarkan HAL mengurusi error flag bawaan lainnya jika ada masalah transmisi
    HAL_UART_IRQHandler(&huart1);
}