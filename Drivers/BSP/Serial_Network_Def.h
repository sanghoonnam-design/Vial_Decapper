#ifndef __SERIAL_DEF_H__
#define __SERIAL_DEF_H__

typedef enum
{
    BAUDRATE_9600 = 0,
    BAUDRATE_19200,
    BAUDRATE_38400,
    BAUDRATE_57600,
    BAUDRATE_115200,
    BAUDRATE_230400
} BuadRate_t;

typedef enum
{
    PARITY_NONE = 0,
    PARITY_ODD,
    PARITY_EVEN
} Parity_t;

typedef enum
{
    STOPBIT_1 = 0,
    STOPBIT_2
} StopBit_t;

#pragma pack(push,1)
typedef struct Serial
{
    U8 baudrate;
    U8 parity;
    U8 stopbit;
    U8 rsvd;
} Serial_t;

typedef struct Network
{
    U8 ip[4];
    U8 subnet[4];
    U8 gw[4];
    U16 portNum;
} Network_t;

#pragma pack(pop)

#endif //@end: __SERIAL_DEF_H__