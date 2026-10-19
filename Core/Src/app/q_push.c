#include "app.h"

void Q_Push(const char *tag, const char *text)
{
  uint8_t idx;
  if (q_count >= LOG_QUEUE_LEN) { q_head = (uint8_t)((q_head + 1U) % LOG_QUEUE_LEN); q_count--; }   /* drop oldest */
  idx = (uint8_t)((q_head + q_count) % LOG_QUEUE_LEN);
  strncpy(log_q[idx].tag, tag, sizeof(log_q[idx].tag) - 1U);
  log_q[idx].tag[sizeof(log_q[idx].tag) - 1U] = '\0';
  strncpy(log_q[idx].text, text, sizeof(log_q[idx].text) - 1U);
  log_q[idx].text[sizeof(log_q[idx].text) - 1U] = '\0';
  q_count++;
}
