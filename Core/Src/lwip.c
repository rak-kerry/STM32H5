#include "lwip.h"

uint8_t flag_netif = 0;
struct netif gnetif;
ip4_addr_t ipaddr;
uint8_t lwip_flag;
ip4_addr_t netmask;
ip4_addr_t gw;
extern ETH_HandleTypeDef EthHandle;
uint32_t pRegValue;

void MX_LWIP_Init(void)
{

  /* Ethernet PHY RESET, PA3 for HiWin 3172 RX board */
  printf("Reset LAN8742 PHY...\r\n");
  HAL_GPIO_WritePin(ETH_RST_GPIO_Port,ETH_RST_Pin,GPIO_PIN_RESET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(ETH_RST_GPIO_Port,ETH_RST_Pin,GPIO_PIN_SET);
  HAL_Delay(100);

  /* Initialize the LwIP stack without RTOS */
  printf("Initialize LwIP stack ... \r\n");
  lwip_init();

  /* IP addresses initialization without DHCP (IPv4) */
  IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
  IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
  IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
  printf("Set IP address %d:%d:%d:%d\n\r", IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);

  /* add the network interface (IPv4/IPv6) without RTOS */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);
  /* Registers the default network interface */
  netif_set_default(&gnetif);

  if (netif_is_link_up(&gnetif))
  {
	flag_netif = 1;
    /* When the netif is fully configured this function must be called */
    netif_set_up(&gnetif);
    printf("link is active, network interface up \r\n");
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
    printf("link is down, network interface down \r\n");
  }

  /* Set the link callback function, this function is called on change of link status*/
  netif_set_link_callback(&gnetif, ethernet_link_status_updated);
}

/* Ethernet Frame Handler */
void MX_LWIP_Process(void)
{
	lwip_flag = 0;
	ethernetif_input(&gnetif);
	/* Handle timeouts */
	sys_check_timeouts();
}
