#define __IUART_C__
#include "IUART.h"
#undef __IUART_C__

#include <stdio.h>

#define RS485A_TX_ENABLE()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET)
#define RS485A_RX_ENABLE()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET)

#define BUFF_SIZE_U 1024 // usb
#define BUFF_SIZE_A 256 // 128, RS485
#define BUFF_SIZE_B 256 // 128, RS232

typedef struct QueU
{
    U8 buff[BUFF_SIZE_U];
    U32 head;
    U32 tail;
    U32 size;
} QueU_t;

typedef struct QueA
{
    U8 buff[BUFF_SIZE_A];
    U32 head;
    U32 tail;
    U32 size;
} QueA_t;

typedef struct QueB
{
    U8 buff[BUFF_SIZE_B];
    U32 head;
    U32 tail;
    U32 size;
} QueB_t;

static void UART1_Init(U8 baudrate, U8 parity, U8 stopbit); // usb
static void UART2_Init(U8 baudrate, U8 parity, U8 stopbit); // 485
static void UART4_Init(U8 baudrate, U8 parity, U8 stopbit); // 232

static void GPIO_Init(void);

void USART1_IRQHandler(void); //ISR
void USART2_IRQHandler(void);
void USART4_IRQHandler(void);

static void QueU_Clear(QueU_t *pQue);
static U8 QueU_PutByte(QueU_t *pQue, U8 data);
static U8 QueU_GetByte(QueU_t *pQue, U8 *data);
static U32 QueU_GetSize(QueU_t *pQue);

static void QueA_Clear(QueA_t *pQue);
static U8 QueA_PutByte(QueA_t *pQue, U8 data);
static U8 QueA_GetByte(QueA_t *pQue, U8 *data);
static U32 QueA_GetSize(QueA_t *pQue);

static void QueB_Clear(QueB_t *pQue);
static U8 QueB_PutByte(QueB_t *pQue, U8 data);
static U8 QueB_GetByte(QueB_t *pQue, U8 *data);
static U32 QueB_GetSize(QueB_t *pQue);

static UART_HandleTypeDef UartHandle1; // usb by KPS
int fputc(int ch, FILE *f);

#ifdef USE_RX_BUFFER_USB // by KPS
static QueU_t rxQueU;
#endif
static QueU_t txQueU;
static QueA_t rxQueA, txQueA;
static QueB_t rxQueB, txQueB;

void IUART_Init(Serial_t *s0, Serial_t *s1, Serial_t *s2)
{
    GPIO_Init();

    UART1_Init(s0->baudrate, s0->parity, s0->stopbit);
    UART2_Init(s1->baudrate, s1->parity, s1->stopbit);
    UART4_Init(s2->baudrate, s2->parity, s2->stopbit);

    QueU_Clear(&txQueU);
    QueA_Clear(&txQueA);
    QueB_Clear(&txQueB);
#ifdef USE_RX_BUFFER_USB // by KPS
    QueU_Clear(&rxQueU);
#endif
    QueA_Clear(&rxQueA);
    QueB_Clear(&rxQueB);

    IUART1_Callback = NULL; // by KPS

    setvbuf(stdout, NULL, _IONBF, 0);
}

void IUART1_RecvBufClear(void)
{
#ifdef USE_RX_BUFFER_USB // by KPS
    QueU_Clear(&rxQueU);
#endif
}

void IUART2_RecvBufClear(void)
{
    QueA_Clear(&rxQueA);
}

void IUART4_RecvBufClear(void)
{
    QueB_Clear(&rxQueB);
}

U8 IUART1_CheckRecvEnd(void)
{
    U8 ret = false;
    
    if(IUART1_RecvFlag)
    {
        if(IUART1_RecvTick == 0)
        {
            IUART1_RecvFlag = false;
            ret             = true;
        }
        else
        {
            IUART1_RecvTick--;
        }
    }
    
    return ret;
}

U8 IUART2_CheckRecvEnd(void)
{
    U8 ret = false;
    
    if(IUART2_RecvFlag)
    {
        if(IUART2_RecvTick == 0)
        {
            IUART2_RecvFlag = false;
            ret             = true;
        }
        else
        {
            IUART2_RecvTick--;
        }
    }
    
    return ret;
}

U8 IUART4_CheckRecvEnd(void)
{
    U8 ret = false;
    
    if(IUART4_RecvFlag)
    {
        if(IUART4_RecvTick == 0)
        {
            IUART4_RecvFlag = false;
            ret             = true;
        }
        else
        {
            IUART4_RecvTick--;
        }
    }
    
    return ret;
}

U32 IUART1_GetRecvSize(void)
{
#ifdef USE_RX_BUFFER_USB // by KPS
    return QueU_GetSize(&rxQueU);
#else
    return 0;
#endif
}

U32 IUART2_GetRecvSize(void)
{
    return QueA_GetSize(&rxQueA);
}

U32 IUART4_GetRecvSize(void)
{
    return QueB_GetSize(&rxQueB);
}

void IUART1_WriteBytes(uint8_t *pData, uint16_t size)
{
    for (int i = 0; i < size; i++)
    {
        QueU_PutByte(&txQueU, pData[i]);
    }

    SET_BIT(USART1->CR1, USART_CR1_TXEIE);
}

void IUART2_WriteBytes(uint8_t *pData, uint16_t size)
{
    for (int i = 0; i < size; i++)
    {
        QueA_PutByte(&txQueA, pData[i]);
    }

    RS485A_TX_ENABLE();
    SET_BIT(USART2->CR1, USART_CR1_TXEIE);
}

void IUART4_WriteBytes(uint8_t *pData, uint16_t size)
{
    for(int i=0; i<size; i++)
    {
        QueB_PutByte(&txQueB, pData[i]);
    }

    SET_BIT(UART4->CR1, USART_CR1_TXEIE);
}

U8 IUART1_ReadByte(void)
{
#ifdef USE_RX_BUFFER_USB // by KPS
    U8 data;
    QueU_GetByte(&rxQueU, &data);

    return data;
#else
    return 0;
#endif
}

U8 IUART2_ReadByte(void)
{
    U8 data;

    QueA_GetByte(&rxQueA, &data);

    return data;
}

U8 IUART4_ReadByte(void)
{
    U8 data;

    QueB_GetByte(&rxQueB, &data);

    return data;
}

static void UART1_Init(U8 baudrate, U8 parity, U8 stopbit)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    U32 _baudrate;
    U32 _parity;
    U32 _stopbit;

    switch (baudrate)
    {
    case BAUDRATE_9600: _baudrate = 9600; break;
    case BAUDRATE_19200: _baudrate = 19200; break;
    case BAUDRATE_38400: _baudrate = 38400; break;
    case BAUDRATE_57600: _baudrate = 57600; break;
    case BAUDRATE_115200: _baudrate = 115200; break;
    case BAUDRATE_230400: _baudrate = 230400; break;
    default: 
        _baudrate = 115200;
    }

    switch (parity)
    {
    case PARITY_NONE: _parity = UART_PARITY_NONE; break;
    case PARITY_ODD: _parity = UART_PARITY_EVEN;  break;
    case PARITY_EVEN: _parity = UART_PARITY_ODD;  break;
    default: 
        _parity = UART_PARITY_NONE;
    }

    switch (stopbit)
    {
    case STOPBIT_1: _stopbit = UART_STOPBITS_1; break;
    case STOPBIT_2: _stopbit = UART_STOPBITS_2; break;
    default:
        _stopbit = UART_STOPBITS_1;
    }

    UartHandle1.Instance = USART1;

    UartHandle1.Init.BaudRate = _baudrate;
    UartHandle1.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle1.Init.StopBits = _stopbit;
    UartHandle1.Init.Parity = _parity;
    UartHandle1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    UartHandle1.Init.Mode = UART_MODE_TX_RX;

    HAL_UART_Init(&UartHandle1);

    HAL_NVIC_SetPriority(USART1_IRQn, 0xF, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    HAL_Delay(2);
    __HAL_UART_FLUSH_DRREGISTER(&UartHandle1);

    __HAL_UART_ENABLE_IT(&UartHandle1, UART_IT_RXNE);
}

static void UART2_Init(U8 baudrate, U8 parity, U8 stopbit)
{
    static UART_HandleTypeDef UartHandle;
    
    __HAL_RCC_USART2_CLK_ENABLE();
    
    U32 _baudrate;
    U32 _parity;
    U32 _stopbit;
    
    switch(baudrate)
    {
    case    BAUDRATE_9600   : _baudrate = 9600;   break;
    case    BAUDRATE_19200  : _baudrate = 19200;  break;
    case    BAUDRATE_38400  : _baudrate = 38400;  break;
    case    BAUDRATE_57600  : _baudrate = 57600;  break;
    case    BAUDRATE_115200 : _baudrate = 115200; break;
    case    BAUDRATE_230400 : _baudrate = 230400; break;
    default : _baudrate                 = 115200;
    }
    
    switch(parity)
    {
    case    PARITY_NONE    : _parity = UART_PARITY_NONE; break;
    case    PARITY_ODD     : _parity = UART_PARITY_EVEN; break;
    case    PARITY_EVEN    : _parity = UART_PARITY_ODD;  break;
    default : _parity                = UART_PARITY_NONE;
    }
     
    switch(stopbit)
    {
    case    STOPBIT_1      : _stopbit = UART_STOPBITS_1; break;
    case    STOPBIT_2      : _stopbit = UART_STOPBITS_2; break;
    default : _stopbit                = UART_STOPBITS_1;
    }

    UartHandle.Instance = USART2;

    UartHandle.Init.BaudRate   = _baudrate;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits   = _stopbit;
    UartHandle.Init.Parity     = _parity;
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode       = UART_MODE_TX_RX;

    HAL_UART_Init(&UartHandle);
    
    HAL_NVIC_SetPriority(USART2_IRQn, 0xF, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    HAL_Delay(2);
    __HAL_UART_FLUSH_DRREGISTER(&UartHandle);

    RS485A_RX_ENABLE();
    __HAL_UART_ENABLE_IT(&UartHandle, UART_IT_RXNE);
}

static void UART4_Init(U8 baudrate, U8 parity, U8 stopbit)
{
    static UART_HandleTypeDef UartHandle;
  
    __HAL_RCC_UART4_CLK_ENABLE();
    
    U32 _baudrate;
    U32 _parity;
    U32 _stopbit;
    
    switch(baudrate)
    {
    case    BAUDRATE_9600   : _baudrate = 9600;   break;
    case    BAUDRATE_19200  : _baudrate = 19200;  break;
    case    BAUDRATE_38400  : _baudrate = 38400;  break;
    case    BAUDRATE_57600  : _baudrate = 57600;  break;
    case    BAUDRATE_115200 : _baudrate = 115200; break;
    case    BAUDRATE_230400 : _baudrate = 230400; break;
    default : _baudrate                 = 115200;
    }
    
    switch(parity)
    {
    case    PARITY_NONE  : _parity = UART_PARITY_NONE; break;
    case    PARITY_ODD   : _parity = UART_PARITY_EVEN; break;
    case    PARITY_EVEN  : _parity = UART_PARITY_ODD;  break;
    default : _parity              = UART_PARITY_NONE;
    }
     
    switch(stopbit)
    {
    case    STOPBIT_1   : _stopbit = UART_STOPBITS_1; break;
    case    STOPBIT_2   : _stopbit = UART_STOPBITS_2; break;
    default : _stopbit             = UART_STOPBITS_1;
    }

    UartHandle.Instance = UART4;

    UartHandle.Init.BaudRate   = _baudrate;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits   = _stopbit;
    UartHandle.Init.Parity     = _parity;
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode       = UART_MODE_TX_RX;

    HAL_UART_Init(&UartHandle);
    
    HAL_NVIC_SetPriority(UART4_IRQn, 0xF, 0);
    HAL_NVIC_EnableIRQ(UART4_IRQn);

    HAL_Delay(2);
    __HAL_UART_FLUSH_DRREGISTER(&UartHandle);

    __HAL_UART_ENABLE_IT(&UartHandle, UART_IT_RXNE);
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitStruct.Pin       = GPIO_PIN_9  | GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin       = GPIO_PIN_2  | GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin       = GPIO_PIN_0  | GPIO_PIN_1;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin   = GPIO_PIN_0;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void USART1_IRQHandler(void) // USB, 디버깅용으로만 사용
{
    U8 data;

    USART1->ICR = (uint32_t)(UART_FLAG_ORE | UART_FLAG_IDLE);

    if ((USART1->ISR & USART_ISR_RXNE) != RESET)
    {
        IUART1_RecvFlag = true;
        IUART1_RecvTick = TERMINATE_TICK_TIME;
        USART1->RQR |= (uint32_t)UART_RXDATA_FLUSH_REQUEST;

#ifdef USE_RX_BUFFER_USB // by KPS
        QueU_PutByte(&rxQueU, USART1->RDR);
#else
        data = (U8)USART1->RDR;
        if (IUART1_Callback != NULL)
        {
            IUART1_Callback(data);
        }
#endif
    }

    if (((USART1->ISR & USART_ISR_TXE) != RESET) && ((USART1->CR1 & USART_CR1_TXEIE) != RESET))
    {
        if (QueU_GetByte(&txQueU, &data))
        {
            USART1->TDR = data;
        }
        else
        {
            CLEAR_BIT(USART1->CR1, USART_CR1_TXEIE);
            SET_BIT(USART1->CR1, USART_CR1_TCIE);
        }
    }

    if (((USART1->ISR & USART_ISR_TC) != RESET) && ((USART1->CR1 & USART_CR1_TCIE) != RESET))
    {
        CLEAR_BIT(USART1->CR1, USART_CR1_TCIE);
    }
}

void USART2_IRQHandler(void)
{
    U8 data;

    USART2->ICR = (uint32_t)(UART_FLAG_ORE | UART_FLAG_IDLE);

    if((USART2->ISR & USART_ISR_RXNE) != RESET)
    {
        IUART2_RecvFlag  = true;
        IUART2_RecvTick  = TERMINATE_TICK_TIME;
        USART2->RQR     |= (uint32_t)UART_RXDATA_FLUSH_REQUEST;
        QueA_PutByte(&rxQueA, USART2->RDR);
    }

    if( ((USART2->ISR & USART_ISR_TXE) != RESET) && ((USART2->CR1 & USART_CR1_TXEIE) != RESET) )
    {
        if(QueA_GetByte(&txQueA, &data))
        {
            USART2->TDR = data;
        }
        else
        {
            CLEAR_BIT(USART2->CR1, USART_CR1_TXEIE);
            SET_BIT(USART2->CR1, USART_CR1_TCIE);
        }
    }

    if(((USART2->ISR & USART_ISR_TC) != RESET) && ((USART2->CR1 & USART_CR1_TCIE) != RESET))
    {
        RS485A_RX_ENABLE();
        CLEAR_BIT(USART2->CR1, USART_CR1_TCIE);
    }
}

void UART4_IRQHandler(void)
{
    U8 data;

    UART4->ICR = (uint32_t)(UART_FLAG_ORE | UART_FLAG_IDLE);

    if((UART4->ISR & USART_ISR_RXNE) != RESET)
    {
        IUART4_RecvFlag  = true;
        IUART4_RecvTick  = TERMINATE_TICK_TIME;
        UART4->RQR      |= (uint32_t)UART_RXDATA_FLUSH_REQUEST;
        QueB_PutByte(&rxQueB, UART4->RDR);
    }

    if( ((UART4->ISR & USART_ISR_TXE) != RESET) && ((UART4->CR1 & USART_CR1_TXEIE) != RESET) )
    {
        if(QueB_GetByte(&txQueB, &data))
        {
            UART4->TDR = data;
        }
        else
        {
            CLEAR_BIT(UART4->CR1, USART_CR1_TXEIE);
            SET_BIT(UART4->CR1, USART_CR1_TCIE);
        }
    }

    if(((UART4->ISR & USART_ISR_TC) != RESET) && ((UART4->CR1 & USART_CR1_TCIE) != RESET))
    {
        CLEAR_BIT(UART4->CR1, USART_CR1_TCIE);
    }
}

static void QueU_Clear(QueU_t *pQue)
{
    pQue->head = pQue->tail = pQue->size = 0;
    memset(pQue->buff, 0, BUFF_SIZE_U);
}

static U8 QueU_PutByte(QueU_t *pQue, U8 data)
{
    if (QueU_GetSize(pQue) == (BUFF_SIZE_U - 1))
        return FALSE;

    pQue->buff[pQue->head++] = data;
    pQue->head %= BUFF_SIZE_U;

    pQue->size = QueU_GetSize(pQue);

    return TRUE;
}

static U8 QueU_GetByte(QueU_t *pQue, U8 *data)
{
    if (QueU_GetSize(pQue) == 0)
        return FALSE;

    *data = pQue->buff[pQue->tail++];
    pQue->tail %= BUFF_SIZE_U;

    pQue->size = QueU_GetSize(pQue);

    return TRUE;
}

static U32 QueU_GetSize(QueU_t *pQue)
{
    return (pQue->head - pQue->tail + BUFF_SIZE_U) % BUFF_SIZE_U;
}

static void QueA_Clear(QueA_t *pQue)
{
    pQue->head = pQue->tail = pQue->size = 0;
    memset(pQue->buff, 0, BUFF_SIZE_A);
}

static U8 QueA_PutByte(QueA_t *pQue, U8 data)
{
    if (QueA_GetSize(pQue) == (BUFF_SIZE_A - 1))
        return FALSE;

    pQue->buff[pQue->head++] = data;
    pQue->head %= BUFF_SIZE_A;

    pQue->size = QueA_GetSize(pQue);

    return TRUE;
}

static U8 QueA_GetByte(QueA_t *pQue, U8 *data)
{
    if (QueA_GetSize(pQue) == 0)
        return FALSE;

    *data = pQue->buff[pQue->tail++];
    pQue->tail %= BUFF_SIZE_A;

    pQue->size = QueA_GetSize(pQue);

    return TRUE;
}

static U32 QueA_GetSize(QueA_t *pQue)
{
    return (pQue->head - pQue->tail + BUFF_SIZE_A) % BUFF_SIZE_A;
}

static void QueB_Clear(QueB_t *pQue)
{
    pQue->head = pQue->tail = pQue->size = 0;
    memset(pQue->buff, 0, BUFF_SIZE_B);
}

static U8 QueB_PutByte(QueB_t *pQue, U8 data)
{
    if (QueB_GetSize(pQue) == (BUFF_SIZE_B - 1))
        return FALSE;

    pQue->buff[pQue->head++] = data;
    pQue->head %= BUFF_SIZE_B;

    pQue->size = QueB_GetSize(pQue);

    return TRUE;
}

static U8 QueB_GetByte(QueB_t *pQue, U8 *data)
{
    if (QueB_GetSize(pQue) == 0)
        return FALSE;

    *data = pQue->buff[pQue->tail++];
    pQue->tail %= BUFF_SIZE_B;

    pQue->size = QueB_GetSize(pQue);

    return TRUE;
}

static U32 QueB_GetSize(QueB_t *pQue)
{
    return (pQue->head - pQue->tail + BUFF_SIZE_B) % BUFF_SIZE_B;
}

void IUART1_SetBaudrate(U8 baudrate)
{
    U32 _baudrate;

    switch (baudrate)
    {
    case BAUDRATE_9600:
        _baudrate = 9600;
        break;
    case BAUDRATE_19200:
        _baudrate = 19200;
        break;
    case BAUDRATE_38400:
        _baudrate = 38400;
        break;
    case BAUDRATE_57600:
        _baudrate = 57600;
        break;
    case BAUDRATE_115200:
        _baudrate = 115200;
        break;
    case BAUDRATE_230400:
        _baudrate = 230400;
        break;
    default:
        _baudrate = 115200;
    }

    UartHandle1.Instance = USART1;
    __HAL_UART_DISABLE(&UartHandle1);

    UartHandle1.Init.BaudRate = _baudrate;
    if (HAL_UART_Init(&UartHandle1) != HAL_OK)
    {
        printf("UART(USB) initialization failed.\n");
        // Error_Handler();
        ;
    }

    __HAL_UART_ENABLE(&UartHandle1);
}

void IUART2_SetBaudrate(U8 baudrate)
{
    static UART_HandleTypeDef UartHandle;
    U32 _baudrate;

    switch (baudrate)
    {
    case BAUDRATE_9600:
        _baudrate = 9600;
        break;
    case BAUDRATE_19200:
        _baudrate = 19200;
        break;
    case BAUDRATE_38400:
        _baudrate = 38400;
        break;
    case BAUDRATE_57600:
        _baudrate = 57600;
        break;
    case BAUDRATE_115200:
        _baudrate = 115200;
        break;
    case BAUDRATE_230400:
        _baudrate = 230400;
        break;
    default:
        _baudrate = 115200;
    }

    UartHandle.Instance = USART2;
    __HAL_UART_DISABLE(&UartHandle);

    UartHandle.Init.BaudRate = _baudrate;
    if (HAL_UART_Init(&UartHandle) != HAL_OK)
    {
        printf("UART2(RS485 1ch) initialization failed.\n");
        // Error_Handler();
        ;
    }

    __HAL_UART_ENABLE(&UartHandle);
}

void IUART4_SetBaudrate(U8 baudrate)
{
    static UART_HandleTypeDef UartHandle;
    U32 _baudrate;

    switch (baudrate)
    {
    case BAUDRATE_9600:
        _baudrate = 9600;
        break;
    case BAUDRATE_19200:
        _baudrate = 19200;
        break;
    case BAUDRATE_38400:
        _baudrate = 38400;
        break;
    case BAUDRATE_57600:
        _baudrate = 57600;
        break;
    case BAUDRATE_115200:
        _baudrate = 115200;
        break;
    case BAUDRATE_230400:
        _baudrate = 230400;
        break;
    default:
        _baudrate = 115200;
    }

    UartHandle.Instance = UART4;
    __HAL_UART_DISABLE(&UartHandle);

    UartHandle.Init.BaudRate = _baudrate;
    if (HAL_UART_Init(&UartHandle) != HAL_OK)
    {
        printf("UART4(RS485 2ch) initialization failed.\n");
        // Error_Handler();
        ;
    }

    __HAL_UART_ENABLE(&UartHandle);
}

//int fputc(int ch, FILE *f) // by KPS, for printf(...)
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&UartHandle1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
