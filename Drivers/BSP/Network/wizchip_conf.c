//*****************************************************************************
//! \file wizchip_conf.c
//! \brief WIZCHIP Config — W5300 / W5500 only build.
//*****************************************************************************
#include <stddef.h>
#include "wizchip_conf.h"

/* ── Default no-op callbacks ─────────────────────────────────────────────── */
void    wizchip_cris_enter(void)                       {}
void    wizchip_cris_exit(void)                        {}
void    wizchip_cs_select(void)                        {}
void    wizchip_cs_deselect(void)                      {}

iodata_t wizchip_bus_readdata(uint32_t AddrSel)
{
    return *(volatile iodata_t *)(_WIZCHIP_IO_BASE_ + ((AddrSel & 0xFFFF) << 1));
}

void wizchip_bus_writedata(uint32_t AddrSel, iodata_t wb)
{
    *(volatile iodata_t *)(_WIZCHIP_IO_BASE_ + ((AddrSel & 0xFFFF) << 1)) = wb;
}

void wizchip_bus_read_buf(uint32_t AddrSel, iodata_t *buf, int16_t len, uint8_t addrinc)
{
    uint16_t i;
    if (addrinc) addrinc = sizeof(iodata_t);
    for (i = 0; i < len; i++) {
        *buf++ = WIZCHIP.IF.BUS._read_data(AddrSel);
        AddrSel += (uint32_t)addrinc;
    }
}

void wizchip_bus_write_buf(uint32_t AddrSel, iodata_t *buf, int16_t len, uint8_t addrinc)
{
    uint16_t i;
    if (addrinc) addrinc = sizeof(iodata_t);
    for (i = 0; i < len; i++) {
        WIZCHIP.IF.BUS._write_data(AddrSel, *buf++);
        AddrSel += (uint32_t)addrinc;
    }
}

uint8_t wizchip_spi_readbyte(void)              { return 0; }
void    wizchip_spi_writebyte(uint8_t wb)        { (void)wb; }

void wizchip_spi_readburst(uint8_t *pBuf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
        *pBuf++ = WIZCHIP.IF.SPI._read_byte();
}

void wizchip_spi_writeburst(uint8_t *pBuf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
        WIZCHIP.IF.SPI._write_byte(*pBuf++);
}

/* ── WIZCHIP global instance ─────────────────────────────────────────────── */
_WIZCHIP WIZCHIP = {
    _WIZCHIP_IO_MODE_,
    _WIZCHIP_ID_,
    { wizchip_cris_enter, wizchip_cris_exit },
    { wizchip_cs_select,  wizchip_cs_deselect },
    { { wizchip_bus_readdata, wizchip_bus_writedata } }
};

/* ── DNS / DHCP state (not stored in chip registers) ─────────────────────── */
static uint8_t   _DNS_[4];
static dhcp_mode _DHCP_;

/* ── Callback registration ───────────────────────────────────────────────── */
void reg_wizchip_cris_cbfunc(void (*cris_en)(void), void (*cris_ex)(void))
{
    WIZCHIP.CRIS._enter = cris_en ? cris_en : wizchip_cris_enter;
    WIZCHIP.CRIS._exit  = cris_ex ? cris_ex : wizchip_cris_exit;
}

void reg_wizchip_cs_cbfunc(void (*cs_sel)(void), void (*cs_desel)(void))
{
    WIZCHIP.CS._select   = cs_sel   ? cs_sel   : wizchip_cs_select;
    WIZCHIP.CS._deselect = cs_desel ? cs_desel : wizchip_cs_deselect;
}

void reg_wizchip_bus_cbfunc(iodata_t (*bus_rb)(uint32_t addr), void (*bus_wb)(uint32_t addr, iodata_t wb))
{
    while (!(WIZCHIP.if_mode & _WIZCHIP_IO_MODE_BUS_))
        ;
    WIZCHIP.IF.BUS._read_data  = bus_rb ? bus_rb : wizchip_bus_readdata;
    WIZCHIP.IF.BUS._write_data = bus_wb ? bus_wb : wizchip_bus_writedata;
}

void reg_wizchip_spi_cbfunc(uint8_t (*spi_rb)(void), void (*spi_wb)(uint8_t wb))
{
    while (!(WIZCHIP.if_mode & _WIZCHIP_IO_MODE_SPI_))
        ;
    WIZCHIP.IF.SPI._read_byte  = spi_rb ? spi_rb : wizchip_spi_readbyte;
    WIZCHIP.IF.SPI._write_byte = spi_wb ? spi_wb : wizchip_spi_writebyte;
}

void reg_wizchip_spiburst_cbfunc(void (*spi_rb)(uint8_t *pBuf, uint16_t len),
                                  void (*spi_wb)(uint8_t *pBuf, uint16_t len))
{
    while (!(WIZCHIP.if_mode & _WIZCHIP_IO_MODE_SPI_))
        ;
    WIZCHIP.IF.SPI._read_burst  = spi_rb ? spi_rb : wizchip_spi_readburst;
    WIZCHIP.IF.SPI._write_burst = spi_wb ? spi_wb : wizchip_spi_writeburst;
}

/* ── ctlwizchip ──────────────────────────────────────────────────────────── */
int8_t ctlwizchip(ctlwizchip_type cwtype, void *arg)
{
#if _WIZCHIP_ == W5500
    uint8_t tmp = *(uint8_t *)arg;
#endif
    uint8_t *ptmp[2] = {0, 0};

    switch (cwtype)
    {
    case CW_RESET_WIZCHIP:
        wizchip_sw_reset();
        break;

    case CW_INIT_WIZCHIP:
        if (arg) {
            ptmp[0] = (uint8_t *)arg;
            ptmp[1] = ptmp[0] + _WIZCHIP_SOCK_NUM_;
        }
        return wizchip_init(ptmp[0], ptmp[1]);

    case CW_CLR_INTERRUPT:
        wizchip_clrinterrupt(*(intr_kind *)arg);
        break;

    case CW_GET_INTERRUPT:
        *(intr_kind *)arg = wizchip_getinterrupt();
        break;

    case CW_SET_INTRMASK:
        wizchip_setinterruptmask(*(intr_kind *)arg);
        break;

    case CW_GET_INTRMASK:
        *(intr_kind *)arg = wizchip_getinterruptmask();
        break;

    case CW_GET_ID:
        ((uint8_t *)arg)[0] = WIZCHIP.id[0];
        ((uint8_t *)arg)[1] = WIZCHIP.id[1];
        ((uint8_t *)arg)[2] = WIZCHIP.id[2];
        ((uint8_t *)arg)[3] = WIZCHIP.id[3];
        ((uint8_t *)arg)[4] = WIZCHIP.id[4];
        ((uint8_t *)arg)[5] = WIZCHIP.id[5];
        ((uint8_t *)arg)[6] = 0;
        break;

#if _WIZCHIP_ == W5500
    case CW_SET_INTRTIME:
        setINTLEVEL(*(uint16_t *)arg);
        break;

    case CW_GET_INTRTIME:
        *(uint16_t *)arg = getINTLEVEL();
        break;

    case CW_RESET_PHY:
        wizphy_reset();
        break;

    case CW_SET_PHYCONF:
        wizphy_setphyconf((wiz_PhyConf *)arg);
        break;

    case CW_GET_PHYCONF:
        wizphy_getphyconf((wiz_PhyConf *)arg);
        break;

    case CW_GET_PHYSTATUS:
        wizphy_getphystat((wiz_PhyConf *)arg);
        break;

    case CW_SET_PHYPOWMODE:
        return wizphy_setphypmode(*(uint8_t *)arg);

    case CW_GET_PHYPOWMODE:
        tmp = wizphy_getphypmode();
        if ((int8_t)tmp == -1) return -1;
        *(uint8_t *)arg = tmp;
        break;

    case CW_GET_PHYLINK:
        tmp = wizphy_getphylink();
        if ((int8_t)tmp == -1) return -1;
        *(uint8_t *)arg = tmp;
        break;
#endif /* W5500 */

    default:
        return -1;
    }
    return 0;
}

/* ── ctlnetwork ──────────────────────────────────────────────────────────── */
int8_t ctlnetwork(ctlnetwork_type cntype, void *arg)
{
    switch (cntype)
    {
    case CN_SET_NETINFO:
        wizchip_setnetinfo((wiz_NetInfo *)arg);
        break;
    case CN_GET_NETINFO:
        wizchip_getnetinfo((wiz_NetInfo *)arg);
        break;
    case CN_SET_NETMODE:
        return wizchip_setnetmode(*(netmode_type *)arg);
    case CN_GET_NETMODE:
        *(netmode_type *)arg = wizchip_getnetmode();
        break;
    case CN_SET_TIMEOUT:
        wizchip_settimeout((wiz_NetTimeout *)arg);
        break;
    case CN_GET_TIMEOUT:
        wizchip_gettimeout((wiz_NetTimeout *)arg);
        break;
    default:
        return -1;
    }
    return 0;
}

/* ── Software reset ──────────────────────────────────────────────────────── */
void wizchip_sw_reset(void)
{
    uint8_t gw[4], sn[4], sip[4];
    uint8_t mac[6];

#if _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_BUS_INDIR_
    uint16_t mr = (uint16_t)getMR();
    setMR(mr | MR_IND);
#endif

    getSHAR(mac);
    getGAR(gw); getSUBR(sn); getSIPR(sip);
    setMR(MR_RST);
    getMR(); /* delay */

#if _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_BUS_INDIR_
    setMR(mr | MR_IND);
#endif

    setSHAR(mac);
    setGAR(gw);
    setSUBR(sn);
    setSIPR(sip);
}

/* ── Socket buffer initialisation ───────────────────────────────────────── */
int8_t wizchip_init(uint8_t *txsize, uint8_t *rxsize)
{
    int8_t i;
    int8_t tmp = 0;

    WIZCHIP_Init();
    wizchip_sw_reset();

    if (txsize) {
        tmp = 0;
#if _WIZCHIP_ == W5300
        for (i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
            if (txsize[i] > 64) return -1;
            tmp += txsize[i];
            if (tmp > 128) return -1;
        }
        if (tmp % 8) return -1;
#else /* W5500 */
        for (i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
            tmp += txsize[i];
            if (tmp > 16) return -1;
        }
#endif
        for (i = 0; i < _WIZCHIP_SOCK_NUM_; i++)
            setSn_TXBUF_SIZE(i, txsize[i]);
    }

    if (rxsize) {
        tmp = 0;
#if _WIZCHIP_ == W5300
        for (i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
            if (rxsize[i] > 64) return -1;
            tmp += rxsize[i];
            if (tmp > 128) return -1;
        }
        if (tmp % 8) return -1;
#else /* W5500 */
        for (i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
            tmp += rxsize[i];
            if (tmp > 16) return -1;
        }
#endif
        for (i = 0; i < _WIZCHIP_SOCK_NUM_; i++)
            setSn_RXBUF_SIZE(i, rxsize[i]);
    }

    return 0;
}

/* ── Interrupt control ───────────────────────────────────────────────────── */
void wizchip_clrinterrupt(intr_kind intr)
{
    uint8_t ir  = (uint8_t)intr;
    uint8_t sir = (uint8_t)((uint16_t)intr >> 8);

#if _WIZCHIP_ == W5300
    ir |= (1 << 4); /* IK_FMTU */
    setIR((uint16_t)((uint16_t)ir << 8) | sir);
#elif _WIZCHIP_ == W5500
    setIR(ir);
    for (ir = 0; ir < 8; ir++) {
        if (sir & (0x01 << ir))
            setSn_IR(ir, 0xFF);
    }
#endif
}

intr_kind wizchip_getinterrupt(void)
{
    uint8_t  ir  = 0;
    uint8_t  sir = 0;
    uint32_t ret = 0;

#if _WIZCHIP_ == W5300
    ret = getIR();
    ir  = (uint8_t)(ret >> 8);
    sir = (uint8_t)ret;
#elif _WIZCHIP_ == W5500
    ir  = getIR();
    sir = getSIR();
#endif

    ret = sir;
    ret = (ret << 8) + ir;
    return (intr_kind)ret;
}

void wizchip_setinterruptmask(intr_kind intr)
{
    uint8_t imr  = (uint8_t)intr;
    uint8_t simr = (uint8_t)((uint16_t)intr >> 8);

#if _WIZCHIP_ == W5300
    setIMR((uint16_t)((uint16_t)imr << 8) | simr);
#elif _WIZCHIP_ == W5500
    setIMR(imr);
    setSIMR(simr);
#endif
}

intr_kind wizchip_getinterruptmask(void)
{
    uint8_t  imr  = 0;
    uint8_t  simr = 0;
    uint32_t ret  = 0;

#if _WIZCHIP_ == W5300
    ret  = getIMR();
    imr  = (uint8_t)(ret >> 8);
    simr = (uint8_t)ret;
#elif _WIZCHIP_ == W5500
    imr  = getIMR();
    simr = getSIMR();
#endif

    ret = simr;
    ret = (ret << 8) + imr;
    return (intr_kind)ret;
}

/* ── PHY link / power ────────────────────────────────────────────────────── */
int8_t wizphy_getphylink(void)
{
#if _WIZCHIP_ == W5500
    return (getPHYCFGR() & PHYCFGR_LNK_ON) ? PHY_LINK_ON : PHY_LINK_OFF;
#else
    return -1; /* W5300: no software-readable PHY link status register */
#endif
}

int8_t wizphy_getphypmode(void)
{
#if _WIZCHIP_ == W5500
    return ((getPHYCFGR() & PHYCFGR_OPMDC_ALLA) == PHYCFGR_OPMDC_PDOWN)
           ? PHY_POWER_DOWN : PHY_POWER_NORM;
#else
    return -1;
#endif
}

/* ── W5500 PHY configuration ─────────────────────────────────────────────── */
#if _WIZCHIP_ == W5500

void wizphy_reset(void)
{
    uint8_t tmp = getPHYCFGR();
    tmp &= PHYCFGR_RST;
    setPHYCFGR(tmp);
    tmp = getPHYCFGR();
    tmp |= ~PHYCFGR_RST;
    setPHYCFGR(tmp);
}

void wizphy_setphyconf(wiz_PhyConf *phyconf)
{
    uint8_t tmp = 0;
    if (phyconf->by == PHY_CONFBY_SW) tmp |= PHYCFGR_OPMD;
    else                              tmp &= ~PHYCFGR_OPMD;

    if (phyconf->mode == PHY_MODE_AUTONEGO) {
        tmp |= PHYCFGR_OPMDC_ALLA;
    } else {
        if (phyconf->duplex == PHY_DUPLEX_FULL)
            tmp |= (phyconf->speed == PHY_SPEED_100) ? PHYCFGR_OPMDC_100F : PHYCFGR_OPMDC_10F;
        else
            tmp |= (phyconf->speed == PHY_SPEED_100) ? PHYCFGR_OPMDC_100H : PHYCFGR_OPMDC_10H;
    }
    setPHYCFGR(tmp);
    wizphy_reset();
}

void wizphy_getphyconf(wiz_PhyConf *phyconf)
{
    uint8_t tmp = getPHYCFGR();
    phyconf->by = (tmp & PHYCFGR_OPMD) ? PHY_CONFBY_SW : PHY_CONFBY_HW;
    switch (tmp & PHYCFGR_OPMDC_ALLA) {
    case PHYCFGR_OPMDC_ALLA:
    case PHYCFGR_OPMDC_100FA:
        phyconf->mode = PHY_MODE_AUTONEGO; break;
    default:
        phyconf->mode = PHY_MODE_MANUAL;   break;
    }
    phyconf->speed  = (tmp & (PHYCFGR_OPMDC_100FA | PHYCFGR_OPMDC_100F | PHYCFGR_OPMDC_100H))
                      ? PHY_SPEED_100 : PHY_SPEED_10;
    phyconf->duplex = (tmp & (PHYCFGR_OPMDC_100FA | PHYCFGR_OPMDC_100F | PHYCFGR_OPMDC_10F))
                      ? PHY_DUPLEX_FULL : PHY_DUPLEX_HALF;
}

void wizphy_getphystat(wiz_PhyConf *phyconf)
{
    uint8_t tmp = getPHYCFGR();
    phyconf->duplex = (tmp & PHYCFGR_DPX_FULL) ? PHY_DUPLEX_FULL : PHY_DUPLEX_HALF;
    phyconf->speed  = (tmp & PHYCFGR_SPD_100)  ? PHY_SPEED_100   : PHY_SPEED_10;
}

int8_t wizphy_setphypmode(uint8_t pmode)
{
    uint8_t tmp = getPHYCFGR();
    if ((tmp & PHYCFGR_OPMD) == 0) return -1;
    tmp &= ~PHYCFGR_OPMDC_ALLA;
    tmp |= (pmode == PHY_POWER_DOWN) ? PHYCFGR_OPMDC_PDOWN : PHYCFGR_OPMDC_ALLA;
    setPHYCFGR(tmp);
    wizphy_reset();
    tmp = getPHYCFGR();
    if (pmode == PHY_POWER_DOWN)
        return (tmp & PHYCFGR_OPMDC_PDOWN) ? 0 : -1;
    return (tmp & PHYCFGR_OPMDC_ALLA) ? 0 : -1;
}

#endif /* W5500 PHY */

/* ── Network info ────────────────────────────────────────────────────────── */
void wizchip_setnetinfo(wiz_NetInfo *pnetinfo)
{
    setSHAR(pnetinfo->mac);
    setGAR(pnetinfo->gw);
    setSUBR(pnetinfo->sn);
    setSIPR(pnetinfo->ip);
    _DNS_[0] = pnetinfo->dns[0];
    _DNS_[1] = pnetinfo->dns[1];
    _DNS_[2] = pnetinfo->dns[2];
    _DNS_[3] = pnetinfo->dns[3];
    _DHCP_   = pnetinfo->dhcp;
}

void wizchip_getnetinfo(wiz_NetInfo *pnetinfo)
{
    getSHAR(pnetinfo->mac);
    getGAR(pnetinfo->gw);
    getSUBR(pnetinfo->sn);
    getSIPR(pnetinfo->ip);
    pnetinfo->dns[0] = _DNS_[0];
    pnetinfo->dns[1] = _DNS_[1];
    pnetinfo->dns[2] = _DNS_[2];
    pnetinfo->dns[3] = _DNS_[3];
    pnetinfo->dhcp   = _DHCP_;
}

int8_t wizchip_setnetmode(netmode_type netmode)
{
    uint8_t tmp;
#if _WIZCHIP_ == W5500
    if (netmode & ~(NM_WAKEONLAN | NM_PPPOE | NM_PINGBLOCK | NM_FORCEARP)) return -1;
#else
    if (netmode & ~(NM_WAKEONLAN | NM_PPPOE | NM_PINGBLOCK)) return -1;
#endif
    tmp  = getMR();
    tmp |= (uint8_t)netmode;
    setMR(tmp);
    return 0;
}

netmode_type wizchip_getnetmode(void)
{
    return (netmode_type)getMR();
}

void wizchip_settimeout(wiz_NetTimeout *nettime)
{
    setRCR(nettime->retry_cnt);
    setRTR(nettime->time_100us);
}

void wizchip_gettimeout(wiz_NetTimeout *nettime)
{
    nettime->retry_cnt  = getRCR();
    nettime->time_100us = getRTR();
}
