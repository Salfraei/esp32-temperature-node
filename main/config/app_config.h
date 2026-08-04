#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define DS18B20_GPIO 4
#define TEMPERATURE_READ_INTERVAL_MS 5000

#define WIFI_SSID "wifi173"
#define WIFI_PASSWORD "87654321"
#define WIFI_MAXIMUM_RETRY 10

#define MQTT_BROKER_URI "mqtt://broker.emqx.io"
#define MQTT_TOPIC "pavlo/esp32/room1/temperature"
#define MQTT_CLIENT_ID "esp32-temperature-node"

#endif