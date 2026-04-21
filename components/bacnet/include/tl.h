/* Trend Log Object */
#ifndef TL_H
#define TL_H

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

#ifndef MAX_TREND_LOGS
#define MAX_TREND_LOGS 2
#endif

#ifndef TREND_LOG_BUFFER_SIZE
#define TREND_LOG_BUFFER_SIZE 128
#endif

typedef struct trend_log_record {
    BACNET_DATE_TIME timestamp;
    float value;
    uint8_t status;       /* BACnet log status flags */
} TREND_LOG_RECORD;

typedef struct trend_log_descr {
    bool Enable;
    bool Out_Of_Service;
    unsigned Event_State:3;
    uint32_t Buffer_Size;
    uint32_t Record_Count;
    uint32_t Total_Record_Count;
    BACNET_DATE_TIME Start_Time;
    BACNET_DATE_TIME Stop_Time;
    uint32_t Log_Interval;         /* hundredths of seconds */
    TREND_LOG_RECORD Buffer[TREND_LOG_BUFFER_SIZE];
    uint32_t head;                 /* index of oldest record */
} TREND_LOG_DESCR;

void Trend_Log_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);
void Trend_Log_Init(void);
bool Trend_Log_Valid_Instance(uint32_t object_instance);
unsigned Trend_Log_Count(void);
uint32_t Trend_Log_Index_To_Instance(unsigned index);
unsigned Trend_Log_Instance_To_Index(uint32_t object_instance);
bool Trend_Log_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name);
int Trend_Log_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
bool Trend_Log_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
bool Trend_Log_Add_Record(uint32_t object_instance, float value);
void Trend_Log_Intrinsic_Reporting(uint32_t object_instance);

#ifdef __cplusplus
}
#endif
#endif
