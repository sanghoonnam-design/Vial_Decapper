/*******************************************************************************
 * XDefinition.h
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef XDEFINITION_H_
#define XDEFINITION_H_

//============================================================================== @start: ASCII
#define _xSTX (0x02)
#define _xETX (0x03)
#define _xSPACE (0x20)
#define _xCOMMA (0x2C)
#define _xCR (0x0D)
#define _xLF (0x0A)
//============================================================================== @end: ASCII

//============================================================================== @start: Status
#define MALFUNC (0)
#define NORMAL (1)

#define YES (1)
#define NO (0)

#define OFF (0)
#define ON (1)

#define START (1)
#define STOP (0)
#define RUN (1)

#if 0
#define CW (1)
#define CCW (0)
#else
#define CW (1)
#define CCW (-1)
#endif

#define NO_COMMA (0)
#define COMMA (1)
#define PLUS_A (2)

#define xDISABLE (0)
#define xENABLE (1)

#define CLOSE (0)
#define OPEN (1)

#define AUTO (0) // control mode
#define MANUAL (1)
#define READY (2)

#define EMPTY (0)
#define FULL (1)

#define DISCONNECTED (0) // The connection is not working.
#define CONNECTED (1)

#define NON_DETECTION (0)
#define DETECTION (1)

// r u going to give [task] Semaphore or not?
#define SEM_YES (1)
#define SEM_NO (0)

#ifndef HIGH
#define HIGH (GPIO_PIN_SET)
#endif
#ifndef LOW
#define LOW (GPIO_PIN_RESET)
#endif

#define AXIS_UNHOMED (0)
#define AXIS_HOMED (1)

#define TYPE_CHAR_ (0)
#define TYPE_INT__ (1)
#define TYPE_FLOAT (2)
//============================================================================== @end: status

//============================================================================== @start: etc.
#define FOR_PART(From, To) for (i = From; i <= To; i++)
#define FOREACH(x, n) for (int x = 0; x < (n); x++)
#define FOR(x, st, ed) for (int x = st; x < (ed); x++)
//============================================================================== @end: etc.

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//==============================================================================
// setter/getter MACRO
// 1. 인덱스가 없는 경우
#define SET_GET_FUNC_0(__prefix, __type, __field) \
  void __prefix##_Set_##__field(__type value);    \
  __type __prefix##_Get_##__field(void);

#define SET_GET_FUNC_0_IMPL(__prefix, __type, __field) \
  void __prefix##_Set_##__field(__type value)          \
  {                                                    \
    x##__prefix.__field = value;                       \
  }                                                    \
  __type __prefix##_Get_##__field(void)                \
  {                                                    \
    return x##__prefix.__field;                        \
  }

// 2. 인덱스가 1개 있는경우
#define SET_GET_FUNC_1(__prefix, __type, __field)           \
  void __prefix##_Set_##__field(U08 __index, __type value); \
  __type __prefix##_Get_##__field(U08 __index);

#define SET_GET_FUNC_1_IMPL(__prefix, __type, __field)     \
  void __prefix##_Set_##__field(U08 __index, __type value) \
  {                                                        \
    x##__prefix.__field[__index] = value;                  \
  }                                                        \
  __type __prefix##_Get_##__field(U08 __index)             \
  {                                                        \
    return x##__prefix.__field[__index];                   \
  }

// 두 개의 인덱스를 가진 getter, setter 구현 매크로 정의
#define SET_GET_FUNC_2(__prefix, __type, __field)                          \
  void __prefix##_Set_##__field(U08 __index1, U08 __index2, __type value); \
  __type __prefix##_Get_##__field(U08 __index1, U08 __index2);

// 두 개의 인덱스를 가진 getter, setter 구현 매크로 정의
#define SET_GET_FUNC_2_IMPL(__prefix, __type, __field)                    \
  void __prefix##_Set_##__field(U08 __index1, U08 __index2, __type value) \
  {                                                                       \
    x##__prefix.__field[__index1][__index2] = value;                      \
  }                                                                       \
  __type __prefix##_Get_##__field(U08 __index1, U08 __index2)             \
  {                                                                       \
    return x##__prefix.__field[__index1][__index2];                       \
  }
//==============================================================================

//==============================================================================
// 스크립트 파일로 system config 설정할 때 사용
// Enable/Diable 처리
#define CHECK_SET_FLAG(ARG1, ARG2)      \
  do                                    \
  {                                     \
    if (strcmp(key, ARG1) == 0)         \
    {                                   \
      if ((strcmp(data, "1") == 0) ||   \
          (strcmp(data, "y") == 0) ||   \
          (strcmp(data, "yes") == 0) || \
          (strcmp(data, "true") == 0))  \
      {                                 \
        ARG2 = 1;                       \
      }                                 \
      else                              \
      {                                 \
        ARG2 = 0;                       \
      }                                 \
      continue;                         \
    }                                   \
  } while (0)

#if 0
// 정수만 처리
#define CHECK_SET_PARAM(ARG1, ARG2)          \
  do                                         \
  {                                          \
    if (strcmp(key, ARG1) == 0)              \
    {                                        \
      errno = 0;                             \
      temp = (uint32_t)strtol(data, &ep, 0); \
      if (errno == 0 && ep != data)          \
      {                                      \
        ARG2 = temp;                         \
      }                                      \
      continue;                              \
    }                                        \
  } while (0)
#endif

// 정수, 소숫점을 구분해서 처리
#define CHECK_SET_PARAM(ARG1, ARG2)                          \
  do                                                         \
  {                                                          \
    if (strcmp(key, ARG1) == 0)                              \
    {                                                        \
      errno = 0;                                             \
      if (strchr(data, '.'))                                 \
      {                                                      \
        double tempDouble = strtod(data, &ep);               \
        if (errno != 0 || ep == data)                        \
        {                                                    \
          printf("Error: Invalid floating point number.\n"); \
        }                                                    \
        else                                                 \
        {                                                    \
          ARG2 = tempDouble;                                 \
        }                                                    \
      }                                                      \
      else                                                   \
      {                                                      \
        uint32_t temp = (uint32_t)strtol(data, &ep, 0);      \
        if (errno != 0 || ep == data)                        \
        {                                                    \
          printf("Error: Invalid integer number.\n");        \
        }                                                    \
        else                                                 \
        {                                                    \
          ARG2 = temp;                                       \
        }                                                    \
      }                                                      \
      continue;                                              \
    }                                                        \
  } while (0)
//==============================================================================

#define STEP_DELAY_START()   \
  waitCount = gTriggerCount; \
  step++;

#define STEP_DELAY(__time)                  \
  if ((gTriggerCount - waitCount) < __time) \
    break;

#define STEP_REPEATE1(__step, __count) \
  if ((++repeatCount1 < __count))      \
  {                                    \
    step = __step;                     \
  }                                    \
  else                                 \
  {                                    \
    step++;                            \
    repeatCount1 = 0;                  \
  }

#define STEP_REPEATE2(__step, __count) \
  if ((++repeatCount2 < __count))      \
  {                                    \
    step = __step;                     \
  }                                    \
  else                                 \
  {                                    \
    step++;                            \
    repeatCount2 = 0;                  \
  }
//==============================================================================

#endif /*@: XDEFINITION_H_*/
