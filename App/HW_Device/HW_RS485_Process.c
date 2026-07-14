#define __HW_RS485_PROCESS_C__
#include "HW_RS485_Process.h"
#undef __HW_RS485_PROCESS_C__

#include "XDebug.h"
#include "nanomodbus.h"
#include "port.h"
#include "XDebug.h"

// define RS485
#define HIGH_QUEUE_LENGTH 32
#define LOW_QUEUE_LENGTH 32
#define QUEUE_ITEM_SIZE sizeof(RS485_Msg_t)
#define QUEUESET_LENGTH (HIGH_QUEUE_LENGTH + LOW_QUEUE_LENGTH)

VOID TASK_RS485_Handler(void *pvParameters); // RS485 메인 태스크
static U08 _sendMsgHighPriority(RS485_Msg_t msg);
static U08 _sendMsgLowPriority(RS485_Msg_t msg);

static QueueHandle_t highQueue;
static QueueHandle_t lowQueue;

// define Modbus
static nmbs_error _modbus_transaction(nmbs_t *client, ModbusMsg_t *msg);
static nmbs_error _modbus_process(nmbs_t *client, ModbusMsg_t *msg);

static nmbs_t _modbusClient;
static U32 ModbusProcessTimeMax_us = 0;
static U32 gTick_Modbus_us = 0;

void RS485_Init(void)
{
    highQueue = xQueueCreate(HIGH_QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    lowQueue = xQueueCreate(LOW_QUEUE_LENGTH, QUEUE_ITEM_SIZE);

    xTaskCreate(TASK_RS485_Handler, "RS485_Task", 512, NULL, tskIDLE_PRIORITY + 3, NULL);
}

VOID TASK_RS485_Handler(void *pvParameters)
{
#define MODBUS_RECV_TIMEOUT_MS (300) // ms

    U32 startTick = 0;
    RS485_Msg_t msg;
    nmbs_error modbusErr;
    U32 highQueueSize, lowQueueSize;

    modbusErr = nmbs_client_init(&_modbusClient);
    nmbs_set_byte_timeout(&_modbusClient, MODBUS_RECV_TIMEOUT_MS);
    nmbs_set_read_timeout(&_modbusClient, MODBUS_RECV_TIMEOUT_MS);

    if (modbusErr != NMBS_ERROR_NONE)
    {
        LOG_MSG_SEND("nmbs_client_init error %d", modbusErr);
    }

    for (int i = 0; i < 3; i++)
        retryCount_RS485[i] = 0;

    while (1)
    {
        highQueueSize = uxQueueMessagesWaiting(highQueue);
        lowQueueSize = uxQueueMessagesWaiting(lowQueue);

        if (highQueueSize > 0)
        {
            for (int i = 0; i < highQueueSize; i++)
            {
                if (xQueueReceive(highQueue, &msg, 0) == pdTRUE)
                {
                    switch (msg.protocol)
                    {
                    case RS485_PROTOCOL_MODBUS:
                        startTick = ITIMER_StartMeasure_us();
                        _modbus_process(&_modbusClient, &msg.mb);
                        gTick_Modbus_us = ITIMER_StopMeasure_us(startTick);
                        if (ModbusProcessTimeMax_us < gTick_Modbus_us)
                            ModbusProcessTimeMax_us = gTick_Modbus_us;
                        vTaskDelay(10);
                        break;
                    case RS485_PROTOCOL_FFU:
                        vTaskDelay(1);
                        break;
                    case RS485_PROTOCOL_ASCII:
                        vTaskDelay(1);
                        break;
                    }
                }
            }
        }
        else if (lowQueueSize > 0)
        {
            if (xQueueReceive(lowQueue, &msg, 0) == pdTRUE)
            {
                switch (msg.protocol)
                {
                case RS485_PROTOCOL_MODBUS:
                    startTick = ITIMER_StartMeasure_us();
                    _modbus_process(&_modbusClient, &msg.mb);
                    gTick_Modbus_us = ITIMER_StopMeasure_us(startTick);
                    if (ModbusProcessTimeMax_us < gTick_Modbus_us)
                        ModbusProcessTimeMax_us = gTick_Modbus_us;
                    vTaskDelay(10);
                    break;
                case RS485_PROTOCOL_FFU:
                    vTaskDelay(1);
                    break;
                case RS485_PROTOCOL_ASCII:
                    vTaskDelay(1);
                    break;
                }
            }
        }
        else
            vTaskDelay(1);
    }
}

static nmbs_error _modbus_process(nmbs_t *client, ModbusMsg_t *msg)
{
#define RETRY_COUNT (3)
    nmbs_error err;
    U08 retryCount = 1;

    do
    {
        err = _modbus_transaction(client, msg);
        if (err == NMBS_ERROR_NONE)
        {
            (*msg->flag)++;
            break;
        }

        retryCount_RS485[client->dest_address_rtu - 1]++;
    } while (retryCount++ < RETRY_COUNT);

    if (err != NMBS_ERROR_NONE) // 전부 실패했을 때만 1회
        LOG_MSG_SEND("addr %x failed after %d retries: %s",
                     msg->address, RETRY_COUNT, nmbs_strerror(err));

    return err;
}

static nmbs_error _modbus_transaction(nmbs_t *client, ModbusMsg_t *msg)
{
    nmbs_error err = 0;

    nmbs_set_destination_rtu_address(client, msg->id);

    switch (msg->funcCode)
    {
    case MODBUS_FUNC_CODE_READ_COLIS:
        err = nmbs_read_coils(client, msg->address, msg->quantity, (U08 *)msg->dataOut);
        break;
    case MODBUS_FUNC_CODE_READ_DISCRETE_INPUTS:
        err = nmbs_read_discrete_inputs(client, msg->address, msg->quantity, (U08 *)msg->dataOut);
        break;
    case MODBUS_FUNC_CODE_READ_HOLDING_REG:
        err = nmbs_read_holding_registers(client, msg->address, msg->quantity, (U16 *)msg->dataOut);
        break;
    case MODBUS_FUNC_CODE_READ_INPUT_REG:
        err = nmbs_read_input_registers(client, msg->address, msg->quantity, (U16 *)msg->dataOut);
        break;
    case MODBUS_FUNC_CODE_WRITE_SINGLE_COIL:
        err = nmbs_write_single_coil(client, msg->address, (bool)msg->dataIn[0]);
        break;
    case MODBUS_FUNC_CODE_WRITE_SINGLE_REG:
        err = nmbs_write_single_register(client, msg->address, (U16)msg->dataIn[0]);
        break;
    case MODBUS_FUNC_CODE_WRITE_MULTIPLE_COILS:
        // err = nmbs_write_multiple_coils(client, msg->address, msg->quantity, (U08*)msg->dataIn);
        break;
    case MODBUS_FUNC_CODE_WRITE_MULTIPLE_REG:
        err = nmbs_write_multiple_registers(client, msg->address, msg->quantity, (U16 *)msg->dataIn);
        break;
    }

    return err;
}

U08 RS485_ModbusWriteFunc(U08 priority, U08 id, U08 fc, U16 addr, U16 quantity, U16 *dataIn, U08 *flag)
{
    RS485_Msg_t msg;
    U08 ret = 0;

    if (quantity > RS485_MODBUS_DATA_SIZE_MAX)
        return MODBUS_FUNC_ERROR_CODE_SIZE;

    msg.protocol = RS485_PROTOCOL_MODBUS;

    msg.mb.id = id;
    msg.mb.funcCode = fc;
    msg.mb.address = addr;
    msg.mb.quantity = quantity;
    memcpy(msg.mb.dataIn, dataIn, sizeof(U16) * quantity);
    msg.mb.flag = flag;

    if (priority == RS485_MSG_PRIORITY_HIGH)
        ret = _sendMsgHighPriority(msg);
    else if (priority == RS485_MSG_PRIORITY_LOW)
        ret = _sendMsgLowPriority(msg);

    return ret;
}

U08 RS485_ModbusReadFunc(U08 priority, U08 id, U08 fc, U16 addr, U16 quantity, U16 *dataOut, U08 *flag)
{
    RS485_Msg_t msg;
    U08 ret = 0;

    msg.protocol = RS485_PROTOCOL_MODBUS;

    msg.mb.id = id;
    msg.mb.funcCode = fc;
    msg.mb.address = addr;
    msg.mb.quantity = quantity;
    msg.mb.dataOut = dataOut;
    msg.mb.flag = flag;

    if (priority == RS485_MSG_PRIORITY_HIGH)
        ret = _sendMsgHighPriority(msg);
    else if (priority == RS485_MSG_PRIORITY_LOW)
        ret = _sendMsgLowPriority(msg);

    return ret;
}

static U08 _sendMsgHighPriority(RS485_Msg_t msg)
{
    static U08 wasFull = 0;
    if (uxQueueSpacesAvailable(highQueue) > 0)
    {
        xQueueSend(highQueue, &msg, 0);
        wasFull = 0;
    }
    else
    {
        if (!wasFull)
        {
            ERR_MSG_SEND("Queue Full: [High Priority]");
            wasFull = 1;
        }
    }
    return uxQueueSpacesAvailable(highQueue);
}

static U08 _sendMsgLowPriority(RS485_Msg_t msg)
{
    static U08 wasFull = 0;
    if (uxQueueSpacesAvailable(lowQueue) > 0)
    {
        xQueueSend(lowQueue, &msg, 0);
        wasFull = 0;
    }
    else
    {
        if (!wasFull)
        {
            ERR_MSG_SEND("Queue Full: [Low Priority]");
            wasFull = 1;
        }
    }
    return uxQueueSpacesAvailable(lowQueue);
}
