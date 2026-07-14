/*******************************************************************************
 * XRingBuffer.h
 *
 * Created on: 2024.10.21
 * Author    : RND. Kang PilSoon.
 * - Ring Buffer
 * - 동적 메모리 할당 구조
 ******************************************************************************/
#ifndef XRINGBUFFER_H
#define XRINGBUFFER_H

#include "XGlobal.h"

/** 
 * \note 임베디드 시스템에서는 동적 메모리(malloc-쓰레드 안전보장 못함)를 피하고,
 *       정적 메모리 할당을 사용하는 것이 일반적임. 사용시 주의하여 사용
 *       - 여기에서는 freeRTOS의 pvPortMalloc() 함수를 사용함.(쓰레드 안전함)
 */
typedef struct RingBuffer
{
    U08 *buf; //
    U16 head; // push 위치
    U16 tail; // pop 위치
    U16 size; //
} tsXQueue;

/**
 * @brief Buffer 함수 프로토타입 선언
 * */
bool xQueue_Init(tsXQueue *pQ, U16 size);
void xQueue_Free(tsXQueue *pQ);
void xQueue_Clear(tsXQueue *pQ);
bool xQueue_Push(tsXQueue *pQ, U08 data);
bool xQueue_Pop(tsXQueue *pQ, U08 *data);
bool xQueue_Peek(tsXQueue *pQ, U08 *data);
bool xQueue_PeekIndex(tsXQueue *pQ, U16 index, U08 *data);

U16 xQueue_GetSize(tsXQueue *pQ);     // queue에 들어 있는 데이터 수 반환
U16 xQueue_GetCapacity(tsXQueue *pQ); // queue 사이즈 반환

/**
 * @brief debugging code
 * */
void xQueue_DisplayCore(U08 *buf,
                        U16 start,
                        U16 count,
                        U16 size,
                        teCharMode charMode);              // 공통 출력함수
void xQueue_Display(tsXQueue *pQ, teCharMode charMode);    // tail 부터 head 까지 출력(유효 데이터 출력)
void xQueue_DisplayAll(tsXQueue *pQ, teCharMode charMode); // 링버터 0번 부터 사이즈 만큼 모두 출력

#endif /* @end: XRINGBUFFER_H */