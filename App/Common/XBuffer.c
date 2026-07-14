/*******************************************************************************
 * XBuffer.h
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "XBuffer.h"


tsXBuffer *XBuffer_Create(unsigned int buffer_size)
{
    // tsXBuffer *xBuffer = (tsXBuffer *)malloc(sizeof(tsXBuffer));   // 이거 쓰면 안됨.
    tsXBuffer *xBuffer = (tsXBuffer *)pvPortMalloc(sizeof(tsXBuffer));// 동적할당은 조심해서. 쓰세요.
    if (xBuffer == NULL)
    {
        return NULL; // 메모리 할당 실패
    }
    xBuffer->size_   = buffer_size;
    // xBuffer->buffer_ = (char *)malloc(buffer_size);
    xBuffer->buffer_ = (char *)pvPortMalloc(buffer_size);
    if (xBuffer->buffer_ == NULL)
    {
        free(xBuffer);
        return NULL; // 메모리 할당 실패
    }
    XBuffer_Clear(xBuffer);
    return xBuffer;
}

void XBuffer_Destroy(tsXBuffer *xBuffer)
{
    if (xBuffer)
    {
        free(xBuffer->buffer_);
        free(xBuffer);
    }
}

void XBuffer_Start(tsXBuffer *xBuffer)
{
    XBuffer_Clear(xBuffer);
}

void XBuffer_End(tsXBuffer *xBuffer)
{
    xBuffer->buffer_[xBuffer->count_++] = '\r';
    xBuffer->buffer_[xBuffer->count_++] = '\n';
    xBuffer->buffer_[xBuffer->count_] = '\0';
}

void XBuffer_End_NULL(tsXBuffer *xBuffer)
{
    xBuffer->buffer_[xBuffer->count_] = '\0';
}

bool XBuffer_AddChar(tsXBuffer *xBuffer, char data, bool isAddComma)
{
    if (xBuffer->size_ <= xBuffer->count_)
    {
        return false; // 버퍼가 가득 찼습니다.
    }
    xBuffer->buffer_[xBuffer->count_++] = data;
    if (isAddComma)
    {
        xBuffer->buffer_[xBuffer->count_++] = ',';
    }
    return true;
}

bool XBuffer_AddString(tsXBuffer *xBuffer, const char *data, bool isAddComma)
{
    unsigned char size = strlen(data);
    for (unsigned char i = 0; i < size; i++)
    {
        xBuffer->buffer_[xBuffer->count_++] = data[i];
    }
    if (isAddComma)
    {
        xBuffer->buffer_[xBuffer->count_++] = ',';
    }
    return true;
}

bool XBuffer_AddCommandString(tsXBuffer *xBuffer, const char *data, bool isAddComma)
{
    unsigned char size = strlen(data);
    for (unsigned char i = 0; i < size; i++)
    {
        xBuffer->buffer_[xBuffer->count_++] = data[i];
    }

    xBuffer->buffer_[xBuffer->count_++] = ' '; // add space

    if (isAddComma)
    {
        xBuffer->buffer_[xBuffer->count_++] = ',';
    }
    return true;
}

bool XBuffer_AddInt(tsXBuffer *xBuffer, int data, bool isAddComma)
{
    char buf[MAX_CHAR_NUM] = {0};
    unsigned char size = sprintf(buf, "%d", data);
    for (unsigned char i = 0; i < size; i++)
    {
        xBuffer->buffer_[xBuffer->count_++] = buf[i];
    }
    if (isAddComma)
    {
        xBuffer->buffer_[xBuffer->count_++] = ',';
    }
    return true;
}

bool XBuffer_AddLong(tsXBuffer *xBuffer, long data, bool isAddComma)
{
    char buf[MAX_CHAR_NUM] = {0};
    unsigned char size = sprintf(buf, "%ld", data);
    for (unsigned char i = 0; i < size; i++)
    {
        xBuffer->buffer_[xBuffer->count_++] = buf[i];
    }
    if (isAddComma)
    {
        xBuffer->buffer_[xBuffer->count_++] = ',';
    }
    return true;
}

bool XBuffer_Addfloat(tsXBuffer *xBuffer, float data, bool isAddComma)
{
    char buf[MAX_CHAR_NUM] = {0};
    sprintf(buf, "%.2f", data); // 소수점 이하 2자리
    unsigned char size = strlen(buf);
    for (unsigned char i = 0; i < size; i++)
    {
        xBuffer->buffer_[xBuffer->count_++] = buf[i];
    }
    if (isAddComma)
    {
        xBuffer->buffer_[xBuffer->count_++] = ',';
    }
    return true;
}

bool XBuffer_AddDouble(tsXBuffer *xBuffer, double data, bool isAddComma)
{
    char buf[MAX_CHAR_NUM] = {0};
    sprintf(buf, "%.2f", data); // 소수점 이하 2자리
    unsigned char size = strlen(buf);
    for (unsigned char i = 0; i < size; i++)
    {
        xBuffer->buffer_[xBuffer->count_++] = buf[i];
    }
    if (isAddComma)
    {
        xBuffer->buffer_[xBuffer->count_++] = ',';
    }
    return true;
}

bool XBuffer_AddDoubleWithPrecision(tsXBuffer *xBuffer, double data, signed char width, signed char prec, bool isAddComma)
{
    char buf[MAX_CHAR_NUM] = {0};
    sprintf(buf, "%*.*f", width, prec, data);
    unsigned char size = strlen(buf);
    for (unsigned char i = 0; i < size; i++)
    {
        xBuffer->buffer_[xBuffer->count_++] = buf[i];
    }
    if (isAddComma)
    {
        xBuffer->buffer_[xBuffer->count_++] = ',';
    }
    return true;
}

bool XBuffer_AddByte(tsXBuffer *xBuffer, unsigned char data, bool isAddComma)
{
    return XBuffer_AddInt(xBuffer, (int)data, isAddComma);
}

bool XBuffer_AddU08(tsXBuffer *xBuffer, U08 data, bool isAddComma)
{
    return XBuffer_AddInt(xBuffer, (int)data, isAddComma);
}

bool XBuffer_AddBool(tsXBuffer *xBuffer, bool data, bool isAddComma)
{
    return XBuffer_AddInt(xBuffer, (int)(data ? 1 : 0), isAddComma);
}

bool XBuffer_AddUnsignedShort(tsXBuffer *xBuffer, unsigned short data, bool isAddComma)
{
    return XBuffer_AddInt(xBuffer, (int)data, isAddComma);
}

void XBuffer_Clear(tsXBuffer *xBuffer)
{
    memset(xBuffer->buffer_, 0, sizeof(char) * xBuffer->size_);
    xBuffer->count_ = 0;
}

bool XBuffer_Get(tsXBuffer *xBuffer, unsigned int index, char *item)
{
    if (index >= xBuffer->count_)
    {
        return false; // 인덱스가 유효하지 않습니다.
    }
    *item = xBuffer->buffer_[index];
    return true;
}

char *XBuffer_GetBuffer(tsXBuffer *xBuffer)
{
    return xBuffer->buffer_;
}

unsigned int XBuffer_Length(tsXBuffer *xBuffer)
{
    return xBuffer->count_;
}

bool XBuffer_GetBufferToChar(tsXBuffer *xBuffer, char buffer[], unsigned int buffer_size)
{
    if (buffer == NULL || xBuffer->size_ < buffer_size)// || buffer_size < 0)
    {
        return false; // 버퍼가 유효하지 않습니다.
    }
    for (unsigned int i = 0; i < buffer_size; i++)
    {
        buffer[i] = (char)xBuffer->buffer_[i];
    }
    return true;
}