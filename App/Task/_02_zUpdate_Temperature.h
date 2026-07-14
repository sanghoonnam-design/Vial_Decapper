/*******************************************************************************
 * _02_zUpdate_Temperature.h
 *
 *  Created on: 2025.06.25
 *      Author: RND, Kang PilSoon.
 ******************************************************************************/
#ifndef __ZUPDATE_TEMPERATURE_H__
#define __ZUPDATE_TEMPERATURE_H__

#include "XGlobal.h"
#include "_01_XSystemManagement.h"

//==============================================================================
//==============================================================================
VOID TASK_UpdateTemperature(void *pvParameters);

void Init_UpdateTemperature(void);

float USD_GetLatestTemp(U08 ch);
//==============================================================================
//==============================================================================

#endif /* __ZUPDATE_TEMPERATURE_H__ */
