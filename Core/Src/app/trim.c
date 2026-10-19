#include "app.h"

/* ======================== SERIAL COMMANDS (CLI) ======================= */
char *Trim(char *s)
{
  size_t n;
  while (*s == ' ' || *s == '\t') s++;
  n = strlen(s);
  while (n > 0U && (s[n - 1U] == ' ' || s[n - 1U] == '\t' || s[n - 1U] == '\r' || s[n - 1U] == '\n')) s[--n] = '\0';
  return s;
}
