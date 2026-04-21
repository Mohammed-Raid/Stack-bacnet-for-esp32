/* Integer Value Object */
#ifndef IV_H
#define IV_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "wp.h"
#include "rp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_INTEGER_VALUES
#define MAX_INTEGER_VALUES 4
#endif

typedef struct integer_value_descr {
    bool Out_Of_Service;
    unsigned Event_State:3;
    int32_t Present_Value;
    uint16_t Units;
} INTEGER_VALUE_DESCR;

void Integer_Value_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);
void Integer_Value_Init(void);
bool Integer_Value_Valid_Instance(uint32_t object_instance);
unsigned Integer_Value_Count(void);
uint32_t Integer_Value_Index_To_Instance(unsigned index);
unsigned Integer_Value_Instance_To_Index(uint32_t object_instance);
bool Integer_Value_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name);
int Integer_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
bool Integer_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
int32_t Integer_Value_Present_Value(uint32_t object_instance);
bool Integer_Value_Present_Value_Set(
    uint32_t object_instance,
    int32_t value);
void Integer_Value_Intrinsic_Reporting(uint32_t object_instance);

#ifdef __cplusplus
}
#endif
#endif
