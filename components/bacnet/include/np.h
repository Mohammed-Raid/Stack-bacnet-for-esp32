/* Network Port Object */
#ifndef NP_H
#define NP_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "wp.h"
#include "rp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_NETWORK_PORTS
#define MAX_NETWORK_PORTS 1
#endif

typedef struct network_port_descr {
    bool Out_Of_Network_Port;   /* maps to Out_Of_Service concept */
    bool Changes_Pending;
    uint16_t Network_Number;
    uint8_t IP_Address[4];
    uint8_t IP_Subnet_Mask[4];
    uint8_t IP_Default_Gateway[4];
    uint16_t BACnet_IP_UDP_Port;
    uint8_t IP_DNS_Server[4];
} NETWORK_PORT_DESCR;

void Network_Port_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);
void Network_Port_Init(void);
bool Network_Port_Valid_Instance(uint32_t object_instance);
unsigned Network_Port_Count(void);
uint32_t Network_Port_Index_To_Instance(unsigned index);
unsigned Network_Port_Instance_To_Index(uint32_t object_instance);
bool Network_Port_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name);
int Network_Port_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
bool Network_Port_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
void Network_Port_Intrinsic_Reporting(uint32_t object_instance);

#ifdef __cplusplus
}
#endif
#endif
