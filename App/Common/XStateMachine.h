/*******************************************************************************
 * XStateMachine.h
 *
 *  Created on: 2025.09.20
 *      Author: RND. Kang PilSoon.
 *
 *  @brief Generic State Machine Utilities
 *  - 상태전이 helpers
 *  - Delay/Timeout handling
 *  - Debug-friendly MACRO
 *
 ******************************************************************************/
#ifndef X_STATE_MACHINE_H_
#define X_STATE_MACHINE_H_

#include "_01_XSystemManagement.h"
#include "XDebug.h"

/** ************************************************************************
 * @brief 공통 Step 함수 포인터
 * ************************************************************************/
typedef void (*FSM_StepHandler)(void);

//==========================================================================
// Constants & Macros
//==========================================================================
#define SKIP_STEP (0xFF) ///< nextStep 생략 시 사용, 쀍!~

//==========================================================================
// Inline Helper Functions
//==========================================================================
/**
 * @brief Delay 기반 Step 전환 시작
 * @param nextStep  : 완료 후 이동할 step
 * @param pStep     : 현재 step 포인터
 * @param pWait     : 시작 time 저장 포인터
 */
static inline void FSM_StartAction(U08 nextStep, U08 *pStep, U32 *pWaitTick)
{
  *pWaitTick = gTriggerCount;
  *pStep = nextStep;
}

/**
 * @brief Timeout 체크 후 Step 전환
 * @param nextStep  : 완료 후 이동할 step
 * @param duration  : 지연 시간 (ms 단위)
 * @param pStep     : 현재 step 포인터
 * @param pWait     : 시작 시각 포인터
 * @retval true     : timeout 발생 -> step 전환
 * @retval false    : 아직 대기 중
 */
static inline int FSM_CheckTimeout(U08 nextStep, U32 duration, U08 *pStep, U32 *pWaitTick)
{
  if ((gTriggerCount - *pWaitTick) >= (duration / 10))
  {
    if (nextStep != SKIP_STEP)
      *pStep = nextStep;

    LOG_MSG_SEND("[%s] Timeout reached (%u ms) → Step=%d",
                 __func__, duration, *pStep);
    return 1;
  }
  return 0;
}

/**
 * @brief Delay 시작 (복귀 Step 저장 + duration 세팅)
 * @param delayStep     : delay 전용 step
 * @param nextStep      : 완료 후 복귀할 step
 * @param pStep         : 현재 step 포인터
 * @param pReturnStep   : 복귀 Step 저장 포인터
 * @param pWaitTick     : 시작 시각 저장 포인터
 * @param pDuration     : 지연 시간 저장 포인터 (ms 단위)
 * @param duration_ms   : 지연 시간 (ms 단위)
 */
static inline void FSM_StartDelay(
    U08 delayStep, U08 nextStep,
    U08 *pStep, U08 *pReturnStep,
    U32 *pWaitTick, U32 *pDuration_ms,
    U32 duration_ms)
{
  *pStep = delayStep;          // Delay 상태 진입
  *pReturnStep = nextStep;     // 복귀 Step 저장
  *pWaitTick = gTriggerCount;  // 시작 tick 저장
  *pDuration_ms = duration_ms; // 지연 시간 저장
}

/**
 * @brief Delay 완료 여부 확인
 * @param pStep         : 현재 step 포인터
 * @param pReturnStep   : 복귀 Step 저장 포인터
 * @param pWaitTick     : 시작 시각 포인터
 * @param pDuration     : 지연 시간 포인터 (ms 단위)
 * @retval 1            : timeout 발생 → step 전환 완료
 * @retval 0            : 아직 대기 중
 */
static inline int FSM_CheckDelay(
    U08 *pStep, U08 *pReturnStep,
    U32 *pWaitTick, U32 *pDuration_ms)
{
  if ((gTriggerCount - *pWaitTick) >= (*pDuration_ms / 10))
  {
    *pStep = *pReturnStep; // 저장된 복귀 Step으로 전환
    *pReturnStep = 0;      // 초기화
    *pDuration_ms = 0;     // 초기화
    return 1;
  }
  return 0;
}

#endif /* X_STATE_MACHINE_H_ */