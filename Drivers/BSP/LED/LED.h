#ifndef   __LED_H__
#define   __LED_H__

#include "project.h"

#ifdef __LED_C__
	#define LED_EXT
#else
	#define LED_EXT extern
#endif

typedef enum
{
  STATUS0,
  STATUS1,
  STATUS2,
  STATUS3,
}ELedNum;

typedef enum
{
  LED_CW,
  LED_CCW,
}ELedDir;

typedef enum
{
  TP10,
  TP11,
  TP12,
  TP15,
  TP16,
}ETpNum;

#define TP10_HIGH()             (GPIOD->BSRR = GPIO_PIN_3)
#define TP10_LOW()              (GPIOD->BSRR = (uint32_t)GPIO_PIN_3<<16U)

#define TP11_HIGH()             (GPIOD->BSRR = GPIO_PIN_6)
#define TP11_LOW()              (GPIOD->BSRR = (uint32_t)GPIO_PIN_6<<16U) 

#define TP12_HIGH()             (GPIOF->BSRR = GPIO_PIN_10)
#define TP12_LOW()              (GPIOF->BSRR = (uint32_t)GPIO_PIN_10<<16U) 

#define TP15_HIGH()             (GPIOF->BSRR = GPIO_PIN_11)
#define TP15_LOW()              (GPIOF->BSRR = (uint32_t)GPIO_PIN_11<<16U) 

#define TP16_HIGH()             (GPIOG->BSRR = GPIO_PIN_6)
#define TP16_LOW()              (GPIOG->BSRR = (uint32_t)GPIO_PIN_6<<16U) 

LED_EXT void LED_Init(void);
LED_EXT void LED_OnOff(ELedNum num, U8 OnOff);
LED_EXT void LED_Toggle(ELedNum num);
LED_EXT void LED_Toggle_h(ELedNum num, U32 interval);  // 속도 조절용

LED_EXT void LED_DriveOnOff(U8 ch, ELedDir dir, U8 OnOff); // 스텝모터 LED

LED_EXT void TP_OnOff(ETpNum num, U8 OnOff);
LED_EXT void TP_Toggle(ETpNum num);


#endif






