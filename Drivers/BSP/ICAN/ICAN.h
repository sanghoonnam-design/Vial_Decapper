#ifndef   __ICAN_H__
#define   __ICAN_H__

#include "project.h"
// #include "AppParam.h"

#ifdef __ICAN_C__
	#define ICAN_EXT
#else
	#define ICAN_EXT extern
#endif

typedef enum
{
    BAUDRATE_1M = 0,
    BAUDRATE_500K,
    BAUDRATE_250K,
    BAUDRATE_100K,
    BAUDRATE_50K,
    BAUDRATE_20K,
}CanBuadRate_t;

typedef struct Can
{
    U8  baudrate;
    U32 id;
    U32 mask;
}Can_t;

#pragma pack(push, 1)

typedef struct CanPacket
{
    union
    {
        struct
        {
            U16 StdId;
            U8  RTR;
            U8  DLC;
            U8  buff[8];
        };

        struct //by KPS
        {
            U16 id;
            U8  rtr;
            U8  dlc;
            U8  data[8];
        };
    };
}CanPacket_t;

#pragma pack(pop)  

ICAN_EXT U8     ICAN1_RecvFlag;
ICAN_EXT U8     ICAN1_RecvTick;

ICAN_EXT U32 CAN_TxIsrCount;
ICAN_EXT U32 CAN_RxIsrCount;

ICAN_EXT void   ICAN_Init(Can_t* c); // by KPS

ICAN_EXT U8     ICAN1_CheckRecv(void);
ICAN_EXT U32    ICAN1_GetRecvSize(void);
ICAN_EXT void   ICAN1_WriteData(CanPacket_t* cp);
ICAN_EXT void   ICAN1_ReadData(CanPacket_t* cp);
ICAN_EXT void   ICAN1_RecvBufClear(void);

#endif


