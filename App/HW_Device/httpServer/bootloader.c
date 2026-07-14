#define __BOOTLOADER_C__
#include "bootloader.h"
#undef __BOOTLOADER_C__

#include "IFLASH.h"
#include <string.h>

#define NET_IP 192, 168, 0, 100
#define NET_MASK 255, 255, 255, 0
#define NET_GW 192, 168, 0, 1

#define BOOT_MAGIC_CODE (0xABCDABCD)

#define BKP_SRAM_BASE 0x40024000UL
#define BKP_SHARED_PTR ((BootInfo_t *)BKP_SRAM_BASE)

#pragma pack(push, 1)
typedef struct
{
	uint32_t magic; // BOOT_MAGIC_CODE
	uint32_t appDownloaded;
	uint32_t appConfirmed;
	Network_t netInfo;
} BootParamInfo_t;

typedef struct
{
	uint32_t bootMode;
	uint8_t model[40];
	uint8_t version[40];
	uint8_t buildDate[40];
} BootInfo_t;
#pragma pack(pop)

static BootInfo_t *BootInfo = BKP_SHARED_PTR;
static BootParamInfo_t BootParamInfo;

void BootInfo_Init(uint8_t *modelStr, uint8_t *versionStr, uint8_t *buildDateStr)
{
	BootInfo_SetFwUpdatFlag(BOOTLOADER_BOOT_MODE);
	memset(BootInfo->model, 0, 40);
	memset(BootInfo->version, 0, 40);
	memset(BootInfo->buildDate, 0, 40);

	strcpy((char *)BootInfo->model, (const char *)modelStr);
	strcpy((char *)BootInfo->version, (const char *)versionStr);
	strcpy((char *)BootInfo->buildDate, (const char *)buildDateStr);
}

void BootInfo_SetFwUpdatFlag(uint32_t value)
{
	BootInfo->bootMode = value;
}

uint32_t BootInfo_GetFwUpdatFlag(void)
{
	return BootInfo->bootMode;
}

void BootParamInfo_ReadParam(void)
{
	IFLASH_Read(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));
	if (BootParamInfo.magic != BOOT_MAGIC_CODE)
	{
		BootParamInfo.magic = BOOT_MAGIC_CODE;
		BootParamInfo.appDownloaded = APP_NOT_DOWNLOADED;
		BootParamInfo.appConfirmed = APP_NOT_CONFIRMED;

		SET_IP(BootParamInfo.netInfo.ip, NET_IP);
		SET_IP(BootParamInfo.netInfo.subnet, NET_MASK);
		SET_IP(BootParamInfo.netInfo.gw, NET_GW);

		IFLASH_EraseRange(BOOTLOADER_PARAM_FLASH_ADDR, 32 * 1024);
		IFLASH_WriteBuffer(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));
	}
}

void BootParamInfo_SetAppConfirm(uint32_t confirm)
{
	IFLASH_Read(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));
	BootParamInfo.appConfirmed = confirm;
	IFLASH_EraseRange(BOOTLOADER_PARAM_FLASH_ADDR, 32 * 1024);
	IFLASH_WriteBuffer(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));
}

uint32_t BootParamInfo_GetAppConfirm(void)
{
	IFLASH_Read(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));
	return BootParamInfo.appConfirmed;
}

void BootParamInfo_SetAppDownloaded(uint32_t downloaded)
{
	IFLASH_Read(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));
	BootParamInfo.appDownloaded = downloaded;
	IFLASH_EraseRange(BOOTLOADER_PARAM_FLASH_ADDR, 32 * 1024);
	IFLASH_WriteBuffer(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));
}

uint32_t BootParamInfo_GetAppDownloaded(void)
{
	IFLASH_Read(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));
	return BootParamInfo.appDownloaded;
}

void BootParamInfo_SetNetworkIP(Network_t net)
{
	uint8_t changed = false;

	IFLASH_Read(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));

	if (memcmp(BootParamInfo.netInfo.ip, net.ip, 4) != 0)
		changed = true;

	if (memcmp(BootParamInfo.netInfo.subnet, net.subnet, 4) != 0)
		changed = true;

	if (memcmp(BootParamInfo.netInfo.gw, net.gw, 4) != 0)
		changed = true;

	if (changed)
	{
		memcpy(BootParamInfo.netInfo.ip, net.ip, 4);
		memcpy(BootParamInfo.netInfo.subnet, net.subnet, 4);
		memcpy(BootParamInfo.netInfo.gw, net.gw, 4);

		IFLASH_EraseRange(BOOTLOADER_PARAM_FLASH_ADDR, 32 * 1024);
		IFLASH_WriteBuffer(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));
	}
}

void BootParamInfo_GetNetworkIP(Network_t *net)
{
	IFLASH_Read(BOOTLOADER_PARAM_FLASH_ADDR, (uint8_t *)&BootParamInfo, sizeof(BootParamInfo_t));
	memcpy(net->ip, BootParamInfo.netInfo.ip, 4);
	memcpy(net->subnet, BootParamInfo.netInfo.subnet, 4);
	memcpy(net->gw, BootParamInfo.netInfo.gw, 4);
}

uint8_t *BootInfo_GetModelStr(void)
{
	BootInfo->model[39] = 0;
	return BootInfo->model;
}

uint8_t *BootInfo_GetVersionStr(void)
{
	BootInfo->version[39] = 0;
	return BootInfo->version;
}

uint8_t *BootInfo_GetBuildDateStr(void)
{
	BootInfo->buildDate[39] = 0;
	return BootInfo->buildDate;
}
