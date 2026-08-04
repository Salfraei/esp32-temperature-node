#ifndef DS18B20_DRIVER_H
#define DS18B20_DRIVER_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t ds18b20_driver_init(void);
esp_err_t ds18b20_driver_read_temperature(float *temperature);
bool ds18b20_driver_is_ready(void);

#endif