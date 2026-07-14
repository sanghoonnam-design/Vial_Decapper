/*******************************************************************************
 * HW_WatchDog.h
 *
 * Created on: 2024.11.14
 * Author    : RND. Kang PilSoon.
 *
 * Note:
 *       1. main 태스크의 수행여부 상태를 체크해서 watchdog를 동작 시킨다.
 ******************************************************************************/
#ifndef __HW_WATCHDOG_H__
#define __HW_WATCHDOG_H__
#include "XGlobal.h"
#include "_00_HW_Config.h"

/** ************************************************************
 * @brief watchdog 활성화 여부 셋팅
 * @note  주석처리: 사용안하는 경우
 * ************************************************************/
#define configWatchDog_ENABLE _ConfigUSE_HW_WATCH_DOG_
/************************************************************ */

/** ************************************************************
 * @brief 5개의 메인 태스크의 수행여부를 판단하여 watchDog을 실행시킨다.
 *        이를 관리하기 위한 변수 선언.
 * *************************************************************/
#define NUM_TASKS (5)  // 5개의 main Task.
extern volatile bool __xTaskStatus[NUM_TASKS];

VOID TASK_Watchdog(void *pvParameters);

#endif //@end: __HW_WATCHDOG_H__