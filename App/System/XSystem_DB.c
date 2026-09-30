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
teXActionType xAT;

/**
 * @brief SL(관측 상태), CD(명령 데이터), PL(설정값)의 RAM을 초기화한다.
 * PL의 영구 설정은 뒤이어 부팅 저장장치 초기화에서 로드하므로 여기서 공장값을 채우지 않는다.
 */
void SystemDB_Initialize(void)
{
    memset((char *)&xSL, 0, sizeof(tsXStateList));
    memset((char *)&xCD, 0, sizeof(tsXControlData));
    memset((char *)&xPL, 0, sizeof(tsXParameterList));

    //==========================================================================
    SystemDB_SL_Init();
    SystemDB_CD_Init();
    // SystemDB_PL_Init(); // [주의] PL은 부팅후 초기화 부분에서 EEPROM 쪽에서 실행된다.
}

/**
 * @brief 상태 데이터의 장비별 초기값을 넣는 확장 지점이다. 현재 실행문은 없다.
 */
void SystemDB_SL_Init(void)
{
    /** @note USER CODE - START */

    // xSL.Door.isInitialized = NO;
    // xSL.Door.isControllable = YES;

    // xSL.ServoA6.isInitialized = NO;
    // xSL.ServoA6.isControllable = YES;

    /** @note USER CODE - END */
}

/**
 * @brief 명령 데이터의 장비별 초기값을 넣는 확장 지점이다. 현재 실행문은 없다.
 * Decapper 속도 비율 등 구동 초기값은 CDecap_Init에서 설정한다.
 */
void SystemDB_CD_Init(void)
{
    /** @note USER CODE - START */

    /* Decapper의 추가 CD 초기값은 CDecap_Init에서 설정한다. */

    /** @note USER CODE - END */
}

/* 저장 설정을 읽은 뒤 필요한 장비별 보정을 위한 확장 지점. */
/**
 * @brief PL 로드 후 장비별 보정을 넣는 확장 지점이다.
 * 현재 함수 본문은 비어 있으며 직접 EEPROM 읽기나 설정 보정을 수행하지 않는다.
 */
void SystemDB_PL_Init(void)
{
    // ────────────────────────────────────────────────────────────────
    /** @note USER CODE - START                                     **/
    // ────────────────────────────────────────────────────────────────


    // ────────────────────────────────────────────────────────────────
    /** @note USER CODE - END                                       **/
    // ────────────────────────────────────────────────────────────────
}

/**
 * @brief 전달된 PL의 Decapper 전류·분해능·이동 범위와 CAP/DECAP 기본 목표를 채운다.
 * 위치는 매크로로 환산된 pulse 값이다. RAM에 기본값을 넣을 뿐 저장과 드라이버 적용은 호출부의 책임이다.
 */
void SystemDB_PL_Init_Factory(tsXParameterList *pl) {
    // ────────────────────────────────────────────────────────────────
    /** @note USER CODE - START                                     **/
    // ────────────────────────────────────────────────────────────────
	pl->Decapper.SoftLimitEnable = ON;
	// Calculate Pulse Per Revolution
	pl->Decapper.SelMaxCur[aZ] = CUR_MAX_17A;
	pl->Decapper.SelMaxCur[aR] = CUR_MAX_17A; // Motor test 이후 수정 예정
	pl->Decapper.StepResolution = R_3200;

	pl->Decapper.SwNegLimit[aZ] = -Z_Limmit_mm;
	pl->Decapper.SwPosLimit[aZ] = Z_Limmit_mm;

	pl->Decapper.SwNegLimit[aR] =  -R_Limmit_mm;
	pl->Decapper.SwPosLimit[aR] =  R_Limmit_mm;

	pl->Decapper.RunCur[aZ] = 0.9f * 1.0f; // 0.9A * 100%
	pl->Decapper.StopCurRate[aZ] = 60;     // Run Current * 60%

	//Motor test 이후 수정예정
	pl->Decapper.RunCur[aR] = 0.9f * 1.0f; // 0.9A * 100%
	pl->Decapper.StopCurRate[aR] = 60;     // Run Current * 60%

	/*해당 PL값은 실제 기구에 테스트 하면서 수정 예정*/
	// 모터 채널 0: Z, 채널 1: R. Y는 공압 출력으로 제어한다.

	// 모터별 제한 좌표 저장(pulse로 환산된 값)
	pl->Decapper.Limit_PosZ = Z_Limmit_mm;
	pl->Decapper.Limit_PosR = R_Limmit_mm;

	//CDecapping시 z축 속도 조정
	pl->Decapper.ZDecapAcc = ZDECAP_ACC;
	pl->Decapper.ZDecapVel = ZDECAP_VEL;
	//Cap 위까지 이동
	pl->Decapper.ZCap_UpPos = ZCAP_CAP_UP_POS;
	//Cap 옆까지 이동
	pl->Decapper.ZCap_SidePos = ZCAP_CAP_SIDE_POS;
	//원위치 이동
	pl->Decapper.ZCap_Origin_Position = ZCAP_ORIGIN_POSITION;

	//Capping, Decapping 시 Rotate Status 조정
	pl->Decapper.RDecapAcc = RDECAP_ACC;
	pl->Decapper.RDecapVel = RDECAP_VEL;
	pl->Decapper.RDecapPos = RDECAP_POS;

    // ────────────────────────────────────────────────────────────────
    /** @note USER CODE - END                                       **/
    // ──────
}

/**
 * @brief SL 반영 기능을 위한 빈 확장 함수이다. 현재 상태 복사나 하드웨어 갱신은 하지 않는다.
 */
void SystemDB_SL_Push(void)
{
}

/**
 * @brief CD 반영 기능을 위한 빈 확장 함수이다. 현재 액션 등록이나 출력은 하지 않는다.
 */
void SystemDB_CD_Push(void)
{
}

/**
 * @brief PL 반영 기능을 위한 빈 확장 함수이다. 현재 저장이나 드라이버 재설정은 하지 않는다.
 */
void SystemDB_PL_Push(void)
{
}
