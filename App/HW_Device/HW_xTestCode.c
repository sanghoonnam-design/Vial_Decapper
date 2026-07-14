/*******************************************************************************
 * HW_xTestCode.c
 *
 *  Created on: 2024.12.21
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "HW_xTestCode.h"
#include "_01_HW_and_Device.h"
#include "_01_XSystemManagement.h"
#include "XBuffer.h"
#include "XDebug.h"

tsXHwTest xHwTest;

#define TestStep_IDLE (0)
#define TestStep_End (100)

void HWTest_Init(void)
{
    memset((char *)&xHwTest, 0x0, sizeof(tsXHwTest));
    // xString = XBuffer_Create(100);

    xHwTest.isLoopbackEnable_Digital = YES;
}

void HWTest(void)
{ // timer, task도 테스트 할겸.. 타이머 돌린 상태에서 테스트
    HWTest_Digital_Loopback();
    // HWTest_RS232C_Loopback();
    // HWTset_RS485_Loopback();
    // HWTset_CAN_Loopback();

    // HWTset_EEPROM();

    // HWTset_Switch();

    if (xHwTest.isAllStop == YES)
        xHwTest.isAllStop = RESET;
}

void HWTest_Digital_Loopback(void)
{

}

void HWTest_RS232C_Loopback(void)
{
}

void HWTset_RS485_Loopback(void)
{
}

void HWTset_CAN_Loopback(void)
{
}

#define CHUNK_SIZE 512 // 1KB 단위로 처리
void HWTset_EEPROM(void)
{
//    U08 writeData[CHUNK_SIZE] = {0};
//    U08 readData[CHUNK_SIZE] = {0};
//
//    if (xHwTest.isTestEnable_EEPROM == YES)
//    {
//        XTimer_Stop();
//        xHwTest.isTestEnable_EEPROM = NO;
//
//        for (uint32_t i = 0; i < CHUNK_SIZE; i++)
//        {
//            writeData[i] = (uint8_t)(i & 0xFF);
//        }
//
//        LOG_MSG_SEND_N(ANSIESCAPE_TEXT_Yellow"Writing and verifying EEPROM in chunks(%dbyte)..."ANSIESCAPE_TEXT_ORG, CHUNK_SIZE);
//        for (uint32_t addr = 0; addr < EEPROM_SIZE; addr += CHUNK_SIZE)
//        {
//            LOG_MSG_SEND("[addr: %04X] Writing ...", addr);
//            EEPROM_WriteBytes(addr, writeData, CHUNK_SIZE);
//            LOG_MSG_SEND("[addr: %04X] Reading ...", addr);
//            EEPROM_ReadBytes(addr, readData, CHUNK_SIZE);
//
//            // if (addr == 512)
//            //     readData[4] = 100;  // fail case test code
//
//            if (memcmp(writeData, readData, CHUNK_SIZE) != 0)
//            {
//                LOG_MSG_SEND("EEPROM Test Failed at address 0x%X!", addr);
//                return;
//            }
//        }
//        LOG_MSG_SEND(ANSIESCAPE_TEXT_Yellow"EEPROM Full Test Passed!"ANSIESCAPE_TEXT_ORG);
//
//        __prompt();
//        XTimer_Start();
//
//    }
}

void HWTset_Switch(void)
{
//    U08 sw[XHW_SWITCH_NUM] = {0, 0, 0, 0};
//
//    if (xHwTest.isAllStop == YES)
//    {
//        xHwTest.isTestEnable_Switch = NO;
//    }
//
//    if (xHwTest.isTestEnable_Switch == YES)
//    {
//        __xTime_Per(__1sec)
//        {
//            FOR_ALL_SWITCH sw[i] = SW_Read((ESwNum)(i + 1));
//            __newLine();
//            xprintf("SW:" ANSIESCAPE_TEXT_Yellow " %d %d %d %d" ANSIESCAPE_TEXT_ORG, sw[0], sw[1], sw[2], sw[3]);
//        }
//    }
}
