/*******************************************************************************
 * HW_Serial.h
 *
 * Created on: 2025.01.13
 * Author    : RND. Kang PilSoon.
 *
 * Note: RS232C는 CPU 전용 채널 2 channel, FPGA 4 channel로 구성되어 있으며,
 *       두개의 헤더파일의 공통 되는 내용을 정의함.
 *
 ******************************************************************************/
#ifndef __XHW_SERIAL_H__
#define __XHW_SERIAL_H__

#include "XGlobal.h"
#include "IUART.h" // STM32F746ZE의 전용 채널 사용: usb, RS485, TS232

//=================================================================================
/** *******************************************************************************
 * @brief 함수 직접 매칭해서 사용할때 사용
 * *******************************************************************************/
/* IUART.h 사용시 ---------------------------------------------------------------*/
typedef void (*Serial_Write_Func_t)(U08 *data, U16 length);
typedef U32 (*Serial_CheckRecvEnd_Func_t)(void);
typedef U32 (*Serial_GetSize_Func_t)(void);
typedef U08 (*Serial_ReadByte_Func_t)(void);
typedef void (*Serial_ClearRxBuffer_Func_t)(void);

typedef struct // IUART.h 사용시
{
    Serial_Write_Func_t write;
    Serial_CheckRecvEnd_Func_t available; // getSize() 로 사용할것!~
    Serial_GetSize_Func_t getSize;
    Serial_ReadByte_Func_t read;
    Serial_ClearRxBuffer_Func_t rxClear;
} tsXSerial_cpu;

//=================================================================================
void Serial_Init(void);
//=================================================================================

/**
 * @note 아래 함수들 사용시 하용
 */
typedef enum
{
    RS232C_CPU_USB = 0, // CPU, USB 포트
    RS485_CPU_CH = 1,   // CPU, RS485
    TS232_CPU_CH = 2,   // CPU, TS232, 
    SERIAL_MAX_COUNT
} Serial_Channel_t;

void xSerial_Write(Serial_Channel_t ch, U8 *data, U16 length);
U08 xSerial_CheckRecvEnd(Serial_Channel_t ch);
U32 xSerial_GetSize(Serial_Channel_t ch);
U08 xSerial_ReadByte(Serial_Channel_t ch);
void xSerial_RxClear(Serial_Channel_t ch);

#endif //@end: __XHW_SERIAL_H__