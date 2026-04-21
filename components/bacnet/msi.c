/* Multi-state Input Objects */

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
#include "msi.h"
#include "handlers.h"

static MULTI_STATE_INPUT_DESCR MSI_Descr[MAX_MULTI_STATE_INPUTS];

static const int Multi_State_Input_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_NUMBER_OF_STATES,
    -1
};

static const int Multi_State_Input_Properties_Optional[] = {
    PROP_DESCRIPTION,
    -1
};

static const int Multi_State_Input_Properties_Proprietary[] = {
    -1
};

void Multi_State_Input_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Multi_State_Input_Properties_Required;
    if (pOptional)
        *pOptional = Multi_State_Input_Properties_Optional;
    if (pProprietary)
        *pProprietary = Multi_State_Input_Properties_Proprietary;
}

void Multi_State_Input_Init(void)
{
    unsigned i;
    for (i = 0; i < MAX_MULTI_STATE_INPUTS; i++) {
        memset(&MSI_Descr[i], 0, sizeof(MULTI_STATE_INPUT_DESCR));
        MSI_Descr[i].Present_Value = 1;
        MSI_Descr[i].Number_Of_States = 3;
        MSI_Descr[i].Event_State = EVENT_STATE_NORMAL;
    }
}

bool Multi_State_Input_Valid_Instance(uint32_t object_instance)
{
    return object_instance < MAX_MULTI_STATE_INPUTS;
}

unsigned Multi_State_Input_Count(void)
{
    return MAX_MULTI_STATE_INPUTS;
}

uint32_t Multi_State_Input_Index_To_Instance(unsigned index)
{
    return index;
}

unsigned Multi_State_Input_Instance_To_Index(uint32_t object_instance)
{
    if (object_instance < MAX_MULTI_STATE_INPUTS)
        return object_instance;
    return MAX_MULTI_STATE_INPUTS;
}

uint32_t Multi_State_Input_Present_Value(uint32_t object_instance)
{
    unsigned index = Multi_State_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTI_STATE_INPUTS)
        return MSI_Descr[index].Present_Value;
    return 1;
}

bool Multi_State_Input_Present_Value_Set(
    uint32_t object_instance,
    uint32_t value)
{
    unsigned index = Multi_State_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTI_STATE_INPUTS &&
        value >= 1 && value <= MSI_Descr[index].Number_Of_States) {
        MSI_Descr[index].Present_Value = value;
        return true;
    }
    return false;
}

bool Multi_State_Input_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32];
    if (object_instance < MAX_MULTI_STATE_INPUTS) {
        sprintf(text_string, "MULTI-STATE INPUT %lu",
            (unsigned long)object_instance);
        return characterstring_init_ansi(object_name, text_string);
    }
    return false;
}

int Multi_State_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index;
    uint8_t *apdu;
    MULTI_STATE_INPUT_DESCR *CurrentMSI;

    if (!rpdata || !rpdata->application_data || !rpdata->application_data_len)
        return 0;

    apdu = rpdata->application_data;
    object_index = Multi_State_Input_Instance_To_Index(rpdata->object_instance);
    if (object_index >= MAX_MULTI_STATE_INPUTS)
        return BACNET_STATUS_ERROR;
    CurrentMSI = &MSI_Descr[object_index];

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(&apdu[0],
                OBJECT_MULTI_STATE_INPUT, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Multi_State_Input_Object_Name(rpdata->object_instance, &char_string);
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0],
                OBJECT_MULTI_STATE_INPUT);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_unsigned(&apdu[0],
                CurrentMSI->Present_Value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                CurrentMSI->Out_Of_Service);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(&apdu[0],
                EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0],
                CurrentMSI->Out_Of_Service);
            break;
        case PROP_NUMBER_OF_STATES:
            apdu_len = encode_application_unsigned(&apdu[0],
                CurrentMSI->Number_Of_States);
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

bool Multi_State_Input_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false;
    int len;
    BACNET_APPLICATION_DATA_VALUE value;
    unsigned object_index;
    MULTI_STATE_INPUT_DESCR *CurrentMSI;

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
    object_index = Multi_State_Input_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_MULTI_STATE_INPUTS)
        return false;
    CurrentMSI = &MSI_Descr[object_index];

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            /* writable only when Out_Of_Service is TRUE */
            if (!CurrentMSI->Out_Of_Service) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                break;
            }
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                if (value.type.Unsigned_Int >= 1 &&
                    value.type.Unsigned_Int <= CurrentMSI->Number_Of_States) {
                    CurrentMSI->Present_Value = value.type.Unsigned_Int;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    status = false;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentMSI->Out_Of_Service = value.type.Boolean;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }
    return status;
}

void Multi_State_Input_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
