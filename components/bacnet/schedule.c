#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "bacdef.h"
#include "bacdcode.h"
#include "bactext.h"
#include "proplist.h"
#include "timestamp.h"
#include "debug.h"
#include "device.h"
#include "schedule.h"
#include "bacapp.h"

/* --- CORRECTIONS POUR COMPATIBILITÉ --- */

/* Définition des types manquants */
#ifndef BACNET_ARRAY_INDEX
typedef uint8_t BACNET_ARRAY_INDEX;
#endif

#ifndef BACNET_ARRAY_ALL
#define BACNET_ARRAY_ALL 0xFFFFFFFF
#endif

/* Définition des constantes Wildcard manquantes */
#ifndef BACNET_PERMANENT_WILDCARD_YEAR
#define BACNET_PERMANENT_WILDCARD_YEAR 255
#endif

#ifndef BACNET_WEEKDAY_ANY
#define BACNET_WEEKDAY_ANY 255
#endif

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

/* -------------------------------------- */

/* Déclaration de l'instance */
static SCHEDULE_DESCR Schedule_Descr[MAX_SCHEDULES];

/* Propriétés */
/* CORRECTION: 'int' au lieu de 'int32_t' pour correspondre au header */
static const int Schedule_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_EFFECTIVE_PERIOD,
    PROP_SCHEDULE_DEFAULT,
    PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES,
    PROP_PRIORITY_FOR_WRITING,
    PROP_STATUS_FLAGS,
    PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE,
    -1
};

static const int Schedule_Properties_Optional[] = {
    PROP_WEEKLY_SCHEDULE,
    -1
};

static const int Schedule_Properties_Proprietary[] = { -1 };

/* Remplacez int32_t par int ici */
void Schedule_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired) *pRequired = Schedule_Properties_Required;
    if (pOptional) *pOptional = Schedule_Properties_Optional;
    if (pProprietary) *pProprietary = Schedule_Properties_Proprietary;
}

SCHEDULE_DESCR *Schedule_Object(uint32_t object_instance)
{
    unsigned index = Schedule_Instance_To_Index(object_instance);
    if (index < MAX_SCHEDULES) {
        return &Schedule_Descr[index];
    }
    return NULL;
}

void Schedule_Init(void)
{
    unsigned i, j;
    BACNET_DATE start_date;
    BACNET_DATE end_date;
    SCHEDULE_DESCR *psched;

    /* Configuration des dates Wildcard avec valeurs numériques (255 = n'importe quel) */
    start_date.year = 255;  /* Wildcard Year */
    start_date.month = 1;
    start_date.day = 1;
    start_date.wday = 255;  /* Wildcard Day of Week */

    end_date.year = 255;    /* Wildcard Year */
    end_date.month = 12;
    end_date.day = 31;
    end_date.wday = 255;    /* Wildcard Day of Week */

    for (i = 0; i < MAX_SCHEDULES; i++) {
        psched = &Schedule_Descr[i];
        
        /* Copie dates */
        psched->Start_Date = start_date;
        psched->End_Date = end_date;

        /* Reset Weekly Schedule */
        for (j = 0; j < BACNET_WEEKLY_SCHEDULE_SIZE; j++) {
            psched->Weekly_Schedule[j].TV_Count = 0;
        }

        /* Valeur par défaut : 21.0 */
        psched->Schedule_Default.context_specific = false;
        psched->Schedule_Default.tag = BACNET_APPLICATION_TAG_REAL;
        psched->Schedule_Default.type.Real = 21.0f;
        
        /* Copie valeur par défaut dans Present Value */
        psched->Present_Value = psched->Schedule_Default;

        psched->obj_prop_ref_cnt = 0;
        psched->Priority_For_Writing = 16;
        psched->Out_Of_Service = false;
    }
}

unsigned Schedule_Count(void) { return MAX_SCHEDULES; }

uint32_t Schedule_Index_To_Instance(unsigned index) { return index; }

unsigned Schedule_Instance_To_Index(uint32_t instance) {
    return (instance < MAX_SCHEDULES) ? instance : MAX_SCHEDULES;
}

bool Schedule_Valid_Instance(uint32_t object_instance) {
    return (Schedule_Instance_To_Index(object_instance) < MAX_SCHEDULES);
}

bool Schedule_Object_Name(uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32];
    if (Schedule_Valid_Instance(object_instance)) {
        snprintf(text, sizeof(text), "SCHEDULE %u", (unsigned)object_instance);
        characterstring_init_ansi(object_name, text);
        return true;
    }
    return false;
}

/* Encodeur local pour le tableau Weekly Schedule */
static int Schedule_Weekly_Schedule_Encode(uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    SCHEDULE_DESCR *pObject = Schedule_Object(object_instance);
    if (pObject && array_index < BACNET_WEEKLY_SCHEDULE_SIZE) {
        return bacnet_dailyschedule_context_encode(apdu, 0, &pObject->Weekly_Schedule[array_index]);
    }
    return BACNET_STATUS_ERROR;
}

int Schedule_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    int len = 0;
    SCHEDULE_DESCR *CurrentSC = Schedule_Object(rpdata->object_instance);
    uint8_t *apdu = rpdata->application_data;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;

    if (!CurrentSC) return BACNET_STATUS_ERROR;

    switch ((int)rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(apdu, OBJECT_SCHEDULE, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Schedule_Object_Name(rpdata->object_instance, &char_string);
            apdu_len = encode_application_character_string(apdu, &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(apdu, OBJECT_SCHEDULE);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = bacapp_encode_application_data(apdu, &CurrentSC->Present_Value);
            break;
        case PROP_EFFECTIVE_PERIOD:
            apdu_len = encode_application_date(apdu, &CurrentSC->Start_Date);
            apdu_len += encode_application_date(&apdu[apdu_len], &CurrentSC->End_Date);
            break;
        case PROP_WEEKLY_SCHEDULE:
            /* Gestion manuelle du tableau pour éviter la dépendance bacnet_array_encode manquante */
            if (rpdata->array_index == 0) {
                apdu_len = encode_application_unsigned(apdu, BACNET_WEEKLY_SCHEDULE_SIZE);
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                int i;
                for (i = 0; i < BACNET_WEEKLY_SCHEDULE_SIZE; i++) {
                    len = Schedule_Weekly_Schedule_Encode(rpdata->object_instance, i, apdu ? &apdu[apdu_len] : NULL);
                    if (len < 0) return BACNET_STATUS_ERROR;
                    apdu_len += len;
                }
            } else {
                if (rpdata->array_index <= BACNET_WEEKLY_SCHEDULE_SIZE) {
                    apdu_len = Schedule_Weekly_Schedule_Encode(rpdata->object_instance, rpdata->array_index - 1, apdu);
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        case PROP_SCHEDULE_DEFAULT:
            apdu_len = bacapp_encode_application_data(apdu, &CurrentSC->Schedule_Default);
            break;
        case PROP_PRIORITY_FOR_WRITING:
            apdu_len = encode_application_unsigned(apdu, CurrentSC->Priority_For_Writing);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, CurrentSC->Out_Of_Service);
            apdu_len = encode_application_bitstring(apdu, &bit_string);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(apdu, RELIABILITY_NO_FAULT_DETECTED);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(apdu, CurrentSC->Out_Of_Service);
            break;
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
             /* Vide pour l'instant */
             break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    return apdu_len;
}

bool Schedule_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    SCHEDULE_DESCR *CurrentSC = Schedule_Object(wp_data->object_instance);
    bool status = false;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    if (!CurrentSC) return false;

    /* Decode basic properties only */
    len = bacapp_decode_application_data(wp_data->application_data, wp_data->application_data_len, &value);
    (void)len; 

    switch ((int)wp_data->object_property) {
        /* --- AJOUT DU CAS PRESENT VALUE --- */
        case PROP_PRESENT_VALUE:
            if (CurrentSC->Out_Of_Service) {
                /* On accepte l'écriture manuelle SEULEMENT si OutOfService est True */
                CurrentSC->Present_Value = value;
                status = true;
            } else {
                /* Sinon, c'est le calendrier qui décide, écriture interdite */
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
        /* ---------------------------------- */

        case PROP_OUT_OF_SERVICE:
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                CurrentSC->Out_Of_Service = value.type.Boolean;
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        case PROP_WEEKLY_SCHEDULE:
        case PROP_SCHEDULE_DEFAULT:
        case PROP_EFFECTIVE_PERIOD:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;

        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }
    return status;
}/* CORRECTION: Utilisation de BACNET_WEEKDAY */
void Schedule_Recalculate_PV(SCHEDULE_DESCR *desc, BACNET_WEEKDAY wday, const BACNET_TIME *time)
{
    int i;
    /* Reset à la valeur par défaut */
    desc->Present_Value = desc->Schedule_Default;

    /* wday BACnet : 1=Lundi ... 7=Dimanche. Index tableau : 0..6 */
    if (wday >= 1 && wday <= 7) {
        int day_idx = wday - 1;
        for (i = 0; i < desc->Weekly_Schedule[day_idx].TV_Count; i++) {
            /* Compare l'heure actuelle avec l'heure de l'événement */
            if (datetime_compare_time((BACNET_TIME*)time, (BACNET_TIME*)&desc->Weekly_Schedule[day_idx].Time_Values[i].Time) >= 0) {
                /* On a dépassé l'heure de l'événement, c'est la nouvelle valeur courante */
                desc->Present_Value = desc->Weekly_Schedule[day_idx].Time_Values[i].Value;
            }
        }
    }
}