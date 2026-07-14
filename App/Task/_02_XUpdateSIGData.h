/*******************************************************************************
 * XUpdateSigData.h
 *
 *  Created on: 2025.06.20
 *      Author: RND, Kang PilSoon.
 ******************************************************************************/
#ifndef __XUPDATESIGDATA_H__
#define __XUPDATESIGDATA_H__

#include "XGlobal.h"
#include "_01_XSystemManagement.h"

extern Semaphore_Handle semHD_USD;

/** @note USER CODE */
#define ACQUISITION_START_TIME_HUMIDITY /*  */ __2sec
#define ACQUISITION_START_TIME_CO2 /*       */ __2sec
#define ACQUISITION_START_TIME_FAN /*       */ __2sec

//==============================================================================
//==============================================================================
VOID TASK_UpdateSIGData(void *pvParameters);

int Init_UpdateSIGData(int Index);

//==============================================================================
//==============================================================================

#endif /* __XUPDATESIGDATA_H__ */
