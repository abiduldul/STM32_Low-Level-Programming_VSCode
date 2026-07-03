#ifndef __SD_H__
#define __SD_H__

void SDMMC1_SD_Init(void);

HAL_StatusTypeDef SD_read_raw_sector (uint32_t sector_num, uint8_t *pBuffer);
HAL_StatusTypeDef SD_write_raw_sector(uint32_t sector_num, uint8_t *pBuffer);

#endif