/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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
/* USER CODE END Includes */

void xCreateTask(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
