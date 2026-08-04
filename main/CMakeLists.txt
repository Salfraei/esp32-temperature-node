idf_component_register(
    SRCS
        "main.c"
        "drivers/ds18b20_driver.c"
        "services/wifi_manager.c"
        "services/mqtt_manager.c"

    INCLUDE_DIRS
        "include"
)