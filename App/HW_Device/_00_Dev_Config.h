/** ****************************************************************************
 * _00_Dev_Config.h
 *
 * Created on: 2025.07.27
 * Author    : RND. Kang PilSoon.
 *
 * @note: 0. 시스템에 사용되는 Device를 정의하는 헤더파일
 *
 * @note: History
 * 1. ver 10.0.0
 *
 ******************************************************************************/
#ifndef _00_DEV_CONFIG_H_
#define _00_DEV_CONFIG_H_

// slot index
typedef enum
{
  SLOT_UNKNOWN = -1,
  SLOT_1,
  SLOT_2,
  SLOT_3,
  SLOT_4,
  SLOT_5,
  SLOT_6,
  SLOT_COUNT
} teSlot_Index;
#define FOR_ALL_SLOT for (int i = 0; i < SLOT_COUNT; i++)

// Robot Door 관련 define
#define IPIN_ROBOT_DOOR_SENSOR_OPEN (0)
#define IPIN_ROBOT_DOOR_SENSOR_CLOSE (1)

#define DOOR_STEP_MOTOR_CH (0)

#endif //@end: _00_DEV_CONFIG_H_
