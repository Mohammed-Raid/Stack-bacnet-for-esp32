/* Integer Value Objects */

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
#include "iv.h"
#include "handlers.h"

static INTEGER_VALUE_DESCR IV_Descr[MAX_INTEGER_VALUES];

static const int Integer_Value_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    -1
};

static const int Integer_Value_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_UNITS,
    -1
};

static const int Integer_Value_Properties_Proprietary[] = {
    -1
};

void Integer_Value_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Integer_Value_Properties_Required;
    if (pOptional)
        *pOptional = Integer_Value_Properties_Optional;
    if (pProprietary)
        *pProprietary = Integer_Value_Properties_Proprietary;
}

void Integer_Value_Init(void)
{
    unsigned i;
    for (i = 0; i < MAX_INTEGER_VALUES; i++) {
        memset(&IV_Descr[i], 0, sizeof(INTEGER_VALUE_DESCR));
        IV_Descr[i].Units = UNITS_NO_UNITS;
        IV_Descr[i].Event_State = EVENT_STATE_NORMAL;
    }
}

bool Integer_Value_Valid_Instance(uint32_t object_instance)
{
    return object_instance < MAX_INTEGER_VALUES;
}

unsigned Integer_Value_Count(void)
{
    return MAX_INTEGER_VALUES;
}

uint32_t Integer_Value_Index_To_Instance(unsigned index)
{
    return index;
}

unsigned Integer_Value_Instance_To_Index(uint32_t object_instance)
{
    if (object_instance < MAX_INTEGER_VALUES)
        return object_instance;
    return MAX_INTEGER_VALUES;
}

int32_t Integer_Value_Present_Value(uint32_t object_instance)
{
    unsigned index = Integer_Value_Instance_To_Index(object_instance);
    if (index < MAX_INTEGER_VALUES)
        return IV_Descr[index].Present_Value;
    return 0;
}

bool Integer_Value_Present_Value_Set(uint32_t object_instance, int32_t value)
{
    unsigned index = Integer_Value_Instance_To_Index(object_instance);
    if (index < MAX_INTEGER_VALUES) {
        IV_Descr[index].Present_Value = value;
        return true;
    }
    return false;
}

bool Integer_Value_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32];
    if (object_instance < MAX_INTEGER_VALUES) {
        sprintf(text_string, "INTEGER VALUE %lu",
            (unsigned long)object_instance);
        return characterstring_init_ansi(object_name, text_string);
    }
    return false;
}

int Integer_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index;
    uint8_t *apdu;
    INTEGER_VALUE_DESCR *CurrentIV;

    if (!rpdata || !rpdata->application_data || !rpdata->application_data_len)
        return 0;

    apdu = rpdata->application_data;
    object_index = Integer_Value_Instance_To_Index(rpdata->object_instance);
    if (object_index >= MAX_INTEGER_VALUES)
        return BACNET_STATUS_ERROR;
    CurrentIV = &IV_Descr[object_index];

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(&apdu[0],
                OBJECT_INTEGER_VALUE, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Integer_Value_Object_Name(rpdata->object_instance, &char_string);
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0],
                OBJECT_INTEGER_VALUE);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_signed(&apdu[0],
                CurrentIV->Present_Value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                CurrentIV->Out_Of_Service);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(&apdu[0],
                EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0],
                CurrentIV->Out_Of_Service);
            break;
        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0],
                CurrentIV->Units);
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

bool Integer_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false;
    int len;
    unsigned object_index;
    BACNET_APPLICATION_DATA_VALUE value;
    INTEGER_VALUE_DESCR *CurrentIV;

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
    object_index = Integer_Value_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_INTEGER_VALUES)
        return false;
    CurrentIV = &IV_Descr[object_index];

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_SIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentIV->Present_Value = value.type.Signed_Int;
            break;
        case PROP_OUT_OF_SERVICE:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentIV->Out_Of_Service = value.type.Boolean;
            break;
        case PROP_UNITS:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_ENUMERATED,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentIV->Units = value.type.Enumerated;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }
    return status;
}

void Integer_Value_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
