/*******************************************************************************
 * XBuffer.h
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef XBUFFER_H
#define XBUFFER_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "project.h"

#define MAX_CHAR_NUM (10) // command length

typedef struct
{
    char *buffer_;
    unsigned int size_;
    unsigned int count_;

} tsXBuffer;

// Function prototypes
tsXBuffer *XBuffer_Create(unsigned int buffer_size);
void XBuffer_Destroy(tsXBuffer *xBuffer);

void XBuffer_Start(tsXBuffer *xBuffer);
void XBuffer_End(tsXBuffer *xBuffer);
void XBuffer_End_NULL(tsXBuffer *xBuffer);

bool XBuffer_AddChar(tsXBuffer *xBuffer, char data, bool isAddComma);
bool XBuffer_AddString(tsXBuffer *xBuffer, const char *data, bool isAddComma);
bool XBuffer_AddCommandString(tsXBuffer *xBuffer, const char *data, bool isAddComma);
bool XBuffer_AddInt(tsXBuffer *xBuffer, int data, bool isAddComma);
bool XBuffer_AddLong(tsXBuffer *xBuffer, long data, bool isAddComma);
bool XBuffer_Addfloat(tsXBuffer *xBuffer, float data, bool isAddComma);
bool XBuffer_AddDouble(tsXBuffer *xBuffer, double data, bool isAddComma);
bool XBuffer_AddDoubleWithPrecision(tsXBuffer *xBuffer, double data, signed char width, signed char prec, bool isAddComma);
bool XBuffer_AddByte(tsXBuffer *xBuffer, unsigned char data, bool isAddComma);
bool XBuffer_AddU08(tsXBuffer *xBuffer, U08 data, bool isAddComma);
bool XBuffer_AddBool(tsXBuffer *xBuffer, bool data, bool isAddComma);
bool XBuffer_AddUnsignedShort(tsXBuffer *xBuffer, unsigned short data, bool isAddComma);

void XBuffer_Clear(tsXBuffer *xBuffer);
bool XBuffer_Get(tsXBuffer *xBuffer, unsigned int index, char *item);
char *XBuffer_GetBuffer(tsXBuffer *xBuffer);
unsigned int XBuffer_Length(tsXBuffer *xBuffer);
bool XBuffer_GetBufferToChar(tsXBuffer *xBuffer, char buffer[], unsigned int buffer_size);


#endif // XBUFFER_H