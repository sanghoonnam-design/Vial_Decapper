/*******************************************************************************
 * Dev_RobotDoor.c
 *
 * Created on: 2025.10.13
 * Author    : RND. Kang YoungJun.
 ******************************************************************************/
#include "Dev_RobotDoor.h"
#include "XSystem_DB.h"
#include "_00_Dev_Config.h"
#include "_01_XSystemManagement.h"
#include "XStateMachine.h"

tsXRobotDoor xDoor;
sMotionStatus_t MotionStatus;

static sMotionCommand_t MotionCmd;
void Initialize_RobotDoor(void)
{
    memset((char *)&xDoor, 0, sizeof(tsXRobotDoor));
    memset((char *)&MotionCmd, 0, sizeof(sMotionCommand_t));
    memset((char *)&MotionStatus, 0, sizeof(sMotionStatus_t));

    xDoor.Motor.Init /*          */ = RobotDoor_Motor_Init;
    xDoor.Motor.Enable /*        */ = RobotDoor_Motor_Enable;
    xDoor.Motor.Disable /*       */ = RobotDoor_Motor_Disable;
    xDoor.Motor.SetSpeed /*      */ = RobotDoor_Motor_SetSpeed;
    xDoor.Motor.SetResolution /* */ = RobotDoor_Motor_SetResolution;
    xDoor.Motor.SetCurrent /*    */ = RobotDoor_Motor_SetCurrent;
    xDoor.Motor.IsMoving /*      */ = RobotDoor_Motor_IsMoving;

    xDoor.MoveToOpen /*          */ = RobotDoor_Door_MoveToOpen;
    xDoor.MoveToClose /*         */ = RobotDoor_Door_MoveToClose;
    xDoor.Stop /*                */ = RobotDoor_Door_Stop;
    xDoor.IsBusy /*              */ = RobotDoor_Door_IsBusy;
    xDoor.IsStop /*              */ = RobotDoor_Door_IsStop;
    xDoor.IsOpen /*              */ = RobotDoor_Door_IsOpen;
    xDoor.GetState /*            */ = RobotDoor_Door_GetState;

    xDoor.Read_Sensor /*         */ = RobotDoor_Read_Sensor;
    xDoor.Update /*              */ = RobotDoor_Update;

    xDoor.StateMachine /*        */ = RobotDoor_StateMachine;
    //=========================================================================
}

void RobotDoor_Motor_Init(void)
{
    F32 runCurrent = xPL.Door.Motor.NormalCurrent_A;                 // [A]
    U8 stopCurrentRate = (U08)xPL.Door.Motor.HoldingCurrent_Percent; // [%]
    U8 resolution = (U08)xPL.Door.Motor.Resolution;                  //

    Drive_SetCurrent((U8)DOOR_STEP_MOTOR_CH, runCurrent, stopCurrentRate);
    Drive_SelMaxCurrent((U8)DOOR_STEP_MOTOR_CH, (U8)CUR_MAX_17A);
    Drive_SetResoultion((U8)DOOR_STEP_MOTOR_CH, resolution);

    RobotDoor_Motor_Enable();
}

void RobotDoor_Motor_Enable(void)
{
    Drive_PowerEnable((U8)DOOR_STEP_MOTOR_CH, (U8)ENABLE);
    xSL.Door.Motor.isEnabled = YES;
    LOG_MSG_SEND("[Step] Motor Enabled.");
}

void RobotDoor_Motor_Disable(void)
{
    Drive_PowerEnable((U8)DOOR_STEP_MOTOR_CH, (U8)DISABLE);
    xSL.Door.Motor.isEnabled = NO;
    LOG_MSG_SEND("[Step] Motor Disabled.");
}

void RobotDoor_Motor_SetSpeed(U32 pps)
{
    xPL.Door.Motor.Speed_pps = pps;
    xPL.Door.Motor.Accel_ppss = xPL.Door.Motor.Speed_pps * 10;
}

void RobotDoor_Motor_SetCurrent(float current_A, int stopCurrent)
{
    xPL.Door.Motor.NormalCurrent_A = current_A;
    xPL.Door.Motor.HoldingCurrent_Percent = stopCurrent;

    Drive_SetCurrent((U8)DOOR_STEP_MOTOR_CH, (F32)xPL.Door.Motor.NormalCurrent_A, (U8)xPL.Door.Motor.HoldingCurrent_Percent);
}

void RobotDoor_Motor_SetResolution(int resoltion)
{
    // cf) 분해능 설정값.
    // typedef enum
    // {
    //     R_51200 = 0,
    //     R_25600 = 1,
    //     R_12800 = 2,
    //     R_6400  = 3,
    //     R_3200  = 4,
    //     R_1600  = 5,
    //     R_800   = 6,
    //     R_400   = 7,
    //     R_200   = 8,
    // } STEP_RESOLUTION;

    xPL.Door.Motor.Resolution = resoltion;

    Drive_SetResoultion(DOOR_STEP_MOTOR_CH, xPL.Door.Motor.Resolution);
}

int RobotDoor_Motor_IsMoving(void)
{
    return xSL.Door.Motor.isMoving;
}

// open 방향으로 이동. 최종위치는 FSM 에서 센서를 읽고 판단함
void RobotDoor_Door_MoveToOpen(void)
{
    MotionCmd.Axis = (U8)DOOR_STEP_MOTOR_CH;
    MotionCmd.Vel = (S32)xPL.Door.Motor.Speed_pps;
    MotionCmd.Acc = (U32)xPL.Door.Motor.Accel_ppss;
    MotionCmd.Pos = (S32)(xPL.Door.Direction * xPL.Door.RelativeDistance_Count);

    Drive_RelMove(&MotionCmd);
}

// close 방향으로 이동. 최종위치는 FSM 에서 센서를 읽고 판단함
void RobotDoor_Door_MoveToClose(void)
{
    MotionCmd.Axis = (U8)DOOR_STEP_MOTOR_CH;
    MotionCmd.Vel = (S32)xPL.Door.Motor.Speed_pps;
    MotionCmd.Acc = (S32)xPL.Door.Motor.Accel_ppss;
    MotionCmd.Pos = (S32)(-1 * xPL.Door.Direction * xPL.Door.RelativeDistance_Count);

    Drive_RelMove(&MotionCmd);
}

void RobotDoor_Door_Stop(void)
{
    MotionCmd.Axis = (U8)DOOR_STEP_MOTOR_CH;
    MotionCmd.Mode = MODE_NONE;
    MotionCmd.Vel = (S32)xPL.Door.Motor.Speed_pps;
    MotionCmd.Acc = (S32)xPL.Door.Motor.Accel_ppss;
    MotionCmd.Pos = 0;
    Drive_Stop(&MotionCmd);
}

int RobotDoor_Door_IsBusy(void)
{
    return xSL.Door.isBusy;
}

int RobotDoor_Door_IsStop(void)
{
    return xSL.Door.isBusy ? NO : YES;
}

int RobotDoor_Door_IsOpen(void)
{
    return (xSL.Door.Status == ROBOTDOOR_STATE_OPEN) ? YES : NO;
}

int RobotDoor_Door_GetState(void)
{
    return xSL.Door.Status;
}

void RobotDoor_Read_Sensor(void)
{
    xCD.Door.CloseSensor = digitalRead(READ_IN, IPIN_ROBOT_DOOR_SENSOR_CLOSE);
    xCD.Door.OpenSensor = digitalRead(READ_IN, IPIN_ROBOT_DOOR_SENSOR_OPEN);
}

void RobotDoor_Update(void)
{
    int open;
    int close;

    RobotDoor_Read_Sensor();

    open = xCD.Door.OpenSensor;
    close = xCD.Door.CloseSensor;

    if (open == HIGH && close == LOW)
    {
        xSL.Door.Status = ROBOTDOOR_STATE_OPEN;
    }
    else if (open == LOW && close == HIGH)
    {
        xSL.Door.Status = ROBOTDOOR_STATE_CLOSED;
    }
    else if (open == LOW && close == LOW)
    {
        xSL.Door.Status = ROBOTDOOR_STATE_MOVING;
    }
    else
    {
        xSL.Door.Status = ROBOTDOOR_STATE_ERROR;
    }

    xSL.Door.isBusy = xSL.Door.isLogicRunning || xSL.Door.Motor.isMoving; // 사실 APC 뒤로 가야 정확함.

    xCD.Door.CurrentPosition = MotionStatus.Pos;
    xSL.Door.Motor.isMoving = MotionStatus.Bit.bMoving;
}

//==========================================================================
//==========================================================================
/** @brief 여기부터 [Robot Door Motion 상태 머신]                            */
//==========================================================================
//==========================================================================
typedef enum
{
    STEP_DOOR_IDLE = 0,     //=========== Idling
    STEP_DOOR_INIT_1,       //=========== Initialize
    STEP_DOOR_INIT_2,       //
    STEP_DOOR_INIT_3,       //
    STEP_DOOR_INIT_4,       //
    STEP_DOOR_INIT_5,       //
    STEP_DOOR_OPEN_1,       //=========== Open
    STEP_DOOR_OPEN_2,       //
    STEP_DOOR_OPEN_3,       //
    STEP_DOOR_OPEN_4,       //
    STEP_DOOR_OPEN_5,       //
    STEP_DOOR_CLOSE_1,      //=========== Close
    STEP_DOOR_CLOSE_2,      //
    STEP_DOOR_CLOSE_3,      //
    STEP_DOOR_CLOSE_4,      //
    STEP_DOOR_CLOSE_5,      //
    STEP_DOOR_STOP,         //=========== Stop
    STEP_DOOR_ABNORMAL_1,   //=========== Abnormal state
    STEP_DOOR_ABNORMAL_2,   //===========
    STEP_DOOR_END_ERR = 99, //=========== ERR 처리
    STEP_DOOR_END_OK = 100  //=========== 종료
} STEP_DOOR;
/******************************************************************************
 * Robot Door Control State Machine
 ******************************************************************************/
void RobotDoor_StateMachine(void)
{
    static U08 step = STEP_DOOR_IDLE;
    static int retryCount = ROBOT_DOOR_RETRY_COUNT;
    static U32 waitCount = 0;
    static U08 errStep = STEP_DOOR_IDLE;
    U08 state;
    U32 timeOut;
    U32 overTime_Close;
    U32 overTime_Open;

    state = (U08)(xSL.Door.Status);
    timeOut = (U32)(xPL.Door.Door_ControlTimeout_ms / 10);
    overTime_Open = (U32)(xPL.Door.OpenSensorOverTime_ms / 10);
    overTime_Close = (U32)(xPL.Door.CloseSensorOverTime_ms / 10);

    //======================================================================
    // 제어 시작 조건
    //======================================================================

    // if (step == STEP_DOOR_IDLE && xSL.Door.isInitialized)
    if (xSL.Door.isInitialized)
    {
        if (xSL.Door.isControllable)
        {
            // if (step == STEP_DOOR_IDLE && xPL.Door.CMD_StartControl == YES)
            if (xPL.Door.CMD_StartControl == YES)
            {
                switch (xPL.Door.TargetMotion)
                {
                case ROBOTDOOR_COMMAND_CLOSE:
                    retryCount = 0;
                    step = STEP_DOOR_CLOSE_1;
                    break;
                case ROBOTDOOR_COMMAND_OPEN:
                    retryCount = 0;
                    step = STEP_DOOR_OPEN_1;
                    break;
                case ROBOTDOOR_COMMAND_STOP:
                    step = STEP_DOOR_STOP;
                    break;
                default:
                    step = STEP_DOOR_IDLE;
                    break;
                }

                retryCount = ROBOT_DOOR_RETRY_COUNT; // reset
                xPL.Door.CMD_StartControl = NO;
            }
        }
    }
    else
    { // 초기화
        if (step == STEP_DOOR_IDLE)
        {
            step = STEP_DOOR_INIT_1;
            xPL.Door.CMD_StartControl = NO;
        }
    }

    //===========================================================================
    // 상태머신 실행부
    //===========================================================================
    switch (step)
    {
    case STEP_DOOR_IDLE:
        break;
    case STEP_DOOR_INIT_1: //=====================================================
        xDoor.Motor.Init();
        step++;
        break;
    case STEP_DOOR_INIT_2:
    case STEP_DOOR_INIT_3:
    case STEP_DOOR_INIT_4:
    case STEP_DOOR_INIT_5:
        LOG_MSG_SEND("StepMotor Init");
        xSL.Door.isInitialized = YES; // 현재는 사실 의미 없음. 
        step = STEP_DOOR_END_OK;
        break;
    case STEP_DOOR_OPEN_1: //======================================================
        xSL.Door.isLogicRunning = YES;
        xDoor.MoveToOpen();
        waitCount = gTriggerCount;
        step = STEP_DOOR_OPEN_2;
        // step = STEP_DOOR_END_OK;
        break;
    case STEP_DOOR_OPEN_2:
        if ((gTriggerCount - waitCount) < timeOut)
        {
            if (state == ROBOTDOOR_STATE_OPEN)
            {

                waitCount = gTriggerCount;
                step++;
            }
        }
        else
        { // time out
            retryCount--;
            xCD.Door.TotalRetryCount++; // for debugging
            if (retryCount >= 0)
            {
                step = STEP_DOOR_OPEN_1;
                ERR_MSG_SEND("[OPEN]: Robot-Door Retry : %d", xCD.Door.TotalRetryCount);
            }
            else
            {
                xDoor.Stop();
                errStep = STEP_DOOR_OPEN_2;
                step = STEP_DOOR_END_ERR;
            }
        }
        break;
    case STEP_DOOR_OPEN_3:
        if ((gTriggerCount - waitCount) >= overTime_Open)
        {
            xDoor.Stop();
            step++;
        }
        break;
    case STEP_DOOR_OPEN_4:
    case STEP_DOOR_OPEN_5:
        LOG_MSG_SEND("[MRDO 1 = open] Completed.");
        step = STEP_DOOR_END_OK;
        break;
    case STEP_DOOR_CLOSE_1: //=====================================================
        xSL.Door.isLogicRunning = YES;
        xDoor.MoveToClose();
        waitCount = gTriggerCount;
        step = STEP_DOOR_CLOSE_2;
        // step = STEP_DOOR_END_OK;
        break;
    case STEP_DOOR_CLOSE_2:
        if ((gTriggerCount - waitCount) < timeOut)
        {
            if (state == ROBOTDOOR_STATE_CLOSED)
            {
                waitCount = gTriggerCount;
                step++;
            }
        }
        else
        { // time out
            retryCount--;
            xCD.Door.TotalRetryCount++; // for debugging
            if (retryCount >= 0)
            {
                step = STEP_DOOR_CLOSE_1;
                ERR_MSG_SEND("[CLOSE]: Robot-Door Retry : %d", xCD.Door.TotalRetryCount);
            }
            else
            {
                xDoor.Stop();
                errStep = STEP_DOOR_CLOSE_2;
                step = STEP_DOOR_END_ERR;
            }
        }
        break;
    case STEP_DOOR_CLOSE_3:
        if ((gTriggerCount - waitCount) >= overTime_Close)
        {
            xDoor.Stop();
            step++;
        }
        break;
    case STEP_DOOR_CLOSE_4:
    case STEP_DOOR_CLOSE_5:
        LOG_MSG_SEND("[MRDO 0 = close] Completed.");
        step = STEP_DOOR_END_OK;
        break;
    case STEP_DOOR_STOP: //=======================================================
        xDoor.Stop();
        LOG_MSG_SEND("[MRDO 2 = stop] Completed.");
        step = STEP_DOOR_END_OK;
        break;
    case STEP_DOOR_ABNORMAL_1: //===================================================
    case STEP_DOOR_ABNORMAL_2: // TODO
        step = STEP_DOOR_END_OK;
        break;
    case STEP_DOOR_END_ERR: //======================================================
        if (errStep >= STEP_DOOR_OPEN_1 && errStep <= STEP_DOOR_OPEN_5)
        {
            SetErrorCode(ERROR_CODE_ROBOT_DOOR_TIMEOUT, __func__, __LINE__);
            ERR_MSG_SEND("[%s()] Timeout → [OPEN ] Step= %d", __func__, errStep);
        }
        else if (errStep >= STEP_DOOR_CLOSE_1 && errStep <= STEP_DOOR_CLOSE_5)
        {
            SetErrorCode(ERROR_CODE_ROBOT_DOOR_TIMEOUT, __func__, __LINE__);
            ERR_MSG_SEND("[%s()] Timeout → [CLOSE] Step= %d", __func__, errStep);
        }
        else
        {
            ;
        }

        xSL.Door.isLogicRunning = NO;
        xPL.Door.TargetMotion = ROBOTDOOR_COMMAND_NONE; // reset
        step = STEP_DOOR_IDLE;
        break;
    case STEP_DOOR_END_OK: //=======================================================
        xSL.Door.isLogicRunning = NO;
        xPL.Door.TargetMotion = ROBOTDOOR_COMMAND_NONE; // reset
        step = STEP_DOOR_IDLE;
        break;
    default:
        break;
    }
}
