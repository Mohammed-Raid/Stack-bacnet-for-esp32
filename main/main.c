#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

/* Headers BACnet */
#include "ethernet.h"
#include "server_task.h"
#include "device.h"
#include "config.h"
#include "address.h"
#include "bacdef.h"
#include "handlers.h"
#include "client.h"
#include "dlenv.h"
#include "bacdcode.h"
#include "npdu.h"
#include "apdu.h"
#include "iam.h"
#include "tsm.h"
#include "datalink.h"
#include "dcc.h"
#include "getevent.h"
#include "net.h"
#include "txbuf.h"
#include "version.h"
#include "av.h"
#include "bip.h"
#include "schedule.h"

#define SERVER_DEVICE_ID 260127
static const char *TAG = "MAIN";
static SemaphoreHandle_t xIpReadySemaphore = NULL;

/* --- CONFIGURATION DIGNE DU DATASHEET --- */
#define GPIO_PWR_EN         16
#define GPIO_RS485_RTS      8
#define I2C_SDA_PIN         18
#define I2C_SCL_PIN         19
#define I2C_PORT            0
#define PCF8563_ADDR        0x51

#define bcd2dec(x) (((x) / 16) * 10 + ((x) & 0x0f))
#define dec2bcd(x) (((x) / 10) * 16 + ((x) % 10))

void handler_timesynchronization(uint8_t * service_request, uint16_t service_len, BACNET_ADDRESS * src);

/* Fonction pour initialiser le matériel EdgeBox proprement */
static void edgebox_hardware_init(void)
{
    ESP_LOGI("HW", "--- INITIALISATION EDGEBOX ESP-100 ---");

    /* 1. COUPER LE RS485 (CRITIQUE) */
    /* Le GPIO 18 est partage entre I2C_SDA et RS485_RX. */
    /* Si le driver RS485 est en mode reception, il tire la ligne et tue l'I2C. */
    /* On met RTS (GPIO 8) a HIGH pour passer en mode TRANSMISSION (ce qui desactive la reception) */
    gpio_reset_pin(GPIO_RS485_RTS);
    gpio_set_direction(GPIO_RS485_RTS, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_RS485_RTS, 1);
    ESP_LOGI("HW", "RS485 desactive (GPIO 8 HIGH) pour liberer SDA");

    /* 2. ALLUMER LES PERIPHERIQUES */
    gpio_reset_pin(GPIO_PWR_EN);
    gpio_set_direction(GPIO_PWR_EN, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_PWR_EN, 1); /* HIGH = ON */
    ESP_LOGI("HW", "Alimentation Peripheriques ON (GPIO 16 HIGH)");

    /* Pause pour stabilisation des tensions */
    vTaskDelay(pdMS_TO_TICKS(500));

    /* 3. INITIALISER L'I2C */
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
        .clk_flags = 0
    };
    i2c_param_config(I2C_PORT, &conf);
    esp_err_t ret = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    
    if (ret == ESP_OK) {
        ESP_LOGI("HW", "Driver I2C installe sur SDA=%d SCL=%d", I2C_SDA_PIN, I2C_SCL_PIN);
    } else {
        ESP_LOGE("HW", "Echec driver I2C: %s", esp_err_to_name(ret));
    }
}

/* Lecture RTC (PCF8563) */
static void rtc_read_time(void)
{
    uint8_t data[7];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    /* Test de presence avant lecture */
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF8563_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE("RTC", "RTC (0x51) introuvable ! Verifiez l'alimentation ou le conflit RS485.");
        return;
    }

    /* Lecture des registres temps (0x02 a 0x08) */
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF8563_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x02, true); // Registre VL_SECONDS
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF8563_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 7, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        struct tm t = {0};
        t.tm_sec  = bcd2dec(data[0] & 0x7F);
        t.tm_min  = bcd2dec(data[1] & 0x7F);
        t.tm_hour = bcd2dec(data[2] & 0x3F);
        t.tm_mday = bcd2dec(data[3] & 0x3F);
        t.tm_wday = bcd2dec(data[4] & 0x07);
        t.tm_mon  = bcd2dec(data[5] & 0x1F) - 1;
        t.tm_year = bcd2dec(data[6]) + 100;

        struct timeval tv = { .tv_sec = mktime(&t), .tv_usec = 0 };
        settimeofday(&tv, NULL);
        
        ESP_LOGI("RTC", "Heure lue : %02d/%02d/%04d %02d:%02d:%02d",
                 t.tm_mday, t.tm_mon + 1, t.tm_year + 1900, 
                 t.tm_hour, t.tm_min, t.tm_sec);
    }
}

/* Ecriture RTC (PCF8563) */
void rtc_write_current_time(void)
{
    time_t now;
    struct tm t;
    time(&now);
    localtime_r(&now, &t);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF8563_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x02, true); 
    i2c_master_write_byte(cmd, dec2bcd(t.tm_sec), true);
    i2c_master_write_byte(cmd, dec2bcd(t.tm_min), true);
    i2c_master_write_byte(cmd, dec2bcd(t.tm_hour), true);
    i2c_master_write_byte(cmd, dec2bcd(t.tm_mday), true);
    i2c_master_write_byte(cmd, dec2bcd(t.tm_wday), true);
    i2c_master_write_byte(cmd, dec2bcd(t.tm_mon + 1), true);
    i2c_master_write_byte(cmd, dec2bcd(t.tm_year - 100), true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret == ESP_OK) {
        ESP_LOGI("RTC", "SUCCES: PCF8563 mis a jour !");
    } else {
        ESP_LOGE("RTC", "ECHEC ECRITURE (Erreur %s)", esp_err_to_name(ret));
    }
}

void handler_timesynchronization(uint8_t * service_request, uint16_t service_len, BACNET_ADDRESS * src)
{
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    (void) src; 

    len = bacapp_decode_application_data(service_request, service_len, &value);
    if (len < 0 || value.tag != BACNET_APPLICATION_TAG_DATE) return;
    BACNET_DATE bdate = value.type.Date;

    int len2 = bacapp_decode_application_data(&service_request[len], service_len - len, &value);
    if (len2 < 0 || value.tag != BACNET_APPLICATION_TAG_TIME) return;
    BACNET_TIME btime = value.type.Time;

    struct tm t = {0};
    t.tm_year = bdate.year - 1900;
    t.tm_mon = bdate.month - 1;
    t.tm_mday = bdate.day;
    t.tm_hour = btime.hour;
    t.tm_min = btime.min;
    t.tm_sec = btime.sec;
    struct timeval tv = { .tv_sec = mktime(&t), .tv_usec = 0 };
    settimeofday(&tv, NULL);

    ESP_LOGI("BACNET", "Time Synced ! %02d/%02d/%04d %02d:%02d:%02d",
             bdate.day, bdate.month, bdate.year, btime.hour, btime.min, btime.sec);

    rtc_write_current_time();
}

static object_functions_t Object_Table[] = {
   { OBJECT_DEVICE, NULL, Device_Count, Device_Index_To_Instance, Device_Valid_Object_Instance_Number, Device_Object_Name, Device_Read_Property_Local, Device_Write_Property_Local, Device_Property_Lists, NULL, NULL, NULL, NULL, NULL, NULL },
   { OBJECT_ANALOG_VALUE, Analog_Value_Init, Analog_Value_Count, Analog_Value_Index_To_Instance, Analog_Value_Valid_Instance, Analog_Value_Object_Name, Analog_Value_Read_Property, Analog_Value_Write_Property, Analog_Value_Property_Lists, NULL, NULL, NULL, NULL, NULL, NULL },
   { OBJECT_SCHEDULE, Schedule_Init, Schedule_Count, Schedule_Index_To_Instance, Schedule_Valid_Instance, Schedule_Object_Name, Schedule_Read_Property, Schedule_Write_Property, (rpm_property_lists_function)Schedule_Property_Lists, NULL, NULL, NULL, NULL, NULL, NULL },
};

static void Init_Service_Handlers(void)
{
    Device_Init(&Object_Table[0]);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesynchronization);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE, handler_write_property_multiple);
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
}

static void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Ethernet Connecté! IP: " IPSTR, IP2STR(&event->ip_info.ip));
    if (xIpReadySemaphore != NULL) {
        xSemaphoreGive(xIpReadySemaphore);
    }
}

void app_main(void)
{
    xIpReadySemaphore = xSemaphoreCreateBinary();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* --- INITIALISATION MATERIEL --- */
    edgebox_hardware_init();
    rtc_read_time();

    ESP_ERROR_CHECK(ethernet_init());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &on_got_ip, NULL, NULL));

    ESP_LOGI(TAG, "Attente de la connexion Ethernet...");

    if (xSemaphoreTake(xIpReadySemaphore, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Connexion établie ! Démarrage de BACnet...");

        Device_Set_Object_Instance_Number(SERVER_DEVICE_ID);
        address_init();
        Init_Service_Handlers(); 
        dlenv_init();

        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
        if (netif) {
            esp_netif_ip_info_t ip_info;
            esp_netif_get_ip_info(netif, &ip_info);
            bip_set_addr(ip_info.ip.addr);
            bip_set_broadcast_addr(ip_info.ip.addr | ~ip_info.netmask.addr);
            ESP_LOGI(TAG, "BACnet IP corrigée sur : " IPSTR, IP2STR(&ip_info.ip));
        }

        Send_I_Am(&Handler_Transmit_Buffer[0]);
        xTaskCreate(server_task, "bacnet_server", 8000, NULL, 5, NULL);
        ESP_LOGI(TAG, "Serveur BACnet opérationnel !");
        
        while(1) {
            vTaskDelay(pdMS_TO_TICKS(60000));
        }
    }
}