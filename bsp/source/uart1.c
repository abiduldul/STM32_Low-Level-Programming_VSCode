#include "uart1.h"
#include "system.h"

UART_HandleTypeDef hlpuart1;

DMA_HandleTypeDef hdma_lpuart1_tx; // komunikasi PC
DMA_HandleTypeDef hdma_lpuart1_rx;

void LPUART1_Init(void) {
    // 1. Enable Clocks for Port C and LPUART1
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMAMUX1_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_LPUART1_CLK_ENABLE();

    // 2. Configure PC0 (RX) and PC1 (TX)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1; // Target both pins at once
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;        // Alternate Function, Push-Pull
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;  // Tell HAL to route them to LPUART1
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // konfigurasi untuk TX DMA
    hdma_lpuart1_tx.Instance = DMA1_Channel1;                 // Channel for LPUART1_TX
    hdma_lpuart1_tx.Init.Request = DMA_REQUEST_LPUART1_TX;    // Route request to LPUART1
    hdma_lpuart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;    // RAM to UART
    hdma_lpuart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;        // UART register address stays the same
    hdma_lpuart1_tx.Init.MemInc = DMA_MINC_ENABLE;            // Move to next character in RAM string
    hdma_lpuart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_lpuart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_lpuart1_tx.Init.Mode = DMA_NORMAL;                   // Send once, then stop
    hdma_lpuart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&hdma_lpuart1_tx);
    __HAL_LINKDMA(&hlpuart1, hdmatx, hdma_lpuart1_tx);

    // konfigurasi untuk RX DMA
    hdma_lpuart1_rx.Instance = DMA1_Channel2; 
    hdma_lpuart1_rx.Init.Request = DMA_REQUEST_LPUART1_RX;    // Hubungkan ke LPUART1_RX lewat DMAMUX
    hdma_lpuart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;    // Dari UART masuk ke RAM (Memory)
    hdma_lpuart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;        // Register UART alamatnya tetap
    hdma_lpuart1_rx.Init.MemInc = DMA_MINC_ENABLE;            // Increment RAM agar karakter berjejer
    hdma_lpuart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_lpuart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_lpuart1_rx.Init.Mode = DMA_NORMAL;                   // Cukup sekali jalan (Nanti di-reset manual)
    hdma_lpuart1_rx.Init.Priority = DMA_PRIORITY_HIGH;        // Beri prioritas tinggi agar tidak ada byte hilang
    HAL_DMA_Init(&hdma_lpuart1_rx);
    __HAL_LINKDMA(&hlpuart1, hdmarx, hdma_lpuart1_rx);
    
    // 3. Configure the UART Parameters
    hlpuart1.Instance = LPUART1;                  // Point the handle to the physical hardware
    hlpuart1.Init.BaudRate = 115200;              // HAL does the 256 * Clock / Baud math for you!
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;         // Enable both Transmitter and Receiver
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&hlpuart1);

    // Interupt di level CPU
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 7, 0); // Untuk DMA RX
    HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

    HAL_NVIC_SetPriority(LPUART1_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(LPUART1_IRQn);
}

void DMA1_Channel1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_lpuart1_tx);
}

// Tambahkan Interrupt Handler untuk DMA1 Channel 2 (LPUART1_RX)
void DMA1_Channel2_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_lpuart1_rx);
}

// Modifikasi LPUART1_IRQHandler untuk menangkap event IDLE LINE
extern TX_SEMAPHORE pc_rx_sem; 

#define PC_RX_BUFF_SIZE 128
uint8_t pc_rx_buffer[PC_RX_BUFF_SIZE];
uint16_t pc_rx_len = 0;

void LPUART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&hlpuart1);
    // 1. Cek apakah interupsi ini berasal dari IDLE Line
    if (__HAL_UART_GET_FLAG(&hlpuart1, UART_FLAG_IDLE) != RESET) {
        
        // 2. Bersihkan flag IDLE agar tidak terjadi looping interupsi tanpa akhir
        __HAL_UART_CLEAR_IDLEFLAG(&hlpuart1);

        // 3. MATEMATIKA LOW-LEVEL: Ambil nilai register CNDTR milik DMA1 Channel 2
        uint32_t sisa_slot = hlpuart1.hdmarx->Instance->CNDTR;
        
        // Hitung berapa karakter yang sudah berhasil masuk ke RAM
        pc_rx_len = PC_RX_BUFF_SIZE - sisa_slot;

        // 4. Hentikan paksa DMA penerimaan
        HAL_UART_DMAStop(&hlpuart1);

        // 5. Kirim sinyal Semaphore untuk membangunkan ThreadX
        tx_semaphore_put(&pc_rx_sem);
    }

    // Biarkan HAL mengurusi error flag bawaan lainnya
    HAL_UART_IRQHandler(&hlpuart1);
}