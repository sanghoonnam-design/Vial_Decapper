/*******************************************************************************
 * XDiagnose.h
 *
 *  Created on: 2025.06.20
 *      Author: RND. Kang PilSoon.
 * 
 ******************************************************************************/

#ifndef __XDIAGNOSE_H__
#define __XDIAGNOSE_H__

#include "XGlobal.h"

extern Semaphore_Handle semHD_SDG;
extern BOOL DB_isDiagnosisDisabled; // for debugging

//==============================================================================
//==============================================================================
VOID TASK_Diagnose(void *pvParameters);

int Init_Diagnose(int Index);
//==============================================================================
//==============================================================================

// @USER CODE - START

VOI SDG_RobotDoor_CheckControlAvailability(void);
VOI SDG_ServoA6_CheckControlAvailability(void);

U08 SDG_RobotDoor_CheckError(int command);
U08 SDG_ServoA6_CheckError(int controlMode);

// @USER CODE - END
//==============================================================================
#endif  /* __XDIAGNOSE_H__ */
