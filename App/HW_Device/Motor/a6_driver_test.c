#define __A6_DRIVER_TEST_C__
#include "a6_driver_test.h"
#undef __A6_DRIVER_TEST_C__

#include "HW_RS485_Process.h"
#include "panasonic_a6_driver.h"
#include "_01_XSystemManagement.h"
#include "XDebug.h"
#include <math.h>

VOID TASK_A6_Driver_Test_Handler(void *pvParameters);
static void __attribute__((unused)) _Test_1(void);
static void __attribute__((unused)) _Test_2(void);

static void _MoveSlot(U8 slotNum);

void A6_Driver_test_Init(void)
{
    xTaskCreate(TASK_A6_Driver_Test_Handler, "A6_Test_Task", 512, NULL, tskIDLE_PRIORITY + 2, NULL);
}


U8 _slotRunFlag = 0;
U8 _slotNumber = 0;
VOID TASK_A6_Driver_Test_Handler(void *pvParameters)
{
    //RS485_Init();

    A6_DriverCtrl.cmd = A6DRIVER_CTRL_CMD_NONE;

    vTaskDelay(1000);
    while (1)
    {
//        _Test_1();
    	_Test_2();

    	if(_slotRunFlag)
    	{
    		_slotRunFlag = 0;
    		_MoveSlot(_slotNumber);
    	}

        vTaskDelay(1);
    }
}

#define REV_PULSE    10000
#define SLOT_COUNT   6
#define SLOT_WIDTH   (REV_PULSE / (float)SLOT_COUNT)

static void _MoveSlot(U8 slotNum)
{
	if(slotNum < 0 || slotNum >= SLOT_COUNT)
		return;

	// 1) 현재 위치 읽기 (누적값)
	int32_t currentPos = PanasonicA6_GetPos(1);

	// 2) 현재 회전 내 위치 계산
	int32_t curMod = currentPos % REV_PULSE;
	if(curMod < 0) curMod += REV_PULSE;

	// 3) 목표 슬롯 위치 계산
	float targetF = slotNum * SLOT_WIDTH;
	int32_t targetMod = (int32_t)roundf(targetF); // 정수화

	// 4) 이동해야 하는 상대거리 계산
	//    (타겟-현재). 하지만 wraparound 고려해서 가장 짧은 경로 선택
	int32_t delta = targetMod - curMod;

	// delta 를 -5000 ~ +5000 범위에서 가장 짧은 방향으로 조정
	if(delta > (REV_PULSE / 2))
		delta -= REV_PULSE;      // 반시계가 더 짧음
	else if(delta < -(REV_PULSE / 2))
		delta += REV_PULSE;      // 시계방향이 더 짧음

	// 5) 상대이동 명령 전송
	PanasonicA6_MoveRel(1, delta);
}

typedef enum _Test_1_t
{
    INIT = 0,
    MOVE,
    CHECK_MSG,
    CHECK_IN_POSITION,
	INTERVAL,
	ALRAM_CLEAR,
	A6_STOP,
} _Test_1_t;

static _Test_1_t a6_TestState = A6_STOP;
static U32 a6_mesc = 5000;
static S32 A6_Vel = 500;
static U32 A6_Acc = 1000;
static U32 A6_Dec = 1000;

static U32 MoveDelay = 0;
static U32 ClearAlarmDelay = 500;
static U32 EnableDelay = 50;
static S32 velRPM;
static void __attribute__((unused)) _Test_1(void)
{
    static TickType_t start_tick;

    PanasonicA6_UpdateStatus(1);
    PanasonicA6_UpdateVel(1);

    velRPM = PanasonicA6_GetVel(1);
    switch (a6_TestState)
    {
    case INIT:
    	PanasonicA6_Stop(1);
    	PanasonicA6_SetProfile(1, A6_Vel, A6_Acc, A6_Dec);
        PanasonicA6_ClearAlarm(1);
        vTaskDelay(ClearAlarmDelay);
        PanasonicA6_Enable(1);
        vTaskDelay(EnableDelay);
        LOG_MSG_SEND("Init");
        a6_TestState = MOVE;
        break;
    case MOVE:
		vTaskDelay(MoveDelay);
        PanasonicA6_MoveVel(1, 0, a6_mesc);
        LOG_MSG_SEND("Move");
        start_tick = xTaskGetTickCount();
        a6_TestState = CHECK_MSG;
        break;
    case CHECK_MSG:
		if(PanasonicA6_CheckTransaction(A6_FUNC_INDEX_MOVE_VEL) == 1)
		{
			a6_TestState = CHECK_IN_POSITION;
			start_tick = xTaskGetTickCount();
			LOG_MSG_SEND("Check Msg Done");
			break;
		}
		if ((xTaskGetTickCount() - start_tick) > 1000)
		{
			a6_TestState = MOVE;
			LOG_MSG_SEND("Check Msg Fail");
			break;
		}
        break;
    case CHECK_IN_POSITION:
		if(PanasonicA6_IsAlarm(1) == 1)
		{
			a6_TestState = ALRAM_CLEAR;
			break;
		}

		if((PanasonicA6_IsInPos(1) == 1) && (PanasonicA6_IsMoving(1) == 0) && (PanasonicA6_IsZeroSpeed(1) == 1))
		{
			a6_TestState = INTERVAL;
			LOG_MSG_SEND("In Position Done");
			start_tick = xTaskGetTickCount();
			break;
		}

		if ((xTaskGetTickCount() - start_tick) > (a6_mesc+5000))
		{
			a6_TestState = MOVE;
			LOG_MSG_SEND("In Position Fail");
			break;
		}

        break;
    case INTERVAL:
    	if ((xTaskGetTickCount() - start_tick) > 10*1000)
		{
    		a6_TestState = MOVE;
			LOG_MSG_SEND("Interval");
			break;
		}
    	break;
    case ALRAM_CLEAR :
    	PanasonicA6_ClearAlarm(1);
    	vTaskDelay(ClearAlarmDelay);
    	PanasonicA6_Enable(1);
    	vTaskDelay(EnableDelay);
    	LOG_MSG_SEND("Clear Alarm");
    	a6_TestState = MOVE;
    	break;
    case A6_STOP :
    	PanasonicA6_Stop(1);
    	break;
    }
}

static void _Test_2(void)
{
    PanasonicA6_TestRun(&A6_DriverCtrl);

    __xTime_Per(__50msec)
	{
    	PanasonicA6_UpdateStatus(1);
    	PanasonicA6_UpdatePos(1);
    	PanasonicA6_UpdateVel(1);
	}
}

/*
#if 0 // Motor 기능 개별 제어 테스트
            PanasonicA6_TestRun(&A6_DriverCtrl);
            __xTime_Per(__10msec)
			{
            	PanasonicA6_UpdateStatus(1);
			}
#elif 0

#elif 0 // Motor Move Aging 테스트
        __xTime_At(__1sec)
        {
            PanasonicA6_SetVel(1, 500);
            PanasonicA6_SetAcc(1, 100);
            PanasonicA6_SetDec(1, 100);
            PanasonicA6_Enable(1);
        }

        __xTime_After(__5sec)
        {
            __xTime_Per(__10msec)
            {
                PanasonicA6_UpdateStatus(1);

                U8 servoOn;
                U8 inPos;
                U8 isBusy;

                servoOn = PanasonicA6_IsServoOn(1);
                inPos = PanasonicA6_IsInPos(1);
                isBusy = PanasonicA6_IsMoving(1);

                __xTime_Per(__1sec)
                {
                    if (servoOn && inPos && (!isBusy))
                    {
                        MoveRelCount++;
                        PanasonicA6_MoveRel(1, 20000);

                        LOG_MSG_SEND("Move Count:%d", MoveRelCount);
                    }
                }
            }
        }
#endif
*/
