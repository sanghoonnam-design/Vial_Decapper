/*******************************************************************************
 * XApControl.h
 *
 *  Created on: 2025.06.20
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef __XAPPCONTROL_H__
#define __XAPPCONTROL_H__

#include "XGlobal.h"

extern Semaphore_Handle semHD_APC;

//==============================================================================
//==============================================================================
VOID TASK_ApplicationControl(void *pvParameters);
int Init_ApplicationControl(int Index);

void APC_Control_RobotDoor(void); // 로봇도어 제어




//==============================================================================
//==============================================================================
#endif /* __XAPPCONTROL_H__ */
