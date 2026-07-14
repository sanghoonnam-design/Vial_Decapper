
#include "wizchip_conf.h"

wiz_NetInfo g_netinfo = {
    .mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},
    .ip = {192, 168, 0, 150},
    .sn = {255, 255, 255, 0},
    .gw = {192, 168, 0, 1},
    .dns = {8, 8, 8, 8},
    .dhcp = NETINFO_STATIC
};


void net_init(void) {
    uint8_t txsize[8] = {8,8,8,8,8,8,8,8};
    uint8_t rxsize[8] = {8,8,8,8,8,8,8,8};

    wizchip_init(txsize, txsize);
    wizchip_setnetinfo(&g_netinfo);
}