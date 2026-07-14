#ifndef   __IFLASH_H__
#define   __IFLASH_H__

#include "project.h"

#ifdef __IFLASH_C__
	#define IFLASH_EXT
#else
	#define IFLASH_EXT extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================
 * Flash address range (STM32F746 : 1MB Flash)
 * ========================================================= */
#ifndef FLASH_BASE
#define FLASH_BASE      0x08000000UL
#endif

#ifndef FLASH_END
#define FLASH_END       0x080FFFFFUL
#endif

/* =========================================================
 * Return codes
 * ========================================================= */
#define IFLASH_OK           0
#define IFLASH_ERR          1
#define IFLASH_ALIGN_ERR    2
#define IFLASH_RANGE_ERR    3



IFLASH_EXT uint8_t IFLASH_Unlock(void);
IFLASH_EXT uint8_t IFLASH_Lock(void);
IFLASH_EXT uint8_t IFLASH_EraseSector(uint32_t sector);
IFLASH_EXT uint8_t IFLASH_EraseRange(uint32_t start_addr, uint32_t size);
IFLASH_EXT uint8_t IFLASH_WriteWord(uint32_t addr, uint32_t data);
IFLASH_EXT uint8_t IFLASH_WriteBuffer(uint32_t addr,
                           const uint8_t *buf,
                           uint32_t len);

IFLASH_EXT uint8_t IFLASH_Read(uint32_t addr,
                    uint8_t *buf,
                    uint32_t len);

IFLASH_EXT uint8_t IFLASH_IsAddrValid(uint32_t addr);

IFLASH_EXT uint32_t IFLASH_GetSector(uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif /* __IFLASH_H__ */
