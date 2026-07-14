//*****************************************************************************
//! \file socket.c
//! \brief SOCKET APIs — W5300 / W5500 only build.
//*****************************************************************************
#include "stm32f7xx_hal.h"
#include "socket.h"

#define SOCK_ANY_PORT_NUM  0xC000

extern uint32_t HAL_GetTick(void);

static uint16_t sock_any_port    = SOCK_ANY_PORT_NUM;
static uint16_t sock_io_mode     = 0;
static uint16_t sock_is_sending  = 0;

static uint16_t sock_remained_size[_WIZCHIP_SOCK_NUM_] = {0,};
uint8_t         sock_pack_info[_WIZCHIP_SOCK_NUM_]     = {0,};

#if _WIZCHIP_ == W5300
uint8_t sock_remained_byte[_WIZCHIP_SOCK_NUM_] = {0,};
#endif

/* ── Internal macros ─────────────────────────────────────────────────────── */
#define CHECK_SOCKNUM() \
    do { if (sn >= _WIZCHIP_SOCK_NUM_) return SOCKERR_SOCKNUM; } while (0)

#define CHECK_SOCKMODE(mode) \
    do { if ((getSn_MR(sn) & 0x0F) != mode) return SOCKERR_SOCKMODE; } while (0)

#define CHECK_TCPMODE() \
    do { if ((getSn_MR(sn) & 0x03) != 0x01) return SOCKERR_SOCKMODE; } while (0)

#define CHECK_SOCKINIT() \
    do { if (getSn_SR(sn) != SOCK_INIT) return SOCKERR_SOCKINIT; } while (0)

#define CHECK_SOCKDATA() \
    do { if (len == 0) return SOCKERR_DATALEN; } while (0)

/* Spin with 100 ms timeout; breaks on hardware stall */
#define WAIT_CR(sn) \
    do { uint32_t _t = HAL_GetTick(); \
         while (getSn_CR(sn)) { if (HAL_GetTick() - _t > 100u) break; } } while (0)

/* ── socket() ────────────────────────────────────────────────────────────── */
int8_t socket(uint8_t sn, uint8_t protocol, uint16_t port, uint8_t flag)
{
    CHECK_SOCKNUM();

    switch (protocol & 0x0F)
    {
    case Sn_MR_TCP:
    {
        uint32_t taddr;
        getSIPR((uint8_t *)&taddr);
        if (taddr == 0) return SOCKERR_SOCKINIT;
        break;
    }
    case Sn_MR_UDP:
    case Sn_MR_MACRAW:
    case Sn_MR_IPRAW:
        break;
    default:
        return SOCKERR_SOCKMODE;
    }

    if ((flag & 0x04) != 0) return SOCKERR_SOCKFLAG;

    if (flag != 0)
    {
        switch (protocol)
        {
        case Sn_MR_TCP:
#if _WIZCHIP_ == W5300
            if ((flag & (SF_TCP_NODELAY | SF_IO_NONBLOCK | SF_TCP_ALIGN)) == 0)
                return SOCKERR_SOCKFLAG;
#else
            if ((flag & (SF_TCP_NODELAY | SF_IO_NONBLOCK)) == 0)
                return SOCKERR_SOCKFLAG;
#endif
            break;
        case Sn_MR_UDP:
            if ((flag & SF_IGMP_VER2) && !(flag & SF_MULTI_ENABLE))
                return SOCKERR_SOCKFLAG;
#if _WIZCHIP_ == W5500
            if ((flag & SF_UNI_BLOCK) && !(flag & SF_MULTI_ENABLE))
                return SOCKERR_SOCKFLAG;
#endif
            break;
        default:
            break;
        }
    }

    close(sn);

#if _WIZCHIP_ == W5300
    setSn_MR(sn, ((uint16_t)(protocol | (flag & 0xF0))) | (((uint16_t)(flag & 0x02)) << 7));
#else
    setSn_MR(sn, protocol | (flag & 0xF0));
#endif

    if (!port)
    {
        port = sock_any_port++;
        if (sock_any_port == 0xFFF0) sock_any_port = SOCK_ANY_PORT_NUM;
    }
    setSn_PORTR(sn, port);
    setSn_CR(sn, Sn_CR_OPEN);
    WAIT_CR(sn);

    sock_io_mode    &= ~(1 << sn);
    sock_io_mode    |=  ((flag & SF_IO_NONBLOCK) << sn);
    sock_is_sending &= ~(1 << sn);
    sock_remained_size[sn] = 0;
    sock_pack_info[sn]     = PACK_COMPLETED;

    {
        uint32_t _t = HAL_GetTick();
        while (getSn_SR(sn) == SOCK_CLOSED)
        {
            if (HAL_GetTick() - _t > 100u) return SOCKERR_SOCKINIT;
        }
    }
    return (int8_t)sn;
}

/* ── close() ─────────────────────────────────────────────────────────────── */
int8_t close(uint8_t sn)
{
    uint32_t start;

    CHECK_SOCKNUM();

#if _WIZCHIP_ == W5300
    /* W5300 erratum: flush TX buffer via a dummy UDP send before closing TCP */
    if (((getSn_MR(sn) & 0x0F) == Sn_MR_TCP) && (getSn_TX_FSR(sn) != getSn_TxMAX(sn)))
    {
        uint8_t destip[4] = {0, 0, 0, 1};
        setSn_MR(sn, Sn_MR_UDP);
        setSn_PORTR(sn, 0x3000);
        setSn_CR(sn, Sn_CR_OPEN);
        WAIT_CR(sn);
        {
            uint32_t _t = HAL_GetTick();
            while (getSn_SR(sn) != SOCK_UDP)
            {
                if (HAL_GetTick() - _t > 100u) break;
            }
        }
        sendto(sn, destip, 1, destip, 0x3000);
    }
#endif

    setSn_CR(sn, Sn_CR_CLOSE);
    start = HAL_GetTick();
    while (getSn_CR(sn)) { if (HAL_GetTick() - start > 10u) break; }

    setSn_IR(sn, 0xFF);
    sock_io_mode    &= ~(1 << sn);
    sock_is_sending &= ~(1 << sn);
    sock_remained_size[sn] = 0;
    sock_pack_info[sn]     = PACK_NONE;

    start = HAL_GetTick();
    while (getSn_SR(sn) != SOCK_CLOSED) { if (HAL_GetTick() - start > 10u) break; }

    return SOCK_OK;
}

/* ── listen() ────────────────────────────────────────────────────────────── */
int8_t listen(uint8_t sn)
{
    CHECK_SOCKNUM();
    CHECK_TCPMODE();
    CHECK_SOCKINIT();

    setSn_CR(sn, Sn_CR_LISTEN);
    WAIT_CR(sn);

    if (getSn_SR(sn) != SOCK_LISTEN)
    {
        close(sn);
        return SOCKERR_SOCKCLOSED;
    }
    return SOCK_OK;
}

/* ── connect (internal) ──────────────────────────────────────────────────── */
static int8_t connect_IO(uint8_t sn, uint8_t *addr, uint16_t port)
{
    uint32_t taddr;

    CHECK_SOCKNUM();
    CHECK_TCPMODE();
    CHECK_SOCKINIT();

    taddr  = ((uint32_t)addr[0] << 24) | ((uint32_t)addr[1] << 16)
           | ((uint32_t)addr[2] <<  8) |  (uint32_t)addr[3];
    if (taddr == 0xFFFFFFFFu || taddr == 0) return SOCKERR_IPINVALID;

    if (port == 0) return SOCKERR_PORTZERO;

    setSn_DPORTR(sn, port);
    setSn_DIPR(sn, addr);
    setSn_CR(sn, Sn_CR_CONNECT);
    WAIT_CR(sn);

    if (sock_io_mode & (1 << sn)) return SOCK_BUSY;

    while (getSn_SR(sn) != SOCK_ESTABLISHED)
    {
        if (getSn_IR(sn) & Sn_IR_TIMEOUT)
        {
            setSn_IR(sn, Sn_IR_TIMEOUT);
            return SOCKERR_TIMEOUT;
        }
        if (getSn_SR(sn) == SOCK_CLOSED)
            return SOCKERR_SOCKCLOSED;
    }
    return SOCK_OK;
}

int8_t connect_W5x00(uint8_t sn, uint8_t *addr, uint16_t port)
{
    return connect_IO(sn, addr, port);
}

/* ── disconnect() ────────────────────────────────────────────────────────── */
static void socket_wait_tx_idle(uint8_t sn)
{
    uint32_t start = HAL_GetTick();
    while (1)
    {
        if (getSn_TX_FSR(sn) == getSn_TxMAX(sn)) break;
        if (HAL_GetTick() - start > 200u)          break;
    }
}

int8_t disconnect(uint8_t sn)
{
    uint32_t start;
    uint8_t  status;

    CHECK_SOCKNUM();
    CHECK_TCPMODE();

    status = getSn_SR(sn);
    if (status == SOCK_CLOSED) return SOCK_OK;

    socket_wait_tx_idle(sn);

    while (getSn_RX_RSR(sn) > 0)
    {
        uint8_t  dump[32];
        uint16_t rlen = (uint16_t)getSn_RX_RSR(sn);
        if (rlen > sizeof(dump)) rlen = (uint16_t)sizeof(dump);
        recv(sn, dump, rlen);
    }

    setSn_CR(sn, Sn_CR_DISCON);
    WAIT_CR(sn);

    start = HAL_GetTick();
    while (1)
    {
        status = getSn_SR(sn);
        if (status == SOCK_CLOSED) return SOCK_OK;

        if (getSn_IR(sn) & Sn_IR_DISCON)
        {
            setSn_IR(sn, Sn_IR_DISCON);
            close(sn);
            return SOCK_OK;
        }
        if (getSn_IR(sn) & Sn_IR_TIMEOUT)
        {
            setSn_IR(sn, Sn_IR_TIMEOUT);
            close(sn);
            return SOCKERR_TIMEOUT;
        }
        if (HAL_GetTick() - start > 10u)
        {
            close(sn);
            return SOCKERR_TIMEOUT;
        }
    }
}

/* ── send() ──────────────────────────────────────────────────────────────── */
int32_t send(uint8_t sn, uint8_t *buf, uint16_t len)
{
    uint8_t  tmp      = 0;
    uint16_t freesize = 0;

    freesize = getSn_TxMAX(sn);
    if (len > freesize) len = freesize;

    while (1)
    {
        freesize = (uint16_t)getSn_TX_FSR(sn);
        tmp      = getSn_SR(sn);
        if ((tmp != SOCK_ESTABLISHED) && (tmp != SOCK_CLOSE_WAIT))
        {
            if (tmp == SOCK_CLOSED) close(sn);
            return SOCKERR_SOCKSTATUS;
        }
        if ((sock_io_mode & (1 << sn)) && (len > freesize)) return SOCK_BUSY;
        if (len <= freesize) break;
    }

    wiz_send_data(sn, buf, len);
    setSn_TX_WRSR(sn, len);

    if (sock_is_sending & (1 << sn))
    {
        while (!(getSn_IR(sn) & Sn_IR_SENDOK))
        {
            tmp = getSn_SR(sn);
            if ((tmp != SOCK_ESTABLISHED) && (tmp != SOCK_CLOSE_WAIT))
            {
                if ((tmp == SOCK_CLOSED) || (getSn_IR(sn) & Sn_IR_TIMEOUT))
                    close(sn);
                return SOCKERR_SOCKSTATUS;
            }
            if (sock_io_mode & (1 << sn)) return SOCK_BUSY;
        }
        setSn_IR(sn, Sn_IR_SENDOK);
    }

    setSn_CR(sn, Sn_CR_SEND);
    WAIT_CR(sn);
    sock_is_sending |= (1 << sn);
    return len;
}

/* ── recv() ──────────────────────────────────────────────────────────────── */
int32_t recv(uint8_t sn, uint8_t *buf, uint16_t len)
{
    uint8_t  tmp      = 0;
    uint16_t recvsize = 0;
#if _WIZCHIP_ == W5300
    uint8_t  head[2];
    uint16_t mr;
#endif

    CHECK_SOCKNUM();
    CHECK_SOCKMODE(Sn_MR_TCP);
    CHECK_SOCKDATA();

    recvsize = getSn_RxMAX(sn);
    if (recvsize < len) len = recvsize;

#if _WIZCHIP_ == W5300
    if (sock_remained_size[sn] == 0)
    {
#endif
        while (1)
        {
            recvsize = (uint16_t)getSn_RX_RSR(sn);
            tmp      = getSn_SR(sn);
            if (tmp != SOCK_ESTABLISHED)
            {
                if (tmp == SOCK_CLOSE_WAIT)
                {
                    if (recvsize != 0) break;
                    if (getSn_TX_FSR(sn) == getSn_TxMAX(sn)) { close(sn); return SOCKERR_SOCKSTATUS; }
                }
                else { close(sn); return SOCKERR_SOCKSTATUS; }
            }
            if (sock_io_mode & (1 << sn)) return SOCK_BUSY;
            if (recvsize != 0) break;
        }
#if _WIZCHIP_ == W5300
    }

    if ((sock_remained_size[sn] == 0) || (getSn_MR(sn) & Sn_MR_ALIGN))
    {
        mr = getMR();
        if ((getSn_MR(sn) & Sn_MR_ALIGN) == 0)
        {
            wiz_recv_data(sn, head, 2);
            recvsize = (mr & MR_FS)
                       ? (((uint16_t)head[1] << 8) | head[0])
                       : (((uint16_t)head[0] << 8) | head[1]);
            sock_pack_info[sn] = PACK_FIRST;
        }
        sock_remained_size[sn] = recvsize;
    }

    if (len > sock_remained_size[sn]) len = sock_remained_size[sn];
    recvsize = len;

    if (sock_pack_info[sn] & PACK_FIFOBYTE)
    {
        *buf++ = sock_remained_byte[sn];
        sock_pack_info[sn]     &= ~PACK_FIFOBYTE;
        recvsize               -= 1;
        sock_remained_size[sn] -= 1;
    }
    if (recvsize != 0)
    {
        wiz_recv_data(sn, buf, recvsize);
        setSn_CR(sn, Sn_CR_RECV);
        WAIT_CR(sn);
    }
    sock_remained_size[sn] -= recvsize;
    if (sock_remained_size[sn] != 0)
    {
        sock_pack_info[sn] |= PACK_REMAINED;
        if (recvsize & 0x1) sock_pack_info[sn] |= PACK_FIFOBYTE;
    }
    else
    {
        sock_pack_info[sn] = PACK_COMPLETED;
    }
    if (getSn_MR(sn) & Sn_MR_ALIGN)
        sock_remained_size[sn] = 0;

#else /* W5500 */
    if (recvsize < len) len = recvsize;
    wiz_recv_data(sn, buf, len);
    setSn_CR(sn, Sn_CR_RECV);
    WAIT_CR(sn);
#endif

    return (int32_t)len;
}

/* ── sendto (internal) ───────────────────────────────────────────────────── */
static int32_t sendto_IO(uint8_t sn, uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t port)
{
    int      loopCount = 0;
    uint16_t sir;

    setSn_DIPR(sn, addr);
    setSn_DPORT(sn, port);
    wiz_send_data(sn, buf, len);
    setSn_TX_WRSR(sn, len);
    setSn_CR(sn, Sn_CR_SEND);
    WAIT_CR(sn);

    while (1)
    {
        sir = getSn_IR(sn);
        if (sir & Sn_IR_SENDOK) { setSn_IR(sn, Sn_IR_SENDOK); break; }
        if (sir & Sn_IR_TIMEOUT) { setSn_IR(sn, Sn_IR_TIMEOUT); return SOCKERR_TIMEOUT; }
        if (loopCount++ > 5000) HAL_Delay(1);
    }
    return (int32_t)len;
}

int32_t sendto_W5x00(uint8_t sn, uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t port)
{
    return sendto_IO(sn, buf, len, addr, port);
}

/* ── recvfrom (internal) ─────────────────────────────────────────────────── */
static int32_t recvfrom_IO(uint8_t sn, uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t *port)
{
#if _WIZCHIP_ == W5300
    uint16_t mr;
    uint16_t mr1;
#else
    uint8_t  mr;
#endif
    uint8_t  head[8];
    uint16_t pack_len = 0;

    CHECK_SOCKNUM();
    CHECK_SOCKDATA();

#if _WIZCHIP_ == W5300
    mr1 = getMR();
#endif

    switch ((mr = getSn_MR(sn)) & 0x0F)
    {
    case Sn_MR_UDP:
    case Sn_MR_IPRAW:
    case Sn_MR_MACRAW:
        break;
    default:
        return SOCKERR_SOCKMODE;
    }

    if (sock_remained_size[sn] == 0)
    {
        while (1)
        {
            pack_len = (uint16_t)getSn_RX_RSR(sn);
            if (getSn_SR(sn) == SOCK_CLOSED) return SOCKERR_SOCKCLOSED;
            if ((sock_io_mode & (1 << sn)) && (pack_len == 0)) return SOCK_BUSY;
            if (pack_len != 0) break;
        }
    }

    switch (mr & 0x07)
    {
    case Sn_MR_UDP:
        if (sock_remained_size[sn] == 0)
        {
            wiz_recv_data(sn, head, 8);
            setSn_CR(sn, Sn_CR_RECV);
            WAIT_CR(sn);
#if _WIZCHIP_ == W5300
            if (mr1 & MR_FS)
            {
                addr[0] = head[1]; addr[1] = head[0];
                addr[2] = head[3]; addr[3] = head[2];
                *port = ((uint16_t)head[5] << 8) | head[4];
                sock_remained_size[sn] = ((uint16_t)head[7] << 8) | head[6];
            }
            else
            {
#endif
                addr[0] = head[0]; addr[1] = head[1];
                addr[2] = head[2]; addr[3] = head[3];
                *port = ((uint16_t)head[4] << 8) | head[5];
                sock_remained_size[sn] = ((uint16_t)head[6] << 8) | head[7];
#if _WIZCHIP_ == W5300
            }
#endif
            sock_pack_info[sn] = PACK_FIRST;
        }
        pack_len = (len < sock_remained_size[sn]) ? len : sock_remained_size[sn];
        len      = pack_len;
#if _WIZCHIP_ == W5300
        if (sock_pack_info[sn] & PACK_FIFOBYTE)
        {
            *buf++ = sock_remained_byte[sn];
            pack_len               -= 1;
            sock_remained_size[sn] -= 1;
            sock_pack_info[sn]     &= ~PACK_FIFOBYTE;
        }
#endif
        wiz_recv_data(sn, buf, pack_len);
        break;

    case Sn_MR_MACRAW:
        if (sock_remained_size[sn] == 0)
        {
            wiz_recv_data(sn, head, 2);
            setSn_CR(sn, Sn_CR_RECV);
            WAIT_CR(sn);
            sock_remained_size[sn] = (((uint16_t)head[0] << 8) | head[1]) - 2;
#if _WIZCHIP_ == W5300
            if (sock_remained_size[sn] & 0x01)
                sock_remained_size[sn] = sock_remained_size[sn] + 1 - 4;
            else
                sock_remained_size[sn] -= 4;
#endif
            if (sock_remained_size[sn] > 1514)
            {
                close(sn);
                return SOCKFATAL_PACKLEN;
            }
            sock_pack_info[sn] = PACK_FIRST;
        }
        pack_len = (len < sock_remained_size[sn]) ? len : sock_remained_size[sn];
        wiz_recv_data(sn, buf, pack_len);
        break;

    case Sn_MR_IPRAW:
        if (sock_remained_size[sn] == 0)
        {
            wiz_recv_data(sn, head, 6);
            setSn_CR(sn, Sn_CR_RECV);
            WAIT_CR(sn);
            addr[0] = head[0]; addr[1] = head[1];
            addr[2] = head[2]; addr[3] = head[3];
            sock_remained_size[sn] = ((uint16_t)head[4] << 8) | head[5];
            sock_pack_info[sn]     = PACK_FIRST;
            pack_len = (len < sock_remained_size[sn]) ? len : sock_remained_size[sn];
            wiz_recv_data(sn, buf, pack_len);
        }
        break;

    default:
        wiz_recv_ignore(sn, pack_len);
        sock_remained_size[sn] = pack_len;
        break;
    }

    setSn_CR(sn, Sn_CR_RECV);
    WAIT_CR(sn);

    sock_remained_size[sn] -= pack_len;
    if (sock_remained_size[sn] != 0)
    {
        sock_pack_info[sn] |= PACK_REMAINED;
#if _WIZCHIP_ == W5300
        if (pack_len & 0x01) sock_pack_info[sn] |= PACK_FIFOBYTE;
#endif
    }
    else
    {
        sock_pack_info[sn] = PACK_COMPLETED;
    }
#if _WIZCHIP_ == W5300
    pack_len = len;
#endif

    return (int32_t)pack_len;
}

int32_t recvfrom_W5x00(uint8_t sn, uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t *port)
{
    return recvfrom_IO(sn, buf, len, addr, port);
}

/* ── ctlsocket() ─────────────────────────────────────────────────────────── */
int8_t ctlsocket(uint8_t sn, ctlsock_type cstype, void *arg)
{
    uint8_t tmp = *((uint8_t *)arg);

    CHECK_SOCKNUM();

    switch (cstype)
    {
    case CS_SET_IOMODE:
        if      (tmp == SOCK_IO_NONBLOCK) sock_io_mode |=  (1 << sn);
        else if (tmp == SOCK_IO_BLOCK)    sock_io_mode &= ~(1 << sn);
        else                              return SOCKERR_ARG;
        break;
    case CS_GET_IOMODE:
        *((uint8_t *)arg) = (uint8_t)((sock_io_mode >> sn) & 0x0001);
        break;
    case CS_GET_MAXTXBUF:
        *((uint16_t *)arg) = getSn_TxMAX(sn);
        break;
    case CS_GET_MAXRXBUF:
        *((uint16_t *)arg) = getSn_RxMAX(sn);
        break;
    case CS_CLR_INTERRUPT:
        if (tmp > SIK_ALL) return SOCKERR_ARG;
        setSn_IR(sn, tmp);
        break;
    case CS_GET_INTERRUPT:
        *((uint8_t *)arg) = getSn_IR(sn);
        break;
    case CS_SET_INTMASK:
        if (tmp > SIK_ALL) return SOCKERR_ARG;
        setSn_IMR(sn, tmp);
        break;
    case CS_GET_INTMASK:
        *((uint8_t *)arg) = getSn_IMR(sn);
        break;
    default:
        return SOCKERR_ARG;
    }
    return SOCK_OK;
}

/* ── setsockopt() ────────────────────────────────────────────────────────── */
int8_t setsockopt(uint8_t sn, sockopt_type sotype, void *arg)
{
    CHECK_SOCKNUM();

    switch (sotype)
    {
    case SO_TTL:      setSn_TTL(sn,  *(uint8_t *)arg);  break;
    case SO_TOS:      setSn_TOS(sn,  *(uint8_t *)arg);  break;
    case SO_MSS:      setSn_MSSR(sn, *(uint16_t *)arg); break;
    case SO_DESTIP:   setSn_DIPR(sn, (uint8_t *)arg);   break;
    case SO_DESTPORT: setSn_DPORTR(sn, *(uint16_t *)arg); break;
    case SO_KEEPALIVESEND:
        CHECK_TCPMODE();
#if _WIZCHIP_ > W5300
        if (getSn_KPALVTR(sn) != 0) return SOCKERR_SOCKOPT;
#endif
        setSn_CR(sn, Sn_CR_SEND_KEEP);
        {
            uint32_t _t = HAL_GetTick();
            while (getSn_CR(sn) != 0)
            {
                if (getSn_IR(sn) & Sn_IR_TIMEOUT)
                {
                    setSn_IR(sn, Sn_IR_TIMEOUT);
                    return SOCKERR_TIMEOUT;
                }
                if (HAL_GetTick() - _t > 200u) break;
            }
        }
        break;
    case SO_KEEPALIVEAUTO:
        CHECK_TCPMODE();
        setSn_KPALVTR(sn, *(uint8_t *)arg);
        break;
    default:
        return SOCKERR_ARG;
    }
    return SOCK_OK;
}

/* ── getsockopt() ────────────────────────────────────────────────────────── */
int8_t getsockopt(uint8_t sn, sockopt_type sotype, void *arg)
{
    CHECK_SOCKNUM();

    switch (sotype)
    {
    case SO_FLAG:
        *(uint8_t *)arg = getSn_MR(sn) & 0xF0;
        break;
    case SO_TTL:      *(uint8_t *)arg  = getSn_TTL(sn);   break;
    case SO_TOS:      *(uint8_t *)arg  = getSn_TOS(sn);   break;
    case SO_MSS:      *(uint16_t *)arg = getSn_MSSR(sn);  break;
    case SO_DESTIP:   getSn_DIPR(sn, (uint8_t *)arg);     break;
    case SO_DESTPORT: *(uint16_t *)arg = getSn_DPORTR(sn); break;
    case SO_KEEPALIVEAUTO:
        CHECK_TCPMODE();
        *(uint16_t *)arg = getSn_KPALVTR(sn);
        break;
    case SO_SENDBUF:  *(uint16_t *)arg = (uint16_t)getSn_TX_FSR(sn); break;
    case SO_RECVBUF:  *(uint16_t *)arg = (uint16_t)getSn_RX_RSR(sn); break;
    case SO_STATUS:   *(uint8_t *)arg  = getSn_SR(sn);    break;
    case SO_REMAINSIZE:
        if (getSn_MR(sn) & Sn_MR_TCP)
            *(uint16_t *)arg = (uint16_t)getSn_RX_RSR(sn);
        else
            *(uint16_t *)arg = sock_remained_size[sn];
        break;
    case SO_PACKINFO:
#if _WIZCHIP_ != W5300
        if (getSn_MR(sn) == Sn_MR_TCP) return SOCKERR_SOCKMODE;
#endif
        *(uint8_t *)arg = sock_pack_info[sn];
        break;
    default:
        return SOCKERR_SOCKOPT;
    }
    return SOCK_OK;
}
