/* Multi-state Output Objects */

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
#include "mso.h"
#include "handlers.h"

static MULTI_STATE_OUTPUT_DESCR MSO_Descr[MAX_MULTI_STATE_OUTPUTS];

static const int Multi_State_Output_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_NUMBER_OF_STATES,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    -1
};

static const int Multi_State_Output_Properties_Optional[] = {
    PROP_DESCRIPTION,
    -1
};

static const int Multi_State_Output_Properties_Proprietary[] = {
    -1
};

void Multi_State_Output_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Multi_State_Output_Properties_Required;
    if (pOptional)
        *pOptional = Multi_State_Output_Properties_Optional;
    if (pProprietary)
        *pProprietary = Multi_State_Output_Properties_Proprietary;
}

void Multi_State_Output_Init(void)
{
    unsigned i, j;
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    for (i = 0; i < MAX_MULTI_STATE_OUTPUTS; i++) {
        memset(&MSO_Descr[i], 0, sizeof(MULTI_STATE_OUTPUT_DESCR));
        MSO_Descr[i].Number_Of_States = 3;
        MSO_Descr[i].Relinquish_Default = 1;
        MSO_Descr[i].Event_State = EVENT_STATE_NORMAL;
        for (j = 0; j < BACNET_MAX_PRIORITY; j++)
            MSO_Descr[i].Priority_Array[j] = MSO_LEVEL_NULL;
    }
}

bool Multi_State_Output_Valid_Instance(uint32_t object_instance)
{
    return object_instance < MAX_MULTI_STATE_OUTPUTS;
}

unsigned Multi_State_Output_Count(void)
{
    return MAX_MULTI_STATE_OUTPUTS;
}

uint32_t Multi_State_Output_Index_To_Instance(unsigned index)
{
    return index;
}

unsigned Multi_State_Output_Instance_To_Index(uint32_t object_instance)
{
    if (object_instance < MAX_MULTI_STATE_OUTPUTS)
        return object_instance;
    return MAX_MULTI_STATE_OUTPUTS;
}

uint32_t Multi_State_Output_Present_Value(uint32_t object_instance)
{
    unsigned index = Multi_State_Output_Instance_To_Index(object_instance);
    unsigned i;
    if (index >= MAX_MULTI_STATE_OUTPUTS)
        return MSO_Descr[0].Relinquish_Default;
    for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
        if (MSO_Descr[index].Priority_Array[i] != MSO_LEVEL_NULL)
            return MSO_Descr[index].Priority_Array[i];
    }
    return MSO_Descr[index].Relinquish_Default;
}

bool Multi_State_Output_Present_Value_Set(
    uint32_t object_instance,
    uint32_t value,
    uint8_t priority)
{
    unsigned index = Multi_State_Output_Instance_To_Index(object_instance);
    if (index >= MAX_MULTI_STATE_OUTPUTS)
        return false;
    if (priority < 1 || priority > BACNET_MAX_PRIORITY)
        return false;
    if (value != MSO_LEVEL_NULL &&
        (value < 1 || value > MSO_Descr[index].Number_Of_States))
        return false;
    MSO_Descr[index].Priority_Array[priority - 1] = value;
    return true;
}

bool Multi_State_Output_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32];
    if (object_instance < MAX_MULTI_STATE_OUTPUTS) {
        sprintf(text_string, "MULTI-STATE OUTPUT %lu",
            (unsigned long)object_instance);
        return characterstring_init_ansi(object_name, text_string);
    }
    return false;
}

int Multi_State_Output_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    int len = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index;
    unsigned i;
    uint8_t *apdu;
    MULTI_STATE_OUTPUT_DESCR *CurrentMSO;

    if (!rpdata || !rpdata->application_data || !rpdata->application_data_len)
        return 0;

    apdu = rpdata->application_data;
    object_index = Multi_State_Output_Instance_To_Index(rpdata->object_instance);
    if (object_index >= MAX_MULTI_STATE_OUTPUTS)
        return BACNET_STATUS_ERROR;
    CurrentMSO = &MSO_Descr[object_index];

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(&apdu[0],
                OBJECT_MULTI_STATE_OUTPUT, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Multi_State_Output_Object_Name(rpdata->object_instance, &char_string);
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0],
                OBJECT_MULTI_STATE_OUTPUT);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_unsigned(&apdu[0],
                Multi_State_Output_Present_Value(rpdata->object_instance));
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                CurrentMSO->Out_Of_Service);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(&apdu[0],
                EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0],
                CurrentMSO->Out_Of_Service);
            break;
        case PROP_NUMBER_OF_STATES:
            apdu_len = encode_application_unsigned(&apdu[0],
                CurrentMSO->Number_Of_States);
            break;
        case PROP_PRIORITY_ARRAY:
            if (rpdata->array_index == 0) {
                apdu_len = encode_application_unsigned(&apdu[0],
                    BACNET_MAX_PRIORITY);
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                    if (CurrentMSO->Priority_Array[i] == MSO_LEVEL_NULL)
                        len = encode_application_null(&apdu[apdu_len]);
                    else
                        len = encode_application_unsigned(&apdu[apdu_len],
                            CurrentMSO->Priority_Array[i]);
                    if ((apdu_len + len) < MAX_APDU)
                        apdu_len += len;
                    else {
                        rpdata->error_code =
                            ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                        apdu_len = BACNET_STATUS_ABORT;
                        break;
                    }
                }
            } else if (rpdata->array_index <= BACNET_MAX_PRIORITY) {
                i = rpdata->array_index - 1;
                if (CurrentMSO->Priority_Array[i] == MSO_LEVEL_NULL)
                    apdu_len = encode_application_null(&apdu[apdu_len]);
                else
                    apdu_len = encode_application_unsigned(&apdu[apdu_len],
                        CurrentMSO->Priority_Array[i]);
            } else {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
        case PROP_RELINQUISH_DEFAULT:
            apdu_len = encode_application_unsigned(&apdu[0],
                CurrentMSO->Relinquish_Default);
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

bool Multi_State_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false;
    int len;
    unsigned object_index;
    unsigned priority;
    BACNET_APPLICATION_DATA_VALUE value;
    MULTI_STATE_OUTPUT_DESCR *CurrentMSO;

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
    object_index = Multi_State_Output_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_MULTI_STATE_OUTPUTS)
        return false;
    CurrentMSO = &MSO_Descr[object_index];

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            priority = wp_data->priority;
            if (priority && priority <= BACNET_MAX_PRIORITY && priority != 6) {
                if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                    if (value.type.Unsigned_Int >= 1 &&
                        value.type.Unsigned_Int <= CurrentMSO->Number_Of_States) {
                        CurrentMSO->Priority_Array[priority - 1] =
                            value.type.Unsigned_Int;
                        status = true;
                    } else {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                } else if (value.tag == BACNET_APPLICATION_TAG_NULL) {
                    CurrentMSO->Priority_Array[priority - 1] = MSO_LEVEL_NULL;
                    status = true;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else if (priority == 6) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentMSO->Out_Of_Service = value.type.Boolean;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }
    return status;
}

void Multi_State_Output_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
