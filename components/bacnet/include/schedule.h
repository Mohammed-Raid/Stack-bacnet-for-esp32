#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>
/* Includes existants */
#include "bacdef.h"
#include "bacstr.h"
#include "timestamp.h"
#include "dailyschedule.h"
#include "special_event.h"

/* --- AJOUTS POUR CORRIGER LES ERREURS D'EDITEUR --- */
#include "rp.h"  /* Définit BACNET_READ_PROPERTY_DATA */
#include "wp.h"  /* Définit BACNET_WRITE_PROPERTY_DATA */
/* -------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_SCHEDULES
#define MAX_SCHEDULES 1
#endif

#ifndef BACNET_WEEKLY_SCHEDULE_SIZE
#define BACNET_WEEKLY_SCHEDULE_SIZE 7
#endif

#ifndef BACNET_EXCEPTION_SCHEDULE_SIZE
#define BACNET_EXCEPTION_SCHEDULE_SIZE 0 
#endif

/* Définition de la structure de l'objet Schedule */
typedef struct Schedule_Descr {
    BACNET_DATE Start_Date;
    BACNET_DATE End_Date;
    BACNET_DAILY_SCHEDULE Weekly_Schedule[BACNET_WEEKLY_SCHEDULE_SIZE];
    BACNET_APPLICATION_DATA_VALUE Schedule_Default;
    BACNET_APPLICATION_DATA_VALUE Present_Value;
    bool Out_Of_Service;
    uint8_t Priority_For_Writing;
    
    /* Propriétés simplifiées pour l'embarqué */
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE Object_Property_References[2];
    uint8_t obj_prop_ref_cnt;
    
#if BACNET_EXCEPTION_SCHEDULE_SIZE > 0
    BACNET_SPECIAL_EVENT Exception_Schedule[BACNET_EXCEPTION_SCHEDULE_SIZE];
#endif
} SCHEDULE_DESCR;

/* Fonctions publiques */
void Schedule_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);

void Schedule_Init(void);
unsigned Schedule_Count(void);
uint32_t Schedule_Index_To_Instance(unsigned index);
unsigned Schedule_Instance_To_Index(uint32_t instance);
bool Schedule_Valid_Instance(uint32_t object_instance);
bool Schedule_Object_Name(uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
int Schedule_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
bool Schedule_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

SCHEDULE_DESCR *Schedule_Object(uint32_t object_instance);
/* Fonction de calcul (Moteur) */
void Schedule_Recalculate_PV(
    SCHEDULE_DESCR *desc,
    BACNET_WEEKDAY wday,
    const BACNET_TIME *time);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif