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

    /* [주의] memset(0) 만으로는 SLOT_1(=0) 이 되어버린다.
     *        SLOT_UNKNOWN 은 -1 이므로 부팅시 명시적으로 넣어줘야 한다.
     *        (GSTA [07] 은 SlotNum + 1 로 보고하므로, 위치 미확정 = 0 이 되어야 함) */

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


    // ────────────────────────────────────────────────────────────────
    /** @note USER CODE - END                                       **/
    // ────────────────────────────────────────────────────────────────
}

void SystemDB_PL_Init_Factory(tsXParameterList *pl)
{
    // ────────────────────────────────────────────────────────────────
    /** @note USER CODE - START                                     **/
    // ────────────────────────────────────────────────────────────────
	pl->Decapper.SoftLimitEnable = ON;
	// Calculate Pulse Per Revolution
	pl->Decapper.SelMaxCur[aZ] = CUR_MAX_17A;
	pl->Decapper.SelMaxCur[aR] = CUR_MAX_17A; // Motor test 이후 수정 예정
	pl->Decapper.StepResolution = R_3200;

	pl->Decapper.SwNegLimit[aZ] = Z_Limmit_mm;
	pl->Decapper.SwPosLimit[aZ] = Z_Limmit_mm;

	pl->Decapper.SwNegLimit[aR] =  R_Limmit_mm;
	pl->Decapper.SwPosLimit[aR] =  R_Limmit_mm;

	pl->Decapper.RunCur[aZ] = 0.9f * 1.0f; // 0.9A * 100%
	pl->Decapper.StopCurRate[aZ] = 60;     // Run Current * 60%

	//Motor test 이후 수정예정
	pl->Decapper.RunCur[aR] = 0.9f * 1.0f; // 0.9A * 100%
	pl->Decapper.StopCurRate[aR] = 60;     // Run Current * 60%

	/*해당 PL값은 실제 기구에 테스트 하면서 수정 예정*/
	// 0번 모터 Master : Z / Slave : Y

	// 모터별 Limit mm 저장
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

void SystemDB_SL_Push(void)
{
}

void SystemDB_CD_Push(void)
{
}

void SystemDB_PL_Push(void)
{
}
