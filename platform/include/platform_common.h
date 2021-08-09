#ifndef PLATFORM_COMMON_H
#define PLATFORM_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

#define likely(x)       (__builtin_expect(!!(x),1))
#define unlikely(x)     (__builtin_expect(!!(x),0))



#include <stddef.h>
#include "uctypes.h"
#include "vsi.h"
#include "trace.h"


//#define _FPGA_TRACE_TEST_
#include "trace_interface.h"


#define UC_ERR_CODE_OK      1U
#define UC_ERR_CODE_FAIL    0U


#ifndef NULL
#define NULL ((void*)0)
#endif

#define OFFSET(type, member) ((size_t) & (((type *)NULL)->member))



#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif

    


static inline void* os_malloc(u32_t size, const char *file, const char *function, int lineNo)
{
    void *ptr = uc_malloc(size);
    if (NULL == ptr)
    {
        TRACE_EVENT_P4("%s failed: %s %s %d\r\n", __FUNCTION__, file, function, lineNo);
        return NULL;
    }
    return ptr;
}

static inline void os_free(void *ptr)
{
    uc_free(ptr);
    return;
}

#define OS_MALLOC(size)     os_malloc(size, __FILE__, __FUNCTION__, __LINE__)
#define OS_FREE(ptr)        {UC_ASSERT_OP(NULL, !=, (ptr)); os_free(ptr); (ptr) = NULL;}

static inline void os_delay(u32_t sec)
{
    // can't be called before createTask
#ifdef _LINUX_
    sleep(sec);
#else
    //vTaskDelay(pdMS_TO_TICKS(sec * 1000));
    uc_thread_delay(sec * 1000);
#endif
    return;
}

static inline void os_udelay(u32_t ms)
{
#ifdef _LINUX_
    usleep(ms * 1000);
#else
    //vTaskDelay(pdMS_TO_TICKS(ms));
    uc_thread_delay(ms);
#endif
    return;
}

static inline u32_t os_getTimeStamp(void)
{
#ifdef _LINUX_
    struct timeval stamp;
    gettimeofday(&stamp, NULL);
    return (stamp.tv_sec * 1000000 + stamp.tv_usec);
#else
    return *(u32_t*)0x3b0014;
    //return xTaskGetTickCount();
#endif
}


#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_COMMON_H */
