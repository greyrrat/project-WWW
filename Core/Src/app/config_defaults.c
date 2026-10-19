#include "app.h"

void Config_Defaults(SystemConfig_t *c)
{
  memset(c, 0, sizeof(*c));
  c->magic = CONFIG_MAGIC;
  c->log_interval = 10;   c->file_max_size = 2048;  c->timeout = 30;
  c->lumin = 1;           c->lumin_low = 255;       c->lumin_high = 768;
  c->temp_air = 1;        c->min_temp_air = -10;    c->max_temp_air = 60;
  c->hygr = 1;            c->hygr_mint = 0;         c->hygr_maxt = 50;
  c->pressure = 0;        c->pressure_min = 850;    c->pressure_max = 1080;
}
