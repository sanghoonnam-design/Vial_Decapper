#ifndef   __EEPROM_H__
#define   __EEPROM_H__

#include "project.h"

#ifdef __EEPROM_C__
	#define EEPROM_EXT
#else
	#define EEPROM_EXT extern
#endif

#define EEPROM_SIZE  (32 * 1024)  // 32KB EEPROM

EEPROM_EXT void EEPROM_Init(void);
EEPROM_EXT void EEPROM_WriteBytes(U16 addr, U8 *bytes, U16 size);
EEPROM_EXT void EEPROM_ReadBytes(U16 addr, U8 *bytes, U16 size);
#endif


