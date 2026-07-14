#define __IFLASH_C__
    #include "IFLASH.h"
#undef  __IFLASH_C__

#include <string.h>

/* ===============================
 * Address validation
 * =============================== */
uint8_t IFLASH_IsAddrValid(uint32_t addr)
{
    return (addr >= FLASH_BASE && addr <= FLASH_END);
}

/* ===============================
 * Sector mapping (STM32F746)
 * =============================== */
uint32_t IFLASH_GetSector(uint32_t addr)
{
    if (addr < 0x08008000) return FLASH_SECTOR_0;
    if (addr < 0x08010000) return FLASH_SECTOR_1;
    if (addr < 0x08018000) return FLASH_SECTOR_2;
    if (addr < 0x08020000) return FLASH_SECTOR_3;
    if (addr < 0x08040000) return FLASH_SECTOR_4;
    if (addr < 0x08080000) return FLASH_SECTOR_5;
    if (addr < 0x080C0000) return FLASH_SECTOR_6;
    if (addr < 0x08100000) return FLASH_SECTOR_7;

    /* Out of range */
    return FLASH_SECTOR_7;
}

/* ===============================
 * Lock / Unlock
 * =============================== */
uint8_t IFLASH_Unlock(void)
{
    return (HAL_FLASH_Unlock() == HAL_OK) ? IFLASH_OK : IFLASH_ERR;
}

uint8_t IFLASH_Lock(void)
{
    return (HAL_FLASH_Lock() == HAL_OK) ? IFLASH_OK : IFLASH_ERR;
}

/* ===============================
 * Erase
 * =============================== */
uint8_t IFLASH_EraseSector(uint32_t sector)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t error = 0;

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.Sector       = sector;
    erase.NbSectors    = 1;

    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK)
        return IFLASH_ERR;

    return IFLASH_OK;
}

uint8_t IFLASH_EraseRange(uint32_t start_addr, uint32_t size)
{
    if (!IFLASH_IsAddrValid(start_addr))
        return IFLASH_RANGE_ERR;

    uint32_t addr = start_addr;
    uint32_t end  = start_addr + size;

    IFLASH_Unlock();

    while (addr < end)
    {
        uint32_t sector = IFLASH_GetSector(addr);
        if (IFLASH_EraseSector(sector) != IFLASH_OK)
        {
            IFLASH_Lock();
            return IFLASH_ERR;
        }

        switch (sector)
        {
        case FLASH_SECTOR_0: addr = 0x08008000; break;
        case FLASH_SECTOR_1: addr = 0x08010000; break;
        case FLASH_SECTOR_2: addr = 0x08018000; break;
        case FLASH_SECTOR_3: addr = 0x08020000; break;
        case FLASH_SECTOR_4: addr = 0x08040000; break;
        case FLASH_SECTOR_5: addr = 0x08080000; break;
        case FLASH_SECTOR_6: addr = 0x080C0000; break;
        case FLASH_SECTOR_7: addr = 0x08100000; break;
        default: addr = end; break;
        }
    }

    IFLASH_Lock();
    return IFLASH_OK;
}

/* ===============================
 * Write
 * =============================== */
uint8_t IFLASH_WriteWord(uint32_t addr, uint32_t data)
{
    if ((addr & 0x3) != 0)
        return IFLASH_ALIGN_ERR;

    if (!IFLASH_IsAddrValid(addr))
        return IFLASH_RANGE_ERR;

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data) != HAL_OK)
        return IFLASH_ERR;

    if (*(uint32_t *)addr != data)
        return IFLASH_ERR;

    return IFLASH_OK;
}

uint8_t IFLASH_WriteBuffer(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    IFLASH_Unlock();

    for (uint32_t i = 0; i < len; i += 4)
    {
        uint32_t word = 0xFFFFFFFF;
        uint32_t copy = (len - i >= 4) ? 4 : (len - i);

        memcpy(&word, &buf[i], copy);

        if (IFLASH_WriteWord(addr + i, word) != IFLASH_OK)
        {
            IFLASH_Lock();
            return IFLASH_ERR;
        }
    }

    IFLASH_Lock();
    return IFLASH_OK;
}

/* ===============================
 * Read
 * =============================== */
uint8_t IFLASH_Read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (!IFLASH_IsAddrValid(addr))
        return IFLASH_RANGE_ERR;

    memcpy(buf, (const void *)addr, len);
    return IFLASH_OK;
}
