/*******************************************************************************
 * XSystemManagement.h
 *
 *  Created on: 2025.06.20
 *      Author: RND. Kang pilSoon.
 ******************************************************************************/
#ifndef __XSYSTEMMANAGEMENT_H__
#define __XSYSTEMMANAGEMENT_H__

#include "XGlobal.h"
#include "ITIMER.h"
#include "SWRTC.h"

extern Semaphore_Handle semHD_UTT;

/* define Trigger Count */
extern unsigned int gTriggerCount; // system time count
extern float gTriggerTime;

/* RTC 시간관리 */
extern SW_DateTime_t gSWRTC; // 소프트웨어 RTC 시간 정보
//==============================================================================
extern float _Hz;  // frequency : system frequency
extern float _dt;  // time : of sampling.
extern int _dt_ms; // time : of sampling.
extern float _sec; // sec

#define __ms (1)

#define __10msec (1)
#define __20msec (2)
#define __30msec (3)
#define __40msec (4)
#define __50msec (5)
#define __60msec (6)
#define __70msec (7)
#define __80msec (8)
#define __90msec (10)
#define __100msec (10)
#define __200msec (20)
#define __300msec (30)
#define __400msec (40)
#define __500msec (50)
#define __600msec (60)
#define __1sec (100)
#define __2sec (200)
#define __3sec (300)
#define __4sec (400)
#define __5sec (500)
#define __6sec (600)
#define __7sec (700)
#define __8sec (800)
#define __9sec (900)
#define __10sec (1000)
#define __15sec (1500)
#define __20sec (2000)
#define __25sec (2500)
#define __30sec (3000)
#define __60sec (6000)
#define __1min __60sec
#define __30min ((U32)(__1min * 30))
#define __1hour ((U32)(__1min * 60))
#define __1day ((U32)(__1min * 60 * 24))

#define __xTime_Per(__tcount) if ((gTriggerCount % __tcount) == 0)
#define __xTime_At(__tcount) if (gTriggerCount == __tcount)
#define __xTime_After(__tcount) if (gTriggerCount > __tcount)
#define __xTime_After_Per(__tcount1, __tcount2) if ((gTriggerCount > __tcount1) && ((gTriggerCount % __tcount2) == 0))
#define __xTime_Before(__tcount) if (gTriggerCount < __tcount)
#define __xTime_Until(__tcount) if (gTriggerCount < __tcount)
//==============================================================================

//==============================================================================
/* system LOG, time and operation mode management */
VOID TASK_UpdateTriggerTime(void *pvParameters); //
                                                 //
int Init_SystemTrigger(int Index);               //
float GetTriggerTime(void);                      // get system operation time
                                                 //
//==============================================================================

//==============================================================================
#if 0
#define XTimer_Start() Timer2_InterruptStart()
#define XTimer_Stop() Timer2_InterruptStop()
#define XTimer_GetStatus() Timer2_GetInterruptStatus()
#else
#define XTimer_Start()                            \
    {                                             \
        ITIMER_ResetSemaphores();                 \
        Timer2_InterruptStart();                  \
        xSystemInfo.pl_FW_Mode = FW_MODE_DEFAULT; \
    }
#define XTimer_Stop()                                \
    {                                                \
        Timer2_InterruptStop();                      \
        xSystemInfo.pl_FW_Mode = FW_MODE_TIMER_STOP; \
    }
#define XTimer_GetStatus() Timer2_GetInterruptStatus()
#endif
//==============================================================================

/** ****************************************************************************
 * @brief 리부팅 함수
 * @note
 *     1. 사용시 주의: 단독으로 사용하지 말것!~ 시스템이 죽음.,
 *     2. CMD_Handle_REBOOT() 함수 참조
 */
#define XRebooting() NVIC_SystemReset()
/**************************************************************************** */

//==============================================================================
VOI XTask_Idle_Start(void); // task idle mode 제어
VOI XTask_Idle_Stop(void);  //
//==============================================================================

//==============================================================================
void vApplicationIdleHook(void);         // idling task
/**/ void vIdle_CalculateCpuUsage(void); // CPU 사용률 계산 함수
/**/ F32 vIdle_GetCpuUsage(void);
//==============================================================================

//==============================================================================
void xPrint_SystemInfo(void);
/**/ void xCheck_ApplicationFW(void);
/**/ void xCheck_RtosStatus(void);
/**/ void xCheck_Peripheral(void);
/**/ void xCheck_Task(void);
/**/ void xCheck_Stack(void);
//==============================================================================

//==============================================================================
U32 GetCurrentDate(void); // 소프트웨어 RTC에서 날짜와 시간 정보를 가져오는 함수
void Log_CheckDateChange(void);
//==============================================================================

#endif /* __XSYSTEMMANAGEMENT_H__ */
