idf_component_register(SRCS "main.c" "ethernet.c" "led.c" "server_task.c" "client_task.c"
                    INCLUDE_DIRS "." "../components/bacnet/include"
                    REQUIRES nvs_flash esp_netif esp_eth esp_event driver esp_driver_spi)