/*******************************************************************************
 * Trace Recorder Library for Tracealyzer v3.3.0
 * Percepio AB, www.percepio.com
 *
 * trcKernelPort.c
 *
 * The FreeRTOS-specific parts of the trace recorder
 *
 * Terms of Use
 * This file is part of the trace recorder library (RECORDER), which is the 
 * intellectual property of Percepio AB (PERCEPIO) and provided under a
 * license as follows.
 * The RECORDER may be used free of charge for the purpose of recording data
 * intended for analysis in PERCEPIO products. It may not be used or modified
 * for other purposes without explicit permission from PERCEPIO.
 * You may distribute the RECORDER in its original source code form, assuming
 * this text (terms of use, disclaimer, copyright notice) is unchanged. You are
 * allowed to distribute the RECORDER with minor modifications intended for
 * configuration or porting of the RECORDER, e.g., to allow using it on a 
 * specific processor, processor family or with a specific communication
 * interface. Any such modifications should be documented directly below
 * this comment block.  
 *
 * Disclaimer
 * The RECORDER is being delivered to you AS IS and PERCEPIO makes no warranty
 * as to its use or performance. PERCEPIO does not and cannot warrant the 
 * performance or results you may obtain by using the RECORDER or documentation.
 * PERCEPIO make no warranties, express or implied, as to noninfringement of
 * third party rights, merchantability, or fitness for any particular purpose.
 * In no event will PERCEPIO, its technology partners, or distributors be liable
 * to you for any consequential, incidental or special damages, including any
 * lost profits or lost savings, even if a representative of PERCEPIO has been
 * advised of the possibility of such damages, or for any claim by any third
 * party. Some jurisdictions do not allow the exclusion or limitation of
 * incidental, consequential or special damages, or the exclusion of implied
 * warranties or limitations on how long an implied warranty may last, so the
 * above limitations may not apply to you.
 *
 * Tabs are used for indent in this file (1 tab = 4 spaces)
 *
 * Copyright Percepio AB, 2017.
 * www.percepio.com
 ******************************************************************************/
#include "sectdefs.h"
#include "ctrl_cmd.h"
#define REDIRECT_prvPagedEventBufferTransfer_INSPIKE 1

#if 1// (TRC_USE_TRACEALYZER_RECORDER == 1)

#if (configUSE_TICKLESS_IDLE != 0 && (TRC_HWTC_TYPE == TRC_OS_TIMER_INCR || TRC_HWTC_TYPE == TRC_OS_TIMER_DECR))
    /* 	
        The below error message is to alert you on the following issue:
        
        The hardware port selected in trcConfig.h uses the operating system timer for the 
        timestamping, i.e., the periodic interrupt timer that drives the OS tick interrupt.
                
        When using "tickless idle" mode, the recorder needs an independent time source in
        order to correctly record the durations of the idle times. Otherwise, the trace may appear
        to have a different length than in reality, and the reported CPU load is also affected.
        
        You may override this warning by defining the TRC_CFG_ACKNOWLEDGE_TICKLESS_IDLE_WARNING
        macro in your trcConfig.h file. But then the time scale may be incorrect during
        tickless idle periods.
        
        To get this correct, override the default timestamping by setting TRC_CFG_HARDWARE_PORT
        in trcConfig.h to TRC_HARDWARE_PORT_APPLICATION_DEFINED and define the HWTC macros
        accordingly, using a free running counter or an independent periodic interrupt timer.
        See trcHardwarePort.h for details.
                
        For ARM Cortex-M3, M4 and M7 MCUs this is not an issue, since the recorder uses the 
        DWT cycle counter for timestamping in these cases.		
    */
    
    #ifndef TRC_CFG_ACKNOWLEDGE_TICKLESS_IDLE_WARNING
    //#error Trace Recorder: This timestamping mode is not recommended with Tickless Idle.
    #endif
#endif /* (configUSE_TICKLESS_IDLE != 0 && (TRC_HWTC_TYPE == TRC_OS_TIMER_INCR || TRC_HWTC_TYPE == TRC_OS_TIMER_DECR)) */

//#include "task.h"
//#include "queue.h"


#if (TRC_CFG_INCLUDE_TIMER_EVENTS == 1 && TRC_CFG_FREERTOS_VERSION >= TRC_FREERTOS_VERSION_8_X)
/* If the project does not include the FreeRTOS timers, TRC_CFG_INCLUDE_TIMER_EVENTS must be set to 0 */

#include "timers.h"

#endif /* (TRC_CFG_INCLUDE_TIMER_EVENTS == 1 && TRC_CFG_FREERTOS_VERSION >= TRC_FREERTOS_VERSION_8_X) */

#if (TRC_CFG_INCLUDE_EVENT_GROUP_EVENTS == 1 && TRC_CFG_FREERTOS_VERSION >= TRC_FREERTOS_VERSION_8_X)
/* If the project does not include the FreeRTOS event groups, TRC_CFG_INCLUDE_TIMER_EVENTS must be set to 0 */
#include "event_groups.h"
#endif /* (TRC_CFG_INCLUDE_EVENT_GROUP_EVENTS == 1 && TRC_CFG_FREERTOS_VERSION >= TRC_FREERTOS_VERSION_8_X) */

//#if (TRC_CFG_FREERTOS_VERSION >= TRC_FREERTOS_VERSION_10_0_0)
//#include "stream_buffer.h"
//#endif /* (TRC_CFG_FREERTOS_VERSION >= TRC_FREERTOS_VERSION_10_0_0) */

#include "trcKernelPort.h"
#include "trcStreamingPort.h"
#include "trcRecorder.h"

//uint8_t prvTraceGetQueueType(void* handle)
//{
//    // This is either declared in header file in FreeRTOS 8 and later, or as extern above
//
//    return ucQueueGetQueueType(handle);
//
//}

/* Tasks */
//uint16_t prvTraceGetTaskNumberLow16(void* handle)
//{
//
//    return TRACE_GET_LOW16(uxTaskGetTaskNumber(handle));
//
//}

//uint16_t prvTraceGetTaskNumberHigh16(void* handle)
//{
//
//    return TRACE_GET_HIGH16(uxTaskGetTaskNumber(handle));
//
//}

//void prvTraceSetTaskNumberLow16(void* handle, uint16_t value)
//{
//    vTaskSetTaskNumber(handle, TRACE_SET_LOW16(uxTaskGetTaskNumber(handle), value));
//
//}

//void prvTraceSetTaskNumberHigh16(void* handle, uint16_t value)
//{
//#ifndef _LINUX_TRACE_
//    vTaskSetTaskNumber(handle, TRACE_SET_HIGH16(uxTaskGetTaskNumber(handle), value));
//#endif
//}



#if 1 //(TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING)

    
    
#if defined(configSUPPORT_STATIC_ALLOCATION)
#if (configSUPPORT_STATIC_ALLOCATION == 1)
static StackType_t stackTzCtrl[TRC_CFG_CTRL_TASK_STACK_SIZE];
static StaticTask_t tcbTzCtrl;
#endif
#endif



/*******************************************************************************
 * vTraceEnable
 *
 * Function that enables the tracing and creates the control task. It will halt
 * execution until a Start command has been received if haltUntilStart is true.
 *
 ******************************************************************************/


extern void cce_trace_init (void);

void vTraceEnable(int startOption)
{
    int bytes = 0;
    int status;
    extern unsigned int  RecorderEnabled;
    TracealyzerCommandType msg;

    TRC_STREAM_PORT_INIT(); 

    if (startOption == TRC_START_AWAIT_HOST)
    {
        cce_trace_init();
       
        /* We keep trying to read commands until the recorder has been started */
        do
        {
            bytes = 0;
            
            status = TRC_STREAM_PORT_READ_DATA(&msg, sizeof(TracealyzerCommandType), (int*)&bytes);
            
            
            if ((status == 0) && (bytes == sizeof(TracealyzerCommandType)))
            {
                //if (prvIsValidCommand(&msg))
                //{
                    if (msg.cmd_type == CMD_SET_ACTIVE && msg.cmd_data[0] == 1)
                    {
                        /* On start, init and reset the timestamping */
                        //TRC_PORT_SPECIFIC_INIT();
                    }
                    prvProcessCommand(&msg);
                    //log_cmd_proc(&msg);
                    TRC_STREAM_PORT_ON_TRACE_FLAG(CMD_UNVALID);
               // }
            }
        }
        while (RecorderEnabled == 0);
    }
    else if (startOption == TRC_START)
    {
        /* We start streaming directly - this assumes that the interface is ready! */
        //TRC_PORT_SPECIFIC_INIT();
        msg.cmd_type = CMD_SET_ACTIVE;
        msg.cmd_data[0] = 1;
        prvProcessCommand(&msg);
    }
    else
    {
        /* On TRC_INIT */
       // TRC_PORT_SPECIFIC_INIT();
    }
}


/*******************************************************************************
 * prvGetCurrentTaskHandle
 *
 * Function that returns the handle to the currently executing task.
 *
 ******************************************************************************/
//void* prvTraceGetCurrentTaskHandle(void)
//{
//    return xTaskGetCurrentTaskHandle();

//}



/*******************************************************************************
 * TzCtrl
 *
 * Task for sending the trace data from the internal buffer to the stream 
 * interface (assuming TRC_STREAM_PORT_USE_INTERNAL_BUFFER == 1) and for
 * receiving commands from Tracealyzer. Also does some diagnostics.
 ******************************************************************************/
void trace_control(void)
{
    TracealyzerCommandType msg;
    int bytes = 0;
    int status = 0;
//    int iscollect=1;
    
    do
    {
        /* Listen for new commands */
//        if(iscollect && !isRTOSTraceUIConnect()){
//            iscollect=0;
//            proc_disact_all_log();
//            vTraceSetFilterMask(0x0000);
//            //prvSetRecorderEnabled(0);
//        }
        
        bytes = 0;
        status = TRC_STREAM_PORT_READ_DATA(&msg, sizeof(TracealyzerCommandType), (int*)&bytes);

        if ((status == 0) && (bytes == sizeof(TracealyzerCommandType)))
        {
            
            //if (prvIsValidCommand(&msg))
            //{
                prvProcessCommand(&msg);
                //log_cmd_proc(&msg); /* Start or Stop currently... */
                TRC_STREAM_PORT_ON_TRACE_FLAG(CMD_UNVALID);
            //}
        }

/* If the internal buffer is disabled, the COMMIT macro instead sends the data directly 
from the "event functions" (using TRC_STREAM_PORT_WRITE_DATA). */
#if (TRC_STREAM_PORT_USE_INTERNAL_BUFFER == 1)
        /* If there is a buffer page, this sends it to the streaming interface using TRC_STREAM_PORT_WRITE_DATA. */
    #if (!REDIRECT_prvPagedEventBufferTransfer_INSPIKE)

        bytes = prvPagedEventBufferTransfer();
    #else
        bytes=0;
    #endif
#endif
        
    /* If there was data sent or received (bytes != 0), loop around and repeat, if there is more data to send or receive.
    Otherwise, step out of this loop and sleep for a while. */
    
    } while (bytes != 0);

    //prvCheckRecorderStatus();
    
//    return 0;
    return;

}

#endif /*(TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING)*/


/*******************************************************************************
* vTraceSetMessageBufferName(void* object, const char* name)
*
* Parameter object: pointer to the MessageBuffer that shall be named
* Parameter name: the name to set (const string literal)
*
* Sets a name for MessageBuffer objects for display in Tracealyzer.
******************************************************************************/
//void vTraceSetMessageBufferName(void* object, const char* name)
//{
//    prvTraceSetObjectName(TRACE_CLASS_MESSAGEBUFFER, TRACE_GET_OBJECT_NUMBER(STREAMBUFFER, object), name);
//}
//#endif /* (TRC_CFG_FREERTOS_VERSION >= TRC_FREERTOS_VERSION_10_0_0) */


#if _BUILD_REMOVE_WARNING
#if (TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_SNAPSHOT)
    
void* prvTraceGetCurrentTaskHandle()
{
    return xTaskGetCurrentTaskHandle();
}

#endif

#endif /* Snapshot mode */

#endif /*(TRC_USE_TRACEALYZER_RECORDER == 1)*/
