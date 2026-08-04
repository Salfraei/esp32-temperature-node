#include "mqtt_manager.h"
#include "app_config.h"

#include <stdio.h>

#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT_MANAGER";

static esp_mqtt_client_handle_t s_client = NULL;
static bool s_connected = false;

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    esp_mqtt_event_handle_t event =
        (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            s_connected = true;
            ESP_LOGI(TAG, "Connected to MQTT broker");
            break;

        case MQTT_EVENT_DISCONNECTED:
            s_connected = false;
            ESP_LOGW(TAG, "Disconnected from MQTT broker");
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(
                TAG,
                "Message published, ID: %d",
                event->msg_id
            );
            break;

        case MQTT_EVENT_ERROR:
            s_connected = false;
            ESP_LOGE(TAG, "MQTT error");
            break;

        default:
            break;
    }
}

esp_err_t mqtt_manager_init(void)
{
    const esp_mqtt_client_config_t config = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.client_id = MQTT_CLIENT_ID
    };

    s_client = esp_mqtt_client_init(&config);

    if (s_client == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT client");
        return ESP_FAIL;
    }

    esp_err_t result = esp_mqtt_client_register_event(
        s_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to register MQTT handler: %s",
            esp_err_to_name(result)
        );

        return result;
    }

    result = esp_mqtt_client_start(s_client);

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to start MQTT client: %s",
            esp_err_to_name(result)
        );

        return result;
    }

    ESP_LOGI(TAG, "MQTT client started");

    return ESP_OK;
}

esp_err_t mqtt_manager_publish_temperature(float temperature)
{
    if (s_client == NULL || !s_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    char payload[16];

    int length = snprintf(
        payload,
        sizeof(payload),
        "%.2f",
        temperature
    );

    if (length < 0 || length >= sizeof(payload)) {
        ESP_LOGE(TAG, "Failed to format temperature");
        return ESP_FAIL;
    }

    int message_id = esp_mqtt_client_publish(
        s_client,
        MQTT_TOPIC,
        payload,
        0,
        1,
        1
    );

    if (message_id < 0) {
        ESP_LOGE(TAG, "Failed to publish temperature");
        return ESP_FAIL;
    }

    ESP_LOGI(
        TAG,
        "Published %s to %s",
        payload,
        MQTT_TOPIC
    );

    return ESP_OK;
}

bool mqtt_manager_is_connected(void)
{
    return s_connected;
}