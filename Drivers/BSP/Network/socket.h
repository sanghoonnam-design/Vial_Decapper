//*****************************************************************************
//! \file socket.h
//! \brief SOCKET APIs Header file — W5300 / W5500 only build.
//*****************************************************************************
#ifndef _SOCKET_H_
#define _SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wizchip_conf.h"

#ifndef TCP_MSS
#define TCP_MSS  1460
#endif
#ifndef UDP_MSS
#define UDP_MSS  1472
#endif

#define SOCKET  uint8_t

/* ── Return codes ────────────────────────────────────────────────────────── */
#define SOCK_OK               1
#define SOCK_BUSY             0
#define SOCK_FATAL           -1000

#define SOCK_ERROR            0
#define SOCKERR_SOCKNUM      (SOCK_ERROR - 1)
#define SOCKERR_SOCKOPT      (SOCK_ERROR - 2)
#define SOCKERR_SOCKINIT     (SOCK_ERROR - 3)
#define SOCKERR_SOCKCLOSED   (SOCK_ERROR - 4)
#define SOCKERR_SOCKMODE     (SOCK_ERROR - 5)
#define SOCKERR_SOCKFLAG     (SOCK_ERROR - 6)
#define SOCKERR_SOCKSTATUS   (SOCK_ERROR - 7)
#define SOCKERR_ARG          (SOCK_ERROR - 10)
#define SOCKERR_PORTZERO     (SOCK_ERROR - 11)
#define SOCKERR_IPINVALID    (SOCK_ERROR - 12)
#define SOCKERR_TIMEOUT      (SOCK_ERROR - 13)
#define SOCKERR_DATALEN      (SOCK_ERROR - 14)
#define SOCKERR_BUFFER       (SOCK_ERROR - 15)
#define SOCKFATAL_PACKLEN    (SOCK_FATAL - 1)

/* ── Socket flags ────────────────────────────────────────────────────────── */
#define SF_ETHER_OWN    (Sn_MR_MFEN)
#define SF_IGMP_VER2    (Sn_MR_MC)
#define SF_TCP_NODELAY  (Sn_MR_ND)
#define SF_MULTI_ENABLE (Sn_MR_MULTI)

#define Sn_MR2_DHAM     (1 << 1)
#define SF_DHA_MANUAL   (Sn_MR2_DHAM)
#define Sn_MR2_FARP     (1 << 0)
#define SF_FORCE_ARP    (Sn_MR2_FARP)

#if _WIZCHIP_ == W5500
#define SF_BROAD_BLOCK  (Sn_MR_BCASTB)
#define SF_MULTI_BLOCK  (Sn_MR_MMB)
#define SF_IPv6_BLOCK   (Sn_MR_MIP6B)
#define SF_UNI_BLOCK    (Sn_MR_UCASTB)
#endif

#if _WIZCHIP_ == W5300
#define SF_TCP_ALIGN    0x02
#endif

#define SF_IO_NONBLOCK  0x01

/* ── Packet info flags ───────────────────────────────────────────────────── */
#define PACK_FIRST      0x80
#define PACK_REMAINED   0x01
#define PACK_COMPLETED  0x00
#define PACK_FIFOBYTE   0x02  /* W5300 only */
#define PACK_NONE       0x00

/* ── I/O modes ───────────────────────────────────────────────────────────── */
#define SOCK_IO_BLOCK     0
#define SOCK_IO_NONBLOCK  1

/* ── Socket interrupt kind ───────────────────────────────────────────────── */
typedef enum {
    SIK_CONNECTED    = (1 << 0),
    SIK_DISCONNECTED = (1 << 1),
    SIK_RECEIVED     = (1 << 2),
    SIK_TIMEOUT      = (1 << 3),
    SIK_SENT         = (1 << 4),
    SIK_ALL          = 0x1F,
} sockint_kind;

/* ── ctlsocket type ──────────────────────────────────────────────────────── */
typedef enum {
    CS_SET_IOMODE,
    CS_GET_IOMODE,
    CS_GET_MAXTXBUF,
    CS_GET_MAXRXBUF,
    CS_CLR_INTERRUPT,
    CS_GET_INTERRUPT,
    CS_SET_INTMASK,
    CS_GET_INTMASK,
} ctlsock_type;

/* ── Socket option type ──────────────────────────────────────────────────── */
typedef enum {
    SO_FLAG,
    SO_TTL,
    SO_TOS,
    SO_MSS,
    SO_DESTIP,
    SO_DESTPORT,
    SO_KEEPALIVESEND,
    SO_KEEPALIVEAUTO,
    SO_SENDBUF,
    SO_RECVBUF,
    SO_STATUS,
    SO_REMAINSIZE,
    SO_PACKINFO,
} sockopt_type;

/* ── API ─────────────────────────────────────────────────────────────────── */
int8_t  socket(uint8_t sn, uint8_t protocol, uint16_t port, uint8_t flag);
int8_t  close(uint8_t sn);
int8_t  listen(uint8_t sn);
int8_t  disconnect(uint8_t sn);
int32_t send(uint8_t sn, uint8_t *buf, uint16_t len);
int32_t recv(uint8_t sn, uint8_t *buf, uint16_t len);
int8_t  ctlsocket(uint8_t sn, ctlsock_type cstype, void *arg);
int8_t  setsockopt(uint8_t sn, sockopt_type sotype, void *arg);
int8_t  getsockopt(uint8_t sn, sockopt_type sotype, void *arg);

/* W5x00 — IPv4 only variants (only 5-arg sendto/recvfrom needed) */
int8_t  connect_W5x00(uint8_t sn, uint8_t *addr, uint16_t port);
int32_t sendto_W5x00(uint8_t sn, uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t port);
int32_t recvfrom_W5x00(uint8_t sn, uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t *port);

#define connect(sn, addr, port)                    connect_W5x00(sn, addr, port)
#define sendto(sn, buf, len, addr, port)           sendto_W5x00(sn, buf, len, addr, port)
#define recvfrom(sn, buf, len, addr, port)         recvfrom_W5x00(sn, buf, len, addr, port)

#ifdef __cplusplus
}
#endif

#endif /* _SOCKET_H_ */
