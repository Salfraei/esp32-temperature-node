#include "ds18b20_driver.h"

#include "app_config.h"
#include "ds18b20.h"
#include "esp_log.h"
#include "onewire_bus.h"

static const char *TAG = "DS18B20_DRIVER";

static onewire_bus_handle_t s_bus = NULL;
static ds18b20_device_handle_t s_sensor = NULL;
static bool s_is_ready = false;

esp_err_t ds18b20_driver_init(void)
{
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = DS18B20_GPIO,
        .flags = {
            .en_pull_up = true
        }
    };

    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10
    };

    esp_err_t result = onewire_new_bus_rmt(
        &bus_config,
        &rmt_config,
        &s_bus
    );

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create 1-Wire bus: %s",
                 esp_err_to_name(result));
        return result;
    }

    ds18b20_config_t sensor_config = {};

    result = ds18b20_new_device_from_bus(
        s_bus,
        &sensor_config,
        &s_sensor
    );

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "DS18B20 sensor was not found: %s",
                 esp_err_to_name(result));
        return result;
    }

    s_is_ready = true;

    ESP_LOGI(TAG, "DS18B20 initialized on GPIO %d", DS18B20_GPIO);

    return ESP_OK;
}

esp_err_t ds18b20_driver_read_temperature(float *temperature)
{
    if (temperature == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_is_ready || s_sensor == NULL || s_bus == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t result =
        ds18b20_trigger_temperature_conversion_for_all(s_bus);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start temperature conversion: %s",
                 esp_err_to_name(result));
        return result;
    }

    result = ds18b20_get_temperature(s_sensor, temperature);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read temperature: %s",
                 esp_err_to_name(result));
        return result;
    }

    return ESP_OK;
}

bool ds18b20_driver_is_ready(void)
{
    return s_is_ready;
}