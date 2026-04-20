/* Accumulator Object */
#ifndef ACC_H
#define ACC_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "bacenum.h"
#include "datetime.h"
#include "wp.h"
#include "rp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_ACCUMULATORS
#define MAX_ACCUMULATORS 4
#endif

typedef struct accumulator_descr {
    bool Out_Of_Service;
    unsigned Event_State:3;
    uint32_t Present_Value;
    uint32_t Max_Pres_Value;
    uint16_t Units;
    /* Scale: integer numerator, use floating-point multiply for value */
    int32_t Scale_Integer_Exponent;  /* value * 10^exponent */
    uint32_t Prescale_Multiplier;
    uint32_t Prescale_Modulo_Divide;
    BACNET_DATE_TIME Value_Change_Time;
} ACCUMULATOR_DESCR;

void Accumulator_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);
void Accumulator_Init(void);
bool Accumulator_Valid_Instance(uint32_t object_instance);
unsigned Accumulator_Count(void);
uint32_t Accumulator_Index_To_Instance(unsigned index);
unsigned Accumulator_Instance_To_Index(uint32_t object_instance);
bool Accumulator_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name);
int Accumulator_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
bool Accumulator_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
uint32_t Accumulator_Present_Value(uint32_t object_instance);
bool Accumulator_Present_Value_Increment(uint32_t object_instance, uint32_t pulses);
void Accumulator_Intrinsic_Reporting(uint32_t object_instance);

#ifdef __cplusplus
}
#endif
#endif
