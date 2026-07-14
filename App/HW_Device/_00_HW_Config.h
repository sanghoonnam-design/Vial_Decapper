/** *****************************************************************************
 * _00_HW_Config.h
 *
 * Created on: 2024.10.14
 * Author    : RND. Kang PilSoon.
 *
 * @note:
 *       1. 주제어기 HW 정보를 define 한 헤더파일
 *
 ******************************************************************************/
#ifndef _00_HW_CONFIG_H_
#define _00_HW_CONFIG_H_

#include <stdint.h>

/**
 * ============================================================= [HW BSP]
 * */
#include "ITIMER.h" // HW Timer(3) 관련 헤더파일
#include "LED.h"    // status LED, Test Port 관련 헤더파일
#include "IOEXP.h"  // Digital IO 관련 헤더파일
#include "IWDG.h"   // watchDog 헤더파일
#include "IUART.h"  // Serial (CPU)  : USB x 1ch, RS485 x 2 ch
#include "EEPROM.h" // EEPROM 관련 헤더파일
#include "ICAN.h"   // CAN 통신 관련 헤더파일
#include "switch.h" // switch (4channel) 관련 헤더파일
#include "TEMP.h"   //
#include "IADC.h"   //
#include "IPWM.h"   //
#include "Drive.h"  //

/**
 * ============================================================= [HW 추상화]
 * */
#include "HW_Serial.h"
#include "HW_RS485_Process.h"
#include "HW_CAN_Process.h"
#include "HW_WatchDog.h"
#include "HW_TriggerPort_StatusLED.h"
#include "HW_xTestCode.h"
#include "HW_Network.h"

/** ******************************************************************
 * @brief Platform Configuration
 * @note HW 사용여부를 셋팅하는 함수
 ******************************************************************* */
#define _ConfigUSE_HW_RS485_CH_1_ /*   */ (1)
#define _ConfigUSE_HW_RS485_CH_2_ /*   */ (0) // [주의] 소형제어기는 없음, 반드시 0
#define _ConfigUSE_HW_WATCH_DOG_ /*    */ (0) // 와치독 사용 여부
/*********************************************************************/

typedef struct
{
  uint8_t Pin;
  uint8_t On;
  uint8_t Off;

} tDO_Table;

/**
 * @brief Mnemonic: Status LED
 */
#define XHW_STATUS_LED_NUM (4)   // 원본 수정: 2 3 0 1
#define XHW_STATUS_LED_1 STATUS0 // LED_1) // used O : Task debugging or TCP comm.
#define XHW_STATUS_LED_2 STATUS1 // LED_2) // used O : Task debugging
#define XHW_STATUS_LED_3 STATUS2 // LED_3) // used O : Task debugging
#define XHW_STATUS_LED_4 STATUS3 // LED_4) // used O : Task debugging

/**
 * @brief Mnemonic: Test Port
 */
#define XHW_TEST_PORT_NUM (4)
#define XHW_TEST_PORT_1 TP11
#define XHW_TEST_PORT_2 TP10
#define XHW_TEST_PORT_3 TP12
#define XHW_TEST_PORT_4 TP15
#define XHW_TEST_PORT_5 TP16

/**
 * @brief Switch Mnemonic : Digital Input
 */
#define XHW_SWITCH_NUM (SW_COUNT)
#define XHW_SWITCH_1 SW1
#define XHW_SWITCH_2 SW2
#define XHW_SWITCH_3 SW3
#define XHW_SWITCH_4 SW4
#define XHW_SWITCH_ALL SW_ALL // read

#define FOR_ALL_SWITCH for (int i = SW1; i <= SW4; i++)

/**
 * @brief Step Motor Channel Mnemonic 
 */
#define XHW_STEP_CH_0 (STEP_CH0) 
#define XHW_STEP_CH_1 (STEP_CH1)   
// #define FOR_ALL_STEP for (int i = 0; i <= 2; i++)

/**
 * @brief DI: Mnemonic : digital input/output port number
 * */
#define XHW_PIN_DI_0 (0) // TODO
#define XHW_PIN_DI_1 (1)
#define XHW_PIN_DI_2 (2)
#define XHW_PIN_DI_3 (3)
#define XHW_PIN_DI_4 (4)
#define XHW_PIN_DI_5 (5)
#define XHW_PIN_DI_6 (6)
#define XHW_PIN_DI_7 (7)
#define XHW_PIN_DI_8 (8)
#define XHW_PIN_DI_9 (9)
#define XHW_PIN_DI_10 (10)
#define XHW_PIN_DI_11 (11)
#define XHW_PIN_DI_12 (12)
#define XHW_PIN_DI_13 (13)
#define XHW_PIN_DI_14 (14)
#define XHW_PIN_DI_15 (15)

#define XHW_PIN_DO_0 (0)
#define XHW_PIN_DO_1 (1)
#define XHW_PIN_DO_2 (2)
#define XHW_PIN_DO_3 (3)
#define XHW_PIN_DO_4 (4)
#define XHW_PIN_DO_5 (5)
#define XHW_PIN_DO_6 (6)
#define XHW_PIN_DO_7 (7)
#define XHW_PIN_DO_8 (8)
#define XHW_PIN_DO_9 (9)
#define XHW_PIN_DO_10 (10)
#define XHW_PIN_DO_11 (11)
#define XHW_PIN_DO_12 (12)
#define XHW_PIN_DO_13 (13)
#define XHW_PIN_DO_14 (14)
#define XHW_PIN_DO_15 (15)

#define XHW_PWM_CH_0 PWM_CH0
#define XHW_PWM_CH_1 PWM_CH1

#define XHW_TEMP_CH_0 TEMP_CH0
#define XHW_TEMP_CH_1 TEMP_CH1
#define XHW_TEMP_CH_2 TEMP_CH2
#define XHW_TEMP_CH_3 TEMP_CH3

#define XHW_PIN_DI_FIRST XHW_PIN_DI_0
#define XHW_PIN_DI_END XHW_PIN_DI_15
#define XHW_PIN_DI_NUM (16)

#define XHW_PIN_DO_FIRST XHW_PIN_DO_0
#define XHW_PIN_DO_END XHW_PIN_DO_15
#define XHW_PIN_DO_NUM (16)

#define FOR_ALL_DI /*   */ for (int i = XHW_PIN_DI_FIRST; i <= XHW_PIN_DI_END; i++)
#define FOR_ALL_DI_1 /* */ for (i = XHW_PIN_DI_FIRST; i < XHW_PIN_DI_END; i++)
#define FOR_ALL_DO /*   */ for (int i = XHW_PIN_DO_FIRST; i <= XHW_PIN_DO_END; i++)
#define FOR_ALL_DO_1 /* */ for (i = XHW_PIN_DO_FIRST; i < XHW_PIN_DO_END; i++)

/**
 * @brief Mnemonic: ADC 파트
 * @note index를 보드 커넥터 순서대로 다시 정의함.
 *       FADC_GetDigitalValue() 함수 추가함.
 */
#define XHW_ADC_NUM (2)
#define XHW_ADC_CH_0 ADC_CH0 // 0~20mA, 보드 실크마킹: AD2
#define XHW_ADC_CH_1 ADC_CH1 // 0~10V,  보드 실크마킹: A_IN0

#define FOR_ALL_ADC for (int i = 0; i < XHW_ADC_NUM; i++)

#endif //@end: _00_HW_CONFIG_H_
