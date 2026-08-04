#include "wifi_manager.h"
#include "app_config.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "WIFI_MANAGER";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_count = 0;
static bool s_is_connected = false;

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        ESP_LOGI(TAG, "Wi-Fi station started");
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        s_is_connected = false;

        if (s_retry_count < WIFI_MAXIMUM_RETRY) {
            s_retry_count++;

            ESP_LOGW(
                TAG,
                "Connection failed. Retry %d/%d",
                s_retry_count,
                WIFI_MAXIMUM_RETRY
            );

            esp_wifi_connect();
        } else {
            xEventGroupSetBits(
                s_wifi_event_group,
                WIFI_FAIL_BIT
            );
        }
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(
            TAG,
            "Connected. IP address: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        s_retry_count = 0;
        s_is_connected = true;

        xEventGroupSetBits(
            s_wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }
}

esp_err_t wifi_manager_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create Wi-Fi event group");
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_netif_init());

    esp_err_t result = esp_event_loop_create_default();

    if (result != ESP_OK &&
        result != ESP_ERR_INVALID_STATE) {

        ESP_LOGE(
            TAG,
            "Failed to create event loop: %s",
            esp_err_to_name(result)
        );

        return result;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&wifi_init_config)
    );

    esp_event_handler_instance_t wifi_event_instance;
    esp_event_handler_instance_t ip_event_instance;

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            &wifi_event_instance
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL,
            &ip_event_instance
        )
    );

    wifi_config_t wifi_config = {0};

    strncpy(
        (char *)wifi_config.sta.ssid,
        WIFI_SSID,
        sizeof(wifi_config.sta.ssid) - 1
    );

    strncpy(
        (char *)wifi_config.sta.password,
        WIFI_PASSWORD,
        sizeof(wifi_config.sta.password) - 1
    );

    wifi_config.sta.threshold.authmode =
        WIFI_AUTH_WPA2_PSK;

    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(
        TAG,
        "Connecting to Wi-Fi: %s",
        WIFI_SSID
    );

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    }

    ESP_LOGE(
        TAG,
        "Could not connect to Wi-Fi"
    );

    return ESP_FAIL;
}

bool wifi_manager_is_connected(void)
{
    return s_is_connected;
}