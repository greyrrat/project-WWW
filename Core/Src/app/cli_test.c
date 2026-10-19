#include "app.h"
#include "features.h"

/* Test commands (CONFIGURATION and MAINTENANCE modes):
     STATUS                 shows the switches and the SD state
     SDTEST                 write + read-back test of the SD card
     USE_LED=0/1, USE_SD=0/1, USE_LIGHT=0/1, USE_TEMP_HUM=0/1, USE_LCD=0/1, USE_ALL=0/1
   The switches are kept in RAM only: after a reset they come back to features.h.
   Returns false if the line is not a test command. */
bool Cli_Test(char *line)
{
  static const char *const names[5] = { "USE_LED", "USE_SD", "USE_LIGHT", "USE_TEMP_HUM", "USE_LCD" };
  static const bool built[5] = { USE_LED != 0, USE_SD != 0, USE_LIGHT != 0, USE_TEMP_HUM != 0, USE_LCD != 0 };
  bool *flags[5] = { &tog.led, &tog.sd, &tog.light, &tog.temp, &tog.lcd };
  Toggles_t old = tog;
  char buf[RX_MAX];
  char *name, *eq;
  long v;
  int idx = -1;
  bool all;

  snprintf(buf, sizeof(buf), "%s", line);        /* work on a copy: Cli_Execute still needs the line */
  name = Trim(buf);

  if (CiEq(name, "STATUS")) { Tog_Print(); return true; }
  if (CiEq(name, "SDTEST")) { (void)SD_SelfTest(); return true; }

  eq = strchr(name, '=');
  if (eq == NULL) return false;
  *eq = '\0';
  name = Trim(name);
  all = CiEq(name, "USE_ALL");
  for (int i = 0; i < 5; i++) if (CiEq(name, names[i])) idx = i;
  if (!all && idx < 0) return false;

  if (!ParseLong(Trim(eq + 1), &v) || (v != 0 && v != 1)) { printf("ERROR: valeur 0 ou 1\r\n"); return true; }

  for (int i = 0; i < 5; i++)
  {
    if (!all && i != idx) continue;
    if (v == 1 && !built[i])
    {
      printf("ERROR: %s = 0 dans features.h (mettre 1 et recompiler)\r\n", names[i]);
      continue;
    }
    *flags[i] = (v == 1);
  }

  /* what has to happen when a switch changes */
  if (old.sd && !tog.sd) { SD_Unmount(); Err_Clear(ERR_SD_ACC | ERR_SD_FULL); }
  if (old.lcd && !tog.lcd && lcd_ready)
  {
    Lcd_Clear();
    Lcd_WriteByte(0x08, 0);                      /* display off */
    Lcd_Expander(0x00);                          /* backlight off */
  }
  if (!old.lcd && tog.lcd) { lcd_ready = Lcd_Init(); Lcd_Refresh(true); }
  if (!old.light && tog.light) (void)BH1750_Init();
  if (!tog.light) lux_fail = 0U;
  if (!tog.temp)  dht_fail = 0U;
  if (!tog.light || !tog.temp) Err_Clear(ERR_SENS_ACC | ERR_SENS_DATA);

  Tog_Print();
  printf("OK\r\n");
  return true;
}
