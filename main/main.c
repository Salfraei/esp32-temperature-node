#include "ds18b20_driver.h"
#include "app_config.h"

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "APP_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 temperature node");

    esp_err_t result = ds18b20_driver_init();

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to initialize DS18B20: %s",
            esp_err_to_name(result)
        );

        return;
    }

    ESP_LOGI(TAG, "DS18B20 initialized successfully");

    while (true) {
        float temperature = 0.0f;

        result = ds18b20_driver_read_temperature(&temperature);

        if (result == ESP_OK) {
            ESP_LOGI(TAG, "Temperature: %.2f C", temperature);
        } else {
            ESP_LOGE(
                TAG,
                "Failed to read temperature: %s",
                esp_err_to_name(result)
            );
        }

        vTaskDelay(
            pdMS_TO_TICKS(TEMPERATURE_READ_INTERVAL_MS)
        );
    }
}