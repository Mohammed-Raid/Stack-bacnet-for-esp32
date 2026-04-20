/* Large Analog Value Objects (double precision) */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"
#include "rp.h"
#include "wp.h"
#include "lav.h"
#include "handlers.h"

static LARGE_ANALOG_VALUE_DESCR LAV_Descr[MAX_LARGE_ANALOG_VALUES];

static const int Large_Analog_Value_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_UNITS,
    -1
};

static const int Large_Analog_Value_Properties_Optional[] = {
    PROP_DESCRIPTION,
    -1
};

static const int Large_Analog_Value_Properties_Proprietary[] = {
    -1
};

void Large_Analog_Value_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Large_Analog_Value_Properties_Required;
    if (pOptional)
        *pOptional = Large_Analog_Value_Properties_Optional;
    if (pProprietary)
        *pProprietary = Large_Analog_Value_Properties_Proprietary;
}

void Large_Analog_Value_Init(void)
{
    unsigned i;
    for (i = 0; i < MAX_LARGE_ANALOG_VALUES; i++) {
        memset(&LAV_Descr[i], 0, sizeof(LARGE_ANALOG_VALUE_DESCR));
        LAV_Descr[i].Units = UNITS_NO_UNITS;
        LAV_Descr[i].Event_State = EVENT_STATE_NORMAL;
    }
}

bool Large_Analog_Value_Valid_Instance(uint32_t object_instance)
{
    return object_instance < MAX_LARGE_ANALOG_VALUES;
}

unsigned Large_Analog_Value_Count(void)
{
    return MAX_LARGE_ANALOG_VALUES;
}

uint32_t Large_Analog_Value_Index_To_Instance(unsigned index)
{
    return index;
}

unsigned Large_Analog_Value_Instance_To_Index(uint32_t object_instance)
{
    if (object_instance < MAX_LARGE_ANALOG_VALUES)
        return object_instance;
    return MAX_LARGE_ANALOG_VALUES;
}

double Large_Analog_Value_Present_Value(uint32_t object_instance)
{
    unsigned index = Large_Analog_Value_Instance_To_Index(object_instance);
    if (index < MAX_LARGE_ANALOG_VALUES)
        return LAV_Descr[index].Present_Value;
    return 0.0;
}

bool Large_Analog_Value_Present_Value_Set(
    uint32_t object_instance,
    double value)
{
    unsigned index = Large_Analog_Value_Instance_To_Index(object_instance);
    if (index < MAX_LARGE_ANALOG_VALUES) {
        LAV_Descr[index].Present_Value = value;
        return true;
    }
    return false;
}

bool Large_Analog_Value_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32];
    if (object_instance < MAX_LARGE_ANALOG_VALUES) {
        sprintf(text_string, "LARGE ANALOG VALUE %lu",
            (unsigned long)object_instance);
        return characterstring_init_ansi(object_name, text_string);
    }
    return false;
}

int Large_Analog_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index;
    uint8_t *apdu;
    LARGE_ANALOG_VALUE_DESCR *CurrentLAV;

    if (!rpdata || !rpdata->application_data || !rpdata->application_data_len)
        return 0;

    apdu = rpdata->application_data;
    object_index = Large_Analog_Value_Instance_To_Index(rpdata->object_instance);
    if (object_index >= MAX_LARGE_ANALOG_VALUES)
        return BACNET_STATUS_ERROR;
    CurrentLAV = &LAV_Descr[object_index];

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(&apdu[0],
                OBJECT_LARGE_ANALOG_VALUE, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Large_Analog_Value_Object_Name(rpdata->object_instance, &char_string);
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0],
                OBJECT_LARGE_ANALOG_VALUE);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_double(&apdu[0],
                CurrentLAV->Present_Value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                CurrentLAV->Out_Of_Service);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(&apdu[0],
                EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0],
                CurrentLAV->Out_Of_Service);
            break;
        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0],
                CurrentLAV->Units);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    if ((apdu_len >= 0) &&
        (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

bool Large_Analog_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false;
    int len;
    unsigned object_index;
    BACNET_APPLICATION_DATA_VALUE value;
    LARGE_ANALOG_VALUE_DESCR *CurrentLAV;

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
    object_index = Large_Analog_Value_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_LARGE_ANALOG_VALUES)
        return false;
    CurrentLAV = &LAV_Descr[object_index];

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_DOUBLE,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentLAV->Present_Value = value.type.Double;
            break;
        case PROP_OUT_OF_SERVICE:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentLAV->Out_Of_Service = value.type.Boolean;
            break;
        case PROP_UNITS:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_ENUMERATED,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentLAV->Units = value.type.Enumerated;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }
    return status;
}

void Large_Analog_Value_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
