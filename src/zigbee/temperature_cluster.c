#include "temperature_cluster.h"

#include "cluster_common.h"
#include "zigbee/consts.h"
#include "hal/printf_selector.h"
#include "hal/timer.h"
#include "hal/zigbee.h"


#define TEMP_POLL_INTERVAL_MS 30000
#define TEMP_INITIAL_DELAY_MS 2000

static inline void ow_delay_us(uint32_t us) {
  volatile uint32_t i;
  while (us--) {
    for (i = 0; i < 10; i++) {
      ;
    }
  }
}

static inline void ow_delay_ms(uint32_t ms) {
  while (ms--) {
    ow_delay_us(1000);
  }
}

static void ow_release(hal_gpio_pin_t pin) {
  hal_gpio_init(pin, 1, HAL_GPIO_PULL_UP);
}

static void ow_drive_low(hal_gpio_pin_t pin) {
  hal_gpio_init(pin, 0, HAL_GPIO_PULL_NONE);
  hal_gpio_clear(pin);
}

static uint8_t ow_read_pin(hal_gpio_pin_t pin) {
  return hal_gpio_read(pin);
}

static uint8_t ow_reset(hal_gpio_pin_t pin) {
  uint8_t presence;
  ow_release(pin);
  ow_delay_us(5);
  ow_drive_low(pin);
  ow_delay_us(480);
  ow_release(pin);
  ow_delay_us(70);
  presence = (ow_read_pin(pin) == 0);
  ow_delay_us(410);
  return presence;
}

static void ow_write_bit(hal_gpio_pin_t pin, uint8_t bit) {
  if (bit) {
    ow_drive_low(pin);
    ow_delay_us(6);
    ow_release(pin);
    ow_delay_us(64);
  } else {
    ow_drive_low(pin);
    ow_delay_us(60);
    ow_release(pin);
    ow_delay_us(10);
  }
}

static uint8_t ow_read_bit(hal_gpio_pin_t pin) {
  uint8_t bit;
  ow_drive_low(pin);
  ow_delay_us(6);
  ow_release(pin);
  ow_delay_us(9);
  bit = ow_read_pin(pin);
  ow_delay_us(55);
  return bit;
}

static void ow_write_byte(hal_gpio_pin_t pin, uint8_t value) {
  for (uint8_t i = 0; i < 8; i++) {
    ow_write_bit(pin, value & 0x01);
    value >>= 1;
  }
}

static uint8_t ow_read_byte(hal_gpio_pin_t pin) {
  uint8_t value = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (ow_read_bit(pin)) {
      value |= (1u << i);
    }
  }
  return value;
}


static uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t inbyte = data[i];
    for (uint8_t j = 0; j < 8; j++) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) {
        crc ^= 0x8C;
      }
      inbyte >>= 1;
    }
  }
  return crc;
}

static uint8_t ds18b20_read_temperature(hal_gpio_pin_t pin, int16_t *temp_x100) {
  uint8_t scratchpad[9] = {0};

  ow_release(pin);
  if (!ow_reset(pin)) {
    printf("DS18B20: no presence pulse\r\n");
    return 0;
  }

  ow_write_byte(pin, 0xCC); // Skip ROM
  ow_write_byte(pin, 0x44); // Convert T
  ow_release(pin);
  ow_delay_ms(750);

  if (!ow_reset(pin)) {
    printf("DS18B20: missing after convert\r\n");
    return 0;
  }

  ow_write_byte(pin, 0xCC); // Skip ROM
  ow_write_byte(pin, 0xBE); // Read scratchpad

  for (uint8_t i = 0; i < 9; i++) {
    scratchpad[i] = ow_read_byte(pin);
  }

  if (ds18b20_crc8(scratchpad, 8) != scratchpad[8]) {
    printf("DS18B20: CRC mismatch\r\n");
    return 0;
  }

  int16_t raw = (int16_t)((scratchpad[1] << 8) | scratchpad[0]);
  *temp_x100 = (int16_t)((raw * 100) / 16);
  return 1;
}

static void temperature_poll_task(void *arg) {
  zigbee_temperature_cluster *cluster = (zigbee_temperature_cluster *)arg;
  int16_t new_value = cluster->measured_value;

  if (ds18b20_read_temperature(cluster->pin, &new_value)) {
    if (!cluster->initialized || new_value != cluster->measured_value) {
      cluster->measured_value = new_value;
      cluster->initialized = 1;
      printf("Temperature: %d.%02d C\r\n", new_value / 100, abs(new_value % 100));
      hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                          ZCL_CLUSTER_TEMPERATURE_MEASUREMENT,
                                          ZCL_ATTR_TEMPERATURE_MEASURED_VALUE);
    }
  }

  hal_tasks_schedule(&cluster->poll_task, TEMP_POLL_INTERVAL_MS);
}

void temperature_cluster_add_to_endpoint(zigbee_temperature_cluster *cluster,
                                         hal_zigbee_endpoint *endpoint) {
  cluster->endpoint = endpoint->endpoint;
  if (!cluster->initialized) {
    cluster->measured_value = 0;
    cluster->cluster_revision = 1;
  }

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

  cluster->poll_task.handler = temperature_poll_task;
  cluster->poll_task.arg = cluster;
  hal_tasks_init(&cluster->poll_task);
  ow_release(cluster->pin);
  hal_tasks_schedule(&cluster->poll_task, TEMP_INITIAL_DELAY_MS);
}
