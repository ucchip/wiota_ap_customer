#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>
#include "uc_wiota_api.h"
#include "manager_queue.h"
#include "manager_module.h"
#include "manager_logic.h"
#include "manager_task.h"
#include "manager_list.h"
#include "manager_operation_data.h"
#include "manager_userid.h"
#include "net_passthrough.h"
#include "manager_wiota_attribute.h"
#include "manager_wiota_respons.h"
#include "manager_request_reserve_addr.h"
#include "manger_net_messager.h"
#include "manager_iote_message.h"
#include "manager_logic_cmd.h"
#include "uc_coding.h"
#include "cJSON.h"

static void manager_printf_message(char *data, int len)
{
    int i = 0;
    rt_kprintf("messager %d:\n", len);
    for(; i < len; i++)
    {
        rt_kprintf("%x\t", data[i]);
    }
    rt_kprintf("\n");
}

static void manager_control_message_result(void *buf)
{
    t_app_operation_data *operation_data = buf;
    rt_kprintf("manager_control_message_result page->pload = 0x%x\n", operation_data->pload);
   if (RT_NULL != operation_data->pload)
       rt_free(operation_data->pload);
}


void manager_recv_wiota_msg(void *page)
{
    t_app_recv_wiota_info *ul_data = page;
    unsigned char *decode_data = RT_NULL;
    unsigned int decode_data_len = 0;
    app_ps_header_t ps_header = {0};
    
    manager_printf_message(ul_data->data, ul_data->len);
    
    if (0 != app_data_decoding((unsigned char *)ul_data->data, ul_data->len, &decode_data, &decode_data_len, &ps_header))
    {
        rt_kprintf("%s, %d decoding failed\n", __FUNCTION__, __LINE__);
        rt_free(ul_data->data);
        ul_data->data = RT_NULL;
        return;
    }

    rt_kprintf("recv iote cmd %d. ps_header.property.response_flag %d\n", ps_header.cmd_type, ps_header.property.response_flag);

    if (ps_header.property.response_flag)
    {
        if (SERVER_DEFAULT_ADDRESS == ps_header.addr.dest_addr)
        {
            manager_response_message_result(MANAGER_SEND_SUCCESS, ps_header.packet_num);
        }
        rt_free(ul_data->data);
        ul_data->data = RT_NULL;
        if (decode_data != RT_NULL)
        {
            rt_free(decode_data);
            decode_data = RT_NULL;
        }
    }
    else
    {
        //rt_kprintf("decode_data:%s\n", decode_data);
        switch (ps_header.cmd_type)
        {
            case APP_CMD_IOTE_PROPERTY_REPORT:
            case APP_CMD_IOTE_STATE_REPORT:
            case APP_CMD_IOTE_PAIR_REQUEST:
            case APP_CMD_IOTE_MUTICAST_REQUEST:
            {
                if(0 != manager_send_data_logic_to_net(ps_header.cmd_type, decode_data, decode_data_len, ul_data->id))
                    rt_free(decode_data);
                
                rt_free(ul_data->data);
                ul_data->data = RT_NULL;
                break;
            }
            case APP_CMD_NET_CTRL_IOTE:
            {
                if (SERVER_DEFAULT_ADDRESS != ps_header.addr.dest_addr && ps_header.property.is_dest_addr)
                {
                    rt_kprintf("Iote ctrol dest address 0x%x\n", ps_header.addr.dest_addr);
                    /*
                    * the ap do not response to message forwarding.
                    */
                    if (! ps_header.property.is_need_res || AP_DEFAULT_ADDRESS != ps_header.addr.dest_addr)
                        ps_header.addr.src_addr = 0;
                    
                    manager_logic_send_to_operater(ps_header.packet_num, \
                        ps_header.addr.src_addr, \
                        APP_CMD_NET_CTRL_IOTE, \
                        ps_header.addr.dest_addr, \
                        ul_data->len, \
                        ul_data->data, \
                        manager_control_message_result);
                }
                else
                {
                    //manager_send_data_logic_to_net(ps_header.cmd_type, decode_data, decode_data_len, ul_data->id);
                    rt_free(ul_data->data);
                    ul_data->data = RT_NULL;
                }
                if (decode_data != RT_NULL)
                {
                    rt_free(decode_data);
                    decode_data = RT_NULL;
                }
                break;
            }
            default:
            {
                rt_free(ul_data->data);
                ul_data->data = RT_NULL;
                if (decode_data != RT_NULL)
                {
                    rt_free(decode_data);
                    decode_data = RT_NULL;
                }
                break;
            }
        }
     }

}



#endif
