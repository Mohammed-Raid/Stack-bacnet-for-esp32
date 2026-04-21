/* Multi-state Value Object */
#ifndef MSV_H
#define MSV_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "wp.h"
#include "rp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_MULTI_STATE_VALUES
#define MAX_MULTI_STATE_VALUES 4
#endif

/* NULL sentinel for priority array slots (0 means no value commanded) */
#define MSV_LEVEL_NULL 0

typedef struct multi_state_value_descr {
    bool Out_Of_Service;
    unsigned Event_State:3;
    uint32_t Priority_Array[BACNET_MAX_PRIORITY]; /* 1-based; 0 = NULL */
    uint32_t Relinquish_Default;
    uint32_t Number_Of_States;
} MULTI_STATE_VALUE_DESCR;

void Multi_State_Value_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);
void Multi_State_Value_Init(void);
bool Multi_State_Value_Valid_Instance(uint32_t object_instance);
unsigned Multi_State_Value_Count(void);
uint32_t Multi_State_Value_Index_To_Instance(unsigned index);
unsigned Multi_State_Value_Instance_To_Index(uint32_t object_instance);
bool Multi_State_Value_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name);
int Multi_State_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
bool Multi_State_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
uint32_t Multi_State_Value_Present_Value(uint32_t object_instance);
bool Multi_State_Value_Present_Value_Set(
    uint32_t object_instance,
    uint32_t value,
    uint8_t priority);
void Multi_State_Value_Intrinsic_Reporting(uint32_t object_instance);

#ifdef __cplusplus
}
#endif
#endif
