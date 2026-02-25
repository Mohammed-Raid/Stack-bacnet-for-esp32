/* main/ethernet.h */
#ifndef ETHERNET_H
#define ETHERNET_H

#include "esp_err.h"

// Initialise l'Ethernet (SPI + W5500 + Stack IP)
esp_err_t ethernet_init(void);

// Récupère l'adresse MAC (utile pour l'ID BACnet)
void ethernet_get_mac(uint8_t *mac_buff);

#endif // ETHERNET_H