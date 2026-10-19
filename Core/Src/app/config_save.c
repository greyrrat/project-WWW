#include "app.h"

bool Config_Save(void)
{
  uint64_t buf[sizeof(SystemConfig_t) / 8U];
  const size_t n = sizeof(SystemConfig_t) / 8U;
  uint32_t bank, page, page_error = 0U;
  FLASH_EraseInitTypeDef er;
  bool ok;

  sys_cfg.magic = CONFIG_MAGIC;
  sys_cfg.reserved = 0U;
  sys_cfg.checksum = Config_Checksum(&sys_cfg);
  memcpy(buf, &sys_cfg, sizeof(buf));

#if defined(FLASH_OPTR_DBANK)
  if ((FLASH->OPTR & FLASH_OPTR_DBANK) != 0U) { bank = FLASH_BANK_2; page = 255U; }   /* dual-bank (default) */
  else                                        { bank = FLASH_BANK_1; page = 511U; }   /* single-bank         */
#else
  bank = FLASH_BANK_2; page = 255U;
#endif

  memset(&er, 0, sizeof(er));
  er.TypeErase = FLASH_TYPEERASE_PAGES;
  er.Banks = bank;
  er.Page = page;
  er.NbPages = 1U;

  HAL_FLASH_Unlock();
#if defined(FLASH_FLAG_ALL_ERRORS)
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
#else
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
#endif
  ok = (HAL_FLASHEx_Erase(&er, &page_error) == HAL_OK);
  /* the first double-word holds the magic number: write it LAST so a power cut leaves the page invalid */
  for (size_t k = 1U; ok && k <= n; k++)
  {
    size_t i = (k == n) ? 0U : k;
    ok = (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, FLASH_CONFIG_ADDR + (8U * i), buf[i]) == HAL_OK);
  }
  HAL_FLASH_Lock();

  ok = ok && (memcmp((const void *)FLASH_CONFIG_ADDR, &sys_cfg, sizeof(sys_cfg)) == 0);
  DBG(ok ? "[CONFIG] Sauvegarde Flash OK\r\n" : "[CONFIG] ERREUR sauvegarde Flash\r\n");
  return ok;
}
