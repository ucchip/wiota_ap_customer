#ifndef VSI_H
#define VSI_H

#include "adp_sys.h"
#include "uctypes.h"
#include <rtthread.h>

#ifdef _ASIC_

#define VSI_OK 0
#define VSI_TIMEOUT 1
#define VSI_ERROR (-1)
#define OS_ERROR (-1)

#define _ALLOW_TRACE_PRITF_TEST_
#endif


#ifdef _ALLOW_TRACE_PRITF_TEST_

#ifdef _LINUX_
#include <stdio.h>
#define TRACE_PRINTF    printf
#define TRACE_SPRINTF   sprintf
#define TRACE_PUTS      puts

#else

#define TRACE_PRINTF    rt_kprintf  
#define TRACE_SPRINTF(...) 
#define TRACE_PUTS(...)    

#endif

#else
#define TRACE_PRINTF    rt_kprintf
#endif





#endif /* VSI_H */
