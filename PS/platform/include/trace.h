/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   trace.h
 * Author: hguo
 *
 * Created on September 17, 2020, 7:40 PM
 */

#ifndef TRACE_H
#define TRACE_H

#ifdef __cplusplus
extern "C"
{
#endif



//#include "platform_common.h"
#include "uctypes.h"
#include "trace_interface.h"


#define configASSERT( x ) if( ( x ) == 0 ) { for( ;; ); }


#ifdef _LINUX_
#include <assert.h>
#define OS_ASSERT(x) assert(x)
#else
#define OS_ASSERT(x) configASSERT(x)
#endif

#define TO_STR(x) #x

    static inline char *get_fileName(char *file)
    {
        char *fileName = file;
        for (u32_t i = 0; 0 != file[i]; i++)
        {
            if (('/' == file[i]) && (0 != file[i + 1]))
            {
                fileName = file + i + 1;
            }
        }
        return fileName;
    }

#define GET_FILE_NAME() get_fileName(__FILE__)

#define UC_ASSERT(x)                                              \
    {                                                             \
        if (!(x))                                                 \
        {                                                         \
            TRACE_PRINTF("assert failed : %s %d : (%s)\n",        \
                         GET_FILE_NAME(), __LINE__, TO_STR(x));   \
            TRACE_EVENT_P3("assert failed : %s %d : (%s)\n",      \
                           GET_FILE_NAME(), __LINE__, TO_STR(x)); \
            OS_ASSERT(x);                                         \
        }                                                         \
    }

#define UC_ASSERT_OP(a, op, b)                                    \
    {                                                             \
        if (!((a)op(b)))                                          \
        {                                                         \
            TRACE_PRINTF("assert failed:(%s:%d): (%u %s %u)\n",   \
                         GET_FILE_NAME(), __LINE__, (u32_t)(a),   \
                         TO_STR(op), (u32_t)(b));                 \
            TRACE_EVENT_P5("assert failed:(%s:%d): (%u %s %u)", \
                           GET_FILE_NAME(), __LINE__, (u32_t)(a), \
                           TO_STR(op), (u32_t)(b));               \
            OS_ASSERT((a)op(b));                                  \
        }                                                         \
    }

#define UC_ASSERT_OP_DESC(a, op, b, desc)                            \
    {                                                                \
        if (!((a)op(b)))                                             \
        {                                                            \
            TRACE_PRINTF("assert failed:(%s:%d): %s (%u %s %u)\n",   \
                         GET_FILE_NAME(), __LINE__, (desc),          \
                         (u32_t)(a), TO_STR(op), (u32_t)(b));        \
            TRACE_EVENT_P5("assert failed:(%s:%d): %s (%u %s %u)", \
                           GET_FILE_NAME(), __LINE__, (desc),        \
                           (u32_t)(a), TO_STR(op), (u32_t)(b));      \
            OS_ASSERT((a)op(b));                                     \
        }                                                            \
    }

    void trace_show_hex(void *data, u32_t len, const char *funcName, int lineNum);
    //#define TRACE_SHOW_HEX(data, len)   trace_show_hex(data,len,__FUNCTION__,__LINE__)

    void trace_show_hex2(void *data, u32_t len, const char *funcName, int lineNum);
#define TRACE_SHOW_HEX(data, len) trace_show_hex2(data, len, __FUNCTION__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif /* TRACE_H */
