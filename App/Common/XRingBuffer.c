/*******************************************************************************
 * XRingBuffer.c
 *
 *  Created on: 2024.10.21
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "XRingBuffer.h"

bool xQueue_Init(tsXQueue *pQ, U16 size)
{
    // pQ->buf = (U08 *)malloc(size); // 사용시 쓰레드 안정성을 보장못함.
    pQ->buf = (U08 *)pvPortMalloc(size); // 안전함.
    if (pQ->buf == NULL)
    {
        return false;
    }
    pQ->head = pQ->tail = 0;
    pQ->size = size;
    return true;
}

void xQueue_Free(tsXQueue *pQ)
{
    if (pQ->buf != NULL)
    {
        free(pQ->buf);
        pQ->buf = NULL;
    }
    pQ->head = pQ->tail = pQ->size = 0;
}

void xQueue_Clear(tsXQueue *pQ)
{
    pQ->head = pQ->tail = 0;
    if (pQ->buf != NULL)
    {
        memset(pQ->buf, 0, pQ->size);
    }
}

bool xQueue_Push(tsXQueue *pQ, U08 data)
{
    if (xQueue_GetSize(pQ) == (pQ->size - 1))
    {
        return false; // full
    }
    pQ->buf[pQ->head++] = data;
    pQ->head %= pQ->size;

    return true;
}

bool xQueue_Pop(tsXQueue *pQ, U08 *data)
{
    if (xQueue_GetSize(pQ) == 0)
    {
        return false; // empty
    }
    *data = pQ->buf[pQ->tail++];
    pQ->tail %= pQ->size;

    return true;
}

bool xQueue_Peek(tsXQueue *pQ, U08 *data)
{
    if (xQueue_GetSize(pQ) == 0)
    {
        return false;
    }

    *data = pQ->buf[pQ->tail];
    return true;
}

bool xQueue_PeekIndex(tsXQueue *pQ, U16 index, U08 *data)
{
    if (index >= xQueue_GetSize(pQ))
    {
        return false; // 인덱스가 큐 크기보다 크면 오류
    }
    U16 pos = (pQ->tail + index) % pQ->size;
    *data = pQ->buf[pos];
    return true;
}

U16 xQueue_GetSize(tsXQueue *pQ)
{
    return (pQ->head - pQ->tail + pQ->size) % pQ->size;
}

U16 xQueue_GetCapacity(tsXQueue *pQ)
{
    return pQ->size; // 전체 용량 반환
}

// ⚠️TODO
void xQueue_DisplayCore(U08 *buf, U16 start, U16 count, U16 size, teCharMode charMode)
{
    if (buf == NULL)
    {
        printf("Buffer is not initialized.\n");
        return;
    }

    printf("Queue Data: ");
    for (U16 i = 0; i < count; i++)
    {
        U16 pos = (start + i) % size; // 순환 처리 (start는 0일 수도 있음)
        U08 data = buf[pos];

        switch (charMode)
        {
        case eASCII:
            if (data >= 32 && data <= 126)
                printf("%c ", data);
            else
                printf(". "); // 비출력 가능한 문자 대체
            break;

        case eBINARY:
            for (int bit = 7; bit >= 0; bit--)
                printf("%c", (data & (1 << bit)) ? '1' : '0');
            printf(" ");
            break;

        case eHEXA:
        default:
            printf("%02X ", data);
            break;
        }
    }
    printf("\n");
}

void xQueue_Display(tsXQueue *pQ, teCharMode charMode)
{
    if (pQ == NULL || pQ->buf == NULL)
    {
        printf("Queue is not initialized.\n");
        return;
    }

    U16 size = xQueue_GetSize(pQ);
    if (size == 0)
    {
        printf("Queue is empty.\n");
        return;
    }

    printf("Queue Data [Size: %d]: ", size);
    xQueue_DisplayCore(pQ->buf, pQ->tail, size, pQ->size, charMode);
}

void xQueue_DisplayAll(tsXQueue *pQ, teCharMode charMode)
{
    if (pQ == NULL || pQ->buf == NULL)
    {
        printf("Queue is not initialized.\n");
        return;
    }

    if (pQ->size == 0)
    {
        printf("Queue has no capacity.\n");
        return;
    }

    printf("Queue Data [Capacity: %d]: ", pQ->size);
    xQueue_DisplayCore(pQ->buf, 0, pQ->size, pQ->size, charMode);
}