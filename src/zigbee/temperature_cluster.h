#ifndef _TEMPERATURE_CLUSTER_H_
#define _TEMPERATURE_CLUSTER_H_

#include <stdint.h>

#include "hal/gpio.h"
#include "hal/tasks.h"
#include "hal/zigbee.h"

typedef struct {
  hal_gpio_pin_t pin;
  uint8_t endpoint;
  int16_t measured_value;
  uint16_t cluster_revision;
  hal_zigbee_attribute attr_infos[2];
  hal_task_t poll_task;
  uint8_t initialized;
} zigbee_temperature_cluster;

void temperature_cluster_add_to_endpoint(zigbee_temperature_cluster *cluster,
                                         hal_zigbee_endpoint *endpoint);

#endif
