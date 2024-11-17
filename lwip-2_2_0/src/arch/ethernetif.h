#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__
 
 
#include "lwip/err.h"
#include "lwip/netif.h"
 
 
#define ETH_RST_GPIO_Port		GPIOC
#define ETH_RST_Pin					GPIO_PIN_0
 
/* Exported types ------------------------------------------------------------*/
err_t ethernetif_init(struct netif *netif);
void ethernetif_input(struct netif *netif);
void ethernet_link_check_state(struct netif *netif);
#endif