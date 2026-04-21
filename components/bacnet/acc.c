/* Accumulator Objects */

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
#include "acc.h"
#include "handlers.h"
#include "device.h"

static ACCUMULATOR_DESCR ACC_Descr[MAX_ACCUMULATORS];

static const int Accumulator_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_SCALE,
    PROP_UNITS,
    PROP_MAX_PRES_VALUE,
    PROP_VALUE_CHANGE_TIME,
    -1
};

static const int Accumulator_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_PRESCALE,
    -1
};

static const int Accumulator_Properties_Proprietary[] = {
    -1
};

void Accumulator_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Accumulator_Properties_Required;
    if (pOptional)
        *pOptional = Accumulator_Properties_Optional;
    if (pProprietary)
        *pProprietary = Accumulator_Properties_Proprietary;
}

void Accumulator_Init(void)
{
    unsigned i;
    for (i = 0; i < MAX_ACCUMULATORS; i++) {
        memset(&ACC_Descr[i], 0, sizeof(ACCUMULATOR_DESCR));
        ACC_Descr[i].Units = UNITS_NO_UNITS;
        ACC_Descr[i].Max_Pres_Value = 0xFFFFFFFFUL;
        ACC_Descr[i].Scale_Integer_Exponent = 0;
        ACC_Descr[i].Prescale_Multiplier = 1;
        ACC_Descr[i].Prescale_Modulo_Divide = 1;
        ACC_Descr[i].Event_State = EVENT_STATE_NORMAL;
    }
}

bool Accumulator_Valid_Instance(uint32_t object_instance)
{
    return object_instance < MAX_ACCUMULATORS;
}

unsigned Accumulator_Count(void)
{
    return MAX_ACCUMULATORS;
}

uint32_t Accumulator_Index_To_Instance(unsigned index)
{
    return index;
}

unsigned Accumulator_Instance_To_Index(uint32_t object_instance)
{
    if (object_instance < MAX_ACCUMULATORS)
        return object_instance;
    return MAX_ACCUMULATORS;
}

uint32_t Accumulator_Present_Value(uint32_t object_instance)
{
    unsigned index = Accumulator_Instance_To_Index(object_instance);
    if (index < MAX_ACCUMULATORS)
        return ACC_Descr[index].Present_Value;
    return 0;
}

bool Accumulator_Present_Value_Increment(
    uint32_t object_instance,
    uint32_t pulses)
{
    unsigned index = Accumulator_Instance_To_Index(object_instance);
    ACCUMULATOR_DESCR *acc;
    if (index >= MAX_ACCUMULATORS)
        return false;
    acc = &ACC_Descr[index];
    if (acc->Max_Pres_Value - acc->Present_Value < pulses)
        acc->Present_Value = acc->Max_Pres_Value;
    else
        acc->Present_Value += pulses;
    Device_getCurrentDateTime(&acc->Value_Change_Time);
    return true;
}

bool Accumulator_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32];
    if (object_instance < MAX_ACCUMULATORS) {
        sprintf(text_string, "ACCUMULATOR %lu",
            (unsigned long)object_instance);
        return characterstring_init_ansi(object_name, text_string);
    }
    return false;
}

int Accumulator_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index;
    uint8_t *apdu;
    ACCUMULATOR_DESCR *CurrentACC;

    if (!rpdata || !rpdata->application_data || !rpdata->application_data_len)
        return 0;

    apdu = rpdata->application_data;
    object_index = Accumulator_Instance_To_Index(rpdata->object_instance);
    if (object_index >= MAX_ACCUMULATORS)
        return BACNET_STATUS_ERROR;
    CurrentACC = &ACC_Descr[object_index];

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(&apdu[0],
                OBJECT_ACCUMULATOR, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Accumulator_Object_Name(rpdata->object_instance, &char_string);
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_ACCUMULATOR);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_unsigned(&apdu[0],
                CurrentACC->Present_Value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                CurrentACC->Out_Of_Service);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(&apdu[0],
                EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0],
                CurrentACC->Out_Of_Service);
            break;
        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0],
                CurrentACC->Units);
            break;
        case PROP_SCALE:
            /* SCALE is a CHOICE of integer-scale or real-scale.
               Encode as context-tagged integer (choice 0) */
            apdu_len = encode_context_signed(&apdu[0], 0,
                CurrentACC->Scale_Integer_Exponent);
            break;
        case PROP_PRESCALE:
            /* PRESCALE: multiplier [0], modulo-divide [1] */
            apdu_len  = encode_context_unsigned(&apdu[0], 0,
                CurrentACC->Prescale_Multiplier);
            apdu_len += encode_context_unsigned(&apdu[apdu_len], 1,
                CurrentACC->Prescale_Modulo_Divide);
            break;
        case PROP_MAX_PRES_VALUE:
            apdu_len = encode_application_unsigned(&apdu[0],
                CurrentACC->Max_Pres_Value);
            break;
        case PROP_VALUE_CHANGE_TIME:
            apdu_len = encode_application_date(&apdu[0],
                &CurrentACC->Value_Change_Time.date);
            apdu_len += encode_application_time(&apdu[apdu_len],
                &CurrentACC->Value_Change_Time.time);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    if ((apdu_len >= 0) &&
        (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->object_property != PROP_PRESCALE) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

bool Accumulator_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false;
    int len;
    unsigned object_index;
    BACNET_APPLICATION_DATA_VALUE value;
    ACCUMULATOR_DESCR *CurrentACC;

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
    object_index = Accumulator_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_ACCUMULATORS)
        return false;
    CurrentACC = &ACC_Descr[object_index];

    switch (wp_data->object_property) {
        case PROP_OUT_OF_SERVICE:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentACC->Out_Of_Service = value.type.Boolean;
            break;
        case PROP_UNITS:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_ENUMERATED,
                &wp_data->error_class, &wp_data->error_code);
            if (status)
                CurrentACC->Units = value.type.Enumerated;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }
    return status;
}

void Accumulator_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
