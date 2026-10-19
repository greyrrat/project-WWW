#include "app.h"

DSTATUS SD_Disk_Init(BYTE pdrv)
{
  uint8_t ocr[4], r = 0xFF, i, ok = 0;
  uint32_t t;
  (void)pdrv;

  sd_fail_step = 0;
  Spi_SetPrescaler(SD_SPI_SLOW);               /* start-up must be below 400 kHz */
  HAL_Delay(100);
  sd_is_hc = 0;
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
  for (i = 0; i < 10; i++) spi_rw(0xFF);         /* 80 clocks with CS high: wakes the card */

  for (i = 0; i < 20; i++)
  {
    r = sd_cmd(SD_CMD0, 0);
    if (r == 1) break;
    sd_deselect();
    HAL_Delay(10);
  }

  if (r != 1)
  {
    sd_fail_step = 1;            /* the card never answered */
  }
  else
  {
    t = HAL_GetTick();
    if (sd_cmd(SD_CMD8, 0x1AA) == 1)
    {
      for (i = 0; i < 4; i++) ocr[i] = spi_rw(0xFF);
      if (ocr[2] == 0x01 && ocr[3] == 0xAA)
      {
        do { r = sd_cmd(SD_ACMD41, 1UL << 30); } while (r != 0 && (HAL_GetTick() - t) < 2000);
        if (r != 0)                       sd_fail_step = 3;
        else if (sd_cmd(SD_CMD58, 0) != 0) sd_fail_step = 4;
        else
        {
          for (i = 0; i < 4; i++) ocr[i] = spi_rw(0xFF);
          sd_is_hc = (ocr[0] & 0x40) ? 1 : 0;
          if (!sd_is_hc && sd_cmd(SD_CMD16, 512) != 0) sd_fail_step = 6;
          else ok = 1;
        }
      }
      else sd_fail_step = 2;
    }
    else
    {
      do { r = sd_cmd(SD_ACMD41, 0); } while (r != 0 && (HAL_GetTick() - t) < 2000);
      if (r == 0 && sd_cmd(SD_CMD16, 512) == 0) ok = 1;
      else sd_fail_step = 5;
    }
  }
  sd_deselect();
  if (ok) Spi_SetPrescaler(SD_SPI_FAST);         /* card ready: go faster */
  sd_stat = ok ? 0 : STA_NOINIT;
  return sd_stat;
}
