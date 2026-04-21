/* Large Analog Value Object (double precision) */
#ifndef LAV_H
#define LAV_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "wp.h"
#include "rp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_LARGE_ANALOG_VALUES
#define MAX_LARGE_ANALOG_VALUES 4
#endif

typedef struct large_analog_value_descr {
    bool Out_Of_Service;
    unsigned Event_State:3;
    double Present_Value;
    uint16_t Units;
} LARGE_ANALOG_VALUE_DESCR;

void Large_Analog_Value_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);
void Large_Analog_Value_Init(void);
bool Large_Analog_Value_Valid_Instance(uint32_t object_instance);
unsigned Large_Analog_Value_Count(void);
uint32_t Large_Analog_Value_Index_To_Instance(unsigned index);
unsigned Large_Analog_Value_Instance_To_Index(uint32_t object_instance);
bool Large_Analog_Value_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name);
int Large_Analog_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
bool Large_Analog_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
double Large_Analog_Value_Present_Value(uint32_t object_instance);
bool Large_Analog_Value_Present_Value_Set(
    uint32_t object_instance,
    double value);
void Large_Analog_Value_Intrinsic_Reporting(uint32_t object_instance);

#ifdef __cplusplus
}
#endif
#endif
