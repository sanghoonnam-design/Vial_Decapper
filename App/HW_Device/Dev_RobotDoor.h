/*******************************************************************************
 * Dev_RobotDoor.h
 *
 *  Created on: 2025.10.13
 *      Author: RND. Kang YoungJun.
 *
 ******************************************************************************/
#ifndef DEV_ROBOTDOOR_H_
#define DEV_ROBOTDOOR_H_
#include "_00_HW_Config.h"
#include "_00_Dev_Config.h"
#include "XGlobal.h"
#include "TMC2660.h"

/* robot door state **********************************************************/
// 센서기반 처리                             // open  ||  close
#define ROBOTDOOR_STATE_ERROR /*    */ (0) //  1    ||    1
#define ROBOTDOOR_STATE_MOVING /*   */ (1) //  0    ||    0
#define ROBOTDOOR_STATE_CLOSED /*   */ (2) //  0    ||    1
#define ROBOTDOOR_STATE_OPEN /*     */ (3) //  1    ||    0
/*****************************************************************************/

/* c->h command argument mnemonic ********************************************/
#define ROBOTDOOR_COMMAND_CLOSE /*  */ (0)
#define ROBOTDOOR_COMMAND_OPEN /*   */ (1)
#define ROBOTDOOR_COMMAND_STOP /*   */ (2)
#define ROBOTDOOR_COMMAND_NONE /*   */ (4)
/*****************************************************************************/

// robot door params.
#define PL_DEFAULT_STEP_MOTOR_DIRECTION /*               */ (-1)   // [-] direction
#define PL_DEFAULT_ROBOTDOOR_CONTROL_TIMEOUT_ms /*       */ (5000) // [msec]
#define PL_DEFAULT_CLOSE_SENSOR_OVER_TIME_ms /*          */ (450)  // [msec], close 센서 치고 더 동작하는 시간
#define PL_DEFAULT_OPEN_SENSOR_OVER_TIME_ms /*           */ (700)  // [msec], open 센서 치고 더 동작하는 시간

#define PL_DEFAULT_STEP_MOTOR_SPEED_PPS /*               */ (6000)   // [pps], 스텝 모터 기본 속도
#define PL_DEFAULT_STEP_MOTOR_ACCEL_PPSS /*              */ (64000)  // [ppss], 스텝 모터 기본 가속도
#define PL_DEFAULT_STEP_MOTOR_COMMAND_DISPLACEMENT /*    */ (18000)  // 스텝모터 상대이동 위치
#define PL_DEFAULT_STEP_MOTOR_NORMAL_CURRENT_A /*        */ (1.2f)   // normal current
#define PL_DEFAULT_STEP_MOTOR_HOLDING_CURRENT_PERCENT /* */ (30)     // 홀딩 토크
#define PL_DEFAULT_STEP_MOTOR_RESOLUTION /*              */ (R_3200) // 분해능

#if 0
#define GEAR_RATIO /*    */ (36.0f)   // not used.
#define PULSE_PER_REV /* */ (3200.0f) // microstep 1/16, 200*16
#endif

#define ROBOT_DOOR_RETRY_COUNT /* */ (3) // 제어 실패시 재시도 횟수

#pragma pack(push, 1)
typedef struct                          // [1]. State List
{                                       //
    int isInitialized;                  //!> from FSM, door 초기화 여부를 나타냄
    int isControllable;                 //!> from SDG, robot door 제어가능여부, YES=1, NO=0
                                        //
    int Status;                         //!> from USD, robot Door 상태, 0~3
                                        //
    int isBusy;                         //!> = IsMotorMoving || IsLogicRunning; , TODO: 도어상태는 다시한번 생각
    int isLogicRunning;                 //!> [중요] from APC, 로직 동작여부, 비정상 Status를 판단하기 위해 사용
                                        //
    struct                              //
    {                                   //
        int isEnabled;                  //!> from APC,
        int isMoving;                   //!> [중요] from APC, timeout 여부, 비정상 Status를 판단하기 위해 사용
    } Motor;                            //
                                        //
} tsXSL_RobotDoor;                      //
                                        //
typedef struct                          // [2]. Control Data
{                                       //
    int CloseSensor;                    //!> from USD, 0: LOW, 1: HIGH
    int OpenSensor;                     //!> from USD, 0: LOW, 1: HIGH
    U32 CurrentPosition;                //!> from USD, 스텝모터 현재 위치, pulse
                                        //
    U32 TotalRetryCount;                //!> from APC, test code
} tsXCD_RobotDoor;                      //
                                        //
typedef struct                          // [3]. Parameter List
{                                       //
    int CMD_StartControl;               //!> from PC(client), 제어 할텨=1? 말텨=0?
    int TargetMotion;                   //!> from PC(client), command: 0(close),1(open),2(stop)
                                        //
    int Direction;                      //!> from PC or EEPROM, 기본 방향 값(Open 방향값)                            //
    int Door_ControlTimeout_ms;         //!> [msec], from PC or EEPROM, 제어 타이아웃 기준 시간
    U32 CloseSensorOverTime_ms;         //!> [msec], from PC or EEPROM, close 센서 치고 더 동작하는 시간
    U32 OpenSensorOverTime_ms;          //!> [msec], from PC or EEPROM, open 센서 치고 더 동작하는 시간
    U32 RelativeDistance_Count;         //!> [pulse], 스텝모터 상대이동 거리
                                        //
    struct                              //
    {                                   //
        U32 Speed_pps;                  //!> [pps], 스텝모터 속도
        U32 Accel_ppss;                 //!> [ppss], 스텝모터 가속도
        F32 NormalCurrent_A;            //!> [A], normal current
        U32 HoldingCurrent_Percent;     //!> [%], Holding current
        int Resolution;                 //!> [idx], ex) R_3200=4
    } Motor;                            //
                                        //
} tsXPL_RobotDoor;                      //
#pragma pack(pop)                       //
                                        //
typedef struct RobotDoorGroup           // [4]. Action & Method
{                                       //
    struct                              //
    {                                   //
        void (*Init)(void);             //!> Motor Init.
        void (*Enable)(void);           //!> Motor Enable
        void (*Disable)(void);          //!> Motor Enable
        void (*SetSpeed)(U32);          //!> Motor Speed Control
        void (*SetResolution)(int);     //!> Motor Resolution Control
        void (*SetCurrent)(float, int); //!> Motor nomal current
        int (*IsMoving)(void);          //!> is Moving?
    } Motor;                            //
                                        //
    void (*MoveToOpen)(void);           //!> move toward the open direction
    void (*MoveToClose)(void);          //!> move toward the close direction
    void (*Stop)(void);                 //!>
                                        //
    int (*IsBusy)(void);                //!>
    int (*IsStop)(void);                //!>
    int (*IsOpen)(void);                //!>
    int (*GetState)(void);              //!>
                                        //
    void (*Read_Sensor)(void);          //!> from USD, 센서값 읽고,
    void (*Update)(void);               //!> from USD, 도어상태 판단혀봐.
                                        //
    void (*StateMachine)(void);         //!> FSM
} tsXRobotDoor;                         //

extern tsXRobotDoor xDoor;
extern sMotionStatus_t MotionStatus;

//==========================================================================
void Initialize_RobotDoor(void);

void RobotDoor_Motor_Init(void);
void RobotDoor_Motor_Enable(void);
void RobotDoor_Motor_Disable(void);
void RobotDoor_Motor_SetSpeed(U32 pps);
void RobotDoor_Motor_SetCurrent(float current_A, int stopCurrent);
void RobotDoor_Motor_SetResolution(int resolution);
int RobotDoor_Motor_IsMoving(void);

void RobotDoor_Door_MoveToOpen(void);
void RobotDoor_Door_MoveToClose(void);
void RobotDoor_Door_Stop(void);

int RobotDoor_Door_IsBusy(void);
int RobotDoor_Door_IsStop(void);
int RobotDoor_Door_IsOpen(void);
int RobotDoor_Door_GetState(void);

void RobotDoor_Read_Sensor(void);
void RobotDoor_Update(void);

void RobotDoor_StateMachine(void); // FSM

#endif /* DEV_ROBOTDOOR_H_ */
