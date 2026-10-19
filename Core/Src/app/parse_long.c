#include "app.h"

bool ParseLong(const char *s, long *out)
{
  const char *p = s;
  return (ParseNum(&p, out) && *p == '\0');
}
