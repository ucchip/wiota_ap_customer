#ifndef __UC_DEBUG_H__
#define __UC_DEBUG_H__

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#include <stdio.h>
#include "vsi.h"

#ifdef UC_DEBUG
#define  UCDBG(...) TRACE_PRINTF(__VA_ARGS__)
#define  INFO(...) TRACE_PRINTF(__VA_ARGS__)
#else
#define  UCDBG(...)
#define  INFO(...) 
#endif



/* Block Driver is heavy, use a standalone
debug routine
*/
#ifdef _ALLOW_TRACE_PRITF_TEST_
#define HW_DEBUG
#define BD_DEBUG
#define TRACK_UNPORTED
#define TRACK_REMOVE
#else
//#define HW_DEBUG
//#define BD_DEBUG
//#define TRACK_UNPORTED
//#define TRACK_REMOVE
#endif

#ifdef  BD_DEBUG
#define BD_DBG(...) TRACE_PRINTF(__VA_ARGS__)
#else
#define  BD_DBG(...)
#endif

#ifdef  HW_DEBUG
#define HW_DBG(...) TRACE_PRINTF(__VA_ARGS__)
#else
#define  HW_DBG(...)
#endif

#define DSP_DBG

#ifdef TRACK_UNPORTED
#define NOTICE TRACE_PRINTF("\x1b[34mTODO PORT:%s %s\x1b[0m\n",__FILE__,__func__);
#else
#define NOTICE
#endif

#ifdef TRACK_REMOVE
#define REMOVE TRACE_PRINTF("REMOVE:%s %s\n",__FILE__,__func__);
#else
#define REMOVE
#endif

#ifdef _ALLOW_TRACE_PRITF_TEST_
#define FENTER TRACE_PRINTF("\x1b[36mENT:%s %s\x1b[0m\n",__FILE__,__func__);
#define ATTENTION TRACE_PRINTF("\x1b[31mAttn: %s %d %s\x1b[0m\n", __FILE__, __LINE__, __func__);
#define ATTNMSG( M )  TRACE_PRINTF("\x1b[31mAttn: %s %s %d\x1b[0m\n", M, __FILE__, __LINE__);

/* some fake function for porting */
#define MC_SOS_ERROR_EXCEPTION(a,b,c,d,e)  TRACE_PRINTF("EXCEPTION NOTICE %s %d!!!\n", __FILE__, __LINE__);
#define MC_L1C_DRT_DATA8(a, b) {ATTENTION; TRACE_PRINTF("szhuhalt\n"); while(1);}
#define MC_L1C_DRT_DATA16(a, b) {ATTENTION; while(1);}
#else
#define FENTER ;
#define ATTENTION ;
#define ATTNMSG( M )  ;
/* some fake function for porting */
#define MC_SOS_ERROR_EXCEPTION(a,b,c,d,e)  
#define MC_L1C_DRT_DATA8(a, b) {ATTENTION;  while(1);}
#define MC_L1C_DRT_DATA16(a, b) {ATTENTION; while(1);}
    
#endif

#ifndef _LINUX_
#define UCHALT { __asm volatile 	( "csrc mstatus,8" ); \
        TRACE_PRINTF("HALT %s %d \n",__FILE__, __LINE__); for (;;);}
#define UCHALT_a(a) {if(!(a)) (*(s32_t*)(0)) = 1;}
#else
#define UCHALT {(*(s32_t*)(0)) = 1;}
#define UCHALT_a(a) {if(!(a)) (*(s32_t*)(0)) = 1;}
#endif

#endif
