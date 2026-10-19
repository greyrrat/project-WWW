#include "app.h"

void Cli_Reply(bool ok) { printf("%s", ok ? "OK\r\n" : "ERROR\r\n"); }
