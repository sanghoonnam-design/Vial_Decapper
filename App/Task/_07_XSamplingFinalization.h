/*******************************************************************************
 * XSampligFinalization.h
 *
 *  Created on: 2025.06.20
 *      Author: RND, Kang PilSoon.
 ******************************************************************************/
#ifndef __XSAMPLINGFINALIZATION_H__
#define __XSAMPLINGFINALIZATION_H__

#include "XGlobal.h"

extern Semaphore_Handle semHD_SFZ;



//==============================================================================
//==============================================================================
VOID TASK_SamplingFinalization(void * pvParameters);

int Init_SamplingFinalization(int Index);

//==============================================================================
//==============================================================================

void SFZ_TaskMonitoring(void);

#endif /* __XSAMPLINGFINALIZATION_H__ */
