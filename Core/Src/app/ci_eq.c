#include "app.h"

bool CiEq(const char *a, const char *b)
{
  while (*a != '\0' && *b != '\0')
  {
    char x = *a, y = *b;
    if (x >= 'a' && x <= 'z') x = (char)(x - 32);
    if (y >= 'a' && y <= 'z') y = (char)(y - 32);
    if (x != y) return false;
    a++; b++;
  }
  return (*a == '\0' && *b == '\0');
}
