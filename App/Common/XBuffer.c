/*******************************************************************************
 * XBuffer.c
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 *
 *  Refactored: 1. sprintf-free / safe / faster
 ******************************************************************************/

#include "XBuffer.h"

static inline bool XBuffer_CanWrite(tsXBuffer *xb, uint16_t len)
{
    return (xb && (xb->count_ + len < xb->size_));
}

static inline void XBuffer_PushChar(tsXBuffer *xb, char c)
{
    xb->buffer_[xb->count_++] = c;
}

static inline uint32_t _strlen_u32(const char *s)
{
    uint32_t n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static bool _xbuf_add_u32_dec(tsXBuffer *xb, uint32_t v)
{
    char tmp[10];
    int i = 0;

    if (v == 0)
    {
        if (!XBuffer_CanWrite(xb, 1)) return false;
        XBuffer_PushChar(xb, '0');
        return true;
    }

    while (v)
    {
        tmp[i++] = (char)('0' + (v % 10U));
        v /= 10U;
    }

    if (!XBuffer_CanWrite(xb, (uint32_t)i)) return false;

    while (i--)
        XBuffer_PushChar(xb, tmp[i]);

    return true;
}

static bool _xbuf_add_s32_dec(tsXBuffer *xb, int32_t v)
{
    if (v < 0)
    {
        if (!XBuffer_CanWrite(xb, 1)) return false;
        XBuffer_PushChar(xb, '-');

        uint32_t u = (uint32_t)(-(v + 1)) + 1U;
        return _xbuf_add_u32_dec(xb, u);
    }
    return _xbuf_add_u32_dec(xb, (uint32_t)v);
}

static bool _xbuf_add_u64_dec(tsXBuffer *xb, uint64_t v)
{
    char tmp[20];
    int i = 0;

    if (v == 0ULL)
    {
        if (!XBuffer_CanWrite(xb, 1)) return false;
        XBuffer_PushChar(xb, '0');
        return true;
    }

    while (v)
    {
        tmp[i++] = (char)('0' + (char)(v % 10ULL));
        v /= 10ULL;
    }

    if (!XBuffer_CanWrite(xb, (uint32_t)i)) return false;

    while (i--)
        XBuffer_PushChar(xb, tmp[i]);

    return true;
}

static bool _xbuf_add_s64_dec(tsXBuffer *xb, int64_t v)
{
    if (v < 0)
    {
        if (!XBuffer_CanWrite(xb, 1)) return false;
        XBuffer_PushChar(xb, '-');

        uint64_t u = (uint64_t)(-(v + 1)) + 1ULL;
        return _xbuf_add_u64_dec(xb, u);
    }
    return _xbuf_add_u64_dec(xb, (uint64_t)v);
}

static bool _xbuf_add_spaces(tsXBuffer *xb, int count)
{
    if (count <= 0) return true;
    if (!XBuffer_CanWrite(xb, (uint32_t)count)) return false;
    while (count--) XBuffer_PushChar(xb, ' ');
    return true;
}

static bool _xbuf_add_fixed_double(tsXBuffer *xb, double val, int prec)
{
    /* prec: 0..6 정도만 현실적으로 지원 (속도/안정 우선) */
    if (prec < 0) prec = 0;
    if (prec > 6) prec = 6;

    bool neg = (val < 0.0);
    if (neg) val = -val;

    /* pow10 */
    int mul = 1;
    for (int i = 0; i < prec; i++) mul *= 10;

    int64_t ip = (int64_t)val;
    double frac = val - (double)ip;
    int fp = (int)(frac * (double)mul + 0.5); /* round */

    /* carry */
    if (fp >= mul)
    {
        fp = 0;
        ip++;
    }

    uint32_t need = (uint32_t)( (neg ? 1 : 0) + 1 + 20 + (prec ? 1 + prec : 0) );
    if (!XBuffer_CanWrite(xb, need)) /* 보수적으로 넉넉히 체크 */
    {
       ;
    }

    if (neg)
    {
        if (!XBuffer_CanWrite(xb, 1)) return false;
        XBuffer_PushChar(xb, '-');
    }

    if (!_xbuf_add_s64_dec(xb, (int64_t)ip)) return false;

    if (prec > 0)
    {
        if (!XBuffer_CanWrite(xb, (uint32_t)(1 + prec))) return false;
        XBuffer_PushChar(xb, '.');

        /* zero padding */
        int div = mul / 10;
        while (div > 0)
        {
            int digit = (fp / div) % 10;
            XBuffer_PushChar(xb, (char)('0' + digit));
            div /= 10;
        }
    }

    return true;
}

tsXBuffer *XBuffer_Create(unsigned int buffer_size)
{
    tsXBuffer *xBuffer = (tsXBuffer *)pvPortMalloc(sizeof(tsXBuffer));
    if (xBuffer == NULL)
        return NULL;

    xBuffer->size_ = buffer_size;
    xBuffer->count_ = 0;

    xBuffer->buffer_ = (char *)pvPortMalloc(buffer_size);
    if (xBuffer->buffer_ == NULL)
    {
        vPortFree(xBuffer);
        return NULL;
    }

    XBuffer_Clear(xBuffer);
    return xBuffer;
}

void XBuffer_Destroy(tsXBuffer *xBuffer)
{
    if (!xBuffer) return;

    if (xBuffer->buffer_)
        vPortFree(xBuffer->buffer_);

    vPortFree(xBuffer);
}

void XBuffer_Start(tsXBuffer *xBuffer)
{
    XBuffer_Clear(xBuffer);
}

void XBuffer_End(tsXBuffer *xBuffer)
{
    if (!xBuffer) return;

    /* "\r\n\0" 총 3바이트 필요 */
    if (!XBuffer_CanWrite(xBuffer, 3))
        return;

    xBuffer->buffer_[xBuffer->count_++] = '\r';
    xBuffer->buffer_[xBuffer->count_++] = '\n';
    xBuffer->buffer_[xBuffer->count_] = '\0';
}

void XBuffer_End_NULL(tsXBuffer *xBuffer)
{
    if (!xBuffer) return;

    if (!XBuffer_CanWrite(xBuffer, 1))
        return;

    xBuffer->buffer_[xBuffer->count_] = '\0';
}

bool XBuffer_AddChar(tsXBuffer *xBuffer, char data, bool isAddComma)
{
    if (!xBuffer) return false;

    uint32_t need = 1U + (isAddComma ? 1U : 0U);
    if (!XBuffer_CanWrite(xBuffer, need))
        return false;

    XBuffer_PushChar(xBuffer, data);
    if (isAddComma)
        XBuffer_PushChar(xBuffer, ',');

    return true;
}

bool XBuffer_AddString(tsXBuffer *xBuffer, const char *data, bool isAddComma)
{
    if (!xBuffer || !data) return false;

    uint32_t len = _strlen_u32(data);
    uint32_t need = len + (isAddComma ? 1U : 0U);

    if (!XBuffer_CanWrite(xBuffer, need))
        return false;

    while (*data)
        XBuffer_PushChar(xBuffer, *data++);

    if (isAddComma)
        XBuffer_PushChar(xBuffer, ',');

    return true;
}

bool XBuffer_AddCommandString(tsXBuffer *xBuffer, const char *data, bool isAddComma)
{
    if (!xBuffer || !data) return false;

    uint32_t len = _strlen_u32(data);
    /* 문자열 + 공백 1 + (comma?) */
    uint32_t need = len + 1U + (isAddComma ? 1U : 0U);

    if (!XBuffer_CanWrite(xBuffer, need))
        return false;

    while (*data)
        XBuffer_PushChar(xBuffer, *data++);

    XBuffer_PushChar(xBuffer, ' ');

    if (isAddComma)
        XBuffer_PushChar(xBuffer, ',');

    return true;
}

bool XBuffer_AddInt(tsXBuffer *xBuffer, int data, bool isAddComma)
{
    if (!xBuffer) return false;

    uint32_t need = 11U + (isAddComma ? 1U : 0U);
    if (!XBuffer_CanWrite(xBuffer, need))
        return false;

    if (!_xbuf_add_s32_dec(xBuffer, (int32_t)data))
        return false;

    if (isAddComma)
        XBuffer_PushChar(xBuffer, ',');

    return true;
}

bool XBuffer_AddLong(tsXBuffer *xBuffer, long data, bool isAddComma)
{
    if (!xBuffer) return false;

    uint32_t need = 24U + (isAddComma ? 1U : 0U);
    if (!XBuffer_CanWrite(xBuffer, need))
        return false;

    if (!_xbuf_add_s64_dec(xBuffer, (int64_t)data))
        return false;

    if (isAddComma)
        XBuffer_PushChar(xBuffer, ',');

    return true;
}

/* 소수점 2자리 고정 (빠름 / 안전) */
bool XBuffer_AddFloat(tsXBuffer *xBuffer, float data, bool isAddComma)
{
    if (!xBuffer)
        return false;

    bool is_negative = (data < 0.0f);
    if (is_negative)
        data = -data;

    int int_part = (int)data;
    float frac = data - (float)int_part;

    const int mul = 100;
    int frac_part = (int)(frac * (float)mul + 0.5f);

    if (frac_part >= mul)
    {
        frac_part = 0;
        int_part++;
    }

    uint32_t need = (is_negative ? 1U : 0U) + 10U + 3U + (isAddComma ? 1U : 0U);
    if (!XBuffer_CanWrite(xBuffer, need))
        return false;

    if (is_negative)
        XBuffer_PushChar(xBuffer, '-');

    if (!_xbuf_add_s32_dec(xBuffer, (int32_t)int_part))
        return false;

    if (!XBuffer_CanWrite(xBuffer, 3U + (isAddComma ? 1U : 0U)))
        return false;

    XBuffer_PushChar(xBuffer, '.');
    XBuffer_PushChar(xBuffer, (char)('0' + (frac_part / 10)));
    XBuffer_PushChar(xBuffer, (char)('0' + (frac_part % 10)));

    if (isAddComma)
        XBuffer_PushChar(xBuffer, ',');

    return true;
}

bool XBuffer_AddDouble(tsXBuffer *xBuffer, double data, bool isAddComma)
{
    if (!xBuffer) return false;

    uint32_t need = 1U + 20U + 3U + (isAddComma ? 1U : 0U);
    if (!XBuffer_CanWrite(xBuffer, need))
        return false;

    if (!_xbuf_add_fixed_double(xBuffer, data, 2))
        return false;

    if (isAddComma)
        XBuffer_PushChar(xBuffer, ',');

    return true;
}

bool XBuffer_AddDoubleWithPrecision(tsXBuffer *xBuffer, double data, signed char width, signed char prec, bool isAddComma)
{
    if (!xBuffer) return false;

    int w = (int)width;
    int p = (int)prec;

    if (p < 0) p = 0;
    if (p > 6) p = 6;

    uint32_t base_need = 1U + 20U + 1U + 6U + (isAddComma ? 1U : 0U);
    if (!XBuffer_CanWrite(xBuffer, base_need))
        return false;

    char tmp[64];
    tsXBuffer t = { .buffer_ = tmp, .size_ = sizeof(tmp), .count_ = 0 };

    if (!_xbuf_add_fixed_double(&t, data, p))
        return false;

    if (t.count_ >= t.size_) return false;
    tmp[t.count_] = '\0';

    int out_len = (int)t.count_;
    int pad = (w > out_len) ? (w - out_len) : 0;

    uint32_t need = (uint32_t)pad + (uint32_t)out_len + (isAddComma ? 1U : 0U);
    if (!XBuffer_CanWrite(xBuffer, need))
        return false;

    if (!_xbuf_add_spaces(xBuffer, pad))
        return false;

    for (int i = 0; i < out_len; i++)
        XBuffer_PushChar(xBuffer, tmp[i]);

    if (isAddComma)
        XBuffer_PushChar(xBuffer, ',');

    return true;
}

bool XBuffer_AddByte(tsXBuffer *xBuffer, unsigned char data, bool isAddComma)
{
    return XBuffer_AddUInt(xBuffer, (unsigned int)data, isAddComma);
}

bool XBuffer_AddU08(tsXBuffer *xBuffer, U08 data, bool isAddComma)
{
    return XBuffer_AddUInt(xBuffer, (unsigned int)data, isAddComma);
}

bool XBuffer_AddBool(tsXBuffer *xBuffer, bool data, bool isAddComma)
{
    return XBuffer_AddUInt(xBuffer, (unsigned int)(data ? 1U : 0U), isAddComma);
}

bool XBuffer_AddUnsignedShort(tsXBuffer *xBuffer, unsigned short data, bool isAddComma)
{
    return XBuffer_AddUInt(xBuffer, (unsigned int)data, isAddComma);
}

bool XBuffer_AddUInt(tsXBuffer *xBuffer, unsigned int data, bool isAddComma)
{
    if (!xBuffer) return false;

    /* uint32 최대 10자리 + comma */
    uint32_t need = 10U + (isAddComma ? 1U : 0U);
    if (!XBuffer_CanWrite(xBuffer, need))
        return false;

    if (!_xbuf_add_u32_dec(xBuffer, (uint32_t)data))
        return false;

    if (isAddComma)
        XBuffer_PushChar(xBuffer, ',');

    return true;
}

void XBuffer_Clear(tsXBuffer *xBuffer)
{
    if (!xBuffer || !xBuffer->buffer_) return;

    memset(xBuffer->buffer_, 0, (size_t)xBuffer->size_);
    xBuffer->count_ = 0;
}

bool XBuffer_Get(tsXBuffer *xBuffer, unsigned int index, char *item)
{
    if (!xBuffer || !item) return false;
    if (index >= xBuffer->count_) return false;

    *item = xBuffer->buffer_[index];
    return true;
}

char *XBuffer_GetBuffer(tsXBuffer *xBuffer)
{
    if (!xBuffer) return NULL;
    return xBuffer->buffer_;
}

unsigned int XBuffer_Length(tsXBuffer *xBuffer)
{
    if (!xBuffer) return 0U;
    return (unsigned int)xBuffer->count_;
}

bool XBuffer_GetBufferToChar(tsXBuffer *xBuffer, char buffer[], unsigned int buffer_size)
{
    if (!xBuffer || !buffer) return false;
    if (buffer_size == 0) return false;

    /* xBuffer->count_ 만큼만 복사 + null */
    unsigned int n = xBuffer->count_;
    if (n >= buffer_size) n = buffer_size - 1;

    memcpy(buffer, xBuffer->buffer_, n);
    buffer[n] = '\0';
    return true;
}