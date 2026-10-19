#include "app.h"

bool ParseNum(const char **p, long *out)
{
  char *end;
  long v = strtol(*p, &end, 10);
  if (end == *p) return false;
  *out = v;
  *p = end;
  return true;
}
