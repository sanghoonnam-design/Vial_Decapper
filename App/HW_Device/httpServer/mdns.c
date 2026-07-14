#include "mdns.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>

/*
    WIZnet ioLibrary 헤더

    프로젝트 구조에 맞게 include 경로를 수정해야 할 수 있음.
*/
#include "socket.h"
#include "w5300.h"

#define MDNS_PORT 5353

#define DNS_TYPE_A 0x0001
#define DNS_TYPE_AAAA 0x001C
#define DNS_TYPE_ANY 0x00FF

#define DNS_CLASS_IN 0x0001
#define DNS_CLASS_IN_FLUSH 0x8001

#define MDNS_TTL_SECONDS 10

#define MDNS_MULTICAST_IP_0 224
#define MDNS_MULTICAST_IP_1 0
#define MDNS_MULTICAST_IP_2 0
#define MDNS_MULTICAST_IP_3 251

typedef struct
{
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} DnsHeader_t;

static MdnsConfig_t g_mdns;
static bool g_mdns_ready = false;

static uint8_t g_mdns_rx_buf[MDNS_PACKET_MAX_SIZE];
static uint8_t g_mdns_tx_buf[MDNS_PACKET_MAX_SIZE];

static const uint8_t g_mdns_multicast_ip[4] =
    {
        MDNS_MULTICAST_IP_0,
        MDNS_MULTICAST_IP_1,
        MDNS_MULTICAST_IP_2,
        MDNS_MULTICAST_IP_3};

static uint16_t ReadU16(const uint8_t *buf, uint16_t index)
{
    return ((uint16_t)buf[index] << 8) | buf[index + 1];
}

static void WriteU16(uint8_t *buf, uint16_t *index, uint16_t value)
{
    buf[(*index)++] = (uint8_t)(value >> 8);
    buf[(*index)++] = (uint8_t)(value & 0xFF);
}

static void WriteU32(uint8_t *buf, uint16_t *index, uint32_t value)
{
    buf[(*index)++] = (uint8_t)(value >> 24);
    buf[(*index)++] = (uint8_t)(value >> 16);
    buf[(*index)++] = (uint8_t)(value >> 8);
    buf[(*index)++] = (uint8_t)(value & 0xFF);
}

static int StrCaseCmp(const char *a, const char *b)
{
    while (*a && *b)
    {
        char ca = (char)tolower((unsigned char)*a);
        char cb = (char)tolower((unsigned char)*b);

        if (ca != cb)
            return ca - cb;

        a++;
        b++;
    }

    return (unsigned char)*a - (unsigned char)*b;
}

static bool BuildFullName(char *out, uint16_t out_size)
{
    if (out == NULL || out_size == 0)
        return false;

    int ret = snprintf(out, out_size, "%s.local", g_mdns.hostname);

    if (ret <= 0 || ret >= out_size)
        return false;

    return true;
}

static bool WriteDnsName(uint8_t *buf,
                         uint16_t buf_size,
                         uint16_t *index,
                         const char *name)
{
    const char *p = name;

    while (*p)
    {
        const char *label_start = p;
        uint8_t label_len = 0;

        while (*p && *p != '.')
        {
            label_len++;
            p++;
        }

        if (label_len == 0 || label_len > 63)
            return false;

        if ((*index + 1 + label_len) >= buf_size)
            return false;

        buf[(*index)++] = label_len;

        for (uint8_t i = 0; i < label_len; i++)
        {
            buf[(*index)++] = (uint8_t)label_start[i];
        }

        if (*p == '.')
            p++;
    }

    if (*index >= buf_size)
        return false;

    buf[(*index)++] = 0x00;

    return true;
}

static bool ReadDnsNameInternal(const uint8_t *packet,
                                uint16_t packet_len,
                                uint16_t start_index,
                                char *out_name,
                                uint16_t out_size,
                                uint16_t *consumed_len,
                                uint8_t depth)
{
    uint16_t index = start_index;
    uint16_t out_index = 0;

    if (depth > 5)
        return false;

    while (index < packet_len)
    {
        uint8_t len = packet[index++];

        if (len == 0)
        {
            if (out_index > 0 && out_name[out_index - 1] == '.')
                out_index--;

            if (out_index >= out_size)
                return false;

            out_name[out_index] = '\0';

            if (consumed_len != NULL)
                *consumed_len = index - start_index;

            return true;
        }

        /*
            DNS name compression pointer
            11xxxxxx xxxxxxxx
        */
        if ((len & 0xC0) == 0xC0)
        {
            if (index >= packet_len)
                return false;

            uint16_t pointer = ((uint16_t)(len & 0x3F) << 8) | packet[index++];

            if (pointer >= packet_len)
                return false;

            char pointed_name[MDNS_FULLNAME_MAX_LEN];
            uint16_t dummy_consumed = 0;

            if (!ReadDnsNameInternal(packet,
                                     packet_len,
                                     pointer,
                                     pointed_name,
                                     sizeof(pointed_name),
                                     &dummy_consumed,
                                     depth + 1))
            {
                return false;
            }

            uint16_t pointed_len = (uint16_t)strlen(pointed_name);

            if (out_index + pointed_len >= out_size)
                return false;

            memcpy(&out_name[out_index], pointed_name, pointed_len);
            out_index += pointed_len;
            out_name[out_index] = '\0';

            if (consumed_len != NULL)
                *consumed_len = index - start_index;

            return true;
        }

        if ((len & 0xC0) != 0x00)
            return false;

        if (index + len > packet_len)
            return false;

        if (out_index + len + 1 >= out_size)
            return false;

        for (uint8_t i = 0; i < len; i++)
        {
            out_name[out_index++] = (char)packet[index++];
        }

        out_name[out_index++] = '.';
    }

    return false;
}

static bool ReadDnsName(const uint8_t *packet,
                        uint16_t packet_len,
                        uint16_t *index,
                        char *out_name,
                        uint16_t out_size)
{
    uint16_t consumed = 0;

    if (!ReadDnsNameInternal(packet,
                             packet_len,
                             *index,
                             out_name,
                             out_size,
                             &consumed,
                             0))
    {
        return false;
    }

    *index += consumed;
    return true;
}

static bool ParseDnsHeader(const uint8_t *packet,
                           uint16_t packet_len,
                           DnsHeader_t *header)
{
    if (packet == NULL || header == NULL)
        return false;

    if (packet_len < 12)
        return false;

    header->id = ReadU16(packet, 0);
    header->flags = ReadU16(packet, 2);
    header->qdcount = ReadU16(packet, 4);
    header->ancount = ReadU16(packet, 6);
    header->nscount = ReadU16(packet, 8);
    header->arcount = ReadU16(packet, 10);

    return true;
}

static bool IsMdnsQueryForMe(const uint8_t *packet,
                             uint16_t packet_len,
                             uint16_t *out_qtype)
{
    DnsHeader_t header;

    if (!ParseDnsHeader(packet, packet_len, &header))
        return false;

    if (header.qdcount == 0)
        return false;

    uint16_t index = 12;

    char my_full_name[MDNS_FULLNAME_MAX_LEN];

    if (!BuildFullName(my_full_name, sizeof(my_full_name)))
        return false;

    for (uint16_t q = 0; q < header.qdcount; q++)
    {
        char query_name[MDNS_FULLNAME_MAX_LEN];

        if (!ReadDnsName(packet,
                         packet_len,
                         &index,
                         query_name,
                         sizeof(query_name)))
        {
            return false;
        }

        if (index + 4 > packet_len)
            return false;

        uint16_t qtype = ReadU16(packet, index);
        uint16_t qclass = ReadU16(packet, index + 2);
        index += 4;

        if ((qclass & 0x7FFF) != DNS_CLASS_IN)
            continue;

        if (StrCaseCmp(query_name, my_full_name) == 0)
        {
            if (qtype == DNS_TYPE_A || qtype == DNS_TYPE_ANY)
            {
                if (out_qtype != NULL)
                    *out_qtype = qtype;

                return true;
            }
        }
    }

    return false;
}

static uint16_t BuildMdnsAResponse(uint8_t *buf, uint16_t buf_size)
{
    uint16_t index = 0;

    char full_name[MDNS_FULLNAME_MAX_LEN];

    if (!BuildFullName(full_name, sizeof(full_name)))
        return 0;

    if (buf_size < 12)
        return 0;

    /*
        DNS Header
        ID      = 0
        Flags   = 0x8400, response + authoritative answer
        QDCOUNT = 0
        ANCOUNT = 1
    */
    WriteU16(buf, &index, 0x0000);
    WriteU16(buf, &index, 0x8400);
    WriteU16(buf, &index, 0x0000);
    WriteU16(buf, &index, 0x0001);
    WriteU16(buf, &index, 0x0000);
    WriteU16(buf, &index, 0x0000);

    /*
        Answer Section
        NAME     = hostname.local
        TYPE     = A
        CLASS    = IN + cache flush
        TTL      = 120
        RDLENGTH = 4
        RDATA    = IPv4 address
    */
    if (!WriteDnsName(buf, buf_size, &index, full_name))
        return 0;

    if (index + 14 > buf_size)
        return 0;

    WriteU16(buf, &index, DNS_TYPE_A);
    WriteU16(buf, &index, DNS_CLASS_IN_FLUSH);
    WriteU32(buf, &index, MDNS_TTL_SECONDS);
    WriteU16(buf, &index, 4);

    buf[index++] = g_mdns.ip[0];
    buf[index++] = g_mdns.ip[1];
    buf[index++] = g_mdns.ip[2];
    buf[index++] = g_mdns.ip[3];

    return index;
}

MdnsResult_t MDNS_SetHostname(const char *hostname)
{
    if (hostname == NULL)
        return MDNS_ERR_PARAM;

    if (strlen(hostname) == 0)
        return MDNS_ERR_PARAM;

    if (strlen(hostname) >= MDNS_HOSTNAME_MAX_LEN)
        return MDNS_ERR_PARAM;

    /*
        hostname에는 ".local"을 넣지 않는다.
        예: "co2-controller"
    */
    if (strstr(hostname, ".local") != NULL)
        return MDNS_ERR_PARAM;

    memset(g_mdns.hostname, 0, sizeof(g_mdns.hostname));
    strncpy(g_mdns.hostname, hostname, sizeof(g_mdns.hostname) - 1);

    return MDNS_OK;
}

void MDNS_SetIp(const uint8_t ip[4])
{
    if (ip == NULL)
        return;

    memcpy(g_mdns.ip, ip, 4);
}

MdnsResult_t MDNS_Init(const MdnsConfig_t *config)
{
    if (config == NULL)
        return MDNS_ERR_PARAM;

    if (strlen(config->hostname) == 0)
        return MDNS_ERR_PARAM;

    if (strlen(config->hostname) >= MDNS_HOSTNAME_MAX_LEN)
        return MDNS_ERR_PARAM;

    if (strstr(config->hostname, ".local") != NULL)
        return MDNS_ERR_PARAM;

    memset(&g_mdns, 0, sizeof(g_mdns));

    g_mdns.socket_num = config->socket_num;
    strncpy(g_mdns.hostname, config->hostname, sizeof(g_mdns.hostname) - 1);
    memcpy(g_mdns.ip, config->ip, 4);

    /*
        기존 socket 정리
    */
    close(g_mdns.socket_num);

    /*
        W5300 mDNS multicast 수신 설정

        mDNS:
        - Multicast IP : 224.0.0.251
        - UDP Port     : 5353

        W5300에서는 UDP multicast socket으로 열어야 한다.

        일반적인 설정 흐름:
        1. Sn_DIPR   = 224.0.0.251
        2. Sn_DPORTR = 5353
        3. Sn_MR     = UDP | MULTI
        4. OPEN
    */

    setSn_DIPR(g_mdns.socket_num, (uint8_t *)g_mdns_multicast_ip);
    setSn_DPORTR(g_mdns.socket_num, MDNS_PORT);

    /*
        socket(sn, protocol, port, flag)

        사용하는 WIZnet ioLibrary 버전에 따라
        Sn_MR_MULTI를 protocol 인자에 넣는 방식이 다를 수 있음.

        컴파일 에러 발생 시 socket() 함수 원형을 확인해야 한다.
    */
    int8_t ret = socket(g_mdns.socket_num,
                        Sn_MR_UDP | Sn_MR_MULTI,
                        MDNS_PORT,
                        SF_IO_NONBLOCK);

    if (ret != g_mdns.socket_num)
    {
        g_mdns_ready = false;
        return MDNS_ERR_SOCKET;
    }

    g_mdns_ready = true;

    return MDNS_OK;
}

void MDNS_Process(void)
{
    if (!g_mdns_ready)
        return;

    uint8_t remote_ip[4] = {0};
    uint16_t remote_port = 0;

    if (getSn_RX_RSR(g_mdns.socket_num) <= 0)
        return;

    int32_t rx_len = recvfrom(g_mdns.socket_num,
                              g_mdns_rx_buf,
                              sizeof(g_mdns_rx_buf),
                              remote_ip,
                              &remote_port);

    if (rx_len <= 0)
        return;

    if (rx_len > MDNS_PACKET_MAX_SIZE)
        return;

    uint16_t qtype = 0;
    if (!IsMdnsQueryForMe(g_mdns_rx_buf, (uint16_t)rx_len, &qtype))
        return;

    uint16_t tx_len = BuildMdnsAResponse(g_mdns_tx_buf,
                                         sizeof(g_mdns_tx_buf));

    if (tx_len == 0)
        return;

    /*
        mDNS 응답 목적지:
        224.0.0.251:5353
    */
    sendto(g_mdns.socket_num,
           g_mdns_tx_buf,
           tx_len,
           (uint8_t *)g_mdns_multicast_ip,
           MDNS_PORT);
}
