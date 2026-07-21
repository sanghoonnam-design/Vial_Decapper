/*******************************************************************************
 * Dev_ServoMotor_A6.h
 *
 *  Created on: 2025.11.01
 *      Author: RND. Kang YoungJun
 *
 ******************************************************************************/
#ifndef DEV_SERVOMOTOR_A6_H_
#define DEV_SERVOMOTOR_A6_H_
#include "_00_HW_Config.h"
#include "_00_Dev_Config.h"
#include "XGlobal.h"

#define A6_ID (1)

#define PL_DEFAULT_DIRECTION /*                       */ (1) // 1 or -1
#define PL_DEFAULT_HOME_SPEED_FORWARD_RPM /*          */ (15)
#define PL_DEFAULT_HOME_SPEED_BACKWARD_RPM /*         */ (5)
#define PL_DEFAULT_HOME_TIME_ACCEL_MILLIS /*          */ (50)
#define PL_DEFAULT_HOME_OFFSET /*                     */ (100)
#define PL_DEFAULT_SLOT_SPEED_RPM /*                  */ (20)
#define PL_DEFAULT_SLOT_TIME_ACCEL_MILLIS /*          */ (500)
#define PL_DEFAULT_SLOT_TIME_DECEL_MILLIS /*          */ (500)
#define PL_DEFAULT_SLOT_POSITION_OFFSET_1 /*          */ (100) // 티칭해야 되는 값.
#define PL_DEFAULT_SLOT_POSITION_OFFSET_2 /*          */ (100)
#define PL_DEFAULT_SLOT_POSITION_OFFSET_3 /*          */ (100)
#define PL_DEFAULT_SLOT_POSITION_OFFSET_4 /*          */ (100)
#define PL_DEFAULT_SLOT_POSITION_OFFSET_5 /*          */ (100)
#define PL_DEFAULT_SLOT_POSITION_OFFSET_6 /*          */ (100)
#define PL_DEFAULT_JOG_SPEED_RPM /*                   */ (20)
#define PL_DEFAULT_JOG_TIME_ACCEL_MILLIS /*           */ (500)
#define PL_DEFAULT_JOG_TIME_DECEL_MILLIS /*           */ (500)
#define PL_DEFAULT_BASEMOVE_SPEED_RPM /*              */ (20)
#define PL_DEFAULT_BASEMOVE_TIME_ACCEL_MILLIS /*      */ (500)
#define PL_DEFAULT_BASEMOVE_TIME_DECEL_MILLIS /*      */ (500)
// #define PL_DEFAULT_CENTRUFUGERUN_SPEED_RPM /*         */ (50) // not used
// #define PL_DEFAULT_CENTRUFUGERUN_TIME_ACCEL_MILLIS /* */ (500)
// #define PL_DEFAULT_CENTRUFUGERUN_TIME_DECEL_MILLIS /* */ (500)
// #define PL_DEFAULT_CENTRUFUGERUN_MAXACCEL_PULSE /*    */ (13900)
#define PL_DEFAULT_PULSE_PER_REVOLUTION /*            */ (10000)

#define TIMEOUT_HOME (__30sec)
#define TIMEOUT_SLOT (__20sec)
#define TIMEOUT_JOG (__5sec)
#define TIMEOUT_DEFAULT (__10sec)

extern int isSavingParametersA6;

typedef enum                  // [제어모드 정의]
{                             //
    A6_CONTROL_MODE_NONE = 0, // default
    A6_CONTROL_MODE_INIT,     // Initialization
    A6_CONTROL_MODE_ENABLE,   // Enable control
    A6_CONTROL_MODE_DISABLE,  // Disable control
    A6_CONTROL_MODE_ORG,      // Origin
    A6_CONTROL_MODE_HOME,     // Homing = ORG(0) + SLOT 1(offset) = SLOT 1
    A6_CONTROL_MODE_JOG,      // JOG이동
    A6_CONTROL_MODE_ABS,      // 절대이동
    A6_CONTROL_MODE_REL,      // 상대이동
    A6_CONTROL_MODE_CENT,     // cent 명령
    A6_CONTROL_MODE_SLOT,     // slot 이동명령
    A6_CONTROL_MODE_STOP,     // 정지동작
    A6_CONTROL_MODE_ESTOP,    // 긴급정지
    A6_CONTROL_MODE_RESET,    // safety 후 reset 시퀀스
                              // A6_CONTROL_MODE_SAVEA6_HOME_PARAM, // A6 홈파라미터 저장
    A6_CONTROL_MODE_ERRCLEAR  // Error Clear
                              //
} teControlMode_ServoA6;      //
                              //
typedef enum                  // [상태머신 step 정의]
{                             //
    STEP_IDLE,                //
    STEP_INIT,                //
    STEP_ENABLE,              //
    STEP_DISABLE,             //
    STEP_ORG,                 //
    STEP_HOME,                //
    STEP_JOG,                 //
    STEP_ABS,                 //
    STEP_REL,                 //
    STEP_CENT,                //
    STEP_SLOT,                //
    STEP_STOP,                //
    STEP_ESTOP,               //
    STEP_RESET,               //
    STEP_ERR_CLEAR,           //
    STEP_ABNORMAL,            //
    STEP_END_ERR,             //
    STEP_END_OK               //
} teSTEP_ServoA6;             //

#pragma pack(push, 1)                         //
typedef struct                                // [1]. State List
{                                             //
    int isInitialized;                        //!> from APC, 초기화 여부
    int isControllable;                       // from SDG, 제어가능여부, Enable + ServoOn + No Error + etc.
                                              //
    struct                                    //
    {                                         //
        int isConnected;                      // from RS485 logic, A6 통신 연결 상태
        int isEnabled;                        //>! from USD, 모터 Enable(=Servo On) 상태
        int isMoving;                         //>! from USD, 모터 moving 상태
        int isHomed;                          //>! from USD, 모터 home 상태, 홈센서 친 상태
        int isInPosition;                     //>! from USD, 모터 In-Position 여부 상태
        int isError;                          //>! from USD, Servo A6 Error 상태, 1=Error, 0=No Error
        int ErrorCode;                        //>! from USD, A6 상세 에러 코드
    } Driver;                                 //
                                              //
    int isLogicRunning;                       //!> from APC, 로직 동작 상태
    int isCentrifugeRunning;                  //!> from APC, [CENT] 명령이 실행되고 있는지 여부
    int isBusy;                               //!> from USD, = isMoving || isLogicRunning
    int isHomed;                              //!> from home 상태머신, Driver isHomed + offset(slot 1) 이동한 상태
                                              //
} tsXSL_ServoA6;                              //
                                              //
typedef struct                                // [2]. Control Data
{                                             //
    struct                                    //
    {                                         //
        S32 pps;                              // from USD, 현재 속도 [pps]
        F32 rps;                              // from USD, 현재 속도 [rps]
        F32 rpm;                              // from USD, 현재 속도 [rpm]
    } CurrentSpeed;                           //
                                              //
    struct                                    //
    {                                         //
        S32 Pulse;                            // from USD, 현재 위치, pulse
        int SlotNum;                          // from APC, 현재 slot number
    } CurrentPosition;                        //
                                              //
    F32 Centrifugal_Force;                    // 계산된 원심력
                                              //
    struct                                    // [CENT] 명령 데이터, 디버깅용
    {                                         //
        F32 rpm;                              //!> from Command,     PL->CD     , target rpm
        U32 Time_msec_Accel;                  //!> from APC,     T1             , target accel-time
        U32 Time_msec_Run;                    //!> from Command, T2, PL->CD     , target run-time
        U32 Time_msec_Decel;                  //!> from APC,     T3             , target decel-time
        U32 Time_msec_Total;                  //!> from logic,   T = T1 +T2 +T3 , target total-time
        S32 Time_sec_Remain;                  //!> from logic, 남아있는 시간
    } CentCommand;                            //
} tsXCD_ServoA6;                              //
                                              //
typedef struct                                // [3]. Parameter List
{                                             //
    int CMD_StartControl;                     // from PC(client), StartControl
    int CMD_ControlMode;                      // from PC(client), 제어 모드
                                              //
    union                                     // union 사용
    {                                         //
        struct                                //
        {                                     //
            S32 Speed_rpm;                    // [CENT] 명령 목표 rpm
            U32 Time_sec;                     // [CENT] 명령 동작 시간(등속구간)
        } Cent;                               // [CENT] 명령 파라미터 저장용
                                              //
        U32 Position_Pulse;                   // Abs, Inc, SASP 파라미터 저장용
        U32 SlotNum;                          // slot 명령 파라미터 저장용
    } CMD_Param;                              //
                                              // ============================================
                                              //
    int Direction;                            //!> from EEPROM or PC, 이하 EEPROM 에서 읽어옴.
                                              //   [주의] 시스템 방향 기준 설정: 최초 적용 후 수정 불가
    struct                                    //
    {                                         // [HOME] param.
        F32 Speed_Forward_rpm;                // from EEPROM, home forward 이동속도 파라미터
        F32 Speed_Backward_rpm;               // from EEPROM, home backward 이동속도 파라미터
        F32 Time_Accel_millis;                // from EEPROM, home 가속 파라미터
        F32 Offset;                           // from EEPROM, not used, home offset
    } Home;                                   //
                                              //
    struct                                    //
    {                                         // [SLOT, MOVS] 이동 param.
        F32 Speed_rpm;                        // from EEPROM, slot 이동속도 파라미터
        F32 Time_Accel_millis;                // from EEPROM, slot 가속 파라미터
        F32 Time_Decel_millis;                // from EEPROM, slot 감속 파라미터
        U32 PositionOffset_pulse[SLOT_COUNT]; // from EEPROM, slot 위치
    } Slot;                                   //
                                              //
    struct                                    //
    {                                         // [JOG] param.
        F32 Speed_rpm;                        // from EEPROM, Jog 이동속도 파라미터
        F32 Time_Accel_millis;                // from EEPROM, Jog 가속 파라미터
        F32 Time_Decel_millis;                // from EEPROM, Jog 감속 파라미터
    } Jog;                                    //
                                              //
    struct                                    //
    {                                         // [절대이동, 상대이동, 기타] param.
        F32 Speed_rpm;                        // from EEPROM, 일반적인 움직임에 대한 이동속도 파라미터
        F32 Time_Accel_millis;                // from EEPROM, 일반적인 움직임에 대한 가속 파라미터
        F32 Time_Decel_millis;                // from EEPROM, 일반적인 움직임에 대한 감속 파라미터
    } BaseMove;                               //
                                              //
    S32 PulsePerRevolution;                   // from EEPROM,
                                              //
} tsXPL_ServoA6;                              //
#pragma pack(pop)                             //

typedef struct ServoA6Group                // [4]. Action & Method
{                                          //
    void (*Enable)(void);                  //
    void (*Disable)(void);                 //
    void (*Origin)(void);                  //
    void (*Home)(void);                    //
    void (*Move_Jog)(S32);                 // [유지보수 시 사용] Jog 이동
    void (*Move_Abs)(S32);                 // 절대이동
    void (*Move_Rel)(S32);                 // 상대이동
    void (*Move_Cent)(U08, U32);           // ** [위치제어로 속도제어 구현] **
                                           //   -임의의 속도에 주어진 시간이상 움직이도록 설계
                                           //   -정확한 시간은 FSM 에서 제어함.
    void (*Move_Slot)(S32);                //
    void (*Move_Stop)(void);               //
    void (*Move_Estop)(void);              //
    void (*Stop)(void);                    //
                                           //
    void (*Set_Param_Origin)(void);        // [HOME] Homing 파라미터 셋팅
    void (*Set_Param_Home)(void);          // [HOME] Homing 파라미터 셋팅
    void (*Set_Param_Jog)(void);           // [JOGS] Jog(step) 파라미터 셋팅
    void (*Set_Param_Slot)(void);          // [MOVS, SLOT] Slot 이동 파라미터 셋팅
    void (*Set_Param_Cent)(U32, U32, U32); // [CENT] centrifuge Run 명령 파라미터 셋팅
    void (*Set_Param_Abs)(void);           // [MOVA] 절대이동 명령 파라미터 셋팅
    void (*Set_Param_Rel)(void);           // [MOVI] 상대이동 명령 파라미터 셋팅
                                           //
    void (*AlarmClear)(void);              // A6 드라이버 에러 클리어
                                           //
    void (*Update_RS485_Tx)(void);         //
                                           //
    int (*IsHomed)(void);                  //
    int (*IsBusy)(void);                   //
    int (*IsStop)(void);                   //
    int (*IsDriverError)(void);            //
    int (*IsServoOn)(void);                //
    int (*IsEnabled)(void);                //
    int (*GetPosition_SlotNum)(void);      //
                                           //
    void (*ErrorMonitor)(void);            //
                                           //
    void (*Update)(void);                  //
                                           //
    void (*StateMachine)(void);            //
                                           //
} tsXServoA6;                              //

extern tsXServoA6 xServoA6;

//==========================================================================
void Initialize_ServoA6(void);

void ServoA6_Enable(void);
void ServoA6_Disable(void);
void ServoA6_Origin(void);
void ServoA6_Home(void);
void ServoA6_Move_Jog(S32 pulse);
void ServoA6_Move_Abs(S32 pulse);
void ServoA6_Move_Rel(S32 pulse);
void ServoA6_Move_Cent(U08 dir, U32 msec); // int dir, S32 rpm, U32 time_sec);
void ServoA6_Move_Slot(S32 pulse);
void ServoA6_Move_Stop(void);
void ServoA6_Move_Estop(void);

void ServoA6_Set_Param_Origin(void);
void ServoA6_Set_Param_Home(void);
void ServoA6_Set_Param_Jog(void);
void ServoA6_Set_Param_Slot(void);
void ServoA6_Set_Param_Cent(U32 rpm, U32 accTime_ms, U32 decTime_ms);
void ServoA6_Set_Param_Abs(void);
void ServoA6_Set_Param_Rel(void);

void ServoA6_AlarmClear(void);

void ServoA6_Update_RS485_Tx(void);

int ServoA6_IsHomed(void); // utility function
int ServoA6_IsBusy(void);
int ServoA6_IsStop(void);
int ServoA6_IsDriverError(void);
int ServoA6_IsServoOn(void);
int ServoA6_IsEnabled(void);
int ServoA6_GetPosition_SlotNum(void);
void ServoA6_ErrorMonitor(void);

void ServoA6_Update(void);

void ServoA6_StateMachine(void); // FSM

#endif /* DEV_SERVOMOTOR_A6_H_ */
