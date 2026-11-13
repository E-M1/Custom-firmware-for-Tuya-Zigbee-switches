#include "hal/timer.h"
#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)
#include <stdint.h>

uint32_t hal_millis(void) {
  /* clock_time() runs at 16 MHz on TLSR8258. */
  return clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS;
}

void hal_delay_us(uint32_t us) {
  uint32_t start = clock_time();
  uint32_t ticks = us * 16U; /* 16 system-timer ticks per microsecond. */

  while ((uint32_t)(clock_time() - start) < ticks) {
    /* Wait against the hardware system timer for calibrated timing. */
  }
}
