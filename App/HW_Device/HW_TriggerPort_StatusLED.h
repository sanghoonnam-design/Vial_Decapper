/*******************************************************************************
 * HW_TriggerPort_StatusLED.h
 *
 * Created on: 2025.06.21
 * Author    : RND. Kang PilSoon.
 *
 ******************************************************************************/
#ifndef __HW_TRIGGERPORT_STATUSLED_H__
#define __HW_TRIGGERPORT_STATUSLED_H__
#include "XGlobal.h"
#include "stm32f7xx_hal.h"
#include "_00_HW_Config.h"

/** *************************************************************************
 * @defgroup [Debugging Code 1] 테스트 포트 사용유무 설정 매크로
 * @note     - FW 배포시에서는 모두 주석처리해서 배포 할것
 */
/****************************************************************************/
 /****************************************************************************/
                            // silkscreen : TP1 = GND
#define ENABLE_TEST_PORT_1  // silkscreen : TP11
#define ENABLE_TEST_PORT_2  // silkscreen : TP10
#define ENABLE_TEST_PORT_3  // silkscreen : TP12
#define ENABLE_TEST_PORT_4  // silkscreen : TP16
#define ENABLE_TEST_PORT_5  // silkscreen : TP15
/************************************************************************** */

#define TRIGGER_PORT_1 GPIOD
#define TRIGGER_PIN_1 GPIO_PIN_6
#define TRIGGER_PORT_2 GPIOD
#define TRIGGER_PIN_2 GPIO_PIN_3
#define TRIGGER_PORT_3 GPIOF
#define TRIGGER_PIN_3 GPIO_PIN_10
#define TRIGGER_PORT_4 GPIOG
#define TRIGGER_PIN_4 GPIO_PIN_6
#define TRIGGER_PORT_5 GPIOF
#define TRIGGER_PIN_5 GPIO_PIN_11

#define TP_SET_HIGH(__PORT, __PIN) HAL_GPIO_WritePin(__PORT, __PIN, GPIO_PIN_SET)
#define TP_SET_LOW(__PORT, __PIN) HAL_GPIO_WritePin(__PORT, __PIN, GPIO_PIN_RESET)

#define TP_H_1() (TRIGGER_PORT_1->BSRR = TRIGGER_PIN_1)
#define TP_H_2() (TRIGGER_PORT_2->BSRR = TRIGGER_PIN_2)
#define TP_H_3() (TRIGGER_PORT_3->BSRR = TRIGGER_PIN_3)
#define TP_H_4() (TRIGGER_PORT_4->BSRR = TRIGGER_PIN_4)
#define TP_H_5() (TRIGGER_PORT_5->BSRR = TRIGGER_PIN_5)
#define TP_L_1() (TRIGGER_PORT_1->BSRR = (uint32_t)TRIGGER_PIN_1 << 16U)
#define TP_L_2() (TRIGGER_PORT_2->BSRR = (uint32_t)TRIGGER_PIN_2 << 16U)
#define TP_L_3() (TRIGGER_PORT_3->BSRR = (uint32_t)TRIGGER_PIN_3 << 16U)
#define TP_L_4() (TRIGGER_PORT_4->BSRR = (uint32_t)TRIGGER_PIN_4 << 16U)
#define TP_L_5() (TRIGGER_PORT_5->BSRR = (uint32_t)TRIGGER_PIN_5 << 16U)

typedef enum
{
    TP_IDX_taskUTT /*    */ = 0, // group 1
    TP_IDX_taskUSD /*    */ = 1,
    TP_IDX_taskSDG /*    */ = 2,
    TP_IDX_taskAPC /*    */ = 3,
    TP_IDX_taskSFZ /*    */ = 4,
    TP_IDX_taskRS232Tx /**/ = 5, // group 2
    TP_IDX_taskRS232Rx /**/ = 6,
    TP_IDX_taskRS485Tx /**/ = 7,
    TP_IDX_taskRS485Rx /**/ = 8,
    TP_IDX_taskCANTx /*  */ = 9,
    TP_IDX_taskCANRx /*  */ = 10,
    TP_IDX_taskIdling /* */ = 11, // group 3
    TP_IDX_taskCLI /*    */ = 12,
    TP_IDX_taskMSG /*    */ = 13,
    TP_IDX_taskTCP /*    */ = 14, // group 4
    TP_IDX_COUNT
} teTaskIndex;

typedef struct
{
    GPIO_TypeDef *port; // TODO
    uint16_t pin;
    int enableFlag;
} tsTestPort;
extern tsTestPort testPorts[];

typedef enum // 4개의 테스크 포트
{
    TEST_PORT_1 = 0,
    TEST_PORT_2 = 1,
    TEST_PORT_3 = 2,
    TEST_PORT_4 = 3,
    TEST_PORT_5 = 3,
    TEST_PORT_Count
} teTestPort;

typedef struct // Task 정보
{
    int tpAvailability; // test port 사용여부
    teTaskIndex idxTP;  // 태스크별 인덱스
    const char *pHelp;  // 태스크 설명자료
} tsTestTask;
extern tsTestTask testTasks[];

typedef struct
{
    int enabled;  // cli command
    int param[3]; // cli command param.
    int step[10]; // group process step
} tsXTrigger;
extern tsXTrigger xTrigger;

#define __TASK_TRIGGER_START_Using(__testPort, __taskId)                            \
    do                                                                              \
    {                                                                               \
        if (testTasks[__taskId].tpAvailability && testPorts[__testPort].enableFlag) \
            testPorts[__testPort].port->BSRR = testPorts[__testPort].pin;           \
    } while (0)

#define __TASK_TRIGGER_END_Using(__testPort, __taskId)                                       \
    do                                                                                       \
    {                                                                                        \
        if (testTasks[__taskId].tpAvailability && testPorts[__testPort].enableFlag)          \
            testPorts[__testPort].port->BSRR = (uint32_t)(testPorts[__testPort].pin << 16U); \
    } while (0)

/*******************************************************************************/

void Init_TriggerPort(void);

void XTP_ControlTriggerPort(void);

/*******************************************************************************
 * @brief [Debugging Code 2]
 * @note  - This function uses four LEDs to check the schedule of the main task
 *          at 200 msec intervals.
 *        - If the LED does not work, a problem has occurred in the task.
 *        - The tasks connected to each LED are as follows.
 *------------------------------------------------------------------------------
 *  -[LED-1] TASK_UpdateTriggerTime()
 *  -[LED-2] TASK_UpdateSIGData()
 *  -[LED-3] TASK_Diagnose()
 *  -[LED-4] TASK_ApplicationControl()
 *  -[LED-5] TASK_SamplingFinalization()  // 없음.                            */

void XTP_CheckTaskUsingLED(int LedChannel);
/*******************************************************************************/

#endif //@end: __HW_TRIGGERPORT_STATUSLED_H__
