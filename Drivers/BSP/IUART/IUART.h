#ifndef   __IUART_H__
#define   __IUART_H__

#include "project.h"
  // #include "AppParam.h"
#include "Serial_Network_Def.h"

#ifdef __IUART_C__
	#define IUART_EXT
#else
	#define IUART_EXT extern
#endif

/** *************************************************
 * @note: 링버퍼를 사용할지? callback을 사용할지?
 * *************************************************/
//  #define USE_RX_BUFFER_USB  // by KPS
/************************************************* */

#define TERMINATE_TICK_TIME     (2) //2ms

IUART_EXT U8    IUART1_RecvFlag;
IUART_EXT U8    IUART1_RecvTick;

IUART_EXT U8    IUART2_RecvFlag;
IUART_EXT U8    IUART2_RecvTick;

IUART_EXT U8    IUART4_RecvFlag;
IUART_EXT U8    IUART4_RecvTick;

IUART_EXT void  IUART_Init(Serial_t* s0, Serial_t* s1, Serial_t* s2);

IUART_EXT U8    IUART1_CheckRecvEnd(void); // USB port
IUART_EXT U8    IUART2_CheckRecvEnd(void); // RS485
IUART_EXT U8    IUART4_CheckRecvEnd(void); // RS232 TTL 

IUART_EXT U32   IUART1_GetRecvSize(void);
IUART_EXT U32   IUART2_GetRecvSize(void);
IUART_EXT U32   IUART4_GetRecvSize(void);

IUART_EXT void  IUART1_WriteBytes(uint8_t* pData, uint16_t size);
IUART_EXT void  IUART2_WriteBytes(uint8_t* pData, uint16_t size);
IUART_EXT void  IUART4_WriteBytes(uint8_t* pData, uint16_t size);

IUART_EXT U8    IUART1_ReadByte(void);
IUART_EXT U8    IUART2_ReadByte(void);
IUART_EXT U8    IUART4_ReadByte(void);

IUART_EXT void  IUART1_RecvBufClear(void); // by KPS
IUART_EXT void  IUART2_RecvBufClear(void);
IUART_EXT void  IUART4_RecvBufClear(void);

IUART_EXT void  IUART1_SetBaudrate(U8 baudrate);  // by KPS
IUART_EXT void  IUART2_SetBaudrate(U8 baudrate);
IUART_EXT void  IUART4_SetBaudrate(U8 baudrate); 

IUART_EXT void (*IUART1_Callback)(U8 data);  // by KPS

#endif


