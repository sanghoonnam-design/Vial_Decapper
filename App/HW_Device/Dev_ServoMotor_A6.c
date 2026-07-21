/*******************************************************************************
 * Dev_ServoMotor_A6.h
 *
 * Created on: 2025.11.01
 * Author    : RND. Kang YoungJun
 *
 ******************************************************************************/
#include "Dev_ServoMotor_A6.h"
#include "XSystem_DB.h"
#include "XSystemInfo.h"
#include "XStateMachine.h"

#include "panasonic_a6_driver.h"
#include "math.h"

tsXServoA6 xServoA6;

int isSavingParametersA6 = NO;

void Initialize_ServoA6(void)
{
    memset((char *)&xServoA6, 0, sizeof(tsXServoA6));

    xServoA6.Enable /*              */ = ServoA6_Enable;
    xServoA6.Disable /*             */ = ServoA6_Disable;
    xServoA6.Origin /*              */ = ServoA6_Origin;
    xServoA6.Home /*                */ = ServoA6_Home;
    xServoA6.Move_Jog /*            */ = ServoA6_Move_Jog;
    xServoA6.Move_Abs /*            */ = ServoA6_Move_Abs;
    xServoA6.Move_Rel /*            */ = ServoA6_Move_Rel;
    xServoA6.Move_Cent /*           */ = ServoA6_Move_Cent;
    xServoA6.Move_Slot /*           */ = ServoA6_Move_Slot;
    xServoA6.Move_Stop /*           */ = ServoA6_Move_Stop;
    xServoA6.Move_Estop /*          */ = ServoA6_Move_Estop;
    xServoA6.Set_Param_Origin /*    */ = ServoA6_Set_Param_Origin;
    xServoA6.Set_Param_Home /*      */ = ServoA6_Set_Param_Home;
    xServoA6.Set_Param_Jog /*       */ = ServoA6_Set_Param_Jog;
    xServoA6.Set_Param_Slot /*      */ = ServoA6_Set_Param_Slot;
    xServoA6.Set_Param_Cent /*      */ = ServoA6_Set_Param_Cent;
    xServoA6.Set_Param_Abs /*       */ = ServoA6_Set_Param_Abs;
    xServoA6.Set_Param_Rel /*       */ = ServoA6_Set_Param_Rel;
    xServoA6.AlarmClear /*          */ = ServoA6_AlarmClear;
    xServoA6.Move_Estop /*          */ = ServoA6_Move_Estop;

    xServoA6.Update_RS485_Tx /*     */ = ServoA6_Update_RS485_Tx;

    xServoA6.IsHomed /*             */ = ServoA6_IsHomed;
    xServoA6.IsBusy /*              */ = ServoA6_IsBusy;
    xServoA6.IsStop /*              */ = ServoA6_IsStop;
    xServoA6.IsDriverError /*       */ = ServoA6_IsDriverError;
    xServoA6.IsServoOn /*           */ = ServoA6_IsServoOn;
    xServoA6.IsEnabled /*           */ = ServoA6_IsEnabled;
    xServoA6.GetPosition_SlotNum /* */ = ServoA6_GetPosition_SlotNum;
    xServoA6.ErrorMonitor /*        */ = ServoA6_ErrorMonitor;

    xServoA6.Update /*              */ = ServoA6_Update;

    xServoA6.StateMachine /*        */ = ServoA6_StateMachine;
    //=========================================================================
}

void ServoA6_Enable(void)
{
    PanasonicA6_Enable(A6_ID);
}

void ServoA6_Disable(void)
{
    PanasonicA6_Disable(A6_ID);
}

void ServoA6_Origin(void)
{ // parameter setting 값이 다름. 시퀀스 참조
    PanasonicA6_Home(A6_ID, 0);
}

void ServoA6_Home(void)
{
    PanasonicA6_Home(A6_ID, 0);
}

void ServoA6_Move_Jog(S32 pulse)
{
    PanasonicA6_MoveRel(A6_ID, pulse);
}

void ServoA6_Move_Abs(S32 pulse)
{
    PanasonicA6_MoveAbs(A6_ID, pulse);
}

void ServoA6_Move_Rel(S32 pulse)
{
    PanasonicA6_MoveRel(A6_ID, pulse);
}

void ServoA6_Move_Cent(U08 dir, U32 msec)
{
    PanasonicA6_MoveVel(A6_ID, dir, msec);
}

void ServoA6_Move_Slot(S32 pulse)
{
    PanasonicA6_MoveRel(A6_ID, pulse);
}

void ServoA6_Move_Stop(void)
{
    PanasonicA6_Stop(A6_ID);
}

void ServoA6_Move_Estop(void)
{
    PanasonicA6_EStop(A6_ID);
}

void ServoA6_Set_Param_Origin(void)
{
    S32 offset = 0;
    U16 velH = xPL.ServoA6.Home.Speed_Forward_rpm;
    U16 velL = xPL.ServoA6.Home.Speed_Backward_rpm;
    U16 acc = xPL.ServoA6.Home.Time_Accel_millis;

    PanasonicA6_SetHomeParam(A6_ID, offset, velH, velL, acc);
}

/*
ret = 0, pdPASS
ret = 1, errQUEUE_FULL
*/
void ServoA6_Set_Param_Home(void)
{
    S32 offset = 0; // xPL.ServoA6.Home.Offset;
    U16 velH = xPL.ServoA6.Home.Speed_Forward_rpm;
    U16 velL = xPL.ServoA6.Home.Speed_Backward_rpm;
    U16 acc = xPL.ServoA6.Home.Time_Accel_millis;

    PanasonicA6_SetHomeParam(A6_ID, offset, velH, velL, acc);
}

void ServoA6_Set_Param_Jog(void)
{
    U16 vel = xPL.ServoA6.Jog.Speed_rpm;
    U16 acc = xPL.ServoA6.Jog.Time_Accel_millis;
    U16 dec = xPL.ServoA6.Jog.Time_Decel_millis;

    //    PanasonicA6_SetJogProfile(A6_ID, vel, acc, dec);
    PanasonicA6_SetProfile(A6_ID, vel, acc, dec);
}

void ServoA6_Set_Param_Slot(void)
{
    U16 vel = xPL.ServoA6.Slot.Speed_rpm;
    U16 acc = xPL.ServoA6.Slot.Time_Accel_millis;
    U16 dec = xPL.ServoA6.Slot.Time_Decel_millis;

    PanasonicA6_SetProfile(A6_ID, vel, acc, dec);
}

void ServoA6_Set_Param_Cent(U32 rpm, U32 accTime_ms, U32 decTime_ms)
{
    PanasonicA6_SetProfile(A6_ID, rpm, accTime_ms, decTime_ms);
}

void ServoA6_Set_Param_Abs(void)
{
    U16 vel = xPL.ServoA6.BaseMove.Speed_rpm;
    U16 acc = xPL.ServoA6.BaseMove.Time_Accel_millis;
    U16 dec = xPL.ServoA6.BaseMove.Time_Decel_millis;

    PanasonicA6_SetProfile(A6_ID, vel, acc, dec);
}

void ServoA6_Set_Param_Rel(void)
{
    U16 vel = xPL.ServoA6.BaseMove.Speed_rpm;
    U16 acc = xPL.ServoA6.BaseMove.Time_Accel_millis;
    U16 dec = xPL.ServoA6.BaseMove.Time_Decel_millis;

    PanasonicA6_SetProfile(A6_ID, vel, acc, dec);
}

void ServoA6_AlarmClear(void)
{
    PanasonicA6_ClearAlarm(A6_ID);
}

void ServoA6_Update_RS485_Tx(void)
{
    PanasonicA6_UpdateStatus(A6_ID);
    PanasonicA6_UpdatePos(A6_ID);
    PanasonicA6_UpdateVel(A6_ID);
}

int ServoA6_IsHomed(void)
{
    // return xSL.ServoA6.Driver.isHomed;
    return xSL.ServoA6.isHomed;
}

int ServoA6_IsBusy(void)
{
    return xSL.ServoA6.isBusy;
}

int ServoA6_IsStop(void)
{
    return xSL.ServoA6.isBusy ? NO : YES;
}

int ServoA6_IsDriverError(void)
{
    return xSL.ServoA6.Driver.isError;
}

int ServoA6_IsServoOn(void)
{
    return xSL.ServoA6.Driver.isEnabled;
}

int ServoA6_IsEnabled(void)
{
    return xSL.ServoA6.Driver.isEnabled;
}

int ServoA6_GetPosition_SlotNum(void)
{
    return xCD.ServoA6.CurrentPosition.SlotNum;
}

void ServoA6_ErrorMonitor(void)
{
    static U16 old_ErrorCode = 0;
    U16 errorCode;

    if (xSL.ServoA6.Driver.isError)
    {
        SetErrorCode(ERROR_CODE_A6_DRIVER_ERROR, __func__, __LINE__);
    }

    errorCode = PanasonicA6_GetErrorCode(A6_ID);

    if (errorCode != old_ErrorCode)
    {
        ERR_MSG_SEND("Error Code (A6): %d", errorCode);
    }

    old_ErrorCode = errorCode;
}

void ServoA6_Update(void)
{
    // [SL] update
    xSL.ServoA6.Driver.isConnected /*  */ = PanasonicA6_IsConnected(A6_ID);
    xSL.ServoA6.Driver.isEnabled /*    */ = PanasonicA6_IsServoOn(A6_ID);
    xSL.ServoA6.Driver.isMoving /*     */ = PanasonicA6_IsMoving(A6_ID);
    xSL.ServoA6.Driver.isHomed /*      */ = PanasonicA6_IsHomeCompelte(A6_ID);
    xSL.ServoA6.Driver.isInPosition /* */ = PanasonicA6_IsInPos(A6_ID);
    xSL.ServoA6.Driver.isError /*      */ = PanasonicA6_IsAlarm(A6_ID);
    xSL.ServoA6.Driver.ErrorCode /*    */ = PanasonicA6_GetErrorCode(A6_ID);

    // [예외 처리] 통신 끊김이 명령을 영구 차단하지 못하게 함.
    if (xSL.ServoA6.Driver.isConnected == NO)
    {
        xSL.ServoA6.Driver.isMoving = NO;
    }

    xSL.ServoA6.isBusy = xSL.ServoA6.isLogicRunning || xSL.ServoA6.Driver.isMoving;

    // [CD] update
    xCD.ServoA6.CurrentSpeed.rpm = fabs(PanasonicA6_GetVel(A6_ID));
    if (xCD.ServoA6.CurrentSpeed.rpm < 0.02)
        xCD.ServoA6.CurrentSpeed.rpm = 0.0f;
    xCD.ServoA6.CurrentSpeed.pps = xCD.ServoA6.CurrentSpeed.rpm * PANASONIC_A6_PPR / 60;
    xCD.ServoA6.CurrentSpeed.rps = xCD.ServoA6.CurrentSpeed.rpm / 60;
    xCD.ServoA6.CurrentPosition.Pulse = PanasonicA6_GetPos(A6_ID);

    // TODO
    // xCD.ServoA6.Centrifugal_Force = ;
    // xCD.ServoA6.Time.EstimatedTime_Count = ;
}

//==========================================================================
//==========================================================================
//==========================================================================
/** @brief 여기부터 상태 머신                                                */
//==========================================================================
//==========================================================================
//==========================================================================
static U08 step = STEP_INIT; // 초기화 부터 시작
static U08 subStep = 0;
static U08 returnStep = STEP_IDLE;
static U08 returnSubStep = 0;
static U32 waitCount = 0;
static U32 delayTime = 0;

// Step 함수 선언
static void Step_Idle(void);       //
static void Step_Init(void);       //
static void Step_Enable(void);     //
static void Step_Disable(void);    //
static void Step_Org(void);        // origin = home 센서 위치
static void Step_Home(void);       // home = origin + offset =  slot 1 위치
static void Step_Jog(void);        // jog 동작
static void Step_Abs(void);        // 절대이동
static void Step_Rel(void);        // 상대이동
static void Step_Cent(void);       // CENT 명령
static void Step_Slot(void);       // slot 이동
static void Step_Stop(void);       // 정지
static void Step_Estop(void);      // 긴급 정지
static void Step_Reset(void);      // safety reset logic,
                                   // [절차] AC 220V 차단 -> 에러발생 : servo off -> error clear -> servo on -> 상태업데이트
static void Step_AlarmClear(void); // Clear error
static void Step_Abnormal(void);   //
static void Step_End_ERR(void);    //
static void Step_End_OK(void);     //

// ControlMode → Step 매핑
static const teSTEP_ServoA6 controlModeToStep[] = {
    [A6_CONTROL_MODE_NONE] /*      */ = STEP_IDLE,
    [A6_CONTROL_MODE_INIT] /*      */ = STEP_INIT,
    [A6_CONTROL_MODE_ENABLE] /*    */ = STEP_ENABLE,
    [A6_CONTROL_MODE_DISABLE] /*   */ = STEP_DISABLE,
    [A6_CONTROL_MODE_ORG] /*       */ = STEP_ORG,
    [A6_CONTROL_MODE_HOME] /*      */ = STEP_HOME,
    [A6_CONTROL_MODE_JOG] /*       */ = STEP_JOG,
    [A6_CONTROL_MODE_ABS] /*       */ = STEP_ABS,
    [A6_CONTROL_MODE_REL] /*       */ = STEP_REL,
    [A6_CONTROL_MODE_CENT] /*      */ = STEP_CENT,
    [A6_CONTROL_MODE_SLOT] /*      */ = STEP_SLOT,
    [A6_CONTROL_MODE_STOP] /*      */ = STEP_STOP,
    [A6_CONTROL_MODE_ESTOP] /*     */ = STEP_ESTOP,
    [A6_CONTROL_MODE_RESET] /*     */ = STEP_RESET,
    [A6_CONTROL_MODE_ERRCLEAR] /*  */ = STEP_ERR_CLEAR};

// Step 테이블
static FSM_StepHandler stepFunctionTable[] = {
    [STEP_IDLE] /*                 */ = Step_Idle,
    [STEP_INIT] /*                 */ = Step_Init,
    [STEP_ENABLE] /*               */ = Step_Enable,
    [STEP_DISABLE] /*              */ = Step_Disable,
    [STEP_ORG] /*                  */ = Step_Org,
    [STEP_HOME] /*                 */ = Step_Home,
    [STEP_JOG] /*                  */ = Step_Jog,
    [STEP_ABS] /*                  */ = Step_Abs,
    [STEP_REL] /*                  */ = Step_Rel,
    [STEP_CENT] /*                 */ = Step_Cent,
    [STEP_SLOT] /*                 */ = Step_Slot,
    [STEP_STOP] /*                 */ = Step_Stop,
    [STEP_ESTOP] /*                */ = Step_Estop,
    [STEP_RESET] /*                */ = Step_Reset,
    [STEP_ERR_CLEAR] /*            */ = Step_AlarmClear,
    [STEP_ABNORMAL] /*             */ = Step_Abnormal,
    [STEP_END_ERR] /*              */ = Step_End_ERR,
    [STEP_END_OK] /*               */ = Step_End_OK};

//==========================================================================
// 메인 상태머신
//==========================================================================
void ServoA6_StateMachine(void)
{
    int isControllable = xSL.ServoA6.isControllable;
    static int old_isControllable = NO;
    static int initializedOnce = NO; // 최초 상태 변화시 진입 방지

    if (xSL.ServoA6.isInitialized)
    {
        if (isControllable)
        {
            // 새 명령 또는 STOP/ESTOP 즉시 실행
            if (xPL.ServoA6.CMD_StartControl == YES)
            {
                if (xSL.ServoA6.isLogicRunning == NO && step == STEP_IDLE)
                {
                    if (xPL.ServoA6.CMD_ControlMode < ARRAY_SIZE(controlModeToStep))
                        step = controlModeToStep[xPL.ServoA6.CMD_ControlMode];
                    else
                        step = STEP_IDLE;

                    subStep = 0;
                    xPL.ServoA6.CMD_StartControl = NO;
                }
                else if ((xPL.ServoA6.CMD_ControlMode == A6_CONTROL_MODE_STOP ||
                          xPL.ServoA6.CMD_ControlMode == A6_CONTROL_MODE_ESTOP))
                {
                    step = controlModeToStep[xPL.ServoA6.CMD_ControlMode];
                    subStep = 0;
                    xPL.ServoA6.CMD_StartControl = NO;
                }
                else
                {
                    xPL.ServoA6.CMD_StartControl = NO;
                }
            }

            if (step < ARRAY_SIZE(stepFunctionTable) && // step 이 있는지 확인.
                stepFunctionTable[step])                // step function 이 있는지 확인.
            {
                stepFunctionTable[step]();
            }
        }
        else
        { // abnormal 처리
            if (initializedOnce && isControllable != old_isControllable)
            {
                returnStep = step;
                returnSubStep = subStep;
                step = STEP_ABNORMAL;
                stepFunctionTable[step]();
            }
            initializedOnce = YES;
        }
    }
    else
    { // 초기화 : 끝날때까지 계속 호출
        if (step == STEP_INIT || step == STEP_IDLE)
        {
            stepFunctionTable[STEP_INIT]();
            xPL.ServoA6.CMD_StartControl = NO;
        }
        else
        { // 초기화 실패시 실행
            stepFunctionTable[step]();
        }
    }

    xServoA6.ErrorMonitor(); // debugging code: display error .

    old_isControllable = isControllable;
}

static void Step_Idle(void)
{
    xSL.ServoA6.isLogicRunning = NO;
    return;
}

static void Step_Init(void)
{
    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        xSL.ServoA6.isInitialized = NO;

        xServoA6.Set_Param_Home(); // 홈 파라미터 설정
        LOG_MSG_SEND("[Servo Init] Set Home Param.");
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 1:
        // timeout check
        if ((gTriggerCount - waitCount) > TIMEOUT_DEFAULT)
        {
            SetErrorCode(ERROR_CODE_A6_SERVO_TIMEOUT, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }

        // servo ready 상태 확인 후 servo on
        if (PanasonicA6_IsServoReady(A6_ID))
        {
            xServoA6.Enable();
            subStep++;
        }
        break;
    case 2:
        subStep++;
        break;
    case 3:
        subStep++;
        break;
    case 4:
        subStep++;
        break;
    case 5:
        waitCount = gTriggerCount;
        subStep = 19;
        break;
    case 19:
        if ((gTriggerCount - waitCount) > __3sec)
        {
            SetErrorCode(ERROR_CODE_A6_SERVO_ON_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xSL.ServoA6.Driver.isEnabled)
        {
            LOG_MSG_SEND("[Servo Init] Servo On.");
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        xSL.ServoA6.isLogicRunning = NO;
        xSL.ServoA6.isInitialized = YES;
        step = STEP_END_OK;
        LOG_MSG_SEND("[Servo Init] Completed.");
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Enable(void)
{
    switch (subStep)
    {
    case 0:
        subStep++;
        break;
    case 1:
        xSL.ServoA6.isLogicRunning = YES;
        xServoA6.Enable();
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 2:
        if ((gTriggerCount - waitCount) > __3sec)
        {
            SetErrorCode(ERROR_CODE_A6_SERVO_ON_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xSL.ServoA6.Driver.isEnabled)
        {
            waitCount = gTriggerCount;
            subStep = 20;
        }
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        LOG_MSG_SEND("[Servo ENABLE] Servo On");
        xSL.ServoA6.isLogicRunning = NO;
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Disable(void)
{
    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        xServoA6.Disable();
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 1:
        if ((gTriggerCount - waitCount) > __3sec)
        {
            SetErrorCode(ERROR_CODE_A6_SERVO_OFF_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (!xSL.ServoA6.Driver.isEnabled)
        {
            waitCount = gTriggerCount;
            subStep = 20;
        }
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        LOG_MSG_SEND("[Servo DISABLE] Servo Off");
        xSL.ServoA6.isLogicRunning = NO;
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Org(void)
{
    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        if (xServoA6.IsDriverError() == YES)
        {
            LOG_MSG_SEND("[Servo ORG] xServoA6.AlarmClear() - start");
            xServoA6.AlarmClear();
            waitCount = gTriggerCount;
            subStep++;
        }
        else
        {
            subStep = 2;
        }
        break;
    case 1:
        if ((gTriggerCount - waitCount) > __2sec)
        {
            SetErrorCode(ERROR_CODE_A6_DRIVER_ERROR, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xServoA6.IsDriverError() == NO)
        {
            LOG_MSG_SEND("[Servo ORG] xServoA6.AlarmClear() - end");
            ClearError();
            subStep++;
        }
        break;
    case 2: //================================================
        if (!xServoA6.IsServoOn())
        {
            LOG_MSG_SEND("[Servo ORG] xServoA6.Enable() - start.");
            xServoA6.Enable();
            waitCount = gTriggerCount;
            subStep++;
        }
        else
        {
            subStep = 4;
        }
        break;
    case 3:
        if ((gTriggerCount - waitCount) > __2sec)
        {
            SetErrorCode(ERROR_CODE_A6_SERVO_OFF, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xServoA6.IsServoOn())
        {
            LOG_MSG_SEND("[Servo ORG] xServoA6.Enable() - end.");
            ClearError();
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 4:
        if ((gTriggerCount - waitCount) > __500msec)
            subStep++;
        break;
    case 5: //================================================
        xServoA6.Origin();
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 6:
        if ((gTriggerCount - waitCount) > __2sec)
        { // 상태 갱신 delay
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 7:
        if ((gTriggerCount - waitCount) > TIMEOUT_HOME)
        {
            xServoA6.Move_Stop();
            SetErrorCode(ERROR_CODE_A6_SERVO_HOME_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xSL.ServoA6.Driver.isHomed)
        {
            waitCount = gTriggerCount;
            subStep = 20;
        }
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        LOG_MSG_SEND("[Servo ORG] Completed.");
        xSL.ServoA6.isLogicRunning = NO;
        xCD.ServoA6.CurrentPosition.SlotNum = SLOT_UNKNOWN;
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Home(void)
{
    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        xSL.ServoA6.isHomed = NO;
        // xServoA6.Set_Param_Home(); // 적용 안됨.
        // [주의] xServoA6.Set_Param_Home() 는 적용후, PanasonicA6_EEPROM_Write() 을 실행후 -> 전원 off->on 해야함
        //       saveA6 명령 참조
        if (xServoA6.IsDriverError() == YES)
        {
            LOG_MSG_SEND("[Servo ORG] xServoA6.AlarmClear() - start");
            xServoA6.AlarmClear();
            waitCount = gTriggerCount;
            subStep++;
        }
        else
        {
            subStep = 2;
        }
        break;
    case 1:
        if ((gTriggerCount - waitCount) > __2sec)
        {
            SetErrorCode(ERROR_CODE_A6_DRIVER_ERROR, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xServoA6.IsDriverError() == NO)
        {
            LOG_MSG_SEND("[Servo ORG] xServoA6.AlarmClear() - end");
            ClearError();
            subStep++;
        }
        break;
    case 2: //================================================
        if (!xServoA6.IsServoOn())
        {
            LOG_MSG_SEND("[Servo ORG] xServoA6.Enable() - start.");
            xServoA6.Enable();
            waitCount = gTriggerCount;
            subStep++;
        }
        else
        {
            subStep = 4;
        }
        break;
    case 3:
        if ((gTriggerCount - waitCount) > __2sec)
        {
            SetErrorCode(ERROR_CODE_A6_SERVO_OFF, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xServoA6.IsServoOn())
        {
            LOG_MSG_SEND("[Servo ORG] xServoA6.Enable() - end.");
            ClearError();
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 4:
        if ((gTriggerCount - waitCount) > __500msec)
            subStep++;
        break;
    case 5: //================================================
        xServoA6.Home();
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 6:
        if ((gTriggerCount - waitCount) > __1sec)
        { // 상태 갱신 delay
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 7:
        if ((gTriggerCount - waitCount) > TIMEOUT_HOME)
        {
            xServoA6.Move_Stop();
            SetErrorCode(ERROR_CODE_A6_SERVO_HOME_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xSL.ServoA6.Driver.isHomed)
        {
            ClearError();
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 8:
        // S32 offset = xPL.ServoA6.Slot.PositionOffset_pulse[SLOT_1];
        S32 offset = xPL.ServoA6.Home.Offset;
        xServoA6.Move_Rel(offset);
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 9:
        if ((gTriggerCount - waitCount) > __1sec)
        { // 상태 갱신 delay
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 10:
        if ((gTriggerCount - waitCount) > TIMEOUT_DEFAULT)
        {
            xServoA6.Move_Stop();
            SetErrorCode(ERROR_CODE_A6_SERVO_HOME_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xSL.ServoA6.Driver.isMoving == NO)
        {
            waitCount = gTriggerCount;
            subStep = 20;
        }
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __100msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        LOG_MSG_SEND("[Servo HOME] Completed. offset = %d", (int)xPL.ServoA6.Home.Offset);
        xSL.ServoA6.isLogicRunning = NO;
        xSL.ServoA6.isHomed = YES;
        xCD.ServoA6.CurrentPosition.SlotNum = SLOT_1;
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Jog(void)
{
    int dir;
    S32 pulse;

    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        xServoA6.Set_Param_Jog();
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 1:
        if ((gTriggerCount - waitCount) > __200msec)
        {
            subStep++;
        }
        break;
    case 2:
        subStep++;
        break;
    case 3:
        dir = xPL.ServoA6.Direction;
        pulse = (S32)(dir * xPL.ServoA6.CMD_Param.Position_Pulse);
        xServoA6.Move_Jog(pulse);
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 4:
        if ((gTriggerCount - waitCount) > __1sec)
        { // 상태 갱신 delay
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 5:
        if ((gTriggerCount - waitCount) > TIMEOUT_JOG)
        {
            xServoA6.Move_Stop();
            SetErrorCode(ERROR_CODE_A6_SERVO_JOG_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xSL.ServoA6.Driver.isInPosition && !xSL.ServoA6.Driver.isMoving)
        {
            waitCount = gTriggerCount;
            subStep = 20;
        }
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __100msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        LOG_MSG_SEND("[Servo JOGS] Current Position(pulse): %d", xCD.ServoA6.CurrentPosition.Pulse);
        xSL.ServoA6.isLogicRunning = NO;
        xCD.ServoA6.CurrentPosition.SlotNum = SLOT_UNKNOWN; // 현재 slot 위치 모름.
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Abs(void)
{
    int dir;
    S32 pulse;

    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        xServoA6.Set_Param_Abs();
        subStep++;
        break;
    case 1:
        subStep++;
        break;
    case 2:
        subStep++;
        break;
    case 3:
        dir = xPL.ServoA6.Direction;
        pulse = (S32)(dir * xPL.ServoA6.CMD_Param.Position_Pulse);
        xServoA6.Move_Abs(pulse);
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 4:
        if ((gTriggerCount - waitCount) > __1sec)
        { // 상태 갱신 delay
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 5:
        if ((gTriggerCount - waitCount) > TIMEOUT_DEFAULT)
        {
            xServoA6.Move_Stop();
            SetErrorCode(ERROR_CODE_A6_SERVO_MOVE_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xSL.ServoA6.Driver.isInPosition && !xSL.ServoA6.Driver.isMoving)
        {
            waitCount = gTriggerCount;
            subStep = 20;
        }
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __200msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        LOG_MSG_SEND("[Servo MOVA] Current Position(pulse): %d", xCD.ServoA6.CurrentPosition.Pulse);
        xSL.ServoA6.isLogicRunning = NO;
        xCD.ServoA6.CurrentPosition.SlotNum = SLOT_UNKNOWN; // 현위치 모름.
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Rel(void)
{
    int dir;
    S32 pulse;

    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        xServoA6.Set_Param_Rel();
        subStep++;
        break;
    case 1:
        subStep++;
        break;
    case 2:
        subStep++;
        break;
    case 3:
        dir = xPL.ServoA6.Direction;
        pulse = (S32)(dir * xPL.ServoA6.CMD_Param.Position_Pulse);
        xServoA6.Move_Rel(pulse);
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 4:
        if ((gTriggerCount - waitCount) > __1sec)
        { // 상태 갱신 delay
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 5:
        if ((gTriggerCount - waitCount) > TIMEOUT_DEFAULT)
        {
            xServoA6.Move_Stop();
            SetErrorCode(ERROR_CODE_A6_SERVO_MOVE_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xSL.ServoA6.Driver.isInPosition && !xSL.ServoA6.Driver.isMoving)
        {
            waitCount = gTriggerCount;
            subStep = 20;
        }
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __200msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        LOG_MSG_SEND("[Servo MOVI] Current Position(pulse): %d", xCD.ServoA6.CurrentPosition.Pulse);
        xSL.ServoA6.isLogicRunning = NO;
        xCD.ServoA6.CurrentPosition.SlotNum = SLOT_UNKNOWN; // 현위치 모름.
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Cent(void)
{
    F32 rpm = 0.f;                    // [rpm]
    U32 time_accel_ms = 0;            // [msec] -> T1 = 0.03 * rpm
    U32 time_run_ms = 0;              // [msec] -> T2 = 등속구간 시간(msec)
    U32 time_decel_ms = 0;            // [msec] -> T3 = 1.5 * T1
    static U32 time_total_ms = 0;     // [sec] -> T = T1 + T2 + T3
    static S32 time_remain_count = 0; // [count] -> 남아있는 시간, 10msec 간격 count
    static int dir = CW;

    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        xSL.ServoA6.isCentrifugeRunning = YES;

        /** 시험에 의해 결정한 파라미터 */
        /*=======================================================================*/
        rpm = xPL.ServoA6.CMD_Param.Cent.Speed_rpm;                         // [rpm]
        time_accel_ms = (U32)(0.03f * rpm * 1000.0f);                       // [msec] -> T1 = 0.03 * rpm
        time_run_ms = (U32)(xPL.ServoA6.CMD_Param.Cent.Time_sec * 1000.0f); // [msec] -> T2 = 등속구간 시간(msec)
        time_decel_ms = (U32)(1.5f * time_accel_ms);                        // [msec] -> T3 = 1.5 * T1
        time_total_ms = time_accel_ms + time_run_ms + time_decel_ms;        // [msec] -> T = T1 + T2 + T3
        time_remain_count = (S32)(time_total_ms / 10);                      // [count] = sec * 100
        /*=======================================================================*/

        // for debugging
        xCD.ServoA6.CentCommand.rpm = rpm;
        xCD.ServoA6.CentCommand.Time_msec_Accel = time_accel_ms;
        xCD.ServoA6.CentCommand.Time_msec_Run = time_run_ms;
        xCD.ServoA6.CentCommand.Time_msec_Decel = time_decel_ms;
        xCD.ServoA6.CentCommand.Time_msec_Total = time_total_ms;

        xServoA6.Set_Param_Cent(rpm, time_accel_ms, time_decel_ms);
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 1:
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep++;
        }
        break;
    case 2:
        dir ^= 1; // overflow 방지 차원
        xServoA6.Move_Cent((U08)dir, time_total_ms);
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 3:
        time_remain_count--;
        if ((gTriggerCount - waitCount) > __1sec)
        { // 상태 갱신 기다려줌.
            subStep++;
        }
        break;
    case 4:
        time_remain_count--;
        if ((gTriggerCount - waitCount) > ((time_total_ms / 10) + __5sec))
        {
            xServoA6.Move_Stop();
            SetErrorCode(ERROR_CODE_A6_SERVO_CENT_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (!xSL.ServoA6.Driver.isMoving && xSL.ServoA6.Driver.isInPosition)
        {
            waitCount = gTriggerCount;
            subStep = 20;
        }
        break;
    case 20:
        time_remain_count--;
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        LOG_MSG_SEND("[Servo CENT %d] Completed.", (int)rpm);
        xCD.ServoA6.CurrentPosition.SlotNum = SLOT_UNKNOWN;
        xSL.ServoA6.isLogicRunning = NO;
        xSL.ServoA6.isCentrifugeRunning = NO;
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }

    // 남아 있는 시간 갱신
    xCD.ServoA6.CentCommand.Time_sec_Remain = (S32)(time_remain_count / _Hz);
    if (xCD.ServoA6.CentCommand.Time_sec_Remain < 0)
    {
        xCD.ServoA6.CentCommand.Time_sec_Remain = 0;
    }
}

static void Step_Slot(void)
{
    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        xServoA6.Set_Param_Slot();
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 1:
        if ((gTriggerCount - waitCount) > __200msec)
        {
            subStep++;
        }
        break;
    case 2:
        subStep++;
        break;
    case 3:
        int slotNum = xPL.ServoA6.CMD_Param.SlotNum;
        S32 currentPos = xCD.ServoA6.CurrentPosition.Pulse;
        S32 offset = xPL.ServoA6.Slot.PositionOffset_pulse[slotNum];
        S32 pulsePerRev = xPL.ServoA6.PulsePerRevolution;
        S32 curMod = currentPos % pulsePerRev;
        float slotWidth = (float)(pulsePerRev / SLOT_COUNT);

        if (curMod < 0)
            curMod += pulsePerRev;
        float targetF = offset + slotNum * slotWidth;
        int32_t targetMod = (int32_t)roundf(targetF);

        int32_t delta = targetMod - curMod; // slot offset

        if (delta > (pulsePerRev / 2))
            delta -= pulsePerRev;
        else if (delta < -(pulsePerRev / 2))
            delta += pulsePerRev;

        xServoA6.Move_Slot(delta);
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 4:
        if ((gTriggerCount - waitCount) > __1sec)
        { // 상태 갱신 delay
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 5:
        if ((gTriggerCount - waitCount) > TIMEOUT_SLOT)
        {
            xServoA6.Move_Stop();
            xCD.ServoA6.CurrentPosition.SlotNum = SLOT_UNKNOWN;
            SetErrorCode(ERROR_CODE_A6_SERVO_SLOT_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xSL.ServoA6.Driver.isInPosition && !xSL.ServoA6.Driver.isMoving)
        {
            waitCount = gTriggerCount;
            subStep = 20;
        }
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        xCD.ServoA6.CurrentPosition.SlotNum = xPL.ServoA6.CMD_Param.SlotNum;
        xSL.ServoA6.isLogicRunning = NO;
        LOG_MSG_SEND("[Servo MOVS %d] completed.", xCD.ServoA6.CurrentPosition.SlotNum + 1);
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Stop(void)
{
    static int oldState = STOP;

    switch (subStep)
    {
    case 0:
        oldState = xSL.ServoA6.isBusy; // 과거값 저장
        xSL.ServoA6.isLogicRunning = YES;
        subStep++;
        break;
    case 1:
        xServoA6.Move_Stop();
        subStep++;
        break;
    case 2:
        if (oldState == RUN) // 움직이고 있었으면 리셋, 안움직이고 있었으면 값 유지
        {
            xCD.ServoA6.CurrentPosition.SlotNum = SLOT_UNKNOWN; // 현위치 모름.
        }
        waitCount = gTriggerCount;
        subStep = 20;
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        LOG_MSG_SEND("[Servo STOP] completed.");
        xSL.ServoA6.isLogicRunning = NO;
        oldState = STOP; // 초기화
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Estop(void)
{
    static int oldState = STOP;

    switch (subStep)
    {
    case 0:
        oldState = xSL.ServoA6.isBusy; // 과거값 저장
        xSL.ServoA6.isLogicRunning = YES;
        subStep++;
        break;
    case 1:
        xServoA6.Move_Estop();
        subStep++;
        break;
    case 2:
        if (oldState == RUN)
        {
            xCD.ServoA6.CurrentPosition.SlotNum = SLOT_UNKNOWN; // 현위치 모름.
        }
        waitCount = gTriggerCount;
        subStep = 20;
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        LOG_MSG_SEND("[Servo ESTOP] completed.");
        xSL.ServoA6.isLogicRunning = NO;
        oldState = STOP;
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

/** @brief safety reset logic -------------------------------------------
 *  @note
 *  1. 발생 현상    :
 *     1) interlock 발생 -> A6 AC 220V 차단 -> 에러발생 ,
 *     2) 또는 드라이버 에러 발생
 *  2. Reset Logic : servo off -> error clear -> servo on -> 상태업데이트
 *  3. 드라이버 에러 발생시 Bioflow 에서 Robo-C를 reset 할때 사용됨.
 * ---------------------------------------------------------------------*/
static void Step_Reset(void)
{
    switch (subStep)
    {
    case 0:
        LOG_MSG_SEND("[Servo RESET] (A6) start.");
        xSL.ServoA6.isLogicRunning = YES;
        // xSL.ServoA6.isHomed = NO;
        ClearError();
        xServoA6.Disable();
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 1:
        if ((gTriggerCount - waitCount) > __3sec)
        {
            SetErrorCode(ERROR_CODE_A6_SERVO_OFF_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (!xSL.ServoA6.Driver.isEnabled)
        {
            subStep++;
        }
        break;
    case 2:
        if ((gTriggerCount - waitCount) > __600msec)
        {
            xServoA6.AlarmClear();
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 3:
        if ((gTriggerCount - waitCount) > __600msec)
        {
            xServoA6.AlarmClear(); // 한번더
            waitCount = gTriggerCount;
            subStep++;
        }
        break;
    case 4:
        if ((gTriggerCount - waitCount) > __600msec)
        {
            subStep++;
        }
        break;
    case 5:
        xServoA6.Enable();
        subStep++;
        break;
    case 6:
        if ((gTriggerCount - waitCount) > __3sec)
        {
            SetErrorCode(ERROR_CODE_A6_SERVO_ON_FAIL, __func__, __LINE__);
            step = STEP_END_ERR;
            return;
        }
        else if (xSL.ServoA6.Driver.isEnabled)
        {
            ClearError();
            waitCount = gTriggerCount;
            subStep = 20;
        }
        break;
    case 20:
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep = 100;
        }
        break;
    case 100:
        ClearError();
        xSL.ServoA6.isLogicRunning = NO;
        LOG_MSG_SEND("[Servo RESET] (A6) completed.");
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_AlarmClear(void)
{
    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        xServoA6.AlarmClear();
        waitCount = gTriggerCount;
        subStep++;
        break;
    case 1:
        if ((gTriggerCount - waitCount) > __500msec)
        {
            subStep++;
        }
        break;
    case 2:
        ClearError();
        subStep = 100;
        break;
    case 100:
        xSL.ServoA6.isLogicRunning = NO;
        LOG_MSG_SEND("[Servo Alarm Clear] completed.");
        step = STEP_END_OK;
        break;
    default:
        step = STEP_END_ERR;
        SetErrorCode(ERROR_CODE_INVALID_SUBSTEP, __func__, __LINE__);
        break;
    }
}

static void Step_Abnormal(void)
{
#if 0
    switch (subStep)
    {
    case 0:
        xSL.ServoA6.isLogicRunning = YES;
        subStep++;
        break;
    case 1:
        subStep++;
        break;
    case 2:
        xSL.ServoA6.isLogicRunning = NO;
        step = STEP_END_OK;
        break;
    default:
        break;
    }
#else
    xSL.ServoA6.isLogicRunning = NO;
    xSL.ServoA6.isCentrifugeRunning = NO;
    xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_NONE;

    subStep = 0;
    returnStep = 0;
    returnSubStep = 0;
    waitCount = 0;
    delayTime = 0;

    xServoA6.Move_Estop();

    step = STEP_END_OK;
#endif
}

static void Step_End_ERR(void)
{
    ERR_MSG_SEND("%d : %s", GetErrorCode_int(), GetErrorMessage());

    xSL.ServoA6.isLogicRunning = NO;
    xSL.ServoA6.isCentrifugeRunning = NO;
    xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_NONE;

    subStep = 0;
    returnStep = 0;
    returnSubStep = 0;
    waitCount = 0;
    delayTime = 0;

    step = STEP_IDLE;
}

static void Step_End_OK(void)
{
    xSL.ServoA6.isLogicRunning = NO;
    xSL.ServoA6.isCentrifugeRunning = NO;
    xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_NONE;

    subStep = 0;
    returnStep = 0;
    returnSubStep = 0;
    waitCount = 0;
    delayTime = 0;

    step = STEP_IDLE;
}
