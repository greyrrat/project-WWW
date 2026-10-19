#include "app.h"

void Cli_Execute(char *line)
{
  char *name = Trim(line);
  char *eq, *val;
  const char *pname;

  if (*name == '\0') return;
  config_activity_ms = HAL_GetTick();


  if (CiEq(name, "RESET"))   { Config_Defaults(&sys_cfg); Cli_Reply(Config_Save()); return; }
  if (CiEq(name, "VERSION")) { printf("VERSION=%s ; LOT=%s\r\n", FW_VERSION, LOT_NUMBER); return; }
  if (CiEq(name, "PARAMS"))  { Cli_PrintParams(); return; }
  if (CiEq(name, "RAM"))     { Ram_Print(); return; }

  eq = strchr(name, '=');
  if (eq == NULL) { printf("ERROR: commande inconnue\r\n"); return; }
  *eq = '\0';
  val = Trim(eq + 1);
  name = Trim(name);
  pname = name;

  if (CiEq(name, "CLOCK"))                       /* CLOCK=HH:MM:SS */
  {
    const char *p = val;
    long h, m, s;
    if (ParseNum(&p, &h) && ParseSep(&p, ':') && ParseNum(&p, &m) && ParseSep(&p, ':') && ParseNum(&p, &s) &&
        *p == '\0' && h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59)
      { bool ok = Rtc_SetTime((uint8_t)h, (uint8_t)m, (uint8_t)s); if (ok) Rtc_Save(true); Cli_Reply(ok); }
    else
      printf("ERROR: format HH:MM:SS\r\n");
    return;
  }

  if (CiEq(name, "DATE"))                        /* DATE=MM,DD,YYYY */
  {
    const char *p = val;
    long mo, d, y;
    if (ParseNum(&p, &mo) && ParseSep(&p, ',') && ParseNum(&p, &d) && ParseSep(&p, ',') && ParseNum(&p, &y) &&
        *p == '\0' && y >= 2000 && y <= 2099 && mo >= 1 && mo <= 12 && d >= 1 && d <= (long)DaysInMonth(y, mo))
      { bool ok = Rtc_SetDate((uint8_t)(y - 2000), (uint8_t)mo, (uint8_t)d); if (ok) Rtc_Save(true); Cli_Reply(ok); }
    else
      printf("ERROR: format MM,DD,YYYY (2000..2099)\r\n");
    return;
  }

  if (CiEq(name, "DAY"))                         /* DAY=MON..SUN */
  {
    static const char *const days[7] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
    for (uint8_t i = 0U; i < 7U; i++)
    {
      if (CiEq(val, days[i])) { { bool ok = Rtc_SetWeekday((uint8_t)(i + 1U)); if (ok) Rtc_Save(true); Cli_Reply(ok); } return; }
    }
    printf("ERROR: DAY={MON,TUE,WED,THU,FRI,SAT,SUN}\r\n");
    return;
  }

  if (CiEq(name, "LOG_INTERVALL")) pname = "LOG_INTERVAL";     /* the spec uses both spellings */

  for (size_t i = 0; i < PARAM_COUNT; i++)
  {
    if (CiEq(pname, params[i].name))
    {
      long v;
      SystemConfig_t tmp;
      if (!ParseLong(val, &v))                   { printf("ERROR: nombre attendu\r\n"); return; }
      if (v < params[i].min || v > params[i].max)
      {
        printf("ERROR: valeur hors limites (%ld..%ld)\r\n", (long)params[i].min, (long)params[i].max);
        return;
      }
      tmp = sys_cfg;
      *(int32_t *)((uint8_t *)&tmp + params[i].off) = (int32_t)v;
      if (!Config_PairsOk(&tmp)) { printf("ERROR: min/max incoherents\r\n"); return; }
      sys_cfg = tmp;
      Cli_Reply(Config_Save());
      return;
    }
  }
  printf("ERROR: parametre inconnu\r\n");
}
