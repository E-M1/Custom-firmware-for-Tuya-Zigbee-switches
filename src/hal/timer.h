#ifndef _HAL_TIMER_H_
#define _HAL_TIMER_H_

#include <stdint.h>

/**
 * Get milliseconds elapsed since system initialization.
 * @return Number of milliseconds since system started.
 */
uint32_t hal_millis(void);

/**
 * Busy-wait for a short interval with microsecond resolution.
 * Intended for timing-sensitive hardware protocols such as 1-Wire.
 * Keep delays short; do not use this for long waits in application code.
 */
void hal_delay_us(uint32_t us);

#endif
