/** ***************************************************************************
******************************************************************************
* @file           : main.c
* @brief          : Main program body
******************************************************************************
*     Created on  : 2025.06.23
*     Author      : RND. Kang PilSoon.
*     Description :
*       1. HW Spec.
*          - CPU    : STM32F746ZE (216 MHz, FPU)
*          - Flash  : 1024 Kbytes
*          - RAM    : 320 Kbytes
*          - EEPROM : 32 KB
*******************************************************************************
*******************************************************************************/
#include "main.h"
#include "XGlobal.h"
#include "XSystemInfo.h"
#include "_01_XSystemManagement.h"
#include "_02_XUpdateSIGData.h"
#include "_04_XDiagnose.h"
#include "_06_XAppControl.h"
#include "_07_XSamplingFinalization.h"
#include "_10_XSerialCMD_Process.h"
#include "HW_Network.h"
#include "XDebug.h"
#include "_11_XFile.h"

int main(void)
{
  __HAL_DBGMCU_FREEZE_TIM2();
  __HAL_DBGMCU_FREEZE_TIM6();

  /*[1]. HW & Device & System 초기화 */
  System_Initialize();

  /*[2]. Task 초기화, 생성 */
  xCreateTask();

  /*[3]. start the HW Timer(ISR) */
  XTimer_Start();

  /*[4]. start the real-time scheduler. */
  vTaskStartScheduler();

  /*[-]. We should never get here as control is now taken by the scheduler. */
  while (1)
    printf("oops!~");
}

void xCreateTask(void)
{
  /*[]. Task & app. init */
  Init_Global(0);
  Init_SystemTrigger(0);
  Init_Debug(0);
  Init_UpdateSIGData(0);
  Init_ApplicationControl(0);
  Init_Diagnose(0);
  Init_SamplingFinalization(0);
  Init_SerialCommand(0);

  /*[]. main Task. */
  xTaskCreate(TASK_UpdateTriggerTime, /**     */ "TASK_mainUTT", /*   */ 256, NULL, TP_UPDATE_TRIGGER_TIME, /*  */ &gTaskHandle_UTT);
  xTaskCreate(TASK_UpdateSIGData, /*          */ "TASK_mainUSD", /*   */ 256, NULL, TP_UPDATE_SIG_DATA, /*      */ &gTaskHandle_USD);
  xTaskCreate(TASK_Diagnose, /*               */ "TASK_mainDG", /*    */ 256, NULL, TP_DIAGNOSE, /*             */ &gTaskHandle_SDG);
  xTaskCreate(TASK_ApplicationControl, /*     */ "TASK_mainAPP", /*   */ 512, NULL, TP_APPLICATION_CONTROL, /*  */ &gTaskHandle_APC);
  xTaskCreate(TASK_SamplingFinalization, /*   */ "TASK_mainSFZ", /*   */ 512, NULL, TP_SAMPLING_FINALIZATION, /**/ &gTaskHandle_SFZ);

  /*[]. TCP/IP & Serial Command Processing    */
  //// xTaskCreate(TASK_SerialCommandLoop, /*      */ "TASK_CMD_Serial", /* */ 256, NULL, TP_UART_COMMAND_PROCESS, /* */ &gTaskHandle_SCPT);
  xTaskCreate(TASK_Network, /*                */ "TASK_CMD_Net", /*    */ 512, NULL, TP_TCP_COMMUNICATION, /*    */ &gTaskHandle_NWPT);

  /*[]. DEBUG : USB : CLI */
  xTaskCreate(Task_USBCommandLineInterface, /**/ "Task_CLI_RX", /*     */ 512, NULL, TP_COMMAND_INTERFACE, /*    */ NULL);
  xTaskCreate(TASK_USBMessagePrint, /*        */ "TASK_CLI_TX", /*     */ 400, NULL, TP_MESSAGE_LOG_PRINT, /*    */ NULL);

  /*[]. RS485모듈 활성화(마스터로 사용 할 경우) */
  // #if configRS485_CHANNEL_1_ENABLE
  //  RS485_Init(); // 반드시 이 구간에서 실행.
  // #endif

  XFile_Init(); // 파일 시스템 초기화, 반드시 이 구간에서 실행.

  /*[]. watchdog task */
#if configWatchDog_ENABLE
  xTaskCreate(TASK_Watchdog, "Task_WDG", 128, NULL, 3, NULL);
#endif
}

/** @brief  IAR Embedded Workbench 환경에서 사용되는 특수 함수
 *          - C startup 코드 실행 전에 가장 먼저 호출되는 초기화 함수
 *  @note 0: .bss 및 .data 초기화 건너뜀
 *  @note 1: .bss 및 .data 초기화 수행됨
 */
int __low_level_init(void)
{
  return 1; // 꼭 1을 반환해야 .bss/.data 섹션이 초기화됨.
}
