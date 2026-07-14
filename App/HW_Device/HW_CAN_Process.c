/*******************************************************************************
 * HW_CAN_Process.c
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "HW_CAN_Process.h"
#include "_01_XSystemManagement.h"
#include "XDebug.h"

/**
 * @brief debugging code
 */
U08 DB_isCANRxFrameDisplayEnabled = 0; // RX/TX frame을 binary로 디스플레이 할텨? 말텨?
U08 DB_isCANTxFrameDisplayEnabled = 0; //

/** @brief 송신처리 : 디버깅하기 위해 한번더 포장함. */
//=====================================================================
void xCAN_SendMessage_1(CanPacket_t msg)
{
    __TASK_TRIGGER_START_Using(TEST_PORT_3, TP_IDX_taskCANTx);

    ICAN1_WriteData(&msg);

    if (DB_isCANTxFrameDisplayEnabled) // debugging code
    {
        xCAN_DisplayDataToBinary(CAN_CH_1, MSG_TX, &msg);
    }

    __TASK_TRIGGER_END_Using(TEST_PORT_3, TP_IDX_taskCANTx);
}

#if 0 // 소형제어기는 1ch 만 있음.
void xCAN_SendMessage_2(CanPacket_t msg)
{
    __TASK_TRIGGER_START_Using(TEST_PORT_3, TP_IDX_taskCANTx);

    ICAN2_WriteData(&msg);

    if (DB_isCANTxFrameDisplayEnabled)
    {
        xCAN_DisplayDataToBinary(CAN_CH_2, MSG_TX, &msg);
    }

    __TASK_TRIGGER_END_Using(TEST_PORT_3, TP_IDX_taskCANTx);
}
#endif

void xCAN_LogError(int channel, const char *errorMsg, const char *functionName)
{
    char buffer[200];

    vTaskDelay(100);
    sprintf(buffer, "%8.2f\tError: %s, channel %d: %s\n", GetTriggerTime(), functionName, channel, errorMsg);
    LOG_MSG_SEND("%s", buffer);
}

/**
 * @brief 디버깅 코드: CAN 통신 Frame 디스플레이 
 * */
void xCAN_DisplayDataToBinary(int channel, teComm_MSG_Type type, CanPacket_t *cp)
{
//    LOG_MSG_SEND("%s"
//                 "%s(%d): "
//                 "ID: 0x%2x, DLC: %d, "
//                 "%s"
//                 "%02x %02x " ANSIESCAPE_TEXT_Yellow "%02x %02x " ANSIESCAPE_TEXT_ORG "%02x %02x " ANSIESCAPE_TEXT_Yellow "%02x %02x" ANSIESCAPE_TEXT_ORG,
//
//                 (type == MSG_TX) ? ANSIESCAPE_TEXT_Green : ANSIESCAPE_TEXT_Red,
//                 (type == MSG_TX) ? "TX" : "RX",
//                 channel,
//                 cp->StdId,
//                 cp->dlc,
//                 ANSIESCAPE_TEXT_ORG,
//                 cp->data[0],
//                 cp->data[1],
//                 cp->data[2],
//                 cp->data[3],
//                 cp->data[4],
//                 cp->data[5],
//                 cp->data[6],
//                 cp->data[7]);
}
