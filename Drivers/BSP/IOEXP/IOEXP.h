#ifndef   __IOEXP_H__
#define   __IOEXP_H__

#include "project.h"

#ifdef __IOEXP_C__
	#define IOEXP_EXT
#else
	#define IOEXP_EXT extern
#endif

#define IO_CH0     0
#define IO_CH1     1

  //When bank bit is 0

  //I/O Direction Register 
  //1 = input, 0 = output
#define REG_IODIR       0x00

  //Input Polarity Port Register 
  //1 = opposite logic state, 0 = same logic state
#define REG_IPOL        0x02

  //Interrupt on change pins, Used with DEFVAL AND INTCON REGISTER
  //1 = Enable, 0 = Disable
#define REG_GPINTEN     0x04

  //Default value register
#define REG_DEFVAL      0x06

  //Interrupt on change control register
  //1 = pin value is compared against DEFVAL
  //0 = pin value is compared against previous pin value
#define REG_INTCON      0x08

#define REG_IOCON       0x0A

  //GPIO PULL-UP RESISTOR REGISTER
  //1 = PULL-UP ENABLED
  //0 = PULL-UP DISBLED
#define REG_GPPU        0x06

  //Interrupt Flag Register
  //1 = Pin caused interrupt
  //0 = Interrupt not pending
#define REG_INTF        0x0C

  //Interrupt captured value for port register
  //1 = logic high
  //0 = logic low
#define REG_INTCAP      0x10

  //General purpose i/o port register
#define REG_GPIO        0x12

#define REG_OLAT        0x14

#define READ_IN         0
#define READ_OUT        1

#define IN0_7           0
#define IN8_15          1

#define OUT0_7          0
#define OUT8_15         1
#define OUT16_23        2

#define NUM_OUT (16)  // by KPS
#define NUM_IN  (16)  // by KPS

IOEXP_EXT void IOEXP_Init(void);

IOEXP_EXT U8   IOEXP_ReadIO(void);
IOEXP_EXT void IOEXP_WriteIO(U16 val);

IOEXP_EXT void IOEXP_WriteIObit(U8 bit, U8 OnOff);
IOEXP_EXT U8   IOEXP_ReadIObit(U8 inout, U8 bit);

IOEXP_EXT void IOEXP_WriteIOclear(void);

IOEXP_EXT void (*IOEXP_IntCallBack)(U8 ch);

IOEXP_EXT U8 (*digitalRead)(U8 inout, U8 ch);  // ch: 0~5, by KPS
IOEXP_EXT void (*digitalWrite)(U8 ch, U8 OnOff);
IOEXP_EXT void (*digitalClear)(void);

/** @note 비트출력 : by KPS */
IOEXP_EXT void IOEXP_GetDigitalInput_str(char *str, int size);
IOEXP_EXT void IOEXP_GetDigitalOutput_str(char *str, int size);

#endif


