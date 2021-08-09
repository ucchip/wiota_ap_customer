#define  L1_TEST_SUBMODULE
#include "platform_common.h"

#include "trace.h"

#define USE_STDIO_H 0

#if !USE_STDIO_H

#define LOWER_CASE 0
#define UPPER_CASE 1

static inline u8_t trace_show_4BitsToHex_lowerCase(u8_t c)
{
    //    UC_ASSERT_OP(c, <, 16);

    if (c < 10)
    {
        return '0' + c;
    }
    else
    {
        return 'a' + (c - 10);
    }
}

#define trace_show_4BitsToHex_lowerCase(c) "0123456789abcdef"[c]

static inline void trace_show_setCharHex(u8_t *buf, u8_t c)
{
    (buf)[0] = ' ';
    (buf)[1] = trace_show_4BitsToHex_lowerCase(c >> 4);
    (buf)[2] = trace_show_4BitsToHex_lowerCase(c & 0x0f);
    //    buf[3] = 0;
    return;
}

//static inline void trace_show_setAddress(u8_t *buf, void *ptr)
//{
//    buf[0] = trace_show_4BitsToHex_lowerCase((((ul32_t) ptr) >> 28)&0x0f);
//    buf[1] = trace_show_4BitsToHex_lowerCase((((ul32_t) ptr) >> 24)&0x0f);
//
//    buf[2] = trace_show_4BitsToHex_lowerCase((((ul32_t) ptr) >> 20)&0x0f);
//    buf[3] = trace_show_4BitsToHex_lowerCase((((ul32_t) ptr) >> 16)&0x0f);
//
//    buf[4] = trace_show_4BitsToHex_lowerCase((((ul32_t) ptr) >> 12)&0x0f);
//    buf[5] = trace_show_4BitsToHex_lowerCase((((ul32_t) ptr) >> 8)&0x0f);
//
//    buf[6] = trace_show_4BitsToHex_lowerCase((((ul32_t) ptr) >> 4)&0x0f);
//    buf[7] = trace_show_4BitsToHex_lowerCase((((ul32_t) ptr) >> 0)&0x0f);
//
//    //    buf[8] = 0;
//
//    return;
//}

void trace_show_hex(void *data, u32_t len, const char *funcName, int lineNum)
{
    u32_t tick0 = os_getTimeStamp();

#define MAX_BYTE_NUM_IN_1_LINE 16
#define BUF_ADDRESS_LEN 0 // 0 means do not show address at the line head.
#define BUF_LEN (BUF_ADDRESS_LEN + (MAX_BYTE_NUM_IN_1_LINE * 3) + 1)
    u8_t lineBuf[BUF_LEN];
    lineBuf[BUF_LEN - 1] = 0;

#define HEAD_LINE " 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f"
    TRACE_EVENT_P5("\nat %s:%d addr=%p len=%u\n%s\n", funcName, lineNum, data, len, HEAD_LINE);

    int bufLen = 0;
    //    bufLen += trace_show_setAddress(buf + bufLen, data); // address
    for (int i = 0; i < len; i++)
    {
        trace_show_setCharHex(lineBuf + bufLen, ((u8_t *)data)[i]);
        bufLen += 3;
        if (0 == ((i + 1) & (MAX_BYTE_NUM_IN_1_LINE - 1)))
        {
            TRACE_EVENT_P1("%s\n", lineBuf);
            bufLen = 0;
            //            bufLen += trace_show_setAddress(buf + bufLen, data + i + 1); // address
        }
    }

    if ((BUF_ADDRESS_LEN < bufLen) || (len <= 0)) // if len is 0, also show a address head.
    {
        lineBuf[bufLen] = 0;
        TRACE_EVENT_P1("%s\n\n", lineBuf);
    }
    else
    {
        TRACE_EVENT("\n");
    }

#if 1
    u32_t tick1 = os_getTimeStamp();
    TRACE_EVENT_P2("%s cost %u\n", __FUNCTION__, tick1 - tick0);
#endif

    return;
}

#else

#include <stdio.h>

void trace_show_hex(void *data, u32_t len, const char *funcName, int line)
{
#define MAX_BYTE_NUM_IN_1_LINE 16
#define BUF_ADDRESS_LEN 8
#define BUF_LEN (BUF_ADDRESS_LEN + (MAX_BYTE_NUM_IN_1_LINE * 3) + 1)
    char buf[BUF_LEN];
    int bufLen = 0;

    bufLen += sprintf(buf + bufLen, "%*c", BUF_ADDRESS_LEN, ' ');

    for (int i = 0; i < MAX_BYTE_NUM_IN_1_LINE; i++)
    {
        bufLen += sprintf(buf + bufLen, " %.02X", i);
    }
    TRACE_EVENT_P4("\nat %s:%d len=%u\n%s\n", funcName, line, len, buf);

    bufLen = 0;

    bufLen += sprintf(buf + bufLen, "%.08X", (u32_t)data); // address
    for (int i = 0; i < len; i++)
    {
        bufLen += sprintf(buf + bufLen, " %.02x", ((u8_t *)data)[i]);
        if (0 == ((i + 1) & (MAX_BYTE_NUM_IN_1_LINE - 1)))
        {
            TRACE_EVENT_P1("%s\n", buf);
            bufLen = 0;

            bufLen += sprintf(buf + bufLen, "%.08X", (u32_t)(data + i + 1)); // address
        }
    }

    if ((BUF_ADDRESS_LEN < bufLen) || (len <= 0)) // if len is 0, also show a address head.
    {
        TRACE_EVENT_P1("%s\n", buf);
    }

    return;
}

#endif

void trace_show_hex2(void *data, u32_t len, const char *funcName, int lineNum)
{
    TRACE_EVENT_P2("%s:%d", funcName, lineNum);
    for (u32_t i = 0; i < len; i++)
    {
        if (i % 16 == 0)
        {
            TRACE_EVENT("\n");
        }
        TRACE_EVENT_P1("%02x ", ((u8_t *)data)[i]);
    }

    TRACE_EVENT("\n");
    return;
}
