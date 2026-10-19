#include "app.h"

/* The board has no battery on VBAT: a power cut stops the clock. So every TIME_SAVE_MS
   (or now if force) the date/time is added to a flash page: 8 bytes per record, the page is
   only erased every 256 records (flash lasts 10 000 erases = decades). */
void Rtc_Save(bool force)
{
  static uint32_t next_ms = 0U;
  uint32_t now = HAL_GetTick(), tr, dr, err = 0U;
  int16_t i;
  uint64_t rec;

  if (!rtc_ok || (RTC->ISR & RTC_ISR_INITS) == 0U) return;    /* time never set: nothing to keep */
  if (!force && (int32_t)(now - next_ms) < 0) return;
  next_ms = now + TIME_SAVE_MS;

  tr = RTC->TR;                                  /* TR first, then DR */
  dr = RTC->DR;
  rec = ((uint64_t)(((uint32_t)TIME_REC_TAG << 24) | (dr & 0x00FFFFFFUL)) << 32) | tr;

  i = (int16_t)(Rtc_LastRec() + 1);
  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
  if (i >= (int16_t)TIME_REC_COUNT || ((const uint64_t *)FLASH_TIME_ADDR)[i] != 0xFFFFFFFFFFFFFFFFULL)
  {
    FLASH_EraseInitTypeDef er;
    memset(&er, 0, sizeof(er));
    er.TypeErase = FLASH_TYPEERASE_PAGES;
    er.Banks = FLASH_BANK_2;                     /* 0x080FF000 = bank 2, page 254 */
    er.Page = 254U;
    er.NbPages = 1U;
    (void)HAL_FLASHEx_Erase(&er, &err);
    i = 0;
  }
  (void)HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, FLASH_TIME_ADDR + 8U * (uint32_t)i, rec);
  HAL_FLASH_Lock();
}
