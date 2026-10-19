#include "app.h"

bool ParseSep(const char **p, char c)
{
  if (**p == c) { (*p)++; return true; }
  return false;
}
