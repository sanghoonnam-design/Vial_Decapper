#ifndef   __ISPI_H__
#define   __ISPI_H__

#include "project.h"

#ifdef __ISPI_C__
	#define ISPI_EXT
#else
	#define ISPI_EXT extern
#endif

ISPI_EXT void ISPI1_Init(void);
ISPI_EXT void ISPI2_Init(void);

ISPI_EXT void SPI1_RST_H(void);
ISPI_EXT void SPI1_RST_L(void);

ISPI_EXT void SPI1_CSH(void);
ISPI_EXT void SPI1_CSL(void);

ISPI_EXT void SPI1_CSH(void);
ISPI_EXT void SPI1_CSL(void);

ISPI_EXT void SPI2_CSH(void);
ISPI_EXT void SPI2_CSL(void); 

ISPI_EXT U8 ISPI1_SendData(U8 data);
ISPI_EXT U8 ISPI2_SendData(U8 data);

#endif