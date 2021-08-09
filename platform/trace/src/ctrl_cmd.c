#include "ctrl_cmd.h"
#include "typedefs.h"
#include "vsi.h"
#include "string.h"
//EXTERN    T_L1S_GLOBAL   l1s;
#define VSI_CALLER 0,

#define ACTIVE_MODU_NUM  2 // 2 *4*8;

#ifndef _FPGA_
u32_t   submodule_active_flag[2] = {0xffffffff};//used to store the act cmd from client,total 64 bits for 64 submodules.
#else
u32_t   submodule_active_flag[2] = {0x0};//used to store the act cmd from client,total 64 bits for 64 submodules.
#endif



void  proc_act_log_list(unsigned char* pData)
{
    memcpy(submodule_active_flag,pData,8);
}
void  proc_act_all_log(){
    s32_t n=0;
    for(;n<2;n++)
        submodule_active_flag[n]=0xffffffff;
}
void  proc_disact_all_log(){
    s32_t n=0;
    for(;n<2;n++)
        submodule_active_flag[n]=0;
}

//void  log_cmd_proc(LOG_CTRL *cmd)
//{
//    switch(cmd->cmd_type)//cmd_type
//    {
//        case SET_ACTIVE_LOG:
//            proc_act_log_list((u32_t*)cmd->cmd_data);
//        break;
//        case SET_CMD_TEST://4
//        
////            TRACE_EVENT_P1("%d\n",l1s.actual_time.fn);
//            //TRACE_FUNCTION(RECEIVE_CMD_TEST);
//#ifndef _LINUX_
//            TRACE_SEND_RIGHT_NOW
//#endif
//            break;
////        case RR_SHOW:
////            rr_show_state((n8_t*)cmd->cmd_data);
////            TRACE_EVENT_P1("%s", cmd->cmd_data);
////            #ifndef _LINUX_
////                TRACE_SEND_RIGHT_NOW
////            #endif
////            break;
//
//        default:
//            break;
//    }
//}


