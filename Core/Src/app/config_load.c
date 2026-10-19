#include "app.h"

void Config_Load(void)
{
  const SystemConfig_t *f = (const SystemConfig_t *)FLASH_CONFIG_ADDR;
  if (f->magic == CONFIG_MAGIC && f->checksum == Config_Checksum(f) && Config_RangesOk(f))
  {
    memcpy(&sys_cfg, f, sizeof(sys_cfg));
    DBG("[CONFIG] Parametres charges depuis la Flash\r\n");
  }
  else
  {
    Config_Defaults(&sys_cfg);
    DBG("[CONFIG] Flash vierge : valeurs par defaut\r\n");
    Config_Save();
  }
}
