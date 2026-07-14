#ifndef MDNS_H
#define MDNS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MDNS_HOSTNAME_MAX_LEN 32
#define MDNS_FULLNAME_MAX_LEN 64
#define MDNS_PACKET_MAX_SIZE 512

    typedef enum
    {
        MDNS_OK = 0,
        MDNS_ERR_PARAM,
        MDNS_ERR_SOCKET,
        MDNS_ERR_NOT_READY
    } MdnsResult_t;

#pragma pack(push, 1)
    typedef struct
    {
        uint8_t socket_num;                   // W5300 socket number
        char hostname[MDNS_HOSTNAME_MAX_LEN]; // 예: "co2-controller", ".local" 제외
        uint8_t ip[4];                        // 장비 IPv4 주소
    } MdnsConfig_t;
#pragma pack(pop)

    /*
        mDNS 초기화

        예:
        MdnsConfig_t cfg =
        {
            .socket_num = 0,
            .hostname = "co2-controller",
            .ip = {192, 168, 0, 55}
        };

        MDNS_Init(&cfg);
    */
    MdnsResult_t MDNS_Init(const MdnsConfig_t *config);

    /*
        메인 루프 또는 RTOS Task에서 주기적으로 호출
    */
    void MDNS_Process(void);

    /*
        DHCP 등으로 IP가 변경되었을 때 호출
    */
    void MDNS_SetIp(const uint8_t ip[4]);

    /*
        hostname 변경 시 호출
        hostname에는 ".local"을 붙이지 않는다.
    */
    MdnsResult_t MDNS_SetHostname(const char *hostname);

#ifdef __cplusplus
}
#endif

#endif
