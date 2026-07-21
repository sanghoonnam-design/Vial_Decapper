/*******************************************************************************
 * XSystem_DB.c
 *
 *  Created on: 2025.09.04
 *      Author: RND. Kang PilSoon.
 *
 ******************************************************************************/
#include "XSystem_DB.h"
#include "XSystemInfo.h"

tsXStateList xSL;
tsXControlData xCD;
tsXParameterList xPL;

void SystemDB_Initialize(void)
{
    memset((char *)&xSL, 0, sizeof(tsXStateList));
    memset((char *)&xCD, 0, sizeof(tsXControlData));
    memset((char *)&xPL, 0, sizeof(tsXParameterList));

    //==========================================================================
    SystemDB_SL_Init();
    // SystemDB_CD_Init();
    // SystemDB_PL_Init(); // [주의] PL은 부팅후 초기화 부분에서 EEPROM 쪽에서 실행된다.
}

void SystemDB_SL_Init(void)
{
    /** @note USER CODE - START */

    // xSL.Door.isInitialized = NO;
    // xSL.Door.isControllable = YES;

    // xSL.ServoA6.isInitialized = NO;
    // xSL.ServoA6.isControllable = YES;

    /** @note USER CODE - END */
}

void SystemDB_CD_Init(void)
{
    /** @note USER CODE - START */

    //;

    /** @note USER CODE - END */
}

/** *************************************************************************
 * @brief EEPROM에서 PL을 읽은 뒤 일부 PL 초기화
 *   1. EEPROM 에서 PL을 읽어온다.
 *   2. PL 일부에 대해 초기화를 진행한다.
 * *************************************************************************/
void SystemDB_PL_Init(void)
{
    // ────────────────────────────────────────────────────────────────
    /** @note USER CODE - START                                     **/
    // ────────────────────────────────────────────────────────────────

    xPL.Header.Size_PL /*     */ = sizeof(tsXParameterList);        //
    xPL.Header.Size_Header /* */ = sizeof(tsXPL_Header);            //
    xPL.Header.FW_Version /*  */ = xSystemInfo.PL_Get_FW_Version(); //

    // ────────────────────────────────────────────────────────────────
    /** @note USER CODE - END                                       **/
    // ────────────────────────────────────────────────────────────────
}

void SystemDB_PL_Init_Factory(tsXParameterList *pl)
{
     //========================================================================================================
    pl->Header.UpdateDate /*                             */ = SWRTC_GetTime_YYMMDDHH();                        // [YYMMDD] parameter를 EEPROM에 저장한 날짜
    pl->Header.DG_CPU_Temperature_Overheat_Criteria /*   */ = PL_DEFAULT_DG_CPU_TEMPERATURE_OVERHEAT_CRITERIA; // [°C] CPU 온도 과열 판단 기준값
    pl->Header.DG_CPU_Temp_Alarm_Interval_10msec /*      */ = PL_DEFAULT_DG_CPU_TEMP_ALARM_INTERVAL_10msec;    // [sec] CPU 온도 과열 알람 주기
    pl->Header.DG_IsDiagnosisEnabled /*                  */ = YES;                                             // 고장진단 활성화 여부
    //========================================================================================================


    /** @note USER CODE - START */

    pl->Door.CMD_StartControl /*                      */ = NO;
    pl->Door.TargetMotion /*                          */ = ROBOTDOOR_COMMAND_CLOSE; // 초기화
    pl->Door.Direction /*                             */ = PL_DEFAULT_STEP_MOTOR_DIRECTION;
    pl->Door.Door_ControlTimeout_ms /*                */ = PL_DEFAULT_ROBOTDOOR_CONTROL_TIMEOUT_ms;
    pl->Door.CloseSensorOverTime_ms /*                */ = PL_DEFAULT_CLOSE_SENSOR_OVER_TIME_ms;
    pl->Door.OpenSensorOverTime_ms /*                 */ = PL_DEFAULT_OPEN_SENSOR_OVER_TIME_ms;
    pl->Door.RelativeDistance_Count /*                */ = PL_DEFAULT_STEP_MOTOR_COMMAND_DISPLACEMENT;
    pl->Door.Motor.Speed_pps /*                       */ = PL_DEFAULT_STEP_MOTOR_SPEED_PPS;
    pl->Door.Motor.Accel_ppss /*                      */ = PL_DEFAULT_STEP_MOTOR_ACCEL_PPSS;
    pl->Door.Motor.NormalCurrent_A /*                 */ = PL_DEFAULT_STEP_MOTOR_NORMAL_CURRENT_A;
    pl->Door.Motor.HoldingCurrent_Percent /*          */ = PL_DEFAULT_STEP_MOTOR_HOLDING_CURRENT_PERCENT;
    pl->Door.Motor.Resolution /*                      */ = PL_DEFAULT_STEP_MOTOR_RESOLUTION;

    pl->ServoA6.CMD_StartControl /*                   */ = NO;
    pl->ServoA6.CMD_ControlMode /*                    */ = A6_CONTROL_MODE_NONE;
    pl->ServoA6.Direction /*                          */ = PL_DEFAULT_DIRECTION; // system direction.
    pl->ServoA6.Home.Speed_Forward_rpm /*             */ = PL_DEFAULT_HOME_SPEED_FORWARD_RPM;
    pl->ServoA6.Home.Speed_Backward_rpm /*            */ = PL_DEFAULT_HOME_SPEED_BACKWARD_RPM;
    pl->ServoA6.Home.Time_Accel_millis /*             */ = PL_DEFAULT_HOME_TIME_ACCEL_MILLIS;
    pl->ServoA6.Home.Offset /*                        */ = PL_DEFAULT_HOME_OFFSET;
    pl->ServoA6.Slot.Speed_rpm /*                     */ = PL_DEFAULT_SLOT_SPEED_RPM;
    pl->ServoA6.Slot.Time_Accel_millis /*             */ = PL_DEFAULT_SLOT_TIME_ACCEL_MILLIS;
    pl->ServoA6.Slot.Time_Decel_millis /*             */ = PL_DEFAULT_SLOT_TIME_DECEL_MILLIS;
    pl->ServoA6.Slot.PositionOffset_pulse[SLOT_1] /*  */ = PL_DEFAULT_SLOT_POSITION_OFFSET_1;
    pl->ServoA6.Slot.PositionOffset_pulse[SLOT_2] /*  */ = PL_DEFAULT_SLOT_POSITION_OFFSET_2;
    pl->ServoA6.Slot.PositionOffset_pulse[SLOT_3] /*  */ = PL_DEFAULT_SLOT_POSITION_OFFSET_3;
    pl->ServoA6.Slot.PositionOffset_pulse[SLOT_4] /*  */ = PL_DEFAULT_SLOT_POSITION_OFFSET_4;
    pl->ServoA6.Slot.PositionOffset_pulse[SLOT_5] /*  */ = PL_DEFAULT_SLOT_POSITION_OFFSET_5;
    pl->ServoA6.Slot.PositionOffset_pulse[SLOT_6] /*  */ = PL_DEFAULT_SLOT_POSITION_OFFSET_6;
    pl->ServoA6.Jog.Speed_rpm /*                      */ = PL_DEFAULT_JOG_SPEED_RPM;
    pl->ServoA6.Jog.Time_Accel_millis /*              */ = PL_DEFAULT_JOG_TIME_ACCEL_MILLIS;
    pl->ServoA6.Jog.Time_Decel_millis /*              */ = PL_DEFAULT_JOG_TIME_DECEL_MILLIS;
    pl->ServoA6.BaseMove.Speed_rpm /*                 */ = PL_DEFAULT_BASEMOVE_SPEED_RPM;
    pl->ServoA6.BaseMove.Time_Accel_millis /*         */ = PL_DEFAULT_BASEMOVE_TIME_ACCEL_MILLIS;
    pl->ServoA6.BaseMove.Time_Decel_millis /*         */ = PL_DEFAULT_BASEMOVE_TIME_DECEL_MILLIS;
    pl->ServoA6.PulsePerRevolution /*                 */ = PL_DEFAULT_PULSE_PER_REVOLUTION;
    
    /** @note USER CODE - END */
}

void SystemDB_SL_Push(void)
{
}

void SystemDB_CD_Push(void)
{
}

void SystemDB_PL_Push(void)
{
}
