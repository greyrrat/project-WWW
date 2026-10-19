#include "app.h"

/* Writes the waiting lines in order. A line that cannot be written stays queued for the next try. */
void SD_Flush(void)
{
  while (q_count > 0U)
  {
    if (!SD_AppendEntry(&log_q[q_head])) return;
    q_head = (uint8_t)((q_head + 1U) % LOG_QUEUE_LEN);
    q_count--;
  }
  Err_Clear(ERR_SD_ACC | ERR_SD_FULL);
}
