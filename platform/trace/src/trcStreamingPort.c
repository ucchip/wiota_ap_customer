//#include "sectdefs.h"
/*******************************************************************************
 * Trace Recorder Library for Tracealyzer v3.3.0
 * Percepio AB, www.percepio.com
 *
 * trcStreamingPort.c
 *
 * Supporting functions for trace streaming, used by the "stream ports" 
 * for reading and writing data to the interface.
 * Existing ports can easily be modified to fit another setup, e.g., a 
 * different TCP/IP stack, or to define your own stream port.
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

#include "trcRecorder.h"
#include "cce_addr.h"
#include "string.h"
#include "trcStreamingConfig.h"
#include "adp_sem.h"

__attribute__((section(".rv_trace_buff"))) unsigned char rv_trace_buf[ RV_TRACE_BUFFER_SIZE ];


#define REDIRECT_prvPagedEventBufferTransfer_INSPIKE 1
volatile uint8_t  *riscv_Buffter = NULL;
volatile uint8_t *dump_flag = NULL;
volatile unsigned int  *rv_sizeHH = NULL;
volatile uint8_t *rv_sizeHL = NULL;
volatile uint8_t *rv_sizeLH = NULL;
volatile uint8_t *rv_sizeLL = NULL;
volatile uint8_t *rv_read_addr=NULL;
volatile void *trace_tool_lock = NULL;

#if (REDIRECT_prvPagedEventBufferTransfer_INSPIKE==1)
PageType *PageInfo;
char* EventBuffer;
unsigned int  *redirectTotalBytesRemaining;
PageType CurentRingBuferAddress = {0};
#endif

#if 1 //(TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING)  
#if (1)


//FILE* traceFile = NULL;

void BuffterInit(void)
{
    int offset=0;
    
    memset(RV_TRACE_BUFFER, 0, RV_TRACE_BUFFER_SIZE);

    dump_flag  =    (uint8_t *)                 (RV_TRACE_BUFFER + offset);offset+=8; 
    rv_sizeHH =    (unsigned int  *)                (RV_TRACE_BUFFER + offset);offset+=4;
    rv_sizeHL =    (uint8_t *)                 (RV_TRACE_BUFFER + offset);offset+=4;
    rv_sizeLH =    (uint8_t *)                 (RV_TRACE_BUFFER + offset);offset+=4;
    rv_sizeLL =    (uint8_t *)                 (RV_TRACE_BUFFER + offset);offset+=4;
    rv_read_addr = (uint8_t *)                 (RV_TRACE_BUFFER + offset);offset+=16;
    PageInfo   =    (PageType *)                (RV_TRACE_BUFFER + offset);offset+=sizeof(PageType);
    redirectTotalBytesRemaining=(unsigned int *)     (RV_TRACE_BUFFER + offset);offset+=sizeof(unsigned int );
    redirectTotalBytesRemaining[0]= TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE;
    riscv_Buffter =   (uint8_t *)                 (RV_TRACE_BUFFER + offset);
    EventBuffer =   (char*)riscv_Buffter;
  
    unsigned int  *p=(unsigned int *)dump_flag;
    //this is ring buffer address.
    p[0]=(unsigned int )EventBuffer;//start address
    p[1]=(unsigned int )(EventBuffer+TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE);// end address
    
    PageInfo->read = EventBuffer;
    PageInfo->write = EventBuffer; //EventBuffer;
    
    CurentRingBuferAddress.read = EventBuffer;
    CurentRingBuferAddress.write = EventBuffer;
    
    //TRACE_PRINTF("BuffterInit buffaddr 0x%x startaddr 0x%x endaddr 0x%x\n", &(RV_TRACE_BUFFER), p[0],p[1]);
    trace_tool_lock = uc_create_lock("trace");
    return;
}

void trace_lock(void)
{
    if (trace_tool_lock)
        uc_lock((void*)trace_tool_lock, SYS_SEM_WAITING_FOREVER);
}

void trace_unlock(void)
{
    if (trace_tool_lock)
        uc_unlock((void*)trace_tool_lock);
}

int32_t writeToBuffter(void* data, unsigned int  size, int32_t *ptrBytesWritten)
{
    memcpy((void *)riscv_Buffter,data,size);
    *ptrBytesWritten = size;
    *rv_sizeLL = (uint8_t)(size);
    *rv_sizeLH = (uint8_t)(size >> 8);
    *dump_flag = 2;
    return 0;
}

int readToBuffter(void* data,unsigned int  size,int *ptrBytesRead)
{
    memcpy(data,(void*)rv_read_addr,size);
    if(rv_sizeHL[0]==0){
        *ptrBytesRead = 0;
        return 0;
    }
    *ptrBytesRead = size;
    return 0;
}

//uint8_t isRTOSTraceUIConnect()
//{
//    return rv_sizeHH[0];
//}

void vaild(uint8_t flag)
{
    *rv_sizeHL = flag;
    //rv_read_addr[0]=flag;
    return ;
}

void CloseBuffter()
{
    //TRACE_PRINTF("riscv_Buffter close!");
    //*dump_flag = 3;
    return ;
}
#endif 
#endif /*(TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING)*/
