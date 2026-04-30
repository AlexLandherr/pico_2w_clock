#ifndef WIZNET_ETHERNET_H
#define WIZNET_ETHERNET_H

#include <stdbool.h>

void wiznet_ethernet_init(void);
void wiznet_ethernet_poll(void);
bool wiznet_ethernet_has_ip(void);

#endif
