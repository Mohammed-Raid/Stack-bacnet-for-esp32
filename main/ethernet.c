/* main/ethernet.c */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_mac.h"
#include "esp_eth_mac_spi.h"
#include "ethernet.h"

static const char *TAG = "EDGEBOX_ETH";
static esp_eth_handle_t s_eth_handle = NULL;

// Pins EdgeBox-ESP-100
#define ETH_SPI_HOST         SPI2_HOST
#define ETH_SPI_MISO_GPIO    11  // Inversion test
#define ETH_SPI_MOSI_GPIO    12  // Inversion test
#define ETH_SPI_SCLK_GPIO    13
#define ETH_SPI_CS_GPIO      10
#define ETH_SPI_INT_GPIO     14
#define ETH_SPI_RST_GPIO     15

// Reset manuel du W5500
static void w5500_hw_reset(void) {
    gpio_reset_pin(ETH_SPI_RST_GPIO);
    gpio_set_direction(ETH_SPI_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ETH_SPI_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(ETH_SPI_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

esp_err_t ethernet_init(void) {
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) return ret;

    ret = esp_event_loop_create_default();
    
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);

    gpio_install_isr_service(0);
    w5500_hw_reset();

    spi_bus_config_t buscfg = {
        .miso_io_num = ETH_SPI_MISO_GPIO,
        .mosi_io_num = ETH_SPI_MOSI_GPIO,
        .sclk_io_num = ETH_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .command_bits = 16, .address_bits = 8, .mode = 0,
        .clock_speed_hz = 16 * 1000 * 1000,
        .spics_io_num = ETH_SPI_CS_GPIO, .queue_size = 20
    };

    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(ETH_SPI_HOST, &devcfg);
    w5500_config.int_gpio_num = ETH_SPI_INT_GPIO;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.reset_gpio_num = -1;
    phy_config.phy_addr = 1;
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &s_eth_handle));

    // Application de l'adresse MAC unique
    uint8_t base_mac_addr[ETH_ADDR_LEN];
    ESP_ERROR_CHECK(esp_read_mac(base_mac_addr, ESP_MAC_ETH));
    ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_MAC_ADDR, base_mac_addr));

    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(s_eth_handle)));
    ESP_ERROR_CHECK(esp_eth_start(s_eth_handle));
    
    ESP_LOGI(TAG, "Ethernet initialise avec succes");
    return ESP_OK;
}

void ethernet_get_mac(uint8_t *mac_buff) {
    if (s_eth_handle) esp_eth_ioctl(s_eth_handle, ETH_CMD_G_MAC_ADDR, mac_buff);
    else esp_read_mac(mac_buff, ESP_MAC_ETH);
}