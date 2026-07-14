/*******************************************************************************
 * XModbus.h
 *
 *  Created on: 2024.11.12
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef _XMODBUS_H_
#define _XMODBUS_H_
#include "XGlobal.h"

typedef enum MODBUS_FuncCode
{
    FuncCode_Read_CoilStatus_0x01 /*       */ = 0x01,
    FuncCode_Read_InputStatus_0x02 /*      */ = 0x02,
    FuncCode_Read_HoldingRegister_0x03 /*  */ = 0x03,
    FuncCode_Read_InputRegister_0x04 /*    */ = 0x04,
    FuncCode_Write_SingleCoil_0x05 /*      */ = 0x05,
    FuncCode_Write_SingleRegister_0x06 /*  */ = 0x06,
    FuncCode_Write_MultipleRegister_0x10 /**/ = 0x10

} teXMODBUS_FuncCode;

U08 xModbus_CreateTxFrame(
    U08 *frame,
    U08 slaveID,
    teXMODBUS_FuncCode functionCode,
    U16 startAddress,
    U16 quantity,
    U16 *data);

U16 CRC_Calculate(U08 *data, U16 bufLen);
U08 CRC_Validate(U08 *frame, U16 length);

#endif /* @end: _XMODBUS_H_ */
