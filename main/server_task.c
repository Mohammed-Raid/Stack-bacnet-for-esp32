#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* --- BACnet Includes --- */
#include "bacdef.h"
#include "config.h"
#include "bacdcode.h"
#include "apdu.h"
#include "datalink.h"
#include "npdu.h"
#include "handlers.h"
#include "txbuf.h"
#include "device.h"     
#include "av.h"         
#include "schedule.h"   

#define MAX_MPDU 1476

static const char *TAG = "BACNET_SERVER";
static uint8_t rx_buffer[MAX_MPDU];

/* --- Prototype manuel pour éviter l'erreur de compilation --- */
/* (Au cas où il manquerait dans schedule.h) */
extern SCHEDULE_DESCR *Schedule_Object(uint32_t object_instance);

void server_task(void *arg)
{
    BACNET_ADDRESS src = { 0 };
    uint16_t pdu_len = 0;
    
    time_t now;
    struct tm timeinfo;
    BACNET_DATE bdate;
    BACNET_TIME btime;

    ESP_LOGI(TAG, "BACnet Server Ready");

    for (;;) {
        /* 1. Gestion Réseau BACnet (Réception) */
        pdu_len = datalink_receive(&src, &rx_buffer[0], MAX_MPDU, 50);
        if (pdu_len) {
            npdu_handler(&src, &rx_buffer[0], pdu_len);
        }

        /* 2. Gestion du Temps */
        /* On récupère l'heure de l'ESP32 */
        time(&now);
        localtime_r(&now, &timeinfo);

        /* Conversion format C -> format BACnet */
        bdate.year = timeinfo.tm_year + 1900;
        bdate.month = timeinfo.tm_mon + 1;
        bdate.day = timeinfo.tm_mday;
        bdate.wday = (timeinfo.tm_wday == 0) ? 7 : timeinfo.tm_wday; 
        
        btime.hour = timeinfo.tm_hour;
        btime.min = timeinfo.tm_min;
        btime.sec = timeinfo.tm_sec;
        btime.hundredths = 0;

        /* NOTE : On a supprimé Device_Set_Local_Date/Time ici */
        /* car le fichier device.c le fait déjà tout seul quand on l'interroge. */

        /* 3. LOGIQUE DE LIAISON (SCHEDULE -> ANALOG VALUE) */
        SCHEDULE_DESCR *sched_obj = Schedule_Object(0); 
        
        if (sched_obj) {
            /* A. Recalcul automatique (Seulement si PAS Out of Service) */
            /* On donne l'heure qu'on vient de calculer directement au moteur du calendrier */
            if (!sched_obj->Out_Of_Service) {
                Schedule_Recalculate_PV(sched_obj, bdate.wday, &btime);
            }

            /* B. APPLICATION DE LA VALEUR */
            if (sched_obj->Present_Value.tag == BACNET_APPLICATION_TAG_REAL) {
                float val_schedule = sched_obj->Present_Value.type.Real;
                
                /* Priorité 16 (la plus basse) */
                Analog_Value_Present_Value_Set(0, val_schedule, 16);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}