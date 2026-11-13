#include "temperature_cluster.h"

#include "cluster_common.h"
#include "zigbee/consts.h"
#include "hal/printf_selector.h"
#include "hal/timer.h"
#include "hal/zigbee.h"

#include <stdint.h>

#define TEMP_POLL_INTERVAL_MS 30000UL
#define TEMP_CONVERSION_DELAY_MS 750UL
#define TEMP_INITIAL_DELAY_MS 2000UL
#define DS18B20_INVALID_VALUE ((int16_t)0x8000)

/* TLSR8258 uses its 16 MHz hardware system timer. The loop is only a host
 * fallback; the Telink firmware never relies on compiler-loop timing. */
static inline void ow_delay_us(uint32_t us) {
#ifdef HAL_TELINK
  hal_delay_us(us);
#else
  volatile uint32_t i;
  while (us--) {
    for (i = 0; i < 10; i++) {
      ;
    }
  }
#endif
}

static void ow_release(hal_gpio_pin_t pin) {
  hal_gpio_onewire_release(pin);
}

static void ow_drive_low(hal_gpio_pin_t pin) {
  hal_gpio_onewire_low(pin);
}

static uint8_t ow_read_pin(hal_gpio_pin_t pin) {
  return hal_gpio_read(pin);
}

/* DS18B20 reset/presence sequence. */
static uint8_t ow_reset(hal_gpio_pin_t pin) {
  uint8_t presence;

  ow_release(pin);
  ow_delay_us(10);
  if (!ow_read_pin(pin)) {
    printf("DS18B20: bus is LOW while idle\r\n");
    return 0;
  }

  ow_drive_low(pin);
  ow_delay_us(480);
  ow_release(pin);
  ow_delay_us(70);
  presence = (ow_read_pin(pin) == 0);
  ow_delay_us(410);

  if (!ow_read_pin(pin)) {
    printf("DS18B20: bus did not return HIGH after presence pulse\r\n");
    return 0;
  }
  if (!presence) {
    printf("DS18B20: no presence pulse on configured pin\r\n");
    return 0;
  }
  return 1;
}

static void ow_write_bit(hal_gpio_pin_t pin, uint8_t bit) {
  ow_drive_low(pin);
  if (bit) {
    ow_delay_us(6);
    ow_release(pin);
    ow_delay_us(64);
  } else {
    ow_delay_us(60);
    ow_release(pin);
    ow_delay_us(10);
  }
}

/* Read slot: release early and sample near the end of the DS18B20's
 * 15-us data-valid window, leaving the remainder of the 60-us slot free. */
static uint8_t ow_read_bit(hal_gpio_pin_t pin) {
  uint8_t bit;
  ow_drive_low(pin);
  ow_delay_us(2);
  ow_release(pin);
  ow_delay_us(10);
  bit = ow_read_pin(pin);
  ow_delay_us(55);
  return bit;
}

static void ow_write_byte(hal_gpio_pin_t pin, uint8_t value) {
  uint8_t bit;
  for (bit = 0; bit < 8; bit++) {
    ow_write_bit(pin, value & 0x01);
    value >>= 1;
  }
}

/* The DS18B20 transmits every byte least-significant bit first. */
static uint8_t ow_read_byte(hal_gpio_pin_t pin) {
  uint8_t value = 0;
  uint8_t bit;
  for (bit = 0; bit < 8; bit++) {
    if (ow_read_bit(pin)) {
      value |= (uint8_t)(1U << bit);
    }
  }
  return value;
}

static uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  uint8_t i;
  for (i = 0; i < len; i++) {
    uint8_t inbyte = data[i];
    uint8_t j;
    for (j = 0; j < 8; j++) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}

static uint8_t ds18b20_start_conversion(hal_gpio_pin_t pin) {
  if (!ow_reset(pin)) return 0;
  ow_write_byte(pin, 0xCC); /* Skip ROM: one probe on this bus. */
  ow_write_byte(pin, 0x44); /* Convert T. */
  ow_release(pin);
  return 1;
}

static uint8_t ds18b20_read_temperature(hal_gpio_pin_t pin,
                                        int16_t *temp_x100) {
  uint8_t scratchpad[9];
  uint8_t all_zero = 1;
  uint8_t all_ff = 1;
  uint8_t i;
  int16_t raw;

  if (!ow_reset(pin)) return 0;
  ow_write_byte(pin, 0xCC); /* Skip ROM. */
  ow_write_byte(pin, 0xBE); /* Read scratchpad. */

  for (i = 0; i < 9; i++) {
    scratchpad[i] = ow_read_byte(pin);
    if (scratchpad[i] != 0x00) all_zero = 0;
    if (scratchpad[i] != 0xFF) all_ff = 0;
  }
  ow_release(pin);

  /* An all-zero scratchpad has a zero CRC too; reject it explicitly. */
  if (all_zero || all_ff) {
    printf("DS18B20: blank scratchpad received\r\n");
    return 0;
  }
  if (ds18b20_crc8(scratchpad, 8) != scratchpad[8]) {
    printf("DS18B20: scratchpad CRC mismatch\r\n");
    return 0;
  }

  raw = (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);
  if (raw < -880 || raw > 2000) {
    printf("DS18B20: raw temperature outside valid range: %d\r\n", raw);
    return 0;
  }

  *temp_x100 = (int16_t)(((int32_t)raw * 100) / 16);
  return 1;
}

static void temperature_poll_task(void *arg) {
  zigbee_temperature_cluster *cluster =
      (zigbee_temperature_cluster *)arg;
  int16_t new_value;

  if (!cluster->conversion_pending) {
    if (ds18b20_start_conversion(cluster->pin)) {
      cluster->conversion_pending = 1;
      hal_tasks_schedule(&cluster->poll_task, TEMP_CONVERSION_DELAY_MS);
      return;
    }
    hal_tasks_schedule(&cluster->poll_task, TEMP_POLL_INTERVAL_MS);
    return;
  }

  cluster->conversion_pending = 0;
  if (ds18b20_read_temperature(cluster->pin, &new_value)) {
    if (!cluster->initialized || new_value != cluster->measured_value) {
      cluster->measured_value = new_value;
      cluster->initialized = 1;
      printf("DS18B20 temperature: %d.%02u C\r\n", new_value / 100,
             (unsigned)(new_value < 0 ? -(new_value % 100) : new_value % 100));
      hal_zigbee_notify_attribute_changed(
          cluster->endpoint, ZCL_CLUSTER_TEMPERATURE_MEASUREMENT,
          ZCL_ATTR_TEMPERATURE_MEASURED_VALUE);
    }
  }
  hal_tasks_schedule(&cluster->poll_task, TEMP_POLL_INTERVAL_MS);
}

void temperature_cluster_add_to_endpoint(zigbee_temperature_cluster *cluster,
                                         hal_zigbee_endpoint *endpoint) {
  cluster->endpoint = endpoint->endpoint;
  cluster->measured_value = DS18B20_INVALID_VALUE;
  cluster->initialized = 0;
  cluster->conversion_pending = 0;
  cluster->cluster_revision = 1;

  SETUP_ATTR(0, ZCL_ATTR_TEMPERATURE_MEASURED_VALUE, ZCL_DATA_TYPE_INT16,
             ATTR_READONLY, cluster->measured_value);
  SETUP_ATTR(1, ZCL_ATTR_GLOBAL_CLUSTER_REVISION, ZCL_DATA_TYPE_UINT16,
             ATTR_READONLY, cluster->cluster_revision);

  endpoint->clusters[endpoint->cluster_count].cluster_id =
      ZCL_CLUSTER_TEMPERATURE_MEASUREMENT;
  endpoint->clusters[endpoint->cluster_count].attribute_count = 2;
  endpoint->clusters[endpoint->cluster_count].attributes = cluster->attr_infos;
  endpoint->clusters[endpoint->cluster_count].is_server = 1;
  endpoint->clusters[endpoint->cluster_count].cmd_callback = NULL;
  endpoint->cluster_count++;

  /* Configure D7 once; bit slots now only toggle the output driver. */
  hal_gpio_onewire_init(cluster->pin);
  cluster->poll_task.handler = temperature_poll_task;
  cluster->poll_task.arg = cluster;
  hal_tasks_init(&cluster->poll_task);
  hal_tasks_schedule(&cluster->poll_task, TEMP_INITIAL_DELAY_MS);
}
