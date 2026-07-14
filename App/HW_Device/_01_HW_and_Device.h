/*******************************************************************************
 * _01_HW_and_Device.h
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef _01_HW_AND_DEVICE_H_
#define _01_HW_AND_DEVICE_H_

#include "_00_HW_Config.h"
#include "_00_Dev_Config.h"

//============================================================= [Device]
#include "Dev_LED.h"
#include "Dev_RobotDoor.h"
#include "Dev_ServoMotor_A6.h"
#include "Dev_Temperature.h"
//============================================================= [Device]

/**
 * @brief 하드웨어(HW) & 시스템 장치(DEV) 초기화
 * @note  1. HW_Init() 부분은 모든 프로젝트에 동일하게 적용
 *        2. Dev_Init() 부분은 프로젝트에 맞게 수정해서 사용
 * */
void HWDev_Initialize(void); // HW & DEV 초기화
void HW_Init(void);          // 하드웨어 초기화
void DEV_Init(void);         // 프로젝트별 디바이스 초기화

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

#endif /* _01_HW_AND_DEVICE_H_ */
