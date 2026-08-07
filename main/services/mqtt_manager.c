#include "mqtt_manager.h"
#include "app_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT_MANAGER";

static esp_mqtt_client_handle_t s_client = NULL;
static bool s_connected = false;

/*
 * Тут зберігатиметься унікальний MQTT Client ID.
 *
 * Приклад:
 * esp32-temperature-room1-AABBCCDDEEFF
 */
static char s_client_id[64];

static esp_err_t mqtt_create_client_id(void)
{
    uint8_t mac[6];

    esp_err_t result = esp_read_mac(
        mac,
        ESP_MAC_WIFI_STA
    );

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to read MAC address: %s",
            esp_err_to_name(result)
        );

        return result;
    }

    int length = snprintf(
        s_client_id,
        sizeof(s_client_id),
        "%s-%02X%02X%02X%02X%02X%02X",
        MQTT_CLIENT_ID_PREFIX,
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]
    );

    if (length < 0 ||
        length >= (int)sizeof(s_client_id)) {
        ESP_LOGE(TAG, "Failed to create MQTT Client ID");
        return ESP_FAIL;
    }

    ESP_LOGI(
        TAG,
        "MQTT Client ID: %s",
        s_client_id
    );

    return ESP_OK;
}

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    (void)handler_args;
    (void)event_base;

    esp_mqtt_event_handle_t event =
        (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(
                TAG,
                "Connecting to MQTT broker: %s",
                MQTT_BROKER_URI
            );
            break;

        case MQTT_EVENT_CONNECTED:
            s_connected = true;

            ESP_LOGI(
                TAG,
                "Connected to MQTT broker"
            );

            ESP_LOGI(
                TAG,
                "Client ID: %s",
                s_client_id
            );

            ESP_LOGI(
                TAG,
                "Publishing topic: %s",
                MQTT_TOPIC
            );
            break;

        case MQTT_EVENT_DISCONNECTED:
            s_connected = false;

            ESP_LOGW(
                TAG,
                "Disconnected from MQTT broker"
            );
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

            if (event->error_handle != NULL) {
                ESP_LOGE(
                    TAG,
                    "MQTT error type: %d",
                    event->error_handle->error_type
                );
            } else {
                ESP_LOGE(TAG, "Unknown MQTT error");
            }

            break;

        default:
            break;
    }
}

esp_err_t mqtt_manager_init(void)
{
    if (s_client != NULL) {
        ESP_LOGW(
            TAG,
            "MQTT client is already initialized"
        );

        return ESP_OK;
    }

    esp_err_t result = mqtt_create_client_id();

    if (result != ESP_OK) {
        return result;
    }

    const esp_mqtt_client_config_t config = {
        .broker.address.uri = MQTT_BROKER_URI,

        .credentials.client_id = s_client_id,

        /*
         * false означає clean session.
         */
        .session.disable_clean_session = false,

        /*
         * Дозволяємо ESP-IDF автоматично
         * перепідключатися після втрати зв'язку.
         */
        .network.disable_auto_reconnect = false,
        .network.reconnect_timeout_ms = 5000,
    };

    s_client = esp_mqtt_client_init(&config);

    if (s_client == NULL) {
        ESP_LOGE(
            TAG,
            "Failed to create MQTT client"
        );

        return ESP_FAIL;
    }

    result = esp_mqtt_client_register_event(
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

        esp_mqtt_client_destroy(s_client);
        s_client = NULL;

        return result;
    }

    result = esp_mqtt_client_start(s_client);

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to start MQTT client: %s",
            esp_err_to_name(result)
        );

        esp_mqtt_client_destroy(s_client);
        s_client = NULL;

        return result;
    }

    ESP_LOGI(TAG, "MQTT client started");

    return ESP_OK;
}

esp_err_t mqtt_manager_publish_temperature(float temperature)
{
    if (s_client == NULL) {
        ESP_LOGW(
            TAG,
            "MQTT client is not initialized"
        );

        return ESP_ERR_INVALID_STATE;
    }

    if (!s_connected) {
        ESP_LOGW(
            TAG,
            "Temperature was not published: MQTT disconnected"
        );

        return ESP_ERR_INVALID_STATE;
    }

    char payload[32];

    int payload_length = snprintf(
        payload,
        sizeof(payload),
        "%.2f",
        (double)temperature
    );

    if (payload_length < 0 ||
        payload_length >= (int)sizeof(payload)) {
        ESP_LOGE(
            TAG,
            "Failed to format temperature"
        );

        return ESP_FAIL;
    }

    int message_id = esp_mqtt_client_publish(
        s_client,
        MQTT_TOPIC,
        payload,
        payload_length,
        0, /* QoS 0 */
        0  /* retain false */
    );

    if (message_id < 0) {
        ESP_LOGE(
            TAG,
            "Failed to publish temperature"
        );

        return ESP_FAIL;
    }

    ESP_LOGI(
        TAG,
        "Published topic=%s payload=%s message_id=%d",
        MQTT_TOPIC,
        payload,
        message_id
    );

    return ESP_OK;
}

bool mqtt_manager_is_connected(void)
{
    return s_connected;
}