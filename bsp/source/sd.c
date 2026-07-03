#include "system.h"
#include "sd.h"

extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart1;

SD_HandleTypeDef hsd_sdmmc1;

void SDMMC1_SD_Init(void) {
    char hw_log[120];

    // 1. Log Awal Port C
    sprintf(hw_log, "\r\n[DEBUG_HW] 1. Mengaktifkan GPIO Port C & D...\r\n");
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)hw_log, strlen(hw_log), 200);

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_SDMMC1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Konfigurasi Port C
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP; 
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Konfigurasi Port D
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;             
    GPIO_InitStruct.Pull = GPIO_PULLUP;             
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; 
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Konfigurasi Parameter Struktur SDMMC1
    hsd_sdmmc1.Instance = SDMMC1;
    hsd_sdmmc1.Init.ClockEdge = SDMMC_CLOCK_EDGE_FALLING;
    hsd_sdmmc1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    hsd_sdmmc1.Init.BusWide = SDMMC_BUS_WIDE_1B; 
    hsd_sdmmc1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd_sdmmc1.Init.Transceiver = SDMMC_TRANSCEIVER_DISABLE;
    // TRICK: Naikkan nilai ClockDiv ke 32 a   tau 64 (Bikin sinyal sangat lambat agar toleransi noise tinggi)
    hsd_sdmmc1.Init.ClockDiv = 32; 

    // 2. Log Tepat Sebelum Masuk Pintu HAL_SD_Init
    sprintf(hw_log, "[DEBUG_HW] 2. Memasuki fungsi HAL_SD_Init (Menunggu Handshake Kartu)...\r\n");
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)hw_log, strlen(hw_log), 200);

    // Jalankan Inisialisasi
    HAL_StatusTypeDef status = HAL_SD_Init(&hsd_sdmmc1);
    
    // 3. Log Tepat Setelah Keluar dari HAL_SD_Init
    sprintf(hw_log, "[DEBUG_HW] 3. Keluar dari HAL_SD_Init dengan Status: %d\r\n", status);
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)hw_log, strlen(hw_log), 200);

    if (status != HAL_OK) {
        sprintf(hw_log, "❌ [DEBUG_HW] GAGAL HAL_SD_Init!\r\n");
        HAL_UART_Transmit(&hlpuart1, (uint8_t *)hw_log, strlen(hw_log), 200);
        return; 
    }

    sprintf(hw_log, "[DEBUG_HW] 4. Mencoba masuk ke Mode Bus 4-Bit...\r\n");
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)hw_log, strlen(hw_log), 200);

    if (HAL_SD_ConfigWideBusOperation(&hsd_sdmmc1, SDMMC_BUS_WIDE_4B) != HAL_OK) {
        sprintf(hw_log, "❌ [DEBUG_HW] Gagal Konfigurasi Bus 4-Bit!\r\n");
        HAL_UART_Transmit(&hlpuart1, (uint8_t *)hw_log, strlen(hw_log), 200);
        return;
    }
    
    sprintf(hw_log, "✅ [DEBUG_HW] Selesai! Semua Jalur Hardware Siap.\r\n");
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)hw_log, strlen(hw_log), 200);
}

// sector_num = alamat sector di SD
// *pBuffer = alamat data RAM yang akan dikirim
HAL_StatusTypeDef SD_write_raw_sector(uint32_t sector_num, uint8_t *pBuffer) {
    HAL_StatusTypeDef status;

    status = HAL_SD_WriteBlocks(&hsd_sdmmc1, pBuffer, sector_num, 1, 100); // 1 = jumlah block yang akan ditulis, 100 = timeout

    if (status == HAL_OK) {
        // wait until the SD card finishes its internal physical write process
        while (HAL_SD_GetCardState(&hsd_sdmmc1) != HAL_SD_CARD_TRANSFER) {
            tx_thread_sleep(1);
        }
    }
    return status;
}

HAL_StatusTypeDef SD_read_raw_sector (uint32_t sector_num, uint8_t *pBuffer) {
    HAL_StatusTypeDef status;

    status = HAL_SD_ReadBlocks(&hsd_sdmmc1, pBuffer, sector_num, 1, 100);

    if (status == HAL_OK) {
        while (HAL_SD_GetCardState(&hsd_sdmmc1) != HAL_SD_CARD_TRANSFER) {
            tx_thread_sleep(1);
        }
    }
    return status;
}