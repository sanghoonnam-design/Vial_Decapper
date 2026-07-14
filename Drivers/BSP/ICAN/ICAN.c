#define __ICAN_C__
#include "ICAN.h"
#undef __ICAN_C__
#include "XDebug.h"

CAN_HandleTypeDef hcan1;

static void CAN1_Init(U8 baudrate);
static void CAN1_FilterInit(U32 id, U32 mask);

static void GPIO_Init(void);

#define CAN_BUFF_SIZE 128

typedef struct Que
{
    U16 StdId[CAN_BUFF_SIZE];
    U8 DLC[CAN_BUFF_SIZE];
    U8 buff[CAN_BUFF_SIZE][8];
    U32 head;
    U32 tail;
    U32 size;
} Que_t;

#define SW_TX_QUEUE_SIZE 64

typedef struct {
    CAN_TxHeaderTypeDef header;
    uint8_t data[8];
} CAN_TxMessage_t;

typedef struct {
    CAN_TxMessage_t msg[SW_TX_QUEUE_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t size;
} CAN_TxQueue_t;

static CAN_TxQueue_t txQueue;


void CAN1_RX0_IRQHandler(void);

static void Que_Clear(Que_t *pQue);
static U8 Que_PutByte(Que_t *pQue, U16 id, U8 len, U8 *data);
static U8 Que_GetByte(Que_t *pQue, CanPacket_t *cp);
static U32 Que_GetSize(Que_t *pQue);

static Que_t rxQue[2];
// static Que_t   txQue[2]; // not used, by KPS

static void CAN_TxEnqueue(CAN_TxHeaderTypeDef *pHeader, uint8_t *pData);
static void CAN_TrySend(CAN_HandleTypeDef *hcan);
static void CAN_TxBufClear(void);




void ICAN_Init(Can_t *c)
{
    GPIO_Init();
    
    CAN1_Init(c->baudrate);    
    CAN1_FilterInit(c->id, c->mask);
    
    HAL_CAN_Start(&hcan1);

    Que_Clear(&rxQue[0]);
    CAN_TxBufClear();

    CAN_TxIsrCount = 0;
    CAN_RxIsrCount = 0;
}

static void CAN_TxBufClear(void)
{
	memset(&txQueue, 0, sizeof(CAN_TxQueue_t));
}


void ICAN1_RecvBufClear(void)
{
    Que_Clear(&rxQue[0]);
}

/*
현재 ICAN1_CheckRecv는 단순히 head != tail 만 확인 → 경쟁 조건(race condition) 가능
예: head/tail을 읽는 도중 인터럽트가 걸려 값이 바뀌면 잘못된 판정 가능
*/
U8 ICAN1_CheckRecv(void)
{
    U8 ret = false;
    
    if(rxQue[0].head != rxQue[0].tail) ret = true;
    
    return ret;
}

U32 ICAN1_GetRecvSize(void)
{
    return Que_GetSize(&rxQue[0]);
}

void ICAN1_WriteData(CanPacket_t* cp)
{
    CAN_TxHeaderTypeDef txHeader;

    txHeader.StdId = cp->StdId;
    txHeader.IDE   = CAN_ID_STD;
    txHeader.RTR   = cp->RTR;
    txHeader.DLC   = cp->DLC;

    CAN_TxEnqueue(&txHeader, cp->data);
    CAN_TrySend(&hcan1);
}

// 소프트웨어 송신 큐에 넣기
static void CAN_TxEnqueue(CAN_TxHeaderTypeDef *pHeader, uint8_t *pData) {
    uint16_t next = (txQueue.head + 1) % SW_TX_QUEUE_SIZE;
    if (txQueue.size >= SW_TX_QUEUE_SIZE) {
        // 큐가 가득 참 (정책 필요: 버림 or 덮어쓰기)
        return;
    }
    txQueue.msg[txQueue.head].header = *pHeader;
    for (int i = 0; i < pHeader->DLC; i++) {
        txQueue.msg[txQueue.head].data[i] = pData[i];
    }
    txQueue.head = next;
    txQueue.size++;   // 사이즈 증가
}

// Mailbox 비었으면 즉시 송신
static void CAN_TrySend(CAN_HandleTypeDef *hcan) {
    if (txQueue.size == 0) return; // 큐 비어있음

    uint32_t mailbox;
    if (HAL_CAN_AddTxMessage(hcan,
                             &txQueue.msg[txQueue.tail].header,
                             txQueue.msg[txQueue.tail].data,
                             &mailbox) == HAL_OK) {
        // 성공 → tail 이동 + size 감소
        txQueue.tail = (txQueue.tail + 1) % SW_TX_QUEUE_SIZE;
        if (txQueue.size > 0) txQueue.size--;

        CAN_TxIsrCount++;
    }
    // 실패(HAL_BUSY)면 Mailbox 꽉 찬 상태 → 인터럽트에서 다시 시도
}

void ICAN1_ReadData(CanPacket_t* cp)
{
    Que_GetByte(&rxQue[0], cp);
}

static void CAN1_Init(U8 baudrate)
{
    U32 _scale;
    U32 _bs1, _bs2;
    
    switch(baudrate)
    {
    case    BAUDRATE_1M    : _scale = 6;   _bs1 = CAN_BS1_6TQ;     _bs2 = CAN_BS2_2TQ;     break;
    case    BAUDRATE_500K  : _scale = 12;  _bs1 = CAN_BS1_4TQ;     _bs2 = CAN_BS2_4TQ;     break;
    case    BAUDRATE_250K  : _scale = 12;  _bs1 = CAN_BS1_9TQ;     _bs2 = CAN_BS2_8TQ;     break;
    case    BAUDRATE_100K  : _scale = 30;  _bs1 = CAN_BS1_10TQ;    _bs2 = CAN_BS2_7TQ;     break;
    case    BAUDRATE_50K   : _scale = 54;  _bs1 = CAN_BS1_12TQ;    _bs2 = CAN_BS2_7TQ;     break;
    case    BAUDRATE_20K   : _scale = 108; _bs1 = CAN_BS1_16TQ;    _bs2 = CAN_BS2_8TQ;     break;
    default : _scale                = 12;  _bs1 = CAN_BS1_4TQ;     _bs2 = CAN_BS2_4TQ;
    }
    
    hcan1.Instance                  = CAN1;
    hcan1.Init.Prescaler            = _scale;
    hcan1.Init.Mode                 = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth        = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1             = _bs1;
    hcan1.Init.TimeSeg2             = _bs2;
    hcan1.Init.TimeTriggeredMode    = DISABLE;
    hcan1.Init.AutoBusOff           = DISABLE;
    hcan1.Init.AutoWakeUp           = DISABLE;
    hcan1.Init.AutoRetransmission   = ENABLE;
    hcan1.Init.ReceiveFifoLocked    = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    
    HAL_CAN_Init(&hcan1);
}

static void CAN1_FilterInit(U32 id, U32 mask)
{
    CAN_FilterTypeDef   CanFilterConfig;
    
    CanFilterConfig.FilterIdHigh         = id << 5;
    CanFilterConfig.FilterIdLow          = id << 5;
    CanFilterConfig.FilterMaskIdHigh     = mask << 5;
    CanFilterConfig.FilterMaskIdLow      = mask << 5;
    CanFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    CanFilterConfig.FilterBank           = 0;
    CanFilterConfig.FilterMode           = CAN_FILTERMODE_IDMASK;
    CanFilterConfig.FilterScale          = CAN_FILTERSCALE_16BIT;
    CanFilterConfig.FilterActivation     = ENABLE;
    CanFilterConfig.SlaveStartFilterBank = 0;
    
    HAL_CAN_ConfigFilter(&hcan1, &CanFilterConfig);
    HAL_CAN_ActivateNotification(&hcan1,
        CAN_IT_TX_MAILBOX_EMPTY |
        CAN_IT_RX_FIFO0_MSG_PENDING |
        CAN_IT_ERROR_WARNING     |   // 에러 경고
        CAN_IT_ERROR_PASSIVE     |   // 에러 패시브
        CAN_IT_BUSOFF            |   // 버스 오프
        CAN_IT_ERROR);               // 에러 발생
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    __HAL_RCC_CAN1_CLK_ENABLE();
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitStruct.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHead;
    U8 RxData[8];
  
    if(hcan->Instance == CAN1)
    {
        HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &RxHead, RxData);
        
        Que_PutByte(&rxQue[0], RxHead.StdId, RxHead.DLC, RxData);
        CAN_RxIsrCount++;
    }
}

// Mailbox 0 전송 완료
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
    CAN_TrySend(hcan);
}

// Mailbox 1 전송 완료
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
    CAN_TrySend(hcan);
}

// Mailbox 2 전송 완료
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
    CAN_TrySend(hcan);
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
#if 0
    if (hcan->Instance == CAN1) {
        uint32_t err = hcan->ErrorCode;

        if (err & HAL_CAN_ERROR_EWG) {
            xcprintf("CAN1 Error: Protocol Error Warning\r\n");
        }
        if (err & HAL_CAN_ERROR_EPV) {
            xcprintf("CAN1 Error: Error Passive\r\n");
        }
        if (err & HAL_CAN_ERROR_BOF) {
            xcprintf("CAN1 Error: Bus-Off\r\n");
        }
        if (err & HAL_CAN_ERROR_STF) {
            xcprintf("CAN1 Error: Stuff Error\r\n");
        }
        if (err & HAL_CAN_ERROR_FOR) {
            xcprintf("CAN1 Error: Form Error\r\n");
        }
        if (err & HAL_CAN_ERROR_ACK) {
            xcprintf("CAN1 Error: Acknowledgment Error\r\n");
        }
        if (err & HAL_CAN_ERROR_BR) {
            xcprintf("CAN1 Error: Bit Recessive Error\r\n");
        }
        if (err & HAL_CAN_ERROR_BD) {
            xcprintf("CAN1 Error: Bit Dominant Error\r\n");
        }
        if (err & HAL_CAN_ERROR_CRC) {
            xcprintf("CAN1 Error: CRC Error\r\n");
        }
        if (err & HAL_CAN_ERROR_RX_FOV0) {
            xcprintf("CAN1 Error: Rx FIFO0 Overrun\r\n");
        }
        if (err & HAL_CAN_ERROR_RX_FOV1) {
            xcprintf("CAN1 Error: Rx FIFO1 Overrun\r\n");
        }
        if (err & HAL_CAN_ERROR_TX_ALST0) {
            xcprintf("CAN1 Error: TxMailbox 0 Arbitration Lost\r\n");
        }
        if (err & HAL_CAN_ERROR_TX_TERR0) {
            xcprintf("CAN1 Error: TxMailbox 0 Transmit Error\r\n");
        }
        if (err & HAL_CAN_ERROR_TX_ALST1) {
            xcprintf("CAN1 Error: TxMailbox 1 Arbitration Lost\r\n");
        }
        if (err & HAL_CAN_ERROR_TX_TERR1) {
            xcprintf("CAN1 Error: TxMailbox 1 Transmit Error\r\n");
        }
        if (err & HAL_CAN_ERROR_TX_ALST2) {
            xcprintf("CAN1 Error: TxMailbox 2 Arbitration Lost\r\n");
        }
        if (err & HAL_CAN_ERROR_TX_TERR2) {
            xcprintf("CAN1 Error: TxMailbox 2 Transmit Error\r\n");
        }
        if (err & HAL_CAN_ERROR_TIMEOUT) {
            xcprintf("CAN1 Error: Timeout Error\r\n");
        }
        if (err & HAL_CAN_ERROR_NOT_INITIALIZED) {
            xcprintf("CAN1 Error: Peripheral Not Initialized\r\n");
        }
        if (err & HAL_CAN_ERROR_NOT_READY) {
            xcprintf("CAN1 Error: Peripheral Not Ready\r\n");
        }
        if (err & HAL_CAN_ERROR_NOT_STARTED) {
            xcprintf("CAN1 Error: Peripheral Not Started\r\n");
        }
        if (err & HAL_CAN_ERROR_PARAM) {
            xcprintf("CAN1 Error: Parameter Error\r\n");
        }
    }
#endif
}


void CAN1_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}

static void Que_Clear(Que_t* pQue)
{
    pQue->head = pQue->tail = pQue->size = 0;
    memset(pQue->buff, 0, CAN_BUFF_SIZE * 8);
}

static U8 Que_PutByte(Que_t* pQue, U16 id, U8 len, U8 *data)
{
    U8 i;
  
    if(Que_GetSize(pQue) == (CAN_BUFF_SIZE-1)) return FALSE;
    
    pQue->StdId[pQue->head] = id;
    pQue->DLC[pQue->head]   = len;
    
    for(i=0; i<8; i++)
      pQue->buff[pQue->head][i] = data[i];
    
    pQue->head++;
    pQue->head %= CAN_BUFF_SIZE;
    
    pQue->size = Que_GetSize(pQue);
    
    return TRUE;
}

static U8 Que_GetByte(Que_t* pQue, CanPacket_t* cp)
{
    U8 i;
    
    if(Que_GetSize(pQue) == 0) return FALSE;
    
    cp->StdId = pQue->StdId[pQue->tail];
    cp->DLC   = pQue->DLC[pQue->tail];
    
    for(i=0; i<8; i++)
      cp->buff[i] = pQue->buff[pQue->tail][i];    
    
    pQue->tail++;    
    pQue->tail %= CAN_BUFF_SIZE;
    
    pQue->size = Que_GetSize(pQue);
    
    return TRUE;
}

static U32 Que_GetSize(Que_t* pQue)
{
    return (pQue->head - pQue->tail + CAN_BUFF_SIZE) % CAN_BUFF_SIZE;
}
