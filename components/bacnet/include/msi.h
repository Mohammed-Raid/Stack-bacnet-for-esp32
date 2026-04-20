/* Multi-state Input Object */
#ifndef MSI_H
#define MSI_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "wp.h"
#include "rp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_MULTI_STATE_INPUTS
#define MAX_MULTI_STATE_INPUTS 4
#endif

typedef struct multi_state_input_descr {
    bool Out_Of_Service;
    unsigned Event_State:3;
    uint32_t Present_Value;      /* 1-based state index */
    uint32_t Number_Of_States;
} MULTI_STATE_INPUT_DESCR;

void Multi_State_Input_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);
void Multi_State_Input_Init(void);
bool Multi_State_Input_Valid_Instance(uint32_t object_instance);
unsigned Multi_State_Input_Count(void);
uint32_t Multi_State_Input_Index_To_Instance(unsigned index);
unsigned Multi_State_Input_Instance_To_Index(uint32_t object_instance);
bool Multi_State_Input_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name);
int Multi_State_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
bool Multi_State_Input_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
uint32_t Multi_State_Input_Present_Value(uint32_t object_instance);
bool Multi_State_Input_Present_Value_Set(
    uint32_t object_instance,
    uint32_t value);
void Multi_State_Input_Intrinsic_Reporting(uint32_t object_instance);

#ifdef __cplusplus
}
#endif
#endif
