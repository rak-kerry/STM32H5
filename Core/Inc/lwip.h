#ifndef __LWIP_H_
#define __LWIP_H_

#include "main.h"
#include "lwip/opt.h"
#include "lwip/init.h"
#include "netif/etharp.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif
#include "ethernetif.h"
#include "app_ethernet.h"
#include "lwip/tcp.h"
#include "lan8742.h"

/* LAN8742 nRST */
#define ETH_RST_GPIO_Port		GPIOA
#define ETH_RST_Pin				GPIO_PIN_3

/*设置的IP*/
#define IP_ADDR0   ((uint8_t) 192U)    /*换为自己需要配置的IP地址*/
#define IP_ADDR1   ((uint8_t) 168U)
#define IP_ADDR2   ((uint8_t) 1U)
#define IP_ADDR3   ((uint8_t) 20U)

/*子网掩码*/
#define NETMASK_ADDR0   ((uint8_t) 255U) /*换为自己需要配置的子网掩码*/
#define NETMASK_ADDR1   ((uint8_t) 255U)
#define NETMASK_ADDR2   ((uint8_t) 255U)
#define NETMASK_ADDR3   ((uint8_t) 0U)

/*网关地址*/
#define GW_ADDR0   ((uint8_t) 192U) /*换为自己需要配置的网关地址*/
#define GW_ADDR1   ((uint8_t) 168U)
#define GW_ADDR2   ((uint8_t) 1U)
#define GW_ADDR3   ((uint8_t) 254U)

void MX_LWIP_Init(void);
void Reset_network(void);
void MX_LWIP_Process(void);

#endif
