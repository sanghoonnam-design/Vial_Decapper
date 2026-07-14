#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#include "project.h"
#include "Serial_Network_Def.h"

#ifdef __BOOTLOADER_C__
#define BOOTLOADER_EXT
#else
#define BOOTLOADER_EXT extern
#endif

#define BOOTLOADER_PARAM_FLASH_ADDR (0x08018000)
#define APP_PARAM_FLASH_ADDR (0x08020000)

#define SET_IP(ip, ...)                 \
	do                                  \
	{                                   \
		uint8_t _tmp[] = {__VA_ARGS__}; \
		(ip)[0] = _tmp[0];              \
		(ip)[1] = _tmp[1];              \
		(ip)[2] = _tmp[2];              \
		(ip)[3] = _tmp[3];              \
	} while (0)

enum
{
	BOOTLOADER_NONE = 0x0,
	BOOTLOADER_BOOT_MODE = 0xABCDABC0,
	BOOTLOADER_APP_MODE = 0xABCDABC1
};

enum
{
	APP_DOWNLOADED_NONE = 0x0,
	APP_NOT_DOWNLOADED,
	APP_DOWNLOADED
};

enum
{
	APP_CONFIRMED_NONE = 0x0,
	APP_NOT_CONFIRMED,
	APP_CONFIRMED
};

BOOTLOADER_EXT void BootInfo_Init(uint8_t *modelStr, uint8_t *versionStr, uint8_t *buildDateStr);

BOOTLOADER_EXT void BootInfo_SetFwUpdatFlag(uint32_t value);
BOOTLOADER_EXT uint32_t BootInfo_GetFwUpdatFlag(void);

BOOTLOADER_EXT void BootParamInfo_ReadParam(void);
BOOTLOADER_EXT void BootParamInfo_SetNetworkIP(Network_t net);
BOOTLOADER_EXT void BootParamInfo_GetNetworkIP(Network_t *net);
BOOTLOADER_EXT void BootParamInfo_SetAppDownloaded(uint32_t downloaded);
BOOTLOADER_EXT uint32_t BootParamInfo_GetAppDownloaded(void);
BOOTLOADER_EXT void BootParamInfo_SetAppConfirm(uint32_t confirm);
BOOTLOADER_EXT uint32_t BootParamInfo_GetAppConfirm(void);

BOOTLOADER_EXT uint8_t *BootInfo_GetModelStr(void);
BOOTLOADER_EXT uint8_t *BootInfo_GetVersionStr(void);
BOOTLOADER_EXT uint8_t *BootInfo_GetBuildDateStr(void);

#endif
