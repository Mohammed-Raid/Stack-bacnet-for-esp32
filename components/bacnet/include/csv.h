/* CharacterString Value Object */
#ifndef CSV_H
#define CSV_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "bacstr.h"
#include "wp.h"
#include "rp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_CHARACTERSTRING_VALUES
#define MAX_CHARACTERSTRING_VALUES 4
#endif

#ifndef MAX_CSV_STRING_LEN
#define MAX_CSV_STRING_LEN 64
#endif

typedef struct characterstring_value_descr {
    bool Out_Of_Service;
    unsigned Event_State:3;
    char Present_Value[MAX_CSV_STRING_LEN];
} CHARACTERSTRING_VALUE_DESCR;

void CharacterString_Value_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);
void CharacterString_Value_Init(void);
bool CharacterString_Value_Valid_Instance(uint32_t object_instance);
unsigned CharacterString_Value_Count(void);
uint32_t CharacterString_Value_Index_To_Instance(unsigned index);
unsigned CharacterString_Value_Instance_To_Index(uint32_t object_instance);
bool CharacterString_Value_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name);
int CharacterString_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
bool CharacterString_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
bool CharacterString_Value_Present_Value(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *value);
bool CharacterString_Value_Present_Value_Set(
    uint32_t object_instance,
    const char *value);
void CharacterString_Value_Intrinsic_Reporting(uint32_t object_instance);

#ifdef __cplusplus
}
#endif
#endif
