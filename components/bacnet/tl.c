/* Trend Log Objects */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"
#include "datetime.h"
#include "rp.h"
#include "wp.h"
#include "tl.h"
#include "handlers.h"

static TREND_LOG_DESCR TL_Descr[MAX_TREND_LOGS];

static const int Trend_Log_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_ENABLE,
    PROP_BUFFER_SIZE,
    PROP_LOG_BUFFER,
    PROP_RECORD_COUNT,
    PROP_TOTAL_RECORD_COUNT,
    PROP_EVENT_STATE,
    PROP_STATUS_FLAGS,
    -1
};

static const int Trend_Log_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_START_TIME,
    PROP_STOP_TIME,
    PROP_LOG_INTERVAL,
    -1
};

static const int Trend_Log_Properties_Proprietary[] = {
    -1
};

void Trend_Log_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Trend_Log_Properties_Required;
    if (pOptional)
        *pOptional = Trend_Log_Properties_Optional;
    if (pProprietary)
        *pProprietary = Trend_Log_Properties_Proprietary;
}

void Trend_Log_Init(void)
{
    unsigned i;
    for (i = 0; i < MAX_TREND_LOGS; i++) {
        memset(&TL_Descr[i], 0, sizeof(TREND_LOG_DESCR));
        TL_Descr[i].Enable = true;
        TL_Descr[i].Buffer_Size = TREND_LOG_BUFFER_SIZE;
        TL_Descr[i].Log_Interval = 100; /* 1 second in hundredths */
        datetime_wildcard_set(&TL_Descr[i].Start_Time);
        datetime_wildcard_set(&TL_Descr[i].Stop_Time);
    }
}

bool Trend_Log_Valid_Instance(uint32_t object_instance)
{
    return object_instance < MAX_TREND_LOGS;
}

unsigned Trend_Log_Count(void)
{
    return MAX_TREND_LOGS;
}

uint32_t Trend_Log_Index_To_Instance(unsigned index)
{
    return index;
}

unsigned Trend_Log_Instance_To_Index(uint32_t object_instance)
{
    if (object_instance < MAX_TREND_LOGS)
        return object_instance;
    return MAX_TREND_LOGS;
}

bool Trend_Log_Add_Record(uint32_t object_instance, float value)
{
    unsigned index = Trend_Log_Instance_To_Index(object_instance);
    TREND_LOG_DESCR *tl;
    uint32_t slot;

    if (index >= MAX_TREND_LOGS || !TL_Descr[index].Enable)
        return false;

    tl = &TL_Descr[index];
    if (tl->Record_Count < TREND_LOG_BUFFER_SIZE) {
        slot = tl->Record_Count;
        tl->Record_Count++;
    } else {
        slot = tl->head;
        tl->head = (tl->head + 1) % TREND_LOG_BUFFER_SIZE;
    }
    tl->Total_Record_Count++;
    /* store value; timestamp would be set by the application */
    tl->Buffer[slot].value = value;
    tl->Buffer[slot].status = 0;
    return true;
}

bool Trend_Log_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32];
    if (object_instance < MAX_TREND_LOGS) {
        sprintf(text_string, "TREND LOG %lu",
            (unsigned long)object_instance);
        return characterstring_init_ansi(object_name, text_string);
    }
    return false;
}

int Trend_Log_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index;
    uint8_t *apdu;
    TREND_LOG_DESCR *CurrentTL;

    if (!rpdata || !rpdata->application_data || !rpdata->application_data_len)
        return 0;

    apdu = rpdata->application_data;
    object_index = Trend_Log_Instance_To_Index(rpdata->object_instance);
    if (object_index >= MAX_TREND_LOGS)
        return BACNET_STATUS_ERROR;
    CurrentTL = &TL_Descr[object_index];

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(&apdu[0],
                OBJECT_TRENDLOG, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Trend_Log_Object_Name(rpdata->object_instance, &char_string);
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_TRENDLOG);
            break;
        case PROP_ENABLE:
            apdu_len = encode_application_boolean(&apdu[0], CurrentTL->Enable);
            break;
        case PROP_BUFFER_SIZE:
            apdu_len = encode_application_unsigned(&apdu[0], CurrentTL->Buffer_Size);
            break;
        case PROP_RECORD_COUNT:
            apdu_len = encode_application_unsigned(&apdu[0], CurrentTL->Record_Count);
            break;
        case PROP_TOTAL_RECORD_COUNT:
            apdu_len = encode_application_unsigned(&apdu[0], CurrentTL->Total_Record_Count);
            break;
        case PROP_LOG_INTERVAL:
            apdu_len = encode_application_unsigned(&apdu[0], CurrentTL->Log_Interval);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(&apdu[0],
                EVENT_STATE_NORMAL);
            break;
        case PROP_START_TIME:
            apdu_len = encode_application_date(&apdu[0],
                &CurrentTL->Start_Time.date);
            apdu_len += encode_application_time(&apdu[apdu_len],
                &CurrentTL->Start_Time.time);
            break;
        case PROP_STOP_TIME:
            apdu_len = encode_application_date(&apdu[0],
                &CurrentTL->Stop_Time.date);
            apdu_len += encode_application_time(&apdu[apdu_len],
                &CurrentTL->Stop_Time.time);
            break;
        case PROP_LOG_BUFFER:
            /* Log buffer requires ReadRange service; return empty list here */
            rpdata->error_class = ERROR_CLASS_SERVICES;
            rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    if ((apdu_len >= 0) &&
        (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->object_property != PROP_LOG_BUFFER) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

bool Trend_Log_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false;
    int len;
    unsigned object_index;
    BACNET_APPLICATION_DATA_VALUE value;
    TREND_LOG_DESCR *CurrentTL;

    len = bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    if (len < 0) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    object_index = Trend_Log_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_TREND_LOGS)
        return false;
    CurrentTL = &TL_Descr[object_index];

    switch (wp_data->object_property) {
        case PROP_ENABLE:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentTL->Enable = value.type.Boolean;
            break;
        case PROP_RECORD_COUNT:
            /* writing 0 clears the log */
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status && value.type.Unsigned_Int == 0) {
                CurrentTL->Record_Count = 0;
                CurrentTL->head = 0;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                status = false;
            }
            break;
        case PROP_LOG_INTERVAL:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentTL->Log_Interval = value.type.Unsigned_Int;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }
    return status;
}

void Trend_Log_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
