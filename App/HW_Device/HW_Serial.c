/*******************************************************************************
 * HW_Serial.c
 *
 * Created on: 2025.01.13
 * Author    : RND. Kang PilSoon.
 *
 ******************************************************************************/

#include "HW_Serial.h"
#include "XDebug.h"

void Serial_Init(void)
{
    ;
}

void xSerial_Write(Serial_Channel_t ch, U8 *data, U16 length)
{
    if (ch >= SERIAL_MAX_COUNT || data == NULL || length == 0)
        return;

    switch (ch)
    {
    case RS232C_CPU_USB:
#if 1
        IUART1_WriteBytes(data, length);
#else
        U16 len = (U16)strlen((const char *)data);
        IUART1_WriteBytes(data, len);
#endif
        break;
    case RS485_CPU_CH:
        IUART2_WriteBytes(data, length);
        break;
    case TS232_CPU_CH:
        IUART4_WriteBytes(data, length);
        break;
    default:
        ERR_MSG_SEND("%s() : Invalid Channel.", __func__);
        break;
    }
}

U08 xSerial_CheckRecvEnd(Serial_Channel_t ch)
{
    U08 rtn = 0;

    if (ch >= SERIAL_MAX_COUNT)
        return 0;

    switch (ch)
    {
    case RS232C_CPU_USB:
        rtn = IUART1_CheckRecvEnd();
        break;
    case RS485_CPU_CH:
        rtn = IUART2_CheckRecvEnd();
        break;
    case TS232_CPU_CH:
        rtn = IUART4_CheckRecvEnd();
        break;
    default:
        ERR_MSG_SEND("%s() : Invalid Channel.", __func__);
        break;
    }

    return rtn;
}

U32 xSerial_GetSize(Serial_Channel_t ch)
{
    U32 size = 0;

    if (ch >= SERIAL_MAX_COUNT)
        return 0;

    switch (ch)
    {
    case RS232C_CPU_USB:
        size = IUART1_GetRecvSize();
        break;
    case RS485_CPU_CH: // RS485
        size = IUART2_GetRecvSize();
        break;
    case TS232_CPU_CH:
        size = IUART4_GetRecvSize();
        break;
    default:
        ERR_MSG_SEND("%s() : Invalid Channel.", __func__);
        break;
    }

    return size;
}

U08 xSerial_ReadByte(Serial_Channel_t ch)
{
    U08 byte = 0x00;

    if (ch >= SERIAL_MAX_COUNT)
        return 0;

    switch (ch)
    {
    case RS232C_CPU_USB:
        byte = IUART1_ReadByte();
        break;
    case RS485_CPU_CH:
        byte = IUART2_ReadByte();
        break;
    case TS232_CPU_CH:
        byte = IUART4_ReadByte();
        break;
    default:
        ERR_MSG_SEND("%s() : Invalid Channel.", __func__);
        break;
    }

    return byte;
}

void xSerial_RxClear(Serial_Channel_t ch)
{
    if (ch >= SERIAL_MAX_COUNT)
        return;

    switch (ch)
    {
    case RS232C_CPU_USB:
        IUART1_RecvBufClear();
        break;
    case RS485_CPU_CH:
        IUART2_RecvBufClear();
        break;
    case TS232_CPU_CH:
        IUART4_RecvBufClear();
        break;
    default:
        ERR_MSG_SEND("%s() : Invalid Channel.", __func__);
        break;
    }
}