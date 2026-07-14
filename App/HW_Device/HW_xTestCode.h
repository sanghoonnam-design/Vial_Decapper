/*******************************************************************************
 * HW_xTestCode.h
 *
 *  Created on: 2024.12.21
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef __HW_XTESTCODE_H__
#define __HW_XTESTCODE_H__
#include "XGlobal.h"

#pragma pack(push, 1)
typedef struct HW_Test_Group
{ /** @note FW_TEST 모드에서만 동작하도록 작성할것!~*/           

    xuint32 isAllStop                 : 1;  // 모든 테스트 정지
    xuint32 resv1                     : 7;  // 예약 비트
                                            //
    xuint32 isLoopbackEnable_All      : 1;  // 모든 디바이스 loopback 실행
    xuint32 isLoopbackEnable_Digital  : 1;  // DO/DI loopback 실행
    xuint32 isLoopbackEnable_RS232C   : 1;  // RS232C 모든채널 loopback 실행
    xuint32 isLoopbackEnable_RS485    : 1;  // RS485 모든채널 loopback   실행
    xuint32 isLoopbackEnable_CAN      : 1;  // CAN 통신 loopback 실행
    xuint32 resv2                     : 3;  // 예약 비트
                                            //
    xuint32 resv3                     : 8;  // 예약 비트
                                            //
    xuint32 isTestEnable_EEPROM       : 1;  // EEPROM 시험
    xuint32 isTestEnable_RAM          : 1;  // RAM 시험
    xuint32 isTestEnable_SD           : 1;  // SD 시험
    xuint32 isTestEnable_Switch       : 1;  // 예약 비트
    xuint32 resv4                     : 4;  // 예약 비트

                                    //=======================================================
} tsXHwTest;                        // = [4 Bytes]
#pragma pack(pop)

extern tsXHwTest xHwTest;

void HWTest_Init(void);

void HWTest(void);
/*  */void HWTest_Digital_Loopback(void);
/*  */void HWTest_RS232C_Loopback(void);
/*  */void HWTset_RS485_Loopback(void);
/*  */void HWTset_CAN_Loopback(void);
// /*  */void HWTset_ADC(void);
/*  */void HWTset_EEPROM(void);
// /*  */void HWTset_SDcard(void);
/*  */void HWTset_Switch(void);


#endif //@end: __HW_XTESTCODE_H__
