#include "app_config.h"
#include "ds18b20_driver.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "APP_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 temperature node");

    esp_err_t result = nvs_flash_init();

    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }

    ESP_ERROR_CHECK(result);

    result = ds18b20_driver_init();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Failed to initialize DS18B20: %s",
                 esp_err_to_name(result));
        return;
    }

    ESP_LOGI(TAG, "DS18B20 initialized successfully");

    result = wifi_manager_init();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Wi-Fi initialization failed: %s",
                 esp_err_to_name(result));
        return;
    }

    ESP_LOGI(TAG, "Wi-Fi connection established");

    result = mqtt_manager_init();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "MQTT initialization failed: %s",
                 esp_err_to_name(result));
        return;
    }

    ESP_LOGI(TAG, "MQTT client initialization started");

    while (true)
    {
        float temperature = 0.0f;

        result = ds18b20_driver_read_temperature(&temperature);

        if (result == ESP_OK)
        {
            ESP_LOGI(TAG,
                     "Temperature: %.2f C",
                     temperature);

            if (mqtt_manager_is_connected())
            {
                esp_err_t mqtt_result =
                    mqtt_manager_publish_temperature(temperature);

                if (mqtt_result != ESP_OK)
                {
                    ESP_LOGW(TAG,
                             "Failed to publish temperature: %s",
                             esp_err_to_name(mqtt_result));
                }
            }
            else
            {
                ESP_LOGW(TAG, "MQTT is not connected yet");
            }
        }
        else
        {
            ESP_LOGE(TAG,
                     "Failed to read temperature: %s",
                     esp_err_to_name(result));
        }

        vTaskDelay(pdMS_TO_TICKS(TEMPERATURE_READ_INTERVAL_MS));
    }
}