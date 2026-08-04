idf_component_register(
    SRCS
        "main.c"
        "drivers/ds18b20_driver.c"
        "services/wifi_manager.c"
        "services/mqtt_manager.c"
        "utils/logger.c"

    INCLUDE_DIRS
        "config"
        "drivers"
        "services"
        "utils"

    REQUIRES
        ds18b20
        onewire_bus
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
)