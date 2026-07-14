/*******************************************************************************
 * HW_TriggerPort_StatusLED.c
 *
 * Created on: 2025.06.21
 * Author    : RND. Kang PilSoon.
 *
 ******************************************************************************/
#include "HW_TriggerPort_StatusLED.h"
#include "_01_XSystemManagement.h"
#include "XDebug.h"

/**
 * @brief Test 포트 설정: 주석 처리된 매크로에 따라 Port/Pin이 enableFlag 설정됨
 * */
tsTestPort testPorts[] = {
#ifdef ENABLE_TEST_PORT_1
    {TRIGGER_PORT_1, TRIGGER_PIN_1, 1},
#else
    {NULL, 0, 0},
#endif
#ifdef ENABLE_TEST_PORT_2
    {TRIGGER_PORT_2, TRIGGER_PIN_2, 1},
#else
    {NULL, 0, 0},
#endif
#ifdef ENABLE_TEST_PORT_3
    {TRIGGER_PORT_3, TRIGGER_PIN_3, 1},
#else
    {NULL, 0, 0},
#endif
#ifdef ENABLE_TEST_PORT_4
    {TRIGGER_PORT_4, TRIGGER_PIN_4, 1},
#else
    {NULL, 0, 0},
#endif
#ifdef ENABLE_TEST_PORT_5
    {TRIGGER_PORT_5, TRIGGER_PIN_5, 1}
#else
    {NULL, 0, 0}
#endif
};

// Task 별로 Trigger를 사용할지 말지 결정
tsTestTask testTasks[] = {
    /* Availability, idxTP,             task mnemonic */
    /* 1  */ {1, /*  */ TP_IDX_taskUTT, /*       */ "Task UTT"},
    /* 2  */ {1, /*  */ TP_IDX_taskUSD, /*       */ "Task USD"},
    /* 3  */ {1, /*  */ TP_IDX_taskSDG, /*       */ "Task SDG"},
    /* 4  */ {1, /*  */ TP_IDX_taskAPC, /*       */ "Task APC"},
    /* 5  */ {1, /*  */ TP_IDX_taskSFZ, /*       */ "Task SFZ"},
    /* 6  */ {0, /*  */ TP_IDX_taskRS232Tx, /*   */ "Task RS232C Tx"},
    /* 7  */ {0, /*  */ TP_IDX_taskRS232Rx, /*   */ "Task RS232C Rx"},
    /* 8  */ {0, /*  */ TP_IDX_taskRS485Tx, /*   */ "Task RS485C Tx"},
    /* 9  */ {0, /*  */ TP_IDX_taskRS485Rx, /*   */ "Task RS485C Rx"},
    /* 10 */ {0, /*  */ TP_IDX_taskCANTx, /*     */ "Task CAN Tx"},
    /* 11 */ {0, /*  */ TP_IDX_taskCANRx, /*     */ "Task CAN Rx"},
    /* 12 */ {0, /*  */ TP_IDX_taskIdling, /*    */ "Task Idle"}, // vApplicationIdleHook()
    /* 13 */ {0, /*  */ TP_IDX_taskCLI, /*       */ "Task CLI"},
    /* 14 */ {0, /*  */ TP_IDX_taskMSG, /*       */ "Task MSG"}, // usb port: debug log
    /* 15 */ {0, /*  */ TP_IDX_taskTCP, /*       */ "Task TCP"}

};

tsXTrigger xTrigger;

void Init_TriggerPort(void)
{
    memset((char *)&xTrigger, 0, sizeof(tsXTrigger));
}

/**
 * @brief command scenario >> TT (group)
 * TT h(H or ?): help
 * TT          : (1)all disable         -> (2)Task Group -> (3)Comm. Group -> (4)etc. Group -> (5) TCP
 * TT 1        : (1)Task Group disable  -> (2)task 1     -> (3)task 2      -> (4)task 3     -> (5)task 4   -> (6)task 5
 * TT 2        : (1)Comm. Group disable -> (2)RS232C Tx  -> (3)RS232C Rx   -> (4)RS485 Tx   -> (5)RS485 Rx -> (6)CAN Tx -> (7)CAN Rx
 * TT 3        : (1)etc. Group disalbe  -> (2)Idling     -> (3)CLI         -> (4)LOG msg
 * TT 4        : (1)TCP Off             -> (2)TCP On
 * TT 5, (testTasks index): Toggle the Availability
 * TT 8        : all trigger port disable/enable(toggle) */
void XTP_ControlTriggerPort(void) // ⚠️TODO: 다시 작성할것!~ 개판이네.
{
    int i;

    if (xTrigger.enabled == ENABLE)
    {
        XTimer_Stop();
        xTrigger.enabled = xDISABLE;

        if (xTrigger.param[0] == 'h' || xTrigger.param[0] == 'H' || xTrigger.param[0] == '?')
        { // h=0x68 (104), H=0x48 (72), ?=0x3F (63)
            LOG_MSG_SEND("[Trigger Port Command Info.]");
            xprintf("\t\t * TT h(H or ?): help");
            xprintf("\t\t * TT   : (1)all off   -> (2)Task Grp.-> (3)Comm.    -> (4)etc. Grp.-> (5) TCP");
            xprintf("\t\t * TT 1 : (1)Task off  -> (2)task 1   -> (3)task 2   -> (4)task 3   -> (5)task 4  -> (6)task 5");
            xprintf("\t\t * TT 2 : (1)Comm. off -> (2)RS232 Tx -> (3)RS232 Rx -> (4)RS485 Tx -> (5)RS485 Rx-> (6)CAN Tx -> (7)CAN Rx");
            xprintf("\t\t * TT 3 : (1)etc. off  -> (2)Idling   -> (3)CLI      -> (4)LOG msg");
            xprintf("\t\t * TT 4 : (1)TCP off   -> (2)TCP On");
            xprintf("\t\t * TT 5, (Tasks Index): Toggle the Availability");

            xprintf("\t\t\t  task UTT     = 1    // group 1");
            xprintf("\t\t\t  task USD     = 2");
            xprintf("\t\t\t  task SDG     = 3");
            xprintf("\t\t\t  task APC     = 4");
            xprintf("\t\t\t  task SFZ     = 5");
            xprintf("\t\t\t  task RS232Tx = 6    // group 2");
            xprintf("\t\t\t  task RS232Rx = 7");
            xprintf("\t\t\t  task RS485Tx = 8");
            xprintf("\t\t\t  task RS485Rx = 9");
            xprintf("\t\t\t  task CANTx   = 10");
            xprintf("\t\t\t  task CANRx   = 11");
            xprintf("\t\t\t  task Idling  = 12   // group 3");
            xprintf("\t\t\t  task CLI     = 13");
            xprintf("\t\t\t  task MSG     = 14");
            xprintf("\t\t\t  task TCP     = 15   // group 4");

            xprintf("\t\t * TT 8 : All Trigger-port disable/disable(toggle)");
            xprintf("\t\t * TT s : read trigger states");
            xprintf("\n\r");
        }

        if (xTrigger.param[0] == 's' || xTrigger.param[0] == 'S')
        { // 상태 출력
            LOG_MSG_SEND("[Task-specific Test-Port activation status]");
            for (i = 0; i < TP_IDX_COUNT; i++)
            {
                xprintf("\t[%d] %d : %s", i + 1, testTasks[i].tpAvailability, testTasks[i].pHelp);
            }
        }

        if (xTrigger.param[0] == 0)
        { // TT 0 : (1)all disable -> (2)Task Group -> (3)Comm. Group -> (4)etc. Group -> (5) TCP
            xTrigger.step[0]++;
            if (xTrigger.step[0] > 5)
                xTrigger.step[0] = 1;

            switch (xTrigger.step[0])
            {
            case 0:
                break;
            case 1:
                for (i = 0; i < TP_IDX_COUNT; i++)
                {
                    testTasks[i].tpAvailability = xDISABLE;
                }
                LOG_MSG_SEND("All task trigger is disabled.");
                break;
            case 2:
                testTasks[TP_IDX_taskUTT].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskUSD].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskSDG].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskAPC].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskSFZ].tpAvailability = ENABLE;
                LOG_MSG_SEND("[main] Task-UTT trigger is enabled.");
                LOG_MSG_SEND("[main] Task-USD trigger is enabled.");
                LOG_MSG_SEND("[main] Task-SDG trigger is enabled.");
                LOG_MSG_SEND("[main] Task-APC trigger is enabled.");
                LOG_MSG_SEND("[main] Task-SFZ trigger is enabled.\n\r");
                break;
            case 3:
                testTasks[TP_IDX_taskRS232Tx].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskRS232Rx].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskRS485Tx].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskRS485Rx].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskCANTx].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskCANRx].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-RS232-Tx trigger is enabled.");
                LOG_MSG_SEND("Task-RS232-Rx trigger is enabled.");
                LOG_MSG_SEND("Task-RS485-Tx trigger is enabled.");
                LOG_MSG_SEND("Task-RS485-Rx trigger is enabled.");
                LOG_MSG_SEND("Task-CAN-Tx trigger is enabled.");
                LOG_MSG_SEND("Task-CAN-Rx trigger is enabled.\n\r");
                break;
            case 4:
                testTasks[TP_IDX_taskIdling].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskCLI].tpAvailability = ENABLE;
                testTasks[TP_IDX_taskMSG].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-Idling trigger is enabled.");
                LOG_MSG_SEND("Task-CLI trigger is enabled.");
                LOG_MSG_SEND("Task-MSG trigger is enabled.");
                break;
            case 5:
                testTasks[TP_IDX_taskTCP].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-TCP trigger is enabled.");
                break;
            default:
                break;
            }
        }

        if (xTrigger.param[0] == 1)
        { // TT 1 : (1)Task Group disable  -> (2)task 1 -> (3)task 2 -> (4)task 3 -> (5)task 4 -> (6)task 5
            xTrigger.step[1]++;
            if (xTrigger.step[1] > 6)
                xTrigger.step[1] = 1;

            switch (xTrigger.step[1])
            {
            case 0:
                break;
            case 1:
                testTasks[TP_IDX_taskUTT].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskUSD].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskSDG].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskAPC].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskSFZ].tpAvailability = xDISABLE;
                LOG_MSG_SEND("Main-Tasks-trigger(All) is disabled.");
                break;
            case 2:
                testTasks[TP_IDX_taskUTT].tpAvailability = ENABLE;
                LOG_MSG_SEND("[main] Task-UTT trigger is enabled.");
                break;
            case 3:
                testTasks[TP_IDX_taskUSD].tpAvailability = ENABLE;
                LOG_MSG_SEND("[main] Task-USD trigger is enabled.");
                break;
            case 4:
                testTasks[TP_IDX_taskSDG].tpAvailability = ENABLE;
                LOG_MSG_SEND("[main] Task-SDG trigger is enabled.");
                break;
            case 5:
                testTasks[TP_IDX_taskAPC].tpAvailability = ENABLE;
                LOG_MSG_SEND("[main] Task-APC trigger is enabled.");
                break;
            case 6:
                testTasks[TP_IDX_taskSFZ].tpAvailability = ENABLE;
                LOG_MSG_SEND("[main] Task-SFZ trigger is enabled.");
                break;
            default:
                break;
            }
        }

        if (xTrigger.param[0] == 2)
        { // TT 2 : (1)Comm. Group disable -> (2)RS232C Tx  (3)RS232C Rx -> (4)RS485 Tx -> (5)RS485 Rx -> (6)CAN Tx -> (7)CAN Rx
            xTrigger.step[2]++;
            if (xTrigger.step[2] > 7)
                xTrigger.step[2] = 0;

            switch (xTrigger.step[2])
            {
            case 0:
                break;
            case 1:
                testTasks[TP_IDX_taskRS232Tx].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskRS232Rx].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskRS485Tx].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskRS485Rx].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskCANTx].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskCANRx].tpAvailability = xDISABLE;
                LOG_MSG_SEND("Task-RS232-Tx trigger is disabled.");
                LOG_MSG_SEND("Task-RS232-Rx trigger is disabled.");
                LOG_MSG_SEND("Task-RS485-Tx trigger is disabled.");
                LOG_MSG_SEND("Task-RS485-Rx trigger is disabled.");
                LOG_MSG_SEND("Task-CAN-Tx trigger is disabled.");
                LOG_MSG_SEND("Task-CAN-Rx trigger is disabled.\n\r");
                break;
            case 2:
                testTasks[TP_IDX_taskRS232Tx].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-RS232-Tx trigger is enabled.");
                break;
            case 3:
                testTasks[TP_IDX_taskRS232Rx].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-RS232-Rx trigger is enabled.");
                break;
            case 4:
                testTasks[TP_IDX_taskRS485Tx].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-RS485-Tx trigger is enabled.");
                break;
            case 5:
                testTasks[TP_IDX_taskRS485Rx].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-RS485-Rx trigger is enabled.");
                break;
            case 6:
                testTasks[TP_IDX_taskCANTx].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-CAN-Tx trigger is enabled.");
                break;
            case 7:
                testTasks[TP_IDX_taskCANRx].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-CAN-Rx trigger is enabled.");
                break;
            default:
                break;
            }
        }

        if (xTrigger.param[0] == 3)
        { // TT 3 : (1)etc. Group disalbe  (2)Idling -> (3)CLI -> (4)LOG msg
            xTrigger.step[3]++;
            if (xTrigger.step[3] > 4)
                xTrigger.step[3] = 1;

            switch (xTrigger.step[3])
            {
            case 0:
                break;
            case 1:
                testTasks[TP_IDX_taskIdling].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskCLI].tpAvailability = xDISABLE;
                testTasks[TP_IDX_taskMSG].tpAvailability = xDISABLE;
                LOG_MSG_SEND("Task-Idling trigger is disabled.");
                LOG_MSG_SEND("Task-CLI trigger is disabled.");
                LOG_MSG_SEND("Task-MSG trigger is disabled.");
                break;
            case 2:
                testTasks[TP_IDX_taskIdling].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-Idling trigger is enabled.");
                break;
            case 3:
                testTasks[TP_IDX_taskCLI].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-CLI trigger is enabled.");
                break;
            case 4:
                testTasks[TP_IDX_taskMSG].tpAvailability = ENABLE;
                LOG_MSG_SEND("Task-MSG trigger is enabled.");
                break;
            default:
                break;
            }
        }

        if (xTrigger.param[0] == 4)
        { // TT 4: (1)TCP Off -> (2)TCP On
            testTasks[TP_IDX_taskTCP].tpAvailability ^= 0x01;

            if (testTasks[TP_IDX_taskTCP].tpAvailability)
            {
                LOG_MSG_SEND("Task-TCP trigger is enabled.");
            }
            else
            {
                LOG_MSG_SEND("Task-TCP trigger is disabled.");
            }
        }

        if (xTrigger.param[0] == 5)
        { // TT 5, (testTasks index): Toggle the Availability
            int index = xTrigger.param[1] - 1;
            if (xTrigger.param[1] > 0 && index < TP_IDX_COUNT)
            {
                testTasks[index].tpAvailability ^= 0x01;
                if (testTasks[index].tpAvailability)
                {
                    LOG_MSG_SEND("%s is enabled.", testTasks[index].pHelp);
                }
                else
                {
                    LOG_MSG_SEND("%s is disabled.", testTasks[index].pHelp);
                }
            }
            else
            { // index error
                ERR_MSG_SEND("%s(): index(1~15) error", __func__);
            }
        }

        // if (xTrigger.param[0] == 6) // resv.
        // {
        // }

        // if (xTrigger.param[0] == 7)
        // {
        // }

        if (xTrigger.param[0] == 8)
        {
            xTrigger.step[8]++;
            if (xTrigger.step[8] > 2)
                xTrigger.step[8] = 1;

            switch (xTrigger.step[8])
            {
            case 0:
                break;
            case 1:
                for (i = 0; i < TP_IDX_COUNT; i++)
                    testTasks[i].tpAvailability = xDISABLE;
                LOG_MSG_SEND("All task trigger is disabled.");
                break;
            case 2:
                for (i = 0; i < TP_IDX_COUNT; i++)
                    testTasks[i].tpAvailability = ENABLE;
                LOG_MSG_SEND("All task trigger is enabled.");
                break;
            default:
                break;
            }
        }

        // if (xTrigger.param[0] == 9)
        // {
        // }
        XTimer_Start();
    }
}

void XTP_CheckTaskUsingLED(int LedChannel)
{
    static int shiftCount[4] = {70, 50, 30, 10};

    //__xTime_Before(__1sec) return;

    if (((gTriggerCount + shiftCount[LedChannel]) % 100) == 0)
    {
        // if (LedChannel != XHW_STATUS_LED_1) // 1번은 pass, TCP 에 할당함.
        {
            LED_Toggle((ELedNum)LedChannel); //
        }
    }
}
