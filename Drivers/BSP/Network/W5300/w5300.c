//*****************************************************************************
//
//! \file w5300.h
//! \brief W5300 HAL implement File.
//! \version 1.0.0
//! \date 2015/05/01
//! \par  Revision history
//!       <2015/05/01> 1st Released for integrating with ioLibrary 
//!        Download the latest version directly from GitHub. Please visit the our GitHub repository for ioLibrary.
//!        >> https://github.com/Wiznet/ioLibrary_Driver
//! \author MidnightCow
//! \copyright
//!
//! Copyright (c)  2015, WIZnet Co., LTD.
//! All rights reserved.
//! 
//! Redistribution and use in source and binary forms, with or without 
//! modification, are permitted provided that the following conditions 
//! are met: 
//! 
//!     * Redistributions of source code must retain the above copyright 
//! notice, this list of conditions and the following disclaimer. 
//!     * Redistributions in binary form must reproduce the above copyright
//! notice, this list of conditions and the following disclaimer in the
//! documentation and/or other materials provided with the distribution. 
//!     * Neither the name of the <ORGANIZATION> nor the names of its 
//! contributors may be used to endorse or promote products derived 
//! from this software without specific prior written permission. 
//! 
//! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//! AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
//! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//! ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
//! LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
//! CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
//! SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//! INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
//! CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
//! ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
//! THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#include <stdint.h>
#include "wizchip_conf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


extern uint8_t sock_remained_byte[_WIZCHIP_SOCK_NUM_];
extern uint8_t sock_pack_info[_WIZCHIP_SOCK_NUM_];

/***********************
 * Basic I/O  Function *
 ***********************/
//static SemaphoreHandle_t xMutex;
void WIZCHIP_Init(void)
{
/*
	xMutex = xSemaphoreCreateMutex();
	if (xMutex == NULL)
	{
		// 생성 실패 처리
		printf("Mutex Fail!\n");
		while (1);
	}
*/
}

#define WIZCHIP_ENTER_CRITICAL()   //xSemaphoreTake(xMutex, portMAX_DELAY)
#define WIZCHIP_EXIT_CRITICAL()    //xSemaphoreGive(xMutex)


void setTMSR(uint8_t sn,uint8_t tmsr)
{
   uint16_t tmem;
   WIZCHIP_ENTER_CRITICAL();
   tmem = WIZCHIP_READ(WIZCHIP_OFFSET_INC(TMS01R, (sn & 0xFE)));
   if(sn & 0x01)  tmem = (tmem & 0xFF00) | (((uint16_t)tmsr ) & 0x00FF) ;
   else tmem =  (tmem & 0x00FF) | (((uint16_t)tmsr) << 8) ;
   WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(TMS01R, (sn & 0xFE)),tmem);
   WIZCHIP_EXIT_CRITICAL();
}
   
uint8_t getTMSR(uint8_t sn)
{
	uint8_t ret;
	WIZCHIP_ENTER_CRITICAL();

   if(sn & 0x01)
      ret = (uint8_t)(WIZCHIP_READ(WIZCHIP_OFFSET_INC(TMS01R, (sn & 0xFE))) & 0x00FF);
   else
	  ret = (uint8_t)(WIZCHIP_READ(WIZCHIP_OFFSET_INC(TMS01R, (sn & 0xFE))) >> 8);
   WIZCHIP_EXIT_CRITICAL();
   return ret;
}

void setRMSR(uint8_t sn,uint8_t rmsr)
{
   uint16_t rmem;
   WIZCHIP_ENTER_CRITICAL();
   rmem = WIZCHIP_READ(WIZCHIP_OFFSET_INC(RMS01R, (sn & 0xFE)));
   if(sn & 0x01)  rmem = (rmem & 0xFF00) | (((uint16_t)rmsr ) & 0x00FF) ;
   else rmem =  (rmem & 0x00FF) | (((uint16_t)rmsr) << 8) ;
   WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(RMS01R, (sn & 0xFE)),rmem);
   WIZCHIP_EXIT_CRITICAL();
}
   
uint8_t getRMSR(uint8_t sn)
{
	uint8_t ret;
	WIZCHIP_ENTER_CRITICAL();
   if(sn & 0x01)
      ret = (uint8_t)(WIZCHIP_READ(WIZCHIP_OFFSET_INC(RMS01R, (sn & 0xFE))) & 0x00FF);
   else
	  ret = (uint8_t)(WIZCHIP_READ(WIZCHIP_OFFSET_INC(RMS01R, (sn & 0xFE))) >> 8);
   WIZCHIP_EXIT_CRITICAL();
   return ret;
}

uint32_t getSn_TX_FSR(uint8_t sn)
{
   uint32_t free_tx_size=0;
   uint32_t free_tx_size1=1;

   WIZCHIP_ENTER_CRITICAL();
   while(1)
   {
      free_tx_size = (((uint32_t)WIZCHIP_READ(Sn_TX_FSR(sn))) << 16) | 
                     (((uint32_t)WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn),2))) & 0x0000FFFF);                           // read
      if(free_tx_size == free_tx_size1) break;  // if first == sencond, Sn_TX_FSR value is valid.                                                          
      free_tx_size1 = free_tx_size;             // save second value into first                                                   
   }
   WIZCHIP_EXIT_CRITICAL();
   return free_tx_size;                                                    
}                                                                          

uint32_t getSn_RX_RSR(uint8_t sn)
{
   uint32_t received_rx_size=0;
   uint32_t received_rx_size1=1;

   WIZCHIP_ENTER_CRITICAL();
   while(1)
   {
      received_rx_size = (((uint32_t)WIZCHIP_READ(Sn_RX_RSR(sn))) << 16) |
                         (((uint32_t)WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn),2))) & 0x0000FFFF);
      if(received_rx_size == received_rx_size1) break;                                                                         
      received_rx_size1 = received_rx_size;                                      // if first == sencond, Sn_RX_RSR value is valid.
   }                                                                             // save second value into first                
   WIZCHIP_EXIT_CRITICAL();
   return received_rx_size + (uint32_t)((sock_pack_info[sn] & 0x02) ? 1 : 0);   
}


void wiz_send_data(uint8_t sn, uint8_t *wizdata, uint32_t len)
{
   uint32_t i = 0;

   WIZCHIP_ENTER_CRITICAL();
   if(len == 0)
   {
      WIZCHIP_EXIT_CRITICAL();  /* [BUG FIX] was missing before early return */
      return;
   }

   /* [BUG FIX] Original loop read wizdata[i+1] past buffer end when len is odd.
    * Process pairs first, then pad the trailing byte with 0x00. */
   for(i = 0; i + 1 < len; i += 2)
      setSn_TX_FIFOR(sn, ((uint16_t)wizdata[i] << 8) | wizdata[i+1])

   if(len & 1u)
      setSn_TX_FIFOR(sn, (uint16_t)wizdata[len - 1u] << 8)

   WIZCHIP_EXIT_CRITICAL();
}

void wiz_recv_data(uint8_t sn, uint8_t *wizdata, uint32_t len)
{
   uint16_t rd = 0;
   uint32_t i = 0;
   
   WIZCHIP_ENTER_CRITICAL();
   if(len == 0) return;
      
   for(i = 0; i < len; i++)
   {
      if((i & 0x01)==0)
      {
         rd = getSn_RX_FIFOR(sn);
         wizdata[i]   = (uint8_t)(rd >> 8);
      }
      else  wizdata[i] = (uint8_t)rd;  // For checking the memory access violation
   }
   sock_remained_byte[sn] = (uint8_t)rd; // back up the remaind fifo byte.
   WIZCHIP_EXIT_CRITICAL();
}

void wiz_recv_ignore(uint8_t sn, uint32_t len)
{
   uint32_t i = 0;

   WIZCHIP_ENTER_CRITICAL();
   for(i = 0; i < len ; i += 2) getSn_RX_FIFOR(sn)
   WIZCHIP_EXIT_CRITICAL();
}

