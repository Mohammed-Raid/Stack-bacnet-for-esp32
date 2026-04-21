/* Network Port Objects */

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
#include "np.h"
#include "handlers.h"

static NETWORK_PORT_DESCR NP_Descr[MAX_NETWORK_PORTS];

static const int Network_Port_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_STATUS_FLAGS,
    PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE,
    PROP_NETWORK_NUMBER,
    PROP_CHANGES_PENDING,
    PROP_APDU_LENGTH,
    PROP_LINK_SPEED,
    -1
};

static const int Network_Port_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_MAC_ADDRESS,
    PROP_IP_ADDRESS,
    PROP_IP_DEFAULT_GATEWAY,
    PROP_IP_SUBNET_MASK,
    PROP_IP_DNS_SERVER,
    PROP_BACNET_IP_UDP_PORT,
    -1
};

static const int Network_Port_Properties_Proprietary[] = {
    -1
};

void Network_Port_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Network_Port_Properties_Required;
    if (pOptional)
        *pOptional = Network_Port_Properties_Optional;
    if (pProprietary)
        *pProprietary = Network_Port_Properties_Proprietary;
}

void Network_Port_Init(void)
{
    unsigned i;
    for (i = 0; i < MAX_NETWORK_PORTS; i++) {
        memset(&NP_Descr[i], 0, sizeof(NETWORK_PORT_DESCR));
        NP_Descr[i].BACnet_IP_UDP_Port = 0xBAC0; /* 47808 */
        NP_Descr[i].Network_Number = 0;
    }
}

bool Network_Port_Valid_Instance(uint32_t object_instance)
{
    return object_instance < MAX_NETWORK_PORTS;
}

unsigned Network_Port_Count(void)
{
    return MAX_NETWORK_PORTS;
}

uint32_t Network_Port_Index_To_Instance(unsigned index)
{
    return index;
}

unsigned Network_Port_Instance_To_Index(uint32_t object_instance)
{
    if (object_instance < MAX_NETWORK_PORTS)
        return object_instance;
    return MAX_NETWORK_PORTS;
}

bool Network_Port_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32];
    if (object_instance < MAX_NETWORK_PORTS) {
        sprintf(text_string, "NETWORK PORT %lu",
            (unsigned long)object_instance);
        return characterstring_init_ansi(object_name, text_string);
    }
    return false;
}

int Network_Port_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    int len = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_OCTET_STRING octet_string;
    unsigned object_index;
    uint8_t *apdu;
    NETWORK_PORT_DESCR *CurrentNP;

    if (!rpdata || !rpdata->application_data || !rpdata->application_data_len)
        return 0;

    apdu = rpdata->application_data;
    object_index = Network_Port_Instance_To_Index(rpdata->object_instance);
    if (object_index >= MAX_NETWORK_PORTS)
        return BACNET_STATUS_ERROR;
    CurrentNP = &NP_Descr[object_index];

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(&apdu[0],
                OBJECT_NETWORK_PORT, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Network_Port_Object_Name(rpdata->object_instance, &char_string);
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_NETWORK_PORT);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                CurrentNP->Out_Of_Network_Port);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(&apdu[0],
                RELIABILITY_NO_FAULT_DETECTED);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0],
                CurrentNP->Out_Of_Network_Port);
            break;
        case PROP_NETWORK_NUMBER:
            apdu_len = encode_application_unsigned(&apdu[0],
                CurrentNP->Network_Number);
            break;
        case PROP_CHANGES_PENDING:
            apdu_len = encode_application_boolean(&apdu[0],
                CurrentNP->Changes_Pending);
            break;
        case PROP_APDU_LENGTH:
            apdu_len = encode_application_unsigned(&apdu[0], MAX_APDU);
            break;
        case PROP_LINK_SPEED:
            /* report 100 Mbps */
            apdu_len = encode_application_real(&apdu[0], 100000000.0f);
            break;
        case PROP_MAC_ADDRESS:
            /* MAC: IP:port (6 bytes for BACnet/IP) */
            {
                uint8_t mac[6];
                memcpy(mac, CurrentNP->IP_Address, 4);
                mac[4] = (uint8_t)(CurrentNP->BACnet_IP_UDP_Port >> 8);
                mac[5] = (uint8_t)(CurrentNP->BACnet_IP_UDP_Port & 0xFF);
                octetstring_init(&octet_string, mac, 6);
                apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            }
            break;
        case PROP_IP_ADDRESS:
            octetstring_init(&octet_string, CurrentNP->IP_Address, 4);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_IP_DEFAULT_GATEWAY:
            octetstring_init(&octet_string, CurrentNP->IP_Default_Gateway, 4);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_IP_SUBNET_MASK:
            octetstring_init(&octet_string, CurrentNP->IP_Subnet_Mask, 4);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_IP_DNS_SERVER:
            /* array: element 0 = count, ARRAY_ALL = all elements */
            if (rpdata->array_index == 0) {
                apdu_len = encode_application_unsigned(&apdu[0], 1);
            } else if (rpdata->array_index == BACNET_ARRAY_ALL ||
                       rpdata->array_index == 1) {
                octetstring_init(&octet_string, CurrentNP->IP_DNS_Server, 4);
                len = encode_application_octet_string(&apdu[apdu_len], &octet_string);
                apdu_len += len;
            } else {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
        case PROP_BACNET_IP_UDP_PORT:
            apdu_len = encode_application_unsigned(&apdu[0],
                CurrentNP->BACnet_IP_UDP_Port);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    if ((apdu_len >= 0) &&
        (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->object_property != PROP_IP_DNS_SERVER) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

bool Network_Port_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false;
    int len;
    unsigned object_index;
    BACNET_APPLICATION_DATA_VALUE value;
    NETWORK_PORT_DESCR *CurrentNP;

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
    object_index = Network_Port_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_NETWORK_PORTS)
        return false;
    CurrentNP = &NP_Descr[object_index];

    switch (wp_data->object_property) {
        case PROP_BACNET_IP_UDP_PORT:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                CurrentNP->BACnet_IP_UDP_Port =
                    (uint16_t)value.type.Unsigned_Int;
                CurrentNP->Changes_Pending = true;
            }
            break;
        case PROP_NETWORK_NUMBER:
            status = WPValidateArgType(&value,
                BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                CurrentNP->Network_Number =
                    (uint16_t)value.type.Unsigned_Int;
                CurrentNP->Changes_Pending = true;
            }
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }
    return status;
}

void Network_Port_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}

void Network_Port_IP_Set(
    uint32_t object_instance,
    const uint8_t *ip_address,
    const uint8_t *subnet_mask,
    const uint8_t *gateway,
    uint16_t udp_port)
{
    unsigned index = Network_Port_Instance_To_Index(object_instance);
    if (index >= MAX_NETWORK_PORTS)
        return;
    if (ip_address)
        memcpy(NP_Descr[index].IP_Address, ip_address, 4);
    if (subnet_mask)
        memcpy(NP_Descr[index].IP_Subnet_Mask, subnet_mask, 4);
    if (gateway)
        memcpy(NP_Descr[index].IP_Default_Gateway, gateway, 4);
    if (udp_port)
        NP_Descr[index].BACnet_IP_UDP_Port = udp_port;
}
