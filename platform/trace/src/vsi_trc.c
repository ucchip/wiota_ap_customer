/*
+--------------------------------------------------------------------+
| PROJECT: GPF-Frame (8423)             $Workfile::   vsi_trc.c     $|
| Author:: Mp  The company                $Revision::   1.2           $|
| CREATED: 29.06.99                     $Modtime::   19 May 2003 11:$|
| STATE  : code                                                      |
+--------------------------------------------------------------------+

   MODULE  : VSI_TRC

   PURPOSE : This Module defines the virtual system interface part
             about traces.

*/
#include "stdarg.h"
#include "uctypes.h"
#include "trcStreamingPort.h"

extern void vNewStringPrintf( u8_t submoduleId,u8_t nPara,const char* fmt, va_list *vl);

extern void vStatePrintF(u8_t submoduleId, u8_t logId,u8_t nPara,va_list* vl);
extern void vStringPrintF( u8_t submoduleId,u8_t nPara,const n8_t* fmt, va_list *vl);

/**
 * @brief to store the log state,epsecially phy.
 * @param Caller
 * @param subModuleID
 * @param log_id
 * @param para_num
 */
void  vsi_o_tstate(u8_t subModuleID, u8_t log_id, u8_t para_num,...)
{
    va_list vl;
    va_start(vl,para_num);

    vStatePrintF(subModuleID,log_id,para_num,&vl);
    va_end(vl);
    return;
}



/**
 * @brief vsi_o_trace Wrapper to ptrace vTRACE_PRINTF functions.
 * @param Caller caller ID
 * @param subModuleID 
 * @param format
 * @return none.
 */

s16_t vsi_o_ttrace ( u8_t subModuleID, n8_t *format, ... )
{
    n8_t *fmt=format;
    u8_t nAgrs=0;
    s32_t n=0;

        
    for(n=0;n<60;n++)
    {
        if(format[n]==0)
            break;
        else
        {
            if('%'==format[n])
                nAgrs++;
        
        }
    }
    va_list varpars;
    va_start (varpars, format);
    vStringPrintF(subModuleID,nAgrs,fmt, &varpars);
    va_end (varpars);
    return 0;
}

s16_t vsi_o_tstring ( u8_t nAgrs,u8_t subModuleID, n8_t *format, ... )
{
    n8_t *fmt=format;
    va_list varpars;
    trace_lock();
    va_start (varpars, format);
    
    //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    //vStringPrintF(subModuleID,nAgrs,fmt, &varpars);
    //TRACE_PRINTF("vsi_o_tstring: nAgrs %d subModuleID %d\n",nAgrs,subModuleID);
    vNewStringPrintf(subModuleID,nAgrs,fmt, &varpars);
    va_end (varpars);
    trace_unlock();
    return 0;
}
//
//void vsi_o_right_now(){
//    //alloc_impossible_size();
//}



/*
+--------------------------------------------------------------------+
| PROJECT : GSM-Frame (8415)           MODULE  : VSI_TRC             |
| STATE   : code                       ROUTINE : vsi_o_#        |
+--------------------------------------------------------------------+

  PURPOSE : traces a primitive opc and direction

*/
//s16_t vsi_o_ptrace ( s16_t Caller, u8_t subModuleID, u8_t dir )
//{    
//#ifndef DISABLE_NEW_FRAME_TRACE
//static boolean TraceInitialized = FALSE;
//static n8_t ptrace_inBuffer [18];
//static n8_t ptrace_outBuffer[18];
//T_SYST_PRIM *Prim;
//ul32_t Status;
//n8_t opc_len;
//
//  if ( (Caller >= 0) && (TraceMask[Caller] & TC_PRIM) )
//  {
//    Status = os_AllocatePartition ( Caller, (ul32_t**)&Prim, TRACE_PRIM_SIZE,
//                                           SuspendTrace[Caller], TestGroupHandle );
//    if ( Status == OS_OK || Status == OS_WAITED )
//    {
//      Prim->Header.Fmt = TST_MSG_TRACE;
//      Prim->Header.Size = PTRACE_LEN;
//
//      if ( dir )
//        strcpy ( &Prim->Data,"$---OUT:$p0x123456789" );
//      else
//        strcpy ( &Prim->Data,"$--- IN:$p0x123456789" );
//
//      if ( OPC32BIT(opc) )
//      {
//        opc_len = CHARS_FOR_32BIT;
//        Prim->Header.Size = PTRACE_LEN_OPC32; 
//      }
//      else
//      {
//        opc_len = CHARS_FOR_16BIT;
//        Prim->Header.Size = PTRACE_LEN_OPC16; 
//      }
//      HexToASCII ( opc, &Prim->Data + 12, opc_len );
//      strcpy ((&Prim->Data + 12 + opc_len), "$" );
//
//      if ( vsi_o_trace ( Caller, Prim ) == VSI_OK )
//        return VSI_OK;
//    }
//    TracesAborted[Caller]++;
//  }
//  return VSI_ERROR;
//#else //DISABLE_NEW_FRAME_TRACE
//  return VSI_OK;
//#endif //DISABLE_NEW_FRAME_TRACE
//}

/*
+--------------------------------------------------------------------+
| PROJECT : GSM-Frame (8415)           MODULE  : VSI_TRC             |
| STATE   : code                       ROUTINE : vsi_o_strace        |
+--------------------------------------------------------------------+

  PURPOSE : traces a state and state transition

*/
//s16_t vsi_o_strace (s16_t Caller,  const n8_t *const machine,
//                                      const n8_t *const curstate,
//                                      const n8_t *const newstate)
//{
//   if (newstate)
//    UCDBG("SetState[M]%s[CS]%s[NS]%s\n", machine, curstate, newstate);
//   else 
//    UCDBG("GetState[M]%s[CS]%s\n", machine, curstate);
//   return VSI_OK;
//}

/*
+--------------------------------------------------------------------+
| PROJECT : GSM-Frame (8415)           MODULE  : VSI_TRC             |
| STATE   : code                       ROUTINE : vsi_o_assert        |
+--------------------------------------------------------------------+

  PURPOSE : assert

*/
//s16_t vsi_o_assert (s16_t Caller, n8_t *expr, const n8_t *file, s32_t line)
//{
//  NOTICE;
//  while(1);
//}

/*
+--------------------------------------------------------------------+
| PROJECT : GSM-Frame (8415)           MODULE  : VSI_TRC             |
| STATE   : code                       ROUTINE : vsi_settracemask    |
+--------------------------------------------------------------------+

  PURPOSE : set trace mask

*/

//s16_t vsi_settracemask ( s16_t Caller, s16_t Handle, ul32_t Mask )
//{
//#ifndef DISABLE_NEW_FRAME_TRACE
//if ( pf_TaskTable[Handle].Name )
//{
//  TraceMask[Handle] = Mask;
//    return VSI_OK;
//}
//else
//  return VSI_ERROR;
//#else
//  return VSI_OK;
//#endif
//}
/*
+--------------------------------------------------------------------+
| PROJECT : GSM-Frame (8415)           MODULE  : VSI_TRC             |
| STATE   : code                       ROUTINE : vsi_gettracemask    |
+--------------------------------------------------------------------+

  PURPOSE : get trace mask

*/
//
//s16_t vsi_gettracemask ( s16_t Caller, s16_t Handle, ul32_t *Mask )
//{
//#ifndef DISABLE_NEW_FRAME_TRACE
//  if ( pf_TaskTable[Handle].Name )
//  {
//    *Mask = TraceMask[Handle];
//    return VSI_OK;
//  }
//  else
//    return VSI_ERROR;
//#endif    
//  return VSI_OK;
//}

/*
+--------------------------------------------------------------------+
| PROJECT : GSM-Frame (8415)           MODULE  : VSI_TRC             |
| STATE   : code                       ROUTINE : vsi_trcsuspend      |
+--------------------------------------------------------------------+

  PURPOSE : set suspend for traces

*/
//
//s16_t vsi_trcsuspend ( s16_t Caller, s16_t Handle, ul32_t Suspend )
//{
//#ifndef DISABLE_NEW_FRAME_TRACE
//  SuspendTrace[Handle] = Suspend;
//#endif    
//  return VSI_OK;
//}

//#ifdef MEMORY_SUPERVISION

/*
+--------------------------------------------------------------------+
| PROJECT : GSM-Frame (8415)           MODULE  : VSI_TRC             |
| STATE   : code                       ROUTINE : vsi_trc_new         |
+--------------------------------------------------------------------+

  PURPOSE : monitor test interface partition allocation

*/

//void vsi_trc_new ( s16_t Caller, ul32_t *Prim, ul32_t Size )
//{
//}

//#endif /* MEMORY_SUPERVISION */

/*
+--------------------------------------------------------------------+
| PROJECT : GSM-Frame (8415)           MODULE  : VSI_TRC             |
| STATE   : code                       ROUTINE : vsi_trc_free        |
+--------------------------------------------------------------------+

  PURPOSE : monitor test interface partition allocation
*/

//s16_t vsi_trc_free ( s16_t Caller, ul32_t **Prim )
//{
//#ifndef DISABLE_NEW_FRAME_TRACE
//  if ( os_DeallocatePartition ( Caller, *Prim ) != OS_ERROR )
//  {
//    *Prim = NULL;
//    return VSI_OK;
//  }
//  return VSI_ERROR;
//#else
//  return VSI_OK;
//#endif  
//}

/*
+--------------------------------------------------------------------+
| PROJECT : GSM-Frame (8415)           MODULE  : VSI_TRC             |
| STATE   : code                       ROUTINE : InitializeTestpools |
+--------------------------------------------------------------------+

  PURPOSE : initialize supervision, write index to each partition.

*/

//void InitializeTrace ( void )
//{
//    s32_t i;
//    for (i=0;i<NB_MAX_TASK;i++)
//    {
//        if(process_get_ps_name(i)==NULL)
//            break; 
//        xTaskTraceChannel[i] = xTraceRegisterString(process_get_ps_name(i)); /*register trace chan names for defined task */
//    }
//    xTraceStack = xTraceRegisterString("INFO");
//}

