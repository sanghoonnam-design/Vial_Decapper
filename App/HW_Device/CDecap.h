/*
 * CDCap.h
 *
 *  Created on: 2026. 6. 18.
 *      Author: RND
 */

#ifndef   __CDECAP_H__
#define   __CDECAP_H__

#include <stdbool.h>
#include <stdint.h>
#include "XGlobal.h"   // U08, U32, S32, F32 등 기본 타입
#include "TMC2660.h"   // STEP_CH_MAX, sMotionStatus_t, TMC2660_GetMotorRun
#include "_01_XSystemManagement.h"

#ifdef __CDCAP_C__
	#define CDCEAP_EXT
#else
	#define CDECAP_EXT extern
#endif

// 모터 파라미터 관련 (pk223)
#define T_MOTOR_ACC				300000
#define T_MOTOR_VEL				20000
/*
#define MOTOR_ACC (800000)
#define MOTOR_SPD_SAFE (70000) // 모터 속도 안전 마진
#define MOTOR_SPD_XRECOV_SAFE (40000)
 * */
#define MOTOR_ACC				300000
#define MOTOR_SPD_SAFE			20000
#define MOTOR_SPD_XRECOV_SAFE 	40000
#define R_MOTOR_POS				20000

#define ZDECAP_ACC				300000
#define ZDECAP_VEL				20000

//TEST 하여 수정
#define ZCAP_CAP_UP_POS				MM2PULSE_R * 2
#define ZCAP_CAP_SIDE_POS			MM2PULSE_R * 2
#define ZCAP_ORIGIN_POSITION	0

#define RDECAP_ACC				300000
#define RDECAP_VEL				20000
#define RDECAP_POS				MM2PULSE_R * 2

#define MOTOR_SPD_HOME_FAST (20000) // 모터 속도 빠르게
#define MOTOR_SPD_HOME_SLOW (3000)  // 모터 속도 느리게

/* 실제 기구물의 가동 범위에 따른 mm 수정*/
#define Z_TOTAL_mm 50
#define Z_Limmit_mm (MM2PULSE_Z * Z_TOTAL_mm) //

#define R_TOTAL_mm 50
#define R_Limmit_mm (MM2PULSE_R * R_TOTAL_mm) //

// 1.8도 기준 1CYCLE 3200 --> PKp223 0.05도 *36
#define PKP223	36
#define PK266	1
#define PK235	1

#define MM2PULSE_Z (3200 * PK235) //1mm 기준
#define MM2PULSE_R (3200 * PKP223) //1mm 기준

//티칭 포인트==============================================================================
#define CDECAPPOS_Z_CAP_TOP 5 //ex 32
#define CDECAPPOS_Z_MOVE_TOP	MM2PULSE * CDECAPPOS_Z_CAP_TOP

#define CDECAPPOS_Z_CAP_SIDE 5 //ex 32
#define CDECAPPOS_Z_MOVE_SIDE 	MM2PULSE * CDECAPPOS_Z_CAP_SIDE

//==============================================================================

// 축 설정
#define TOP (-1)
#define BOTTOM (1)
#define FRONT (-1)
#define BACK (1)

#define ZH (2)  // Z+: 모터 반대편
#define ZL (3)  // Z-: 모터 있는 쪽

//Master
#define aZ (0) // Z축 모터 채널
#define aR (1) // Rotate

#define DECAPPING (1)
#define CAPPING (2)

#define DEBUG_SL_SIZE (10)
#define DEBUG_CD_SIZE (10)
#define DEBUG_PL_SIZE (10)

#define SEC_TO_TICKS(sec) ((U32)((sec) * 1000.0f / (float)FW_TICK_MS))
#define _TO_TICKS(ms) ((U32)((ms) * 1000.0f / (float)FW_TICK_MS))
#define FW_TICK_MS (10)

#define HOMING_TIMEOUT_MS		(5000U)
#define CAPPING_TIMEOUT_MS		(5000U)
#define DECAPPING_TIMEOUT_MS	(5000U)
#define READY_TIMEOUT_MS   		(5000U)


#define IS_STARTED(idx) (sStartedFlags & (1UL << (idx)))
#define SET_STARTED(idx) (sStartedFlags |= (1UL << (idx)))
#define CLR_STARTED(idx) (sStartedFlags &= ~(1UL << (idx)))

#define IS_DONE(idx) (sDoneFlags & (1UL << (idx)))
#define SET_DONE(idx) (sDoneFlags |= (1UL << (idx)))
#define CLR_DONE(idx) (sDoneFlags &= ~(1UL << (idx)))

/*Sensor Parameter*/
#define Z_H_SENSOR_PIN                 (0U)
#define Z_L_SENSOR_PIN                 (1U)
#define Y_H_SENSOR_PIN                 (2U)
#define Y_L_SENSOR_PIN                 (3U)
#define CT_BODY_GRIP_OPEN_SENSOR_PIN   (4U)
#define CT_BODY_GRIP_CLOSE_SENSOR_PIN  (5U)
#define CT_CAP_GRIP_OPEN_SENSOR_PIN    (6U)
#define CT_CAP_GRIP_CLOSE_SENSOR_PIN   (7U)
#define CT_DETECT_OPEN_SENSOR_PIN      (8U)

/*Y축 공압기*/
#define Y_PIN_H_CONTROLLER		(3)
#define Y_PIN_L_CONTROLLER		(4)

/*CT*/
#define AIR_CT_BODY_GRIP_PIN	(0)
#define AIR_CT_CAP_GRIP_PIN		(1)

typedef void (*FSM_Handler)(void);

/*ACTION */
typedef enum{
    ACTION_NONE = 0,
	/*User Command*/
    ACTION_STOP,
	ACTION_HOME,

	ACTION_DECAP,
	ACTION_CAP,

	ACTION_PAUSE,
	ACTION_RESUME,

	/*Debug Command*/
	ACTION_ORIGIN,

	ACTION_RMOVEZ,
	ACTION_AMOVEZ,
	ACTION_RROTATE,
	ACTION_AROTATE,

	ACTION_Y_H_MOVE,
	ACTION_Y_L_MOVE,

	ACTION_UDECAP,
	ACTION_UCAP,
	ACTION_LONGRUN,

	ACTION_BODY_GRIP,
	ACTION_CAP_GRIP,

    ACTION_MAX 		// 항상 마지막에 추가
} teXActionType;

typedef enum{
    DFSM_IDLE = 0,

	/*User Command*/
	DFSM_STOP,
	DFSM_HOME,

	DFSM_DECAP,
    DFSM_CAP,

	DFSM_PAUSE,
	DFSM_RESUME,

	/*Debug Command*/
	DFSM_ORIGIN,

	DFSM_RROTATE,
	DFSM_AROTATE,

    DFSM_RMOVEZ,
    DFSM_AMOVEZ,

	DFSM_Y_H_MOVE,
	DFSM_Y_L_MOVE,

	DFSM_UDECAP,
	DFSM_UCAP,
	DFSM_LONGRUN,

	DFSM_CT_BODY_GRIP,
	DFSM_CT_CAP_GRIP,

	DFSM_ABNORMAL,
    DFSM_END_OK,
    DFSM_PHASE_MAX
} teFSM_Decapper;

typedef enum{
    CDECAP_HOMING_NONE = 0,
	CDECAP_HOMING_START,
	CDECAP_HOMING_H_SEARCH,
	CDECAP_HOMING_H_REACH,
	CDECAP_HOMING_H_WAIT,
	CDECAP_HOMING_L_SEARCH,
	CDECAP_HOMING_L_REACH_OFF,
	CDECAP_HOMING_L_WAIT,
	CDECAP_HOMING_S_SEARCH,
	CDECAP_HOMING_S_REACH,
	CDECAP_HOMING_S_WAIT,
	CDECAP_HOMING_Z_DONE,
	CDECAP_HOMING_Y_HOME,
	CDECAP_HOMING_Y_WAIT,
	CDECAP_HOMING_COMPLETE,
}CDecap_HomingStep_t;

typedef struct{
	CDecap_HomingStep_t Step;
    int Dir;

    U32 H_Acc;
    S32 H_Vel;
    U32 L_Acc;
    S32 L_Vel;

} sHomingSequence_t;

typedef struct{
    struct{
        int HomingRun;
        int HomingComplete;
        int Run;
        int Dir;
        int PositiveLimit;
        int NegativeLimit;
        int HomeSensor;
    } Bit;
} sDriveStatus_t;


typedef struct{
    int isSWLimit;
} SystemInfo_t;

typedef enum{
    LONGRUN_DECAP = 0,
    LONGRUN_CAP
} teLongRunPhase;

typedef struct{
    S32 Motor_CurPos;
    S32 Motor_TargetPos[2];

    teLongRunPhase phaseDecapCap;
    U32 LongRunCount;
    U08 SpeedPercent;

    SystemInfo_t SystemInfo;

    int debug[DEBUG_CD_SIZE];
    S32 chMotor;

} tsXCD_Decapper;

typedef struct{
    F32 RunCur[2];
    int SelMaxCur[2];
    int StopCurRate[2];
    int StepResolution;
    //int PowerEnable[2];

    //Z,Y,R Limit POS 지정
    S32 Limit_PosZ;
    S32 Limit_PosR;

    //하드웨어적 최대 이동 위치
    S32 SwNegLimit[STEP_CH_MAX];
    S32 SwPosLimit[STEP_CH_MAX];
    U8 SoftLimitEnable;

    //Cap_Decap시 필요한 PL --> DB에 상세히 기록
    U32 ZDecapAcc;
    S32 ZDecapVel;
    S32 ZCap_UpPos;
    S32 ZCap_SidePos;
    S32 ZCap_Origin_Position;

    U32 RDecapAcc;
    S32 RDecapVel;
    S32 RDecapPos; // pulse 증분: CAP은 현재 R + 설정값, DECAP은 현재 R - 설정값

    int ForDebug[DEBUG_PL_SIZE];

} tsXPL_Decapper;

typedef struct{

	bool Cap_is;
    bool Body_is;

    unsigned char Z_HL_isError;
    unsigned char Y_HL_isError;
    unsigned char CT_Cap_Grip_isError;
    unsigned char CT_Body_Grip_isError;

    //int ForDebug[DEBUG_SL_SIZE];

    //sMotionStatus_t MotionStatus[STEP_CH_MAX];

} tsXSL_Decapper;

typedef struct{

	void (*Senser_Update)(void);
    void (*Status_Update)(void);
    void (*CT_Cap_Body_Update)(void);

    void (*CheckSensorValidity)(void);

    void (*Action_Fnc)(teXActionType actionType);       //
} tsxCDcap;

typedef enum{
    SUB_IDLE = 0,
    SUB_zRMOVE,
	SUB_rRMOVE,
	SUB_zAMOVE,
	SUB_rAMOVE,
	SUB_yHMOVE,
	SUB_yLMOVE,
	SUB_HOMEZ,
	SUB_HOMEY,
	SUB_sMOVE,
	SUB_BODY_GRIP_WAIT,
	SUB_CAPPING,
	SUB_DECAPPING,
    SUB_ACTION_COUNT,
	SUB_MAX
} teDecapperSubIndex;

typedef enum{
    MOVE_IDLE = 0,
    MOVING,
    MOVE_DONE
} move_Step_t;

typedef enum{
	Recovery__IDLE = 0,
	Recovery_Z,
	Recovery_Y,
	Recovery_COMPLETE,
	Recovery__DONE
} Recovery_Step_t;

typedef enum{
	CDecapping_Idle = 0,
	CDecapping_Body_Grip,
	CDecapping_Move_Pos,

	CDecapping_Capping_Ready,
	CDecapping_Decapping_Ready,

	CDecapping_Cap_Capgrip,

	CDecapping_sData_Capping,
	CDecapping_sData_Decapping,

	CDecapping_Move_PrevR,
	CDecapping_Move_PrevZ,
	CDecapping_Move_PrevY,
	CDecapping_Complete_Wait,
	CDecapping_Done,

} CDecapping_Statmachin_state;

typedef struct{
	unsigned char Z_H_Limit_Sensor;
	unsigned char Z_L_Limit_Sensor;
	unsigned char Y_H_Limit_Sensor;
	unsigned char Y_L_Limit_Sensor;
	unsigned char CT_Body_Grip_Detect_Open;
	unsigned char CT_Body_Grip_Detect_Close;
	unsigned char CT_Cap_Grip_Detect_Open;
	unsigned char CT_Cap_Grip_Detect_Close;
	unsigned char CT_Detect_Sensor;
} xSLecapping_Sensor;

typedef enum{
	Ready_Idle= 0,
	Ready_Y_L,
	Ready_Done,
}Ready_Statemachin_state;

typedef struct{
	unsigned char Motor_Run;
} xSLecapping_MotorRun;

typedef struct{
	unsigned char Cap;
	unsigned char Body;
} xCDecapping_Grip;

typedef enum{
    PAUSE_Y_NONE = 0,
    PAUSE_Y_HIGH,
    PAUSE_Y_LOW
} Pause_Y_Direction_t;

typedef struct{
    bool isPaused;
    bool resumeZ;
    bool resumeR;
    bool resumeY;

    S32 zTargetPos;
    S32 rTargetPos;

    U32 zAcc;
    S32 zVel;

    U32 rAcc;
    S32 rVel;

    Pause_Y_Direction_t yDirection;
    teFSM_Decapper savedPhase;
    U32 pauseStartTick;

} Pause_Context_t;

extern tsxCDcap xCDecap;

//Initialize
/**
 * @brief 완전히 대기 중일 때만 Decapper 오류와 공통 오류 코드를 해제한다.
 * 동작·대기 명령·유지보수가 있으면 아무것도 바꾸지 않는다. HOME 완료 상태를 새로 만들지는 않는다.
 */
void CDecap_Error_Clear(void);
/**
 * @brief STOP을 독립 플래그와 xAT에 기록하여 다음 제어 주기에 우선 처리하게 한다.
 * 임계 구역에서 기록하므로 뒤따르는 PAUSE/RESUME가 xAT를 바꿔도 STOP 요청은 남는다.
 * 이 함수 자체가 하드웨어 정지 완료를 기다리지는 않는다.
 */
void CDecap_RequestStop(void);
/**
 * @brief 유지보수·STOP·대기 액션이 없고 FSM/Busy 및 샘플된 Z/R 구동 상태가 모두 대기인지 반환한다.
 * 오류 유무와는 별도 판정이다. 오류 상태에서도 완전히 멈췄으면 CLER를 허용하기 위해 true가 될 수 있다.
 */
bool CDecap_IsIdle(void);
/**
 * @brief 저장·초기화 등 유지보수 작업이 일반 동작을 차단 중인지 반환한다.
 */
bool CDecap_IsMaintenanceActive(void);
/**
 * @brief 임계 구역에서 대기 여부 확인과 유지보수 잠금 획득을 함께 수행한다.
 * true일 때만 작업을 시작하며 성공한 호출은 모든 종료 경로에서 EndMaintenance와 짝을 맞춘다.
 * false이면 잠금을 얻지 못했으므로 타이머 정지나 설정 변경을 하면 안 된다.
 */
bool CDecap_BeginMaintenance(void);
/**
 * @brief 획득한 유지보수 잠금을 임계 구역에서 해제한다.
 * 주기 타이머를 정지한 호출부는 타이머를 복구한 뒤 이 함수를 호출한다.
 */
void CDecap_EndMaintenance(void);
/**
 * @brief 대기·오류 없음 확인과 일반 액션 등록을 임계 구역에서 함께 처리한다.
 * true는 요청 접수이며 완료가 아니다. false면 기존 요청을 유지한다.
 * 호출부가 유효한 일반 액션을 전달해야 한다. STOP은 RequestStop으로 별도 요청한다.
 */
bool CDecap_TryRequestAction(teXActionType action);
/**
 * @brief 유지 중인 Decapper 오류를 xSL 및 공통 오류 조회 값과 동기화한다.
 * isError가 있고 저장된 원인이 없으면 상태 오류 2820을 사용한다. 모션 출력은 변경하지 않는다.
 */
void CDecap_ReportError(void);
/**
 * @brief Decapper 콜백, 상태 머신, Z/R 드라이버와 공압 출력을 초기화한다.
 * PL을 읽은 뒤 호출한다. 전류·분해능·리미트를 적용하고 위치 카운터를 0으로 설정한다.
 * 이때의 0은 기구 원점 확정이 아니므로 isHomed는 false이며 HOME이 별도로 필요하다.
 */
void CDecap_Init(void);
//Update SigDate============================================
/**
 * @brief IO 확장기의 Z/Y 리미트, 그리퍼, CT 감지 입력을 xSL에 복사한다.
 * 리미트는 후속 시퀀스에서 0을 도착으로 해석한다. 이 함수 자체는 출력을 변경하지 않는다.
 */
void CDecap_Sensor_Update(void);
/**
 * @brief 현재 헤더에만 남아 있는 미구현 선언이다. 이 프로젝트에서 정의나 호출은 없으며 센서 갱신은 등록된 콜백을 사용한다.
 */
void CDecap_M_S_Detect(void);
/**
 * @brief TMC2660에서 Z/R 모터의 구동 여부를 읽어 xSL에 반영한다.
 * 이름의 Y와 달리 실제 대상은 aZ와 aR이다. 공압 Y축 완료는 리미트 센서로 판단한다.
 */
void YZ_Motor_Status_Update(void);
/**
 * @brief 그리퍼의 Open/Close 입력이 모두 0이면 물체를 잡은 상태로 해석한다.
 * 이 판정으로 xSL.Decapper의 Body_is와 Cap_is를 갱신한다. 별도 CT 감지 입력 판정과는 구분한다.
 */
void Cap_CAP_Body_Detect_Sensor(void);
//Diagnose==================================================
/**
 * @brief 센서 조합의 이상을 xSL.Decapper 오류 플래그에 누적한다.
 * Z/Y 리미트가 모두 0이거나 그리퍼 Open/Close가 모두 1이면 해당 오류를 설정한다.
 * 정상 입력으로 돌아와도 자동 해제하지 않는다. 이 센서 오류를 정지에 연결하는 FSM 인터록은 현재 주석 처리되어 있다.
 */
void CDecapping_CheckSensorValidity(void);
//Application===============================================
/**
 * @brief 애플리케이션 제어 주기마다 요청을 해석하고 현재 FSM 핸들러를 한 번 실행한다.
 * STOP을 최우선 처리하고 유지보수·오류 중에는 일반 동작을 막는다. 일반 요청은 IDLE에서만 시작한다.
 * PAUSE/RESUME 조건과 이동 범위를 검사한다. HOME은 좌표 확립 중이므로 현재 위치 범위 감시에서 제외한다.
 * 센서 조합 오류를 정지에 연결하는 블록은 기구 확인 전까지 주석 상태로 유지되어 있다.
 */
void CDecap_Action_Statemachine(teXActionType actionType);
/**
 * @brief Body 그리퍼 요청값을 xCD.CT.Body에 저장하고 수동 요청 플래그를 설정한다.
 * 즉시 출력하지 않으며 호출부가 ACTION_BODY_GRIP을 별도로 등록해야 한다.
 */
void CDecap_SetBodyGripCommand(U08 onoff);
/**
 * @brief Cap 그리퍼 요청값을 xCD.CT.Cap에 저장하고 수동 요청 플래그를 설정한다.
 * 즉시 출력하지 않으며 호출부가 ACTION_CAP_GRIP을 별도로 등록해야 한다.
 */
void CDecap_SetCapGripCommand(U08 onoff);
/**
 * @brief TMC429의 현재 Z pulse 좌표를 읽어 반환하고 xCD의 표시용 위치도 갱신한다.
 */
S32 CDecap_GetZPosition(void);
/**
 * @brief 새 이동 명령에 적용할 전체 속도 비율을 저장한다.
 * percent는 1~100만 허용하며 범위 밖이면 기존 값을 유지한다. 진행 중인 드라이버 명령을 재발행하지 않는다.
 */
void CDecap_SetSpeedPercent(U08 percent);
/**
 * @brief 현재 xCD에 저장된 전체 속도 비율(%)을 반환한다.
 */
U08 CDecap_GetSpeedPercent(void);
#endif /* APP_HW_DEVICE_CDCAP_H_ */
