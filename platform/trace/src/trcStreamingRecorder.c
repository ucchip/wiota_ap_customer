/*******************************************************************************
 * Trace Recorder Library for Tracealyzer v3.3.0
 * Percepio AB, www.percepio.com
 *
 * trcStreamingRecorder.c
 *
 * The generic core of the trace recorder's streaming mode.
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

#include <stdio.h>
#include <string.h>
#include "trcRecorder.h"
#include "ctrl_cmd.h"
#include "trcKernelPort.h"
#include "trcStreamingConfig.h"


#define REDIRECT_prvPagedEventBufferTransfer_INSPIKE 1
#define TRACE_DATA_FLAG 0x0903
typedef struct{
    uint16_t EventID;
    uint16_t Flag;
    unsigned int  Count;
    unsigned int  TS;
} BaseEvent;

typedef struct{
  BaseEvent base;
  unsigned int  param1;
  unsigned int  param2;
  unsigned int  param3;
} EventWithParam_3;

/* Used in event functions with variable number of parameters. */
typedef struct
{
  BaseEvent base;
  //uint8_t   submoduleID;//if submoduleId == 255,not user even.
  unsigned int   data[15]; /* maximum payload size */
} __attribute((packed)) largestEventType;


typedef struct
{
    BaseEvent base;
    uint8_t   submoduleID;
    uint8_t   logID;
    uint8_t   para_num;
    uint8_t   rsvd;
    unsigned int   data[12];
}stateEventType;//__attribute((packed)) stateEventType;

typedef struct
{
    BaseEvent base;	
    uint8_t   submoduleID;
    //uint8_t   logID;
    uint8_t   data_len;
    uint16_t   msg_id;
    uint8_t   data[256];
}__attribute((packed)) msgEventType;

typedef struct{
  unsigned int  psf;
  uint16_t version;
  uint16_t platform;
  unsigned int  options;
  uint16_t symbolSize;
  uint16_t symbolCount;
  uint16_t objectDataSize;
  uint16_t objectDataCount;
} PSFHeaderInfo;

/* The size of each slot in the Symbol Table */
//#define SYMBOL_TABLE_SLOT_SIZE (sizeof(unsigned int ) + (((TRC_CFG_SYMBOL_MAX_LENGTH)+(sizeof(unsigned int )-1))/sizeof(unsigned int ))*sizeof(unsigned int ))

//#define OBJECT_DATA_SLOT_SIZE (sizeof(unsigned int ) + sizeof(unsigned int ))

/* The total size of the Symbol Table */
//#define SYMBOL_TABLE_BUFFER_SIZE ((TRC_CFG_SYMBOL_TABLE_SLOTS) * SYMBOL_TABLE_SLOT_SIZE)

/* The total size of the Object Data Table */
//#define OBJECT_DATA_TABLE_BUFFER_SIZE ((TRC_CFG_OBJECT_DATA_SLOTS) * OBJECT_DATA_SLOT_SIZE)

/* The Symbol Table type - just a byte array */
//typedef struct{
//  union
//  {
//    unsigned int  pSymbolTableBufferUINT32[SYMBOL_TABLE_BUFFER_SIZE / sizeof(unsigned int )];
//    uint8_t pSymbolTableBufferUINT8[SYMBOL_TABLE_BUFFER_SIZE];
//  } SymbolTableBuffer;
//} SymbolTable;

/* The Object Data Table type - just a byte array */
//typedef struct{
//  union
//  {
//    unsigned int  pObjectDataTableBufferUINT32[OBJECT_DATA_TABLE_BUFFER_SIZE / sizeof(unsigned int )];
//    uint8_t pObjectDataTableBufferUINT8[OBJECT_DATA_TABLE_BUFFER_SIZE];
//  } ObjectDataTableBuffer;
//} ObjectDataTable;



/* Code used for "task address" when no task has started. (NULL = idle task) */
#define HANDLE_NO_TASK 2



#define PSF_ASSERT(_assert, _err) if (! (_assert)){ return; }

/* Part of the PSF format - encodes the number of 32-bit params in an event */
#define PARAM_COUNT(n) ((n & 0xF) << 12)

/* The Symbol Table instance - keeps names of tasks and other named objects. */
//static SymbolTable symbolTable = { { { 0 } } };

/* This points to the first unused entry in the symbol table. */
//static unsigned int  firstFreeSymbolTableIndex = 0;

/* The Object Data Table instance - keeps initial priorities of tasks. */
//static ObjectDataTable objectDataTable = { { { 0 } } };

/* This points to the first unused entry in the object data table. */
//static unsigned int  firstFreeObjectDataTableIndex = 0;

/* Keeps track of ISR nesting */
//static unsigned int  ISR_stack[TRC_CFG_MAX_ISR_NESTING];

/* Keeps track of ISR nesting */
//static int8_t ISR_stack_index = -1;

/* Any error that occurred in the recorder (also creates User Event) */
//static int errorCode = 0;

/* Counts the number of trace sessions (not yet used) */
static unsigned int  SessionCounter = 0u;

/* Master switch for recording (0 => Disabled, 1 => Enabled) */
unsigned int  RecorderEnabled = 0u;

/* Used to determine endian of data (big/little) */
//static unsigned int  PSFEndianessIdentifier = 0x50534600;

/* Used to interpret the data format */
//static uint16_t FormatVersion = 0x0004;

/* The number of events stored. Used as event sequence number. */
static unsigned int  eventCounter = 0;

/* The user event channel for recorder warnings, defined in trcKernelPort.c */
//extern char* trcWarningChannel;
//extern traceString xTraceStack;
/* Remembers if an earlier ISR in a sequence of adjacent ISRs has triggered a task switch.
In that case, vTraceStoreISREnd does not store a return to the previously executing task. */
//int32_t isPendingContextSwitch = 0;

unsigned int  uiTraceTickCount = 0;
unsigned int  timestampFrequency = 0;
//unsigned int  DroppedEventCounter = 0;
//unsigned int  TotalBytesRemaining_LowWaterMark = TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT * TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE;
#if (REDIRECT_prvPagedEventBufferTransfer_INSPIKE==1)
    extern PageType *PageInfo;
    //extern unsigned int *redirectTotalBytesRemaining;
    extern char* EventBuffer;
    extern volatile uint8_t *dump_flag;
    extern volatile unsigned int  *rv_sizeHH;
    //#define TotalBytesRemaining (redirectTotalBytesRemaining[0])
#else
    //unsigned int  TotalBytesRemaining = TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT * TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE;
    PageType PageInfo[TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT];
    char* EventBuffer = NULL;
  
#endif
/*******************************************************************************
 * NoRoomForSymbol
 *
 * Incremented on prvTraceSaveSymbol if no room for saving the symbol name. This
 * is used for storing the names of:
 * - Tasks
 * - Named ISRs (xTraceSetISRProperties)
 * - Named kernel objects (vTraceStoreKernelObjectName)
 * - User event channels (xTraceRegisterString)
 *
 * This variable should be zero. If not, it shows the number of missing slots so
 * far. In that case, increment SYMBOL_TABLE_SLOTS with (at least) this value.
 ******************************************************************************/
//volatile unsigned int  NoRoomForSymbol = 0;

/*******************************************************************************
 * NoRoomForObjectData
 *
 * Incremented on prvTraceSaveObjectData if no room for saving the object data,
 * i.e., the base priorities of tasks. There must be one slot for each task.
 * If not, this variable will show the difference.
 *
 * This variable should be zero. If not, it shows the number of missing slots so
 * far. In that case, increment OBJECT_DATA_SLOTS with (at least) this value.
 ******************************************************************************/
//volatile unsigned int  NoRoomForObjectData = 0;

/*******************************************************************************
 * LongestSymbolName
 *
 * Updated in prvTraceSaveSymbol. Should not exceed TRC_CFG_SYMBOL_MAX_LENGTH, 
 * otherwise symbol names will be truncated. In that case, set 
 * TRC_CFG_SYMBOL_MAX_LENGTH to (at least) this value.
 ******************************************************************************/
//volatile unsigned int  LongestSymbolName = 0;

/*******************************************************************************
 * MaxBytesTruncated
 *
 * Set in prvTraceStoreStringEvent if the total data payload exceeds 60 bytes,
 * including data arguments and the string. For user events, that is 52 bytes
 * for string and data arguments. In that is exceeded, the event is  truncated
 * (usually only the string, unless more than 15 parameters) and this variable
 * holds the maximum number of truncated bytes, from any event.
 ******************************************************************************/
volatile unsigned int  MaxBytesTruncated = 0;

//uint16_t CurrentFilterMask = 0xFFFe;

//uint16_t CurrentFilterGroup = FilterGroup0;
volatile int trace_cnt = 0; 



/* Internal common function for storing string events */
static void prvTraceStoreStringEventHelper(	int nArgs,
                                        uint16_t eventID,
                                        char * userEvtChannel,
                                        const char* str,
                                        va_list* vl);

static void prvTraceStoreNvalue(    int nArgs,
                                        uint16_t eventID,
                                        uint8_t submoduleId,
                                        const char* str,
                                        va_list* vl);

/* Not static to avoid warnings from SysGCC/PPC */ 
//void prvTraceStoreSimpleStringEventHelper(traceString userEvtChannel,
//                                                const char* str);

                                        
/* Stores the header information on Start */
//static void prvTraceStoreHeader(void);

/* Stores the symbol table on Start */
//static void prvTraceStoreSymbolTable(void);

/* Stores the object table on Start */
//static void prvTraceStoreObjectDataTable(void);

/* Store the Timestamp Config on Start */
//static void prvTraceStoreTSConfig(void);

/* Store the current warnings */
//static void prvTraceStoreWarnings(void);

/* Internal function for starting/stopping the recorder. */
static void prvSetRecorderEnabled(unsigned int  isEnabled);

/* Mark the page read as complete. */
//static void prvPageReadComplete(int pageIndex);

/* Retrieve a buffer page to write to. */
//static int prvAllocateBufferPage(int prevPage);

/* Get the current buffer page index (return value) and the number 
of valid bytes in the buffer page (bytesUsed). */
static int prvGetBufferPage(int32_t* bytesUsed);

/* Performs timestamping using definitions in trcHardwarePort.h */
static unsigned int  prvGetTimestamp32(void);

/* Signal an error. */
//void prvTraceError(int errCode);

/* Signal an warning (does not stop the recorder). */
//void prvTraceWarning(int errCode);

//#ifndef _LINUX_TRACE_
//extern void vTaskSuspendAll( void );
//extern BaseType_t xTaskResumeAll( void );
//#endif


void vNewMsgPrintF( unsigned char submoduleId, unsigned short msg_id,unsigned char msg_len,unsigned char* msg_buf);

/******************************************************************************
 * vTraceInstanceFinishedNow
 *
 * Creates an event that ends the current task instance at this very instant.
 * This makes the viewer to splits the current fragment at this point and begin
 * a new actor instance, even if no task-switch has occurred.
 *****************************************************************************/
//void vTraceInstanceFinishedNow(void)
//{
//    if(submodule_active_flag[7] & (1 << 30))
//        prvTraceStoreEvent0(PSF_EVENT_IFE_DIRECT);
//}

/******************************************************************************
 * vTraceInstanceFinishedNext
 *
 * Marks the current "task instance" as finished on the next kernel call.
 *
 * If that kernel call is blocking, the instance ends after the blocking event
 * and the corresponding return event is then the start of the next instance.
 * If the kernel call is not blocking, the viewer instead splits the current
 * fragment right before the kernel call, which makes this call the first event
 * of the next instance.
 *****************************************************************************/
//void vTraceInstanceFinishedNext(void)
//{
//    if(submodule_active_flag[7] & (1 << 30))
//        prvTraceStoreEvent0(PSF_EVENT_IFE_NEXT);
//}

/*******************************************************************************
 * vTraceStoreKernelObjectName
 *
 * Parameter object: pointer to the Event Group that shall be named
 * Parameter name: the name to set (const string literal)
 *
 * Sets a name for a kernel object for display in Tracealyzer.
 ******************************************************************************/
//void vTraceStoreKernelObjectName(void* object, const char* name)
//{
//    /* Always save in symbol table, if the recording has not yet started */
//    //prvTraceSaveSymbol(object, name);
//
//    prvTraceStoreStringEvent(1, PSF_EVENT_OBJ_NAME, name, (unsigned int )object);
//}


/******************************************************************************
* vTraceSetFrequency
*
* Registers the clock rate of the time source for the event timestamping.
* This is normally not required, but if the default value (TRC_HWTC_FREQ_HZ)
* should be incorrect for your setup, you can override it using this function.
*
* Must be called prior to vTraceEnable, and the time source is assumed to
* have a fixed clock frequency after the startup.
*****************************************************************************/
//void vTraceSetFrequency(unsigned int  frequency)
//{
//    timestampFrequency = frequency;
//}

//#if (TRC_CFG_SCHEDULING_ONLY == 0) && (TRC_CFG_INCLUDE_USER_EVENTS == 1)

/*******************************************************************************
* xTraceRegisterString
*
* Stores a name for a user event channel, returns the handle.
******************************************************************************/
//traceString xTraceRegisterString(const char* name)
//{
//    //prvTraceSaveSymbol((const void*)name, name);
//
//    /* Always save in symbol table, if the recording has not yet started */
//    prvTraceStoreStringEvent(1, PSF_EVENT_OBJ_NAME, (const char*)name, (unsigned int )name);
//
//    return (traceString)name;
//}

/******************************************************************************
 * vTracePrint
 *
 * Generates "User Events", with unformatted text.
 *
 * User Events can be used for very efficient application logging, and are shown
 * as yellow labels in the main trace view.
 *
 * You may group User Events into User Event Channels. The yellow User Event 
 * labels shows the logged string, preceded by the channel  name within 
 * brackets. For example:
 *
 *  "[MyChannel] Hello World!"
 *
 * The User Event Channels are shown in the View Filter, which makes it easy to
 * select what User Events you wish to display. User Event Channels are created
 * using xTraceRegisterString().
 *
 * Example:
 *
 *	 traceString chn = xTraceRegisterString("MyChannel");
 *	 ...
 *	 vTracePrint(chn, "Hello World!");
 *
 ******************************************************************************/
//void vTracePrint(traceString chn, const char* str)
//{
//    prvTraceStoreSimpleStringEventHelper(chn, str);
//}

/******************************************************************************
 * vTracePrintF
 *
 * Generates "User Events", with formatted text and data, similar to a "printf".
 * It is very fast since the actual formatting is done on the host side when the
 * trace is displayed.
 *
 * User Events can be used for very efficient application logging, and are shown
 * as yellow labels in the main trace view.
 * An advantage of User Events is that data can be plotted in the "User Event
 * Signal Plot" view, visualizing any data you log as User Events, discrete
 * states or control system signals (e.g. system inputs or outputs).
 *
 * You may group User Events into User Event Channels. The yellow User Event 
 * labels show the logged string, preceded by the channel name within brackets.
 * 
 * Example:
 *
 *  "[MyChannel] Hello World!"
 *
 * The User Event Channels are shown in the View Filter, which makes it easy to
 * select what User Events you wish to display. User Event Channels are created
 * using xTraceRegisterString().
 *
 * Example:
 *
 *	 traceString adc_uechannel = xTraceRegisterString("ADC User Events");
 *	 ...
 *	 vTracePrintF(adc_uechannel,
 *				 "ADC channel %d: %d volts",
 *				 ch, adc_reading);
 *
 * All data arguments are assumed to be 32 bit wide. The following formats are
 * supported:
 * %d - signed integer. The following width and padding format is supported: "%05d" -> "-0042" and "%5d" -> "  -42"
 * %u - unsigned integer. The following width and padding format is supported: "%05u" -> "00042" and "%5u" -> "   42"
 * %X - hexadecimal (uppercase). The following width and padding format is supported: "%04X" -> "002A" and "%4X" -> "  2A"
 * %x - hexadecimal (lowercase). The following width and padding format is supported: "%04x" -> "002a" and "%4x" -> "  2a"
 * %s - string (currently, this must be an earlier stored symbol name)
 *
 * Up to 15 data arguments are allowed, with a total size of maximum 60 byte
 * including 8 byte for the base event fields and the format string. So with
 * one data argument, the maximum string length is 48 chars. If this is exceeded
 * the string is truncated (4 bytes at a time).
 *
 ******************************************************************************/
//void vTracePrintF(traceString chn, const char* fmt, ...)
//{
//    va_list vl;
//    int i = 0;
//
//    int nArgs = 0;
//
//    /* Count the number of arguments in the format string (e.g., %d) */
//    for (i = 0; (fmt[i] != 0) && (i < 52); i++)
//    {
//        if (fmt[i] == '%')
//        {
//            if (fmt[i + 1] != '%')
//            {
//                nArgs++;        /* Found an argument */
//            }
//            
//            i++;      /* Move past format specifier or non-argument '%' */
//        }
//    }
//
//    va_start(vl, fmt);
//    
//    if (chn != NULL)
//    {
//        prvTraceStoreStringEventHelper(nArgs, (uint16_t)(PSF_EVENT_USER_EVENT + nArgs + 1), chn, fmt, &vl);
//    }
//    else
//    {
//        prvTraceStoreStringEventHelper(nArgs, (uint16_t)(PSF_EVENT_USER_EVENT + nArgs), chn, fmt, &vl);
//    }
//    va_end(vl);
//}
//#endif /* (TRC_CFG_SCHEDULING_ONLY == 0) && (TRC_CFG_INCLUDE_USER_EVENTS == 1) */

void vStringPrintF(uint8_t submoduleId,uint8_t nPara,const char* fmt, va_list *vl)
{
    prvTraceStoreStringEvent(1, PSF_EVENT_OBJ_NAME, fmt, (unsigned int )fmt);
    prvTraceStoreNvalue(nPara, (uint16_t)(PSF_EVENT_USER_EVENT_2 + nPara), submoduleId, fmt, vl);
    //prvTraceStoreStringEventHelper(nPara, (uint16_t)(PSF_EVENT_USER_EVENT_2 + nPara), chn, fmt, vl);
    return;
    
} 
void vNewStringPrintf( uint8_t submoduleId,uint8_t nPara,const char* fmt, va_list *vl)
{
    int fmt_len = strlen(fmt);
    vNewMsgPrintF(254,254,fmt_len,(unsigned char*)fmt);  //submodule must be 254 0xfe, cmd_module
    
    //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    //vNewMsgPrintF(submoduleId,254,fmt_len,(unsigned char*)fmt);
    prvTraceStoreNvalue(nPara, (uint16_t)(PSF_EVENT_USER_EVENT_2 + nPara), submoduleId, fmt, vl);
    
    //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    return;
    
}

/**
 * @brief 
 * @param chn
 * @param submoduleId
 * @param logId
 */

void vStatePrintF( uint8_t submoduleId, uint8_t logId,uint8_t nPara,va_list *vl)
{

    int i;
    //va_list vl;
    //TRACE_ALLOC_CRITICAL_SECTION();
    //TRACE_ENTER_CRITICAL_SECTION();
   // trace_lock();
    trace_cnt +=1;

    if (RecorderEnabled)
    {
        int eventSize = (int)sizeof(BaseEvent) + nPara * (int)sizeof(unsigned int ) + 4;
        eventCounter++;

        {
            TRC_STREAM_PORT_ALLOCATE_DYNAMIC_EVENT(stateEventType, event, eventSize);
            if (event != NULL)
            {
                unsigned int * data32;

                event->base.Count = eventCounter;

                event->base.EventID 	= PSF_EVENT_STATE_EVENT;

                event->base.Flag 	= TRACE_DATA_FLAG;//nPara ; //temp;
                    
                event->base.TS 			= prvGetTimestamp32();
                event->submoduleID 		= submoduleId;
                event->logID 			= logId;
                event->para_num 		= nPara;

                /* 32-bit write-pointer for the data argument */
                data32 = (unsigned int *) &(event->data[0]);
                //va_start(vl, nPara);
                for (i = 0; i < nPara; i++)
                {
                    {
                        /* Add data arguments... */
                        data32[i] = va_arg(*vl, unsigned int );
                    }
                }
                
                event->base.TS =(event->base.TS & 0x00ffffff)| (trace_cnt << 28);
                TRC_STREAM_PORT_COMMIT_EVENT(event, (unsigned int )eventSize);
                TRC_STREAM_UPDATE_WRITE_PINT
            }
        }
    }
    trace_cnt -=1;
    //TRACE_EXIT_CRITICAL_SECTION();	
    //trace_unlock();

    return;	
}
/**
 * @brief 
 * @param chn
 * @param submoduleId
 * @param msg_id
 * @param msg_len
 * @param msg_buf
 */
void vNewMsgPrintF( unsigned char submoduleId, unsigned short msg_id,unsigned char msg_len,unsigned char* msg_buf)
{
    //int i;
    uint8_t* data8;
    //GET_MIN_VALUE(msg_len, sizeof(((msgEventType *)0)->data)/sizeof( ((msgEventType *)0)->data[0] ));
    if(msg_len >= 252)
    {
        msg_len = 252;
        //printf("parm is err msg_len = %d\n", msg_len);
    }
         
    //TRACE_ALLOC_CRITICAL_SECTION();
    //TRACE_ENTER_CRITICAL_SECTION();
   // trace_lock();
    //TRACE_PRINTF("vNewMsgPrintF in submoduleId %d, trace_cnt %d\n",submoduleId,trace_cnt);
    trace_cnt +=1;

    if (RecorderEnabled)
    {
        int eventSize = (int)sizeof(BaseEvent) + msg_len + 4 + 4;
        if(eventSize%4){
            eventSize+=(4-eventSize%4);
        }
        eventCounter++;
        {
            TRC_STREAM_PORT_ALLOCATE_DYNAMIC_EVENT(msgEventType, event, eventSize);
            if (event != NULL)
            {
                event->base.EventID 	= PSF_EVENT_MSG_EVENT;
                    
                event->base.Flag 	= TRACE_DATA_FLAG;//(msg_len + 4)  ; //(uint16_t)eventCounter;
                event->base.TS 			= prvGetTimestamp32();
                event->base.Count   = eventCounter;
                event->submoduleID 		= submoduleId;
                event->msg_id 			= msg_id;
                event->data_len			= msg_len + 4;
                //printf("msg_len---------%d\n",msg_len);
                /* 32-bit write-pointer for the data argument */
                data8 = (uint8_t*) &(event->data[0]);
                memcpy(data8,&msg_buf,4);
                memcpy(data8+4,msg_buf,msg_len);
                event->base.TS = (event->base.TS & 0x00ffffff) | (trace_cnt << 28);
                TRC_STREAM_PORT_COMMIT_EVENT(event, (unsigned int )eventSize);
                TRC_STREAM_UPDATE_WRITE_PINT
            }
        }
    }
    trace_cnt -=1;
    //TRACE_PRINTF("vNewMsgPrintF out submoduleId %d, trace_cnt %d\n",submoduleId,trace_cnt);
    //TRACE_EXIT_CRITICAL_SECTION();
    //trace_unlock();

    return;	
}
//void vMsgPrintF(uint8_t submoduleId, uint16_t msg_id,uint8_t msg_len,uint8_t* msg_buf)
//{
//    //int i;
//    uint8_t* data8;
//#ifndef _LINUX_TRACE_
//    //TRACE_ALLOC_CRITICAL_SECTION();
//    //TRACE_ENTER_CRITICAL_SECTION();
//    vTaskSuspendAll();
//    trace_cnt +=1;
//#else
//    pthread_mutex_lock(&mutex_lock);
//#endif
//    if (RecorderEnabled)
//    {
//        int eventSize = (int)sizeof(BaseEvent) + msg_len + 4;
//        if(eventSize%4){
//            eventSize+=(4-eventSize%4);
//        }
//        eventCounter++;
//        {
//            TRC_STREAM_PORT_ALLOCATE_DYNAMIC_EVENT(msgEventType, event, eventSize);
//            if (event != NULL)
//            {
//                event->base.EventID 	= PSF_EVENT_MSG_EVENT;
//                event->base.Flag 	= TRACE_DATA_FLAG;//msg_len ; //(uint16_t)eventCounter;
//                event->base.TS 			= prvGetTimestamp32();
//                event->base.Count  = eventCounter;
//                event->submoduleID 		= submoduleId;
//                event->msg_id 			= msg_id;
//                event->data_len			= msg_len;
//
//                /* 32-bit write-pointer for the data argument */
//                data8 = (uint8_t*) &(event->data[0]);
//                #define		GRR_PD 		0x11
//                #define		MAC_HEAD	64
//                if((msg_id >> 8) == GRR_PD)
//                {
//                    data8[0] = MAC_HEAD;
//                    memcpy(data8 + 1,msg_buf,msg_len);
//                }
//                else
//                {
//                    memcpy(data8,msg_buf,msg_len);
//                }
//                
///*              for (i = 0; i < nPara; i++)
//                {
//                    data8[i] = msg_buf[i];
//                }*/
//                event->base.TS = (event->base.TS & 0x00ffffff) | (trace_cnt << 28);
//                TRC_STREAM_PORT_COMMIT_EVENT(event, (unsigned int )eventSize);
//                
//            }
//        }
//    }
//#ifndef _LINUX_TRACE_
//trace_cnt -=1;
//    //TRACE_EXIT_CRITICAL_SECTION();
//    xTaskResumeAll();
//#else
//    pthread_mutex_unlock(&mutex_lock);
//#endif
//    return;	
//    
//}

//void vPriPrintf(uint8_t submoduleId,uint8_t src,uint8_t dst,unsigned char pri_len,unsigned char* pri_buf)
//{
//    //int i;
//    uint8_t* data8;
//#ifndef _LINUX_TRACE_
//    //TRACE_ALLOC_CRITICAL_SECTION();
//    //TRACE_ENTER_CRITICAL_SECTION();
//    vTaskSuspendAll();
//    trace_cnt +=1;
//#else
//    pthread_mutex_lock(&mutex_lock);
//#endif
//    if (RecorderEnabled)
//    {
//        int eventSize = (int)sizeof(BaseEvent) + pri_len + 6;
//        if(eventSize%4){
//            eventSize+=(4-eventSize%4);
//        }
//        eventCounter++;
//        {
//            TRC_STREAM_PORT_ALLOCATE_DYNAMIC_EVENT(msgEventType, event, eventSize);
//            if (event != NULL)
//            {
//                event->base.EventID 	= PSF_EVENT_MSG_EVENT;
//                event->base.Flag 	= TRACE_DATA_FLAG;//(pri_len + 2) ; //(uint16_t)eventCounter;
//                    
//                event->base.TS 			= prvGetTimestamp32();
//                event->base.Count  = eventCounter;
//                event->submoduleID 		= submoduleId;
//                event->msg_id 			= pri_len + 2;;
//                //event->data_len			= pri_len + 2;
//                event->data[0]			= src;
//                event->data[1]			= dst;
//                /* 32-bit write-pointer for the data argument */
//                data8 = (uint8_t*) &(event->data[0]);
//                //memcpy???
//                #define		GRR_PD 		0x11
//                #define		MAC_HEAD	64
//                memcpy(data8 + 2,pri_buf,pri_len);
//                event->base.TS = (event->base.TS & 0x00ffffff) | (trace_cnt << 28);
//                TRC_STREAM_PORT_COMMIT_EVENT(event, (unsigned int )eventSize);
//                
//            }
//        }
//    }
//#ifndef _LINUX_TRACE_
//trace_cnt -=1;
//    //TRACE_EXIT_CRITICAL_SECTION();
//    xTaskResumeAll();
//#else
//    pthread_mutex_unlock(&mutex_lock);
//#endif
//    return;
//}

/*******************************************************************************
 * xTraceSetISRProperties
 *
 * Stores a name and priority level for an Interrupt Service Routine, to allow
 * for better visualization. Returns a traceHandle used by vTraceStoreISRBegin. 
 *
 * Example:
 *	 #define PRIO_ISR_TIMER1 3 // the hardware priority of the interrupt
 *	 ...
 *	 traceHandle Timer1Handle = xTraceSetISRProperties("ISRTimer1", PRIO_ISR_TIMER1);
 *	 ...
 *	 void ISR_handler()
 *	 {
 *		 vTraceStoreISRBegin(Timer1Handle);
 *		 ...
 *		 vTraceStoreISREnd(0);
 *	 }
 *
 ******************************************************************************/
 #if 0
//traceHandle xTraceSetISRProperties(const char* name, uint8_t priority)
//{
//    /* Save object data in object data table */
//    prvTraceSaveObjectData((const void*)name, priority);
//        
//    /* Note: "name" is used both as a string argument, and the address as ID */
//    prvTraceStoreStringEvent(2, PSF_EVENT_DEFINE_ISR, name, name, priority);
//        
//    /* Always save in symbol table, if the recording has not yet started */
//    prvTraceSaveSymbol((const void*)name, name);
//    
//    return (traceHandle)name;
//}

/*******************************************************************************
 * vTraceStoreISRBegin
 *
 * Registers the beginning of an Interrupt Service Routine, using a traceHandle
 * provided by xTraceSetISRProperties.
 *
 * Example:
 *	 #define PRIO_ISR_TIMER1 3 // the hardware priority of the interrupt
 *	 ...
 *	 traceHandle Timer1Handle = xTraceSetISRProperties("ISRTimer1", PRIO_ISR_TIMER1);
 *	 ...
 *	 void ISR_handler()
 *	 {
 *		 vTraceStoreISRBegin(Timer1Handle);
 *		 ...
 *		 vTraceStoreISREnd(0);
 *	 }
 *
 ******************************************************************************/
//void vTraceStoreISRBegin(traceHandle handle)
//{
//#ifndef _LINUX_TRACE_
//    TRACE_ALLOC_CRITICAL_SECTION();
//    TRACE_ENTER_CRITICAL_SECTION();
//#else
//    pthread_mutex_lock(&mutex_lock);
//#endif
//
//    /* We are at the start of a possible ISR chain. 
//    No context switches should have been triggered now. */
//    if (ISR_stack_index == -1)
//        isPendingContextSwitch = 0; 
//    
//    if (ISR_stack_index < TRC_CFG_MAX_ISR_NESTING - 1)
//    {
//        ISR_stack_index++;
//        ISR_stack[ISR_stack_index] = (unsigned int )handle;
//#if (TRC_CFG_INCLUDE_ISR_TRACING == 1)
//        if(submodule_active_flag[7] & (1 << 30)) prvTraceStoreEvent1(PSF_EVENT_ISR_BEGIN, (unsigned int )handle);
//#endif
//#ifndef _LINUX_TRACE_
//        TRACE_EXIT_CRITICAL_SECTION();
//#else
//        pthread_mutex_unlock(&mutex_lock);
//#endif
//    }
//    else
//    {
//#ifndef _LINUX_TRACE_
//        TRACE_EXIT_CRITICAL_SECTION();
//#else
//        pthread_mutex_unlock(&mutex_lock);
//#endif
//        //prvTraceError(PSF_ERROR_ISR_NESTING_OVERFLOW);
//    }
//}

/*******************************************************************************
 * vTraceStoreISREnd
 *
 * Registers the end of an Interrupt Service Routine.
 *
 * The parameter pendingISR indicates if the interrupt has requested a
 * task-switch (= 1), e.g., by signaling a semaphore. Otherwise (= 0) the 
 * interrupt is assumed to return to the previous context.
 *
 * Example:
 *	 #define PRIO_OF_ISR_TIMER1 3 // the hardware priority of the interrupt
 *	 traceHandle traceHandleIsrTimer1 = 0; // The ID set by the recorder
 *	 ...
 *	 traceHandleIsrTimer1 = xTraceSetISRProperties("ISRTimer1", PRIO_OF_ISR_TIMER1);
 *	 ...
 *	 void ISR_handler()
 *	 {
 *		 vTraceStoreISRBegin(traceHandleIsrTimer1);
 *		 ...
 *		 vTraceStoreISREnd(0);
 *	 }
 *
 ******************************************************************************/
//void vTraceStoreISREnd(int isTaskSwitchRequired)
//{
//#ifndef _LINUX_TRACE_
//    TRACE_ALLOC_CRITICAL_SECTION();
//    TRACE_ENTER_CRITICAL_SECTION();
//#else
//    pthread_mutex_lock(&mutex_lock);
//#endif
//
//    /* Is there a pending task-switch? (perhaps from an earlier ISR) */
//    isPendingContextSwitch |= isTaskSwitchRequired;
//
//    if (ISR_stack_index > 0)
//    {
//        ISR_stack_index--;
//
//#if (TRC_CFG_INCLUDE_ISR_TRACING == 1)
//        /* Store return to interrupted ISR (if nested ISRs)*/
//        prvTraceStoreEvent1(PSF_EVENT_ISR_RESUME, (unsigned int )ISR_stack[ISR_stack_index]);
//#endif
//    }
//    else
//    {
//        ISR_stack_index--;
//        
//        /* Store return to interrupted task, if no context switch will occur in between. */
//        if ((isPendingContextSwitch == 0) || (prvTraceIsSchedulerSuspended()))
//        {
//#if (TRC_CFG_INCLUDE_ISR_TRACING == 1)
//            prvTraceStoreEvent1(PSF_EVENT_TS_RESUME, (unsigned int )TRACE_GET_CURRENT_TASK());
//#endif
//        }
//    }
//#ifndef _LINUX_TRACE_
//    TRACE_EXIT_CRITICAL_SECTION();
//#else
//    pthread_mutex_unlock(&mutex_lock);
//#endif
//}
#endif

#if 0 //jace
/*******************************************************************************
 * xTraceGetLastError
 *
 * Returns the last error or warning, as a string, or NULL if none.
 *****************************************************************************/
const char* xTraceGetLastError(void)
{
    /* Note: the error messages are short, in order to fit in a User Event.
    Instead, the users can read more in the below comments.*/
    
    switch (errorCode)
    {
    
    case PSF_WARNING_SYMBOL_TABLE_SLOTS:
        /* There was not enough symbol table slots for storing symbol names.
        The number of missing slots is counted by NoRoomForSymbol. Inspect this
        variable and increase TRC_CFG_SYMBOL_TABLE_SLOTS by at least that value. */

        return "Exceeded SYMBOL_TABLE_SLOTS (see xTraceGetLastError)";

    case PSF_WARNING_SYMBOL_MAX_LENGTH:
        /* A symbol name exceeded TRC_CFG_SYMBOL_MAX_LENGTH in length.
        Make sure the symbol names are at most TRC_CFG_SYMBOL_MAX_LENGTH,
        or inspect LongestSymbolName and increase TRC_CFG_SYMBOL_MAX_LENGTH
        to at least this value. */

        return "Exceeded SYMBOL_MAX_LENGTH (see xTraceGetLastError)";

    case PSF_WARNING_OBJECT_DATA_SLOTS:
        /* There was not enough symbol object table slots for storing object
        properties, such as task priorites. The number of missing slots is 
        counted by NoRoomForObjectData. Inspect this variable and increase 
        TRC_CFG_OBJECT_DATA_SLOTS by at least that value. */
        
        return "Exceeded OBJECT_DATA_SLOTS (see xTraceGetLastError)";

    case PSF_WARNING_STRING_TOO_LONG:
        /* Some string argument was longer than the maximum payload size
        and has been truncated by "MaxBytesTruncated" bytes.

        This may happen for the following functions:
        - vTracePrint
        - vTracePrintF
        - vTraceStoreKernelObjectName
        - xTraceRegisterString
        - vTraceSetISRProperties

        A PSF event may store maximum 60 bytes payload, including data
        arguments and string characters. For User Events, also the User
        Event Channel (4 bytes) must be squeezed in, if a channel is
        specified (can be NULL). */

        return "String too long (see xTraceGetLastError)";

    case PSF_WARNING_STREAM_PORT_READ:
        /* TRC_STREAM_PORT_READ_DATA is expected to return 0 when completed successfully.
        This means there is an error in the communication with host/Tracealyzer. */

        return "TRC_STREAM_PORT_READ_DATA returned error (!= 0).";

    case PSF_WARNING_STREAM_PORT_WRITE:
        /* TRC_STREAM_PORT_WRITE_DATA is expected to return 0 when completed successfully.
        This means there is an error in the communication with host/Tracealyzer. */

        return "TRC_STREAM_PORT_WRITE_DATA returned error (!= 0).";

    case PSF_ERROR_EVENT_CODE_TOO_LARGE:
        /* The highest allowed event code is 4095, anything higher is an unexpected error. 
        Please contact support@percepio.com for assistance.*/
        
        return "Invalid event code (see xTraceGetLastError)";
    
    case PSF_ERROR_ISR_NESTING_OVERFLOW:
        /* Nesting of ISR trace calls exceeded the limit (TRC_CFG_MAX_ISR_NESTING).
        If this is unlikely, make sure that you call vTraceStoreISRExit in the end 
        of all ISR handlers. Or increase TRC_CFG_MAX_ISR_NESTING. */

        return "Exceeded ISR nesting (see xTraceGetLastError)";

    case PSF_ERROR_DWT_NOT_SUPPORTED:
        /* On ARM Cortex-M only - failed to initialize DWT Cycle Counter since not supported by this chip.
        DWT timestamping is selected automatically for ART Cortex-M3, M4 and higher, based on the __CORTEX_M
        macro normally set by ARM's CMSIS library, since typically available. You can however select
        SysTick timestamping instead by defining adding "#define TRC_CFG_ARM_CM_USE_SYSTICK".*/

        return "DWT not supported (see xTraceGetLastError)";

    case PSF_ERROR_DWT_CYCCNT_NOT_SUPPORTED:
        /* On ARM Cortex-M only - failed to initialize DWT Cycle Counter since not supported by this chip.
        DWT timestamping is selected automatically for ART Cortex-M3, M4 and higher, based on the __CORTEX_M
        macro normally set by ARM's CMSIS library, since typically available. You can however select 
        SysTick timestamping instead by defining adding "#define TRC_CFG_ARM_CM_USE_SYSTICK".*/
        
        return "DWT_CYCCNT not supported (see xTraceGetLastError)";
    
    case PSF_ERROR_TZCTRLTASK_NOT_CREATED:
        /* vTraceEnable failed creating the trace control task (TzCtrl) - incorrect parameters (priority?)
        or insufficient heap size? */
        return "Could not create TzCtrl (see xTraceGetLastError)";
    
    }
    
    return NULL;
}
#endif
/*******************************************************************************
 * vTraceClearError
 *
 * Clears any errors.
 *****************************************************************************/
//void vTraceClearError(void)
//{
//    NoRoomForSymbol = 0;
//    LongestSymbolName = 0;
//    NoRoomForObjectData = 0;
//    MaxBytesTruncated = 0;
//    errorCode = PSF_ERROR_NONE;
//}

/*******************************************************************************
 * vTraceStop
 *
 * Stops the tracing.
 *****************************************************************************/
//void vTraceStop(void)
//{
//    prvSetRecorderEnabled(0);
//}

/*******************************************************************************
 * vTraceSetRecorderDataBuffer
 *
 * If custom allocation is used, this function must be called so the recorder
 * library knows where to save the trace data.
 ******************************************************************************/
#if 0//(TRC_CFG_RECORDER_BUFFER_ALLOCATION == TRC_RECORDER_BUFFER_ALLOCATION_CUSTOM)

extern char* _TzTraceData;

void vTraceSetRecorderDataBuffer(void* pRecorderData)
{
    _TzTraceData = pRecorderData;
}
#endif


/*******************************************************************************
* xTraceIsRecordingEnabled
* Returns true (1) if the recorder is enabled (i.e. is recording), otherwise 0.
******************************************************************************/
//int xTraceIsRecordingEnabled(void)
//{
//    return (int)RecorderEnabled;
//}

//void vTraceSetFilterMask(uint16_t filterMask)
//{
//    CurrentFilterMask = filterMask;
//}

//void vTraceSetFilterGroup(uint16_t filterGroup)
//{
//    CurrentFilterGroup = filterGroup;
//}


/******************************************************************************/
/*** INTERNAL FUNCTIONS *******************************************************/
/******************************************************************************/

/* Internal function for starting/stopping the recorder. */
static void prvSetRecorderEnabled(unsigned int  isEnabled)
{
//    void* currentTask;
   // TRACE_ALLOC_CRITICAL_SECTION();

    //currentTask = TRACE_GET_CURRENT_TASK();
   // TRACE_ENTER_CRITICAL_SECTION();


    RecorderEnabled = isEnabled;
//
//    if (currentTask == NULL)
//    {
//        currentTask = (void*)HANDLE_NO_TASK;
//    }

    if (RecorderEnabled)
    {
        TRC_STREAM_PORT_ON_TRACE_BEGIN();

        #if (TRC_STREAM_PORT_USE_INTERNAL_BUFFER == 1)
        //prvPagedEventBufferInit(_TzTraceData); //Modify by yinkz 20190917, bss optimize
        prvPagedEventBufferInit(NULL);
        #endif
        
        eventCounter = 0;
        //ISR_stack_index = -1;

//        if(submodule_active_flag[7] & (1 << 30))
        prvTraceStoreEvent3(PSF_EVENT_TRACE_START,
                                (unsigned int )TRACE_GET_OS_TICKS(),
                                (unsigned int )NULL,
                                SessionCounter++);
        //prvTraceStoreTSConfig();
        //prvTraceStoreWarnings();
    }
    else
    {
        TRC_STREAM_PORT_ON_TRACE_END();
    }
    
    //TRACE_PRINTF("prvSetRecorderEnabled end\n");
    
#ifndef _LINUX_TRACE_
    //TRACE_EXIT_CRITICAL_SECTION();
#else
    pthread_mutex_unlock(&mutex_lock);
#endif
}

/* Store the current warnings */
//static void prvTraceStoreWarnings(void)
//{
//
//}

///* Store an event with zero parameters (event ID only) */
//void prvTraceStoreEvent0(uint16_t eventID)
//{
//#ifndef _LINUX_TRACE_
//    //TRACE_ALLOC_CRITICAL_SECTION();
//#endif
//
//    PSF_ASSERT(eventID < 4096, PSF_ERROR_EVENT_CODE_TOO_LARGE);
//
//#ifndef _LINUX_TRACE_
//    //TRACE_ENTER_CRITICAL_SECTION();
//    vTaskSuspendAll();
//    trace_cnt +=1;
//#else
//    pthread_mutex_lock(&mutex_lock);
//#endif
//
//    if (RecorderEnabled)
//    {
//        eventCounter++;
//
//        {
//            TRC_STREAM_PORT_ALLOCATE_EVENT(BaseEvent, event, sizeof(BaseEvent));
//            if (event != NULL)
//            {
//                event->EventID = eventID | PARAM_COUNT(0);
//                event->Flag = 0  ;//(uint16_t)eventCounter;
//                event->TS = prvGetTimestamp32();
//                event->Count  = eventCounter;
//                event->TS = (event->TS & 0x00ffffff) | (trace_cnt << 28);
//                TRC_STREAM_PORT_COMMIT_EVENT(event, sizeof(BaseEvent));
//            }
//            TRC_STREAM_UPDATE_WRITE_PINT
//        }
//    }
//#ifndef _LINUX_TRACE_
//    //TRACE_EXIT_CRITICAL_SECTION();
//    trace_cnt -=1;
//    xTaskResumeAll();
//#else
//    pthread_mutex_unlock(&mutex_lock);
//#endif
//}

/* Store an event with one 32-bit parameter (pointer address or an int) */

//void prvTraceStoreEvent1(uint16_t eventID, unsigned int  param1)
//{
//
//    PSF_ASSERT(eventID < 4096, PSF_ERROR_EVENT_CODE_TOO_LARGE);
//    vTaskSuspendAll();
//    trace_cnt +=1;
//
//
//    if (RecorderEnabled)
//    {
//        eventCounter++;
//        
//        {
//            TRC_STREAM_PORT_ALLOCATE_EVENT(EventWithParam_1, event, sizeof(EventWithParam_1));
//            if (event != NULL)
//            {
//                event->base.EventID = eventID | PARAM_COUNT(1); //  0x0100 | (1 << 12) = 0x0900
//                event->base.Flag = TRACE_DATA_FLAG;//1; //(uint16_t)eventCounter;
//
//                event->base.TS = prvGetTimestamp32();
//                event->base.Count  = eventCounter;
//                event->param1 = (unsigned int )param1;
//                event->base.TS = (event->base.TS & 0x00ffffff) | (trace_cnt << 28);
//                TRC_STREAM_PORT_COMMIT_EVENT(event, sizeof(EventWithParam_1));
//                TRC_STREAM_UPDATE_WRITE_PINT
//                
//            }
//            
//        }
//    }
//
//    trace_cnt -=1;
//    xTaskResumeAll();
//
//}

///* Store an event with two 32-bit parameters */
//
//void prvTraceStoreEvent2(uint16_t eventID, unsigned int  param1, unsigned int  param2)
//{
//
//    PSF_ASSERT(eventID < 4096, PSF_ERROR_EVENT_CODE_TOO_LARGE);
//
//    vTaskSuspendAll();
//    trace_cnt +=1;
//
//
//    if (RecorderEnabled)
//    {
//        eventCounter++;
//
//        {
//            TRC_STREAM_PORT_ALLOCATE_EVENT(EventWithParam_2, event, sizeof(EventWithParam_2));
//            if (event != NULL)
//            {
//                event->base.EventID = eventID | PARAM_COUNT(2);
//                event->base.Flag = TRACE_DATA_FLAG;//2 ; //(uint16_t)eventCounter;
//                    
//                event->base.TS = prvGetTimestamp32();
//                event->base.Count  = eventCounter;
//                event->param1 = (unsigned int )param1;
//                event->param2 = param2;
//                event->base.TS = (event->base.TS & 0x00ffffff) | (trace_cnt << 28);
//                TRC_STREAM_PORT_COMMIT_EVENT(event, sizeof(EventWithParam_2));
//                
//                TRC_STREAM_UPDATE_WRITE_PINT
//            }
//            
//        }
//    }
//
//    trace_cnt -=1;
//
//    xTaskResumeAll();
//
//}

/* Store an event with three 32-bit parameters */

void prvTraceStoreEvent3(uint16_t eventID,
                        unsigned int  param1,
                        unsigned int  param2,
                        unsigned int  param3)
{
    PSF_ASSERT(eventID < 4096, PSF_ERROR_EVENT_CODE_TOO_LARGE);

   //trace_lock();
    trace_cnt += 1;

    if (RecorderEnabled)
    {
        eventCounter++;

        {
            TRC_STREAM_PORT_ALLOCATE_EVENT(EventWithParam_3, event, sizeof(EventWithParam_3));
            if (event != NULL)
            {
                event->base.EventID = eventID | PARAM_COUNT(3);
                event->base.Flag = TRACE_DATA_FLAG;//3 ;//(uint16_t)eventCounter;

                event->base.TS = prvGetTimestamp32();
                event->base.Count  = eventCounter;
                event->param1 = (unsigned int )param1;
                event->param2 = param2;
                event->param3 = param3;
                event->base.TS = (event->base.TS & 0x00ffffff) | (trace_cnt << 28);
                TRC_STREAM_PORT_COMMIT_EVENT(event, sizeof(EventWithParam_3));
                TRC_STREAM_UPDATE_WRITE_PINT
            }
        }
    }

    trace_cnt -=1;

   // trace_unlock();
}

/* Stores an event with <nParam> 32-bit integer parameters */
void prvTraceStoreEvent(int nParam, uint16_t eventID, ...)
{
    va_list vl;
    int i;

    PSF_ASSERT(eventID < 4096, PSF_ERROR_EVENT_CODE_TOO_LARGE);


   // trace_lock();
    trace_cnt +=1;

    if (RecorderEnabled)
    {
        int eventSize = (int)sizeof(BaseEvent) + nParam * (int)sizeof(unsigned int );

        eventCounter++;

        {
            TRC_STREAM_PORT_ALLOCATE_DYNAMIC_EVENT(largestEventType, event, eventSize);
            if (event != NULL)
            {
                event->base.EventID = eventID | (uint16_t)PARAM_COUNT(nParam);
                event->base.Flag = TRACE_DATA_FLAG;

                event->base.TS = prvGetTimestamp32();
                event->base.Count  = eventCounter;

                va_start(vl, eventID);
                for (i = 0; i < nParam; i++)
                {
                    unsigned int * tmp = (unsigned int *) &(event->data[i]);   //tmp去向
                    *tmp = va_arg(vl, unsigned int );      
                }
                va_end(vl);
                event->base.TS = (event->base.TS & 0x00ffffff) | (trace_cnt << 28);
                TRC_STREAM_PORT_COMMIT_EVENT(event, (unsigned int )eventSize);
                TRC_STREAM_UPDATE_WRITE_PINT
            }
        }
    }

    trace_cnt -=1;

   // trace_unlock();

}

/* Stories an event with a string and <nParam> 32-bit integer parameters */
void prvTraceStoreStringEvent(int nArgs, uint16_t eventID, const char* str, ...)
{
    va_list vl;
    va_start(vl, str);
    prvTraceStoreStringEventHelper(nArgs, eventID, NULL, str, &vl);
    va_end(vl);
}

/* Internal common function for storing string events */
static void prvTraceStoreStringEventHelper(	int nArgs,
                                        uint16_t eventID,
                                        char * userEvtChannel,
                                        const char* str, va_list* vl)
{
    int len;
    int nWords;
    int nStrWords;
    int i;
    int offset = 0;

    PSF_ASSERT(eventID < 4096, PSF_ERROR_EVENT_CODE_TOO_LARGE);

    for (len = 0; (str[len] != 0) && (len < 52); len++); /* empty loop */
    
    /* The string length in multiples of 32 bit words (+1 for null character) */
    nStrWords = (len+1+3)/4;

    /* If a user event channel is specified, add in the list */
    if (userEvtChannel)
        nArgs++;

    offset = nArgs * 4;

    /* The total number of 32-bit words needed for the whole payload */
    nWords = nStrWords + nArgs;

    if (nWords > 15) /* if attempting to store more than 60 byte (= max) */
    {
        /* Truncate event if too large. The	string characters are stored
        last, so usually only the string is truncated, unless there a lot
        of parameters... */

        /* Diagnostics ... */
        unsigned int  bytesTruncated = (unsigned int )(nWords - 15) * 4;

        if (bytesTruncated > MaxBytesTruncated)
        {
            MaxBytesTruncated = bytesTruncated;
        }

        nWords = 15;
        len = 15 * 4 - offset;
    }

    //TRACE_ENTER_CRITICAL_SECTION();
    //trace_lock();
    trace_cnt +=1;


    if (RecorderEnabled)
    {
        int eventSize = (int)sizeof(BaseEvent) + nWords * (int)sizeof(unsigned int );
        eventCounter++;
        {

            TRC_STREAM_PORT_ALLOCATE_DYNAMIC_EVENT(largestEventType, event, eventSize);
            if (event != NULL)
            {
                unsigned int * data32;
                uint8_t* data8;
                event->base.EventID = (eventID) | (uint16_t)PARAM_COUNT(nWords);
                    
                event->base.Flag = TRACE_DATA_FLAG;//nWords;//(uint16_t)eventCounter;
                event->base.TS = prvGetTimestamp32();
                event->base.Count  = eventCounter;
                /* 32-bit write-pointer for the data argument */
                data32 = (unsigned int *) &(event->data[0]);

                for (i = 0; i < nArgs; i++)
                {
                    if ((userEvtChannel != NULL) && (i == 0))
                    {
                        /* First, add the User Event Channel if not NULL */
                        data32[i] = (unsigned int )userEvtChannel;
                    }
                    else
                    {
                        /* Add data arguments... */
                        data32[i] = va_arg(*vl, unsigned int );
                    }
                }
                data8 = (uint8_t*)&(event->data[0]);
                for (i = 0; i < len; i++)
                {
                    data8[offset + i] = str[i];
                }

                if (len < (15 * 4 - offset))
                    data8[offset + len] = 0;	/* Only truncate if we don't fill up the buffer completely */
                event->base.TS = (event->base.TS & 0x00ffffff)| (trace_cnt << 28);
                TRC_STREAM_PORT_COMMIT_EVENT(event, (unsigned int )eventSize);
                TRC_STREAM_UPDATE_WRITE_PINT
                
            }
        }
    }
    trace_cnt -=1;
    //TRACE_EXIT_CRITICAL_SECTION();
    //trace_unlock();

}


static void prvTraceStoreNvalue(int nArgs,uint16_t eventID,uint8_t submoduleId,const char* str,va_list* vl)
{
    //int len;
    int nWords;
    int i;
    int cnt = 0;
    char * substr;
    va_list vlmv=*vl;

    if(str[0]!=0)
    {
        for(i=1;i<60;i++)
        {

            if(str[i-1]=='%' )
            {
                cnt++;
                if(cnt>nArgs || str[i]=='0')
                break;
                substr = (char *)va_arg(vlmv, unsigned int );
                if(str[i]=='s')
                {
                    eventCounter--;//Head format has been counted,cut off the added cout here.
                    prvTraceStoreStringEvent(1, PSF_EVENT_OBJ_NAME, substr, (unsigned int )substr);
                }
            }
        }
    }  

    PSF_ASSERT(eventID < 4096, PSF_ERROR_EVENT_CODE_TOO_LARGE);

    /* If a user event channel is specified, add in the list */
    nArgs++;
    nArgs++;

    /* The total number of 32-bit words needed for the whole payload */
    nWords =  nArgs;

    if (nWords > 15) /* if attempting to store more than 60 byte (= max) */
    {
        nWords = 15;
    }

    //GET_MIN_VALUE(nArgs, (sizeof(((largestEventType *)0)->data)/sizeof( ((largestEventType *)0)->data[0] ))/4);
    if(nArgs > 15)
    {
        //TRACE_PRINTF("parm is err nArgs = %d\n", nArgs);
        nArgs = 15;
    }

    //TRACE_ENTER_CRITICAL_SECTION();
    //trace_lock();
    //TRACE_PRINTF("prvTraceStoreNvalue in submoduleId %d, trace_cnt %d\n",submoduleId,trace_cnt);
    trace_cnt +=1;


    if (RecorderEnabled)
    {
        int eventSize = (int)sizeof(BaseEvent) + nWords * (int)sizeof(unsigned int );
        //eventCounter++;
        {

            TRC_STREAM_PORT_ALLOCATE_DYNAMIC_EVENT(largestEventType, event, eventSize);
            if (event != NULL)
            {
                unsigned int * data32;
                
                event->base.EventID = (eventID) | (uint16_t)PARAM_COUNT(nWords);
                    
                event->base.Flag = TRACE_DATA_FLAG;//nWords ;//(uint16_t)eventCounter;
                event->base.TS = prvGetTimestamp32();
                event->base.Count  = eventCounter;
                /* 32-bit write-pointer for the data argument */
                data32 = (unsigned int *) &(event->data[0]);
                data32[0]=(unsigned int )str;
                for (i = 1; i < nArgs; i++)
                {
                    if (i == 1)
                    {
                        /* First, add the User Event Channel if not NULL */
                        data32[i] = (unsigned int )submoduleId;
                    }
                    else
                    {
                        /* Add data arguments... */
                        data32[i] = va_arg(*vl, unsigned int );
                    }
                }
                
                event->base.TS = (event->base.TS & 0x00ffffff) | (trace_cnt << 28);
                TRC_STREAM_PORT_COMMIT_EVENT(event, (unsigned int )eventSize);
                TRC_STREAM_UPDATE_WRITE_PINT
            }
        }
    }
    trace_cnt -=1;
    //TRACE_PRINTF("prvTraceStoreNvalue out submoduleId %d, trace_cnt %d\n",submoduleId,trace_cnt);
    //TRACE_EXIT_CRITICAL_SECTION();
    //trace_unlock();

}



/* Checks if the provided command is a valid command */
//int prvIsValidCommand(TracealyzerCommandType* cmd)
//{
    /*uint16_t checksum = (uint16_t)(0xFFFF - (	cmd->cmdCode +
                                                cmd->param1 +
                                                cmd->param2 +
                                                cmd->param3 +
                                                cmd->param4 +
                                                cmd->param5));
                                                */
//	if (cmd->checksumMSB != (unsigned char)(checksum >> 8))
//		return 0;
//
//	if (cmd->checksumLSB != (unsigned char)(checksum & 0xFF))
//		return 0;
//
//	if (cmd->cmdCode > CMD_LAST_COMMAND)
//		return 0;

//    return 1;
//}

/* Executed the received command (Start or Stop) */
void prvProcessCommand(TracealyzerCommandType* cmd)
{
    //rt_kprintf("prvProcessCommand cmd_type %d cmd_data %x %x %x %x %x %x %x %x\n",cmd->cmd_type,
    //    cmd->cmd_data[0],cmd->cmd_data[1],cmd->cmd_data[2],cmd->cmd_data[3],
    //    cmd->cmd_data[4],cmd->cmd_data[5],cmd->cmd_data[6],cmd->cmd_data[7]);
    
    switch(cmd->cmd_type)
    {
        case SET_TRACE_ACTIVE:
            prvSetRecorderEnabled(cmd->cmd_data[0]);
            break;

        case SET_ACTIVE_LOG:
            proc_act_log_list((unsigned char*)cmd->cmd_data);
            proc_act_all_log();
            break;
            
        case SET_CMD_TEST://4
            break;            

        default:
            break;
    }
}



/* If using DWT timestamping (default on ARM Cortex-M3, M4 and M7), make sure the DWT unit is initialized. */
#ifndef TRC_CFG_ARM_CM_USE_SYSTICK
#if ((TRC_CFG_HARDWARE_PORT == TRC_HARDWARE_PORT_ARM_Cortex_M) && (defined (__CORTEX_M) && (__CORTEX_M >= 0x03)))

void prvTraceInitCortexM()
{
    /* Make sure the DWT registers are unlocked, in case the debugger doesn't do this. */
    TRC_REG_ITM_LOCKACCESS = TRC_ITM_LOCKACCESS_UNLOCK;

    /* Make sure DWT is enabled is enabled, if supported */
    TRC_REG_DEMCR |= TRC_DEMCR_TRCENA;

    do
    {
        /* Verify that DWT is supported */
        if (TRC_REG_DEMCR == 0)
        {
            /* This function is called on Cortex-M3, M4 and M7 devices to initialize
            the DWT unit, assumed present. The DWT cycle counter is used for timestamping. 
            
            If the below error is produced, the DWT unit does not seem to be available.
            
            In that case, define the macro TRC_CFG_ARM_CM_USE_SYSTICK in your build
            to use SysTick timestamping instead, or define your own timestamping by 
            setting TRC_CFG_HARDWARE_PORT to TRC_HARDWARE_PORT_APPLICATION_DEFINED
            and make the necessary definitions, as explained in trcHardwarePort.h.*/
            
            //prvTraceError(PSF_ERROR_DWT_NOT_SUPPORTED);
            break;
        }

        /* Verify that DWT_CYCCNT is supported */
        if (TRC_REG_DWT_CTRL & TRC_DWT_CTRL_NOCYCCNT)
        {
            /* This function is called on Cortex-M3, M4 and M7 devices to initialize
            the DWT unit, assumed present. The DWT cycle counter is used for timestamping. 
            
            If the below error is produced, the cycle counter does not seem to be available.
            
            In that case, define the macro TRC_CFG_ARM_CM_USE_SYSTICK in your build
            to use SysTick timestamping instead, or define your own timestamping by 
            setting TRC_CFG_HARDWARE_PORT to TRC_HARDWARE_PORT_APPLICATION_DEFINED
            and make the necessary definitions, as explained in trcHardwarePort.h.*/

            //prvTraceError(PSF_ERROR_DWT_CYCCNT_NOT_SUPPORTED);
            break;
        }

        /* Reset the cycle counter */
        TRC_REG_DWT_CYCCNT = 0;

        /* Enable the cycle counter */
        TRC_REG_DWT_CTRL |= TRC_DWT_CTRL_CYCCNTENA;

    } while(0);	/* breaks above jump here */
}
#endif
#endif

/* Performs timestamping using definitions in trcHardwarePort.h */
//TickType_t xTaskGetTickCountFromISR( void );
static unsigned int  prvGetTimestamp32(void)
{
#if ((TRC_HWTC_TYPE == TRC_FREE_RUNNING_32BIT_INCR) || (TRC_HWTC_TYPE == TRC_FREE_RUNNING_32BIT_DECR))
    return TRC_HWTC_COUNT;
#endif
    
#if ((TRC_HWTC_TYPE == TRC_CUSTOM_TIMER_INCR) || (TRC_HWTC_TYPE == TRC_CUSTOM_TIMER_DECR))
    return TRC_HWTC_COUNT;
#endif
    
#if ((TRC_HWTC_TYPE == TRC_OS_TIMER_INCR) || (TRC_HWTC_TYPE == TRC_OS_TIMER_DECR))
#ifndef _LINUX_TRACE_
    unsigned int  ticks = TRACE_GET_OS_TICKS();
    return (TRC_HWTC_COUNT & 0x00FFFFFFU) + ((ticks & 0x000000FFU) << 24);
#else
    return 0;
#endif
#endif
}

#if 0
/* Store the Timestamp Config event */
static void prvTraceStoreTSConfig(void)
{
    /* If not overridden using vTraceSetFrequency, use default value */
    if (timestampFrequency == 0)
    {
        timestampFrequency = TRC_HWTC_FREQ_HZ;
    }
    
    #if (TRC_HWTC_TYPE == TRC_CUSTOM_TIMER_INCR || TRC_HWTC_TYPE == TRC_CUSTOM_TIMER_DECR)
    
        prvTraceStoreEvent(5, 
                            PSF_EVENT_TS_CONFIG,
                            (unsigned int )timestampFrequency,	                    
                            (unsigned int )TRACE_TICK_RATE_HZ,
                            (unsigned int )TRC_HWTC_TYPE,
                            (unsigned int )TRC_CFG_ISR_TAILCHAINING_THRESHOLD,
                            (unsigned int )TRC_HWTC_PERIOD);
    
    #else
    
    prvTraceStoreEvent(4, 
                        PSF_EVENT_TS_CONFIG,
                        (unsigned int )timestampFrequency,	                    
                        (unsigned int )TRACE_TICK_RATE_HZ,
                        (unsigned int )TRC_HWTC_TYPE,
                        (unsigned int )TRC_CFG_ISR_TAILCHAINING_THRESHOLD);	
    #endif
}
#endif
/* Retrieve a buffer page to write to. */
//static int prvAllocateBufferPage(int prevPage)
//{
//        #if 0
//    int index;
//    int count = 0;
//
//    index = (prevPage + 1) % TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT;
//
//    while((PageInfo[index].Status != PAGE_STATUS_FREE) && (count ++ < TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT))
//    {
//        index = (index + 1) % TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT;
//    }
//
//    if (PageInfo[index].Status == PAGE_STATUS_FREE)
//    {
//        return index;
//    }
//        #else
//        //#define CHECK_PAGEINFO_STATE(prevPage) (PageInfo[!prevPage].Status == PAGE_STATUS_FREE)?(!prevPage):(PageInfo[prevPage].Status == PAGE_STATUS_FREE?prevPage:-1);
//        if (PageInfo[0].Status == PAGE_STATUS_FREE)
//        {
//            return 0;
//        }
//        else  if (PageInfo[1].Status == PAGE_STATUS_FREE)
//        {
//            return 1;
//        }
//        #endif
//
//    return -1;
//}

/* Mark the page read as complete. */
//static void prvPageReadComplete(int pageIndex)
//{
//#ifndef _LINUX_TRACE_
//    TRACE_ALLOC_CRITICAL_SECTION();
//    TRACE_ENTER_CRITICAL_SECTION();
//#else
//    pthread_mutex_lock(&mutex_lock);
//#endif
//
//    #if (REDIRECT_prvPagedEventBufferTransfer_INSPIKE==1)
//    //printf("b:%d\r\n",pageIndex);
//    while(PageInfo[pageIndex].Status ==PAGE_STATUS_READ)
//    {;}
//     //printf("a:%d\r\n",pageIndex);
//    #endif
//    
//	PageInfo[pageIndex].BytesRemaining = TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE;
//	PageInfo[pageIndex].WritePointer = &EventBuffer[pageIndex * TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE];
//	TotalBytesRemaining += TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE;
//	PageInfo[pageIndex].Status = PAGE_STATUS_FREE;
//    
//#ifndef _LINUX_TRACE_
//	TRACE_EXIT_CRITICAL_SECTION();
//#else
//    pthread_mutex_unlock(&mutex_lock);
//#endif
//}

/* Get the current buffer page index and remaining number of bytes. */
static int prvGetBufferPage(int32_t* bytesUsed)
{
/* jace change
    static int8_t lastPage = -1;
    int count = 0;
    int8_t index = (int8_t) ((lastPage + 1) % TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT);

    while((PageInfo[index].Status != PAGE_STATUS_READ) && (count++ < TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT))
    {
        index = (int8_t)((index + 1) % TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT);
    }

    if (PageInfo[index].Status == PAGE_STATUS_READ)
    {
        *bytesUsed = TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE - PageInfo[index].BytesRemaining;
        lastPage = index;
        return index;
    }
    *bytesUsed = 0;
*/
    return -1;
}

/*******************************************************************************
 * unsigned int  prvPagedEventBufferTransfer(void)
 *
 * Transfers one buffer page of trace data, if a full page is available, using
 * the macro TRC_STREAM_PORT_WRITE_DATA as defined in trcStreamingPort.h.
 *
 * This function is intended to be called the periodic TzCtrl task with a suitable
 * delay (e.g. 10-100 ms).
 *
 * Returns the number of bytes sent. If non-zero, it is good to call this 
 * again, in order to send any additional data waiting in the buffer.
 * If zero, wait a while before calling again.
 *
 * In case of errors from the streaming interface, it registers a warning
 * (PSF_WARNING_STREAM_PORT_WRITE) provided by xTraceGetLastError().
 *
 *******************************************************************************/
unsigned int  prvPagedEventBufferTransfer(void)
{
    int8_t pageToTransfer = -1;
    int32_t bytesTransferredTotal = 0;
    int32_t bytesTransferredNow = 0;
    int32_t bytesToTransfer;

    pageToTransfer = (int8_t)prvGetBufferPage(&bytesToTransfer);
    /* bytesToTransfer now contains the number of "valid" bytes in the buffer page, that should be transmitted.
    There might be some unused junk bytes in the end, that must be ignored. */
    
    if (pageToTransfer > -1)
    {
        while (1)  /* Keep going until we have transferred all that we intended to */
        {
            if (TRC_STREAM_PORT_WRITE_DATA(
                    &EventBuffer[pageToTransfer * TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE + bytesTransferredTotal],
                    (unsigned int )(bytesToTransfer - bytesTransferredTotal),
                    &bytesTransferredNow) == 0)
            {
                /* Write was successful. Update the number of transferred bytes. */
                bytesTransferredTotal += bytesTransferredNow;

                if (bytesTransferredTotal == bytesToTransfer)
                {
                    /* All bytes have been transferred. Mark the buffer page as "Read Complete" (so it can be written to) and return OK. */
                    //prvPageReadComplete(pageToTransfer);
                    return (unsigned int )bytesTransferredTotal;
                }
            }
            else
            {
                /* Some error from the streaming interface... */
                //prvTraceWarning(PSF_WARNING_STREAM_PORT_WRITE);
                return 0;
            }
        }
    }
    return 0;
}
/*******************************************************************************
 * void* prvPagedEventBufferGetWritePointer(int sizeOfEvent)
 *
 * Returns a pointer to an available location in the buffer able to store the
 * requested size.
 * 
 * Return value: The pointer.
 * 
 * Parameters:
 * - sizeOfEvent: The size of the event that is to be placed in the buffer.
 *
*******************************************************************************/
extern PageType CurentRingBuferAddress;

void* prvPagedEventBufferGetWritePointer(int sizeOfEvent)
{
    void *ret = NULL;
    unsigned int  *p=(unsigned int *)dump_flag;
    char *startAddress = (char *)(p[0]);
    char *endAddress = (char *)(p[1]);  
    
    unsigned int  checkStart = p[0];
    unsigned int  checkEnd = p[1];
    
    if (checkStart != 0x33b4c4 || checkEnd != 0x33bc90) {
        //TRACE_PRINTF("error0: start 0%x, end 0%x\n",checkStart,checkEnd);
//        p[0] = 0x33b4c4;
//        p[1] = 0x33bc90;
        return NULL;
    }
    
//    TRACE_PRINTF("prvPagedEventBufferGetWritePointer: sizeOfEvent %d\n",sizeOfEvent);
    
    //data too long
    if (sizeOfEvent >= TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE)
    {
        //TRACE_PRINTF("error1 \n");
        return NULL;
    }
    //update read address
    CurentRingBuferAddress.read = PageInfo->read;
    
//    TRACE_PRINTF("before: read %p, write %p\n",
//            CurentRingBuferAddress.read,CurentRingBuferAddress.write);
    
    // address dump
    if (CurentRingBuferAddress.read < startAddress 
        || CurentRingBuferAddress.write < startAddress 
        || CurentRingBuferAddress.read > endAddress 
        || CurentRingBuferAddress.write > endAddress 
        || CurentRingBuferAddress.write == NULL)
    {//repair address
        CurentRingBuferAddress.write = startAddress;
       // TRACE_PRINTF("error2: start %p, end %p\n",startAddress,endAddress);
//        TRACE_PRINTF("error2 \n");
        return NULL;
    }

    if (CurentRingBuferAddress.write >= CurentRingBuferAddress.read 
        && (endAddress - CurentRingBuferAddress.write) > sizeOfEvent)
    {
        ret = (void *)(CurentRingBuferAddress.write);
        CurentRingBuferAddress.write += sizeOfEvent;
    }
    else if (CurentRingBuferAddress.write >=  CurentRingBuferAddress.read 
         && (endAddress - CurentRingBuferAddress.write) < sizeOfEvent 
         && (CurentRingBuferAddress.read - startAddress) > sizeOfEvent) 
    {
        // first clear not used space
        memset((void *)(CurentRingBuferAddress.write),0,(endAddress - CurentRingBuferAddress.write));
        *rv_sizeHH = (unsigned int  )(CurentRingBuferAddress.write);
        //TRACE_PRINTF("prvPagedEventBufferGetWritePointer: rv_sizeHH 0x%x\n", *rv_sizeHH);
        ret = startAddress;
        CurentRingBuferAddress.write = startAddress + sizeOfEvent;
    }    
    else if (CurentRingBuferAddress.write < CurentRingBuferAddress.read 
          && (CurentRingBuferAddress.read - CurentRingBuferAddress.write) > sizeOfEvent)
    {
        ret = (void *)(CurentRingBuferAddress.write);
        CurentRingBuferAddress.write += sizeOfEvent;
    }
    else 
    {
//        TRACE_PRINTF("error3 \n");
//        TRACE_PRINTF("error3: start %p, end %p\n",startAddress,endAddress);
        return NULL;
    }    
    // clear space where will be used
    if (ret) {
        memset(ret, 0, sizeOfEvent);
    }
//    TRACE_PRINTF("return ret %p\n",ret);
    return ret;
}

void prvPagedEventBufferUpdate(void)
{
    PageInfo->write = CurentRingBuferAddress.write;
}

/*******************************************************************************
 * void prvPagedEventBufferInit(char* buffer)
 *
 * Assigns the buffer to use and initializes the PageInfo structure.
 *
 * Return value: void
 * 
 * Parameters:
 * - char* buffer: pointer to the trace data buffer, allocated by the caller.
 *
*******************************************************************************/
void prvPagedEventBufferInit(char* buffer)
{
//    int i;
//#ifndef _LINUX_TRACE_
   // TRACE_ALLOC_CRITICAL_SECTION();
//#endif

#if (REDIRECT_prvPagedEventBufferTransfer_INSPIKE==1)
    if(EventBuffer==NULL||PageInfo==NULL){
        //TRACE_PRINTF("error in prvPagedEventBufferInit\n");
    }
    
#else
    EventBuffer = buffer;
#endif
#ifndef _LINUX_TRACE_
  //  TRACE_ENTER_CRITICAL_SECTION();
#else
    pthread_mutex_lock(&mutex_lock);
#endif
//    for (i = 0; i < TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT; i++)
//    {
//        PageInfo[i].BytesRemaining = TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE;
//        PageInfo[i].WritePointer = &EventBuffer[i * TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE];
//        PageInfo[i].Status = PAGE_STATUS_FREE;
//    }
    //TotalBytesRemaining = TRC_CFG_PAGED_EVENT_BUFFER_PAGE_COUNT * TRC_CFG_PAGED_EVENT_BUFFER_PAGE_SIZE;
#ifndef _LINUX_TRACE_
    //TRACE_EXIT_CRITICAL_SECTION();
#else
    pthread_mutex_unlock(&mutex_lock);
#endif
}


//class str;
