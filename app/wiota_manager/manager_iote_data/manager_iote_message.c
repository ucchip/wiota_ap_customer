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
#include "manager_wiota_frame.h"
#include "manager_userid.h"
#include "net_passthrough.h"
#include "manager_wiota_attribute.h"
#include "manager_wiota_respons.h"
#include "manager_request_reserve_addr.h"
#include "manager_net_messager.h"
#include "manager_data_packet.h"
#include "manager_iote_message.h"
#include "manager_logic_cmd.h"
#include "manager_addrss.h"
#include "manager_update.h"
#include "uc_coding.h"
#include "cJSON.h"

static void manager_printf_message(char *data, int len)
{
    int i = 0;
    rt_kprintf("messager %d:\n", len);
    for (; i < len; i++)
    {
        rt_kprintf("%02x ", data[i]);
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

static void manager_replace_response_to_wiota(void *buf)
{
    t_app_operation_data *operation_data = buf;
    rt_kprintf("manager_message_def_exe_result page->pload = 0x%x\n", operation_data->pload);
    if (RT_NULL != operation_data->pload)
        rt_free(operation_data->pload);
}

static void manager_send_replace_response(unsigned int src_address, unsigned int dest_address, unsigned int new_address)
{
    app_ps_header_t ps_header = {0};
    unsigned char *output_data;
    unsigned int output_len;
    cJSON *root = cJSON_CreateObject();
    char *data;

    cJSON_AddNumberToObject(root, "new_address", new_address);
    data = cJSON_Print(root);

    rt_kprintf("manager_send_replace_response data:%s\n", data);

    //app_set_header_property(PRO_NEED_RES, 1, &ps_header.property);
    app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);

    ps_header.addr.src_addr = SERVER_DEFAULT_ADDRESS;
    ps_header.cmd_type = APP_CMD_IOTE_RESPONSE_REPLACE_ADDR;

    if (0 == app_data_coding(&ps_header, (unsigned char *)data, rt_strlen(data), &output_data, &output_len))
    {
        manager_logic_send_to_operater(MANAGER_OPERATION_MSG,
                                       0,
                                       src_address,
                                       ps_header.cmd_type,
                                       dest_address,
                                       output_len,
                                       output_data,
                                       RT_NULL,
                                       manager_replace_response_to_wiota);
    }

    rt_free(data);
    cJSON_Delete(root);
}

void manager_recv_wiota_msg(void *page)
{
    t_app_recv_wiota_info *ul_data = page;
    unsigned char *decode_data = RT_NULL;
    unsigned int decode_data_len = 0;
    app_ps_header_t ps_header = {0};
    data_packet_t *packet = RT_NULL;

    manager_printf_message(ul_data->data, ul_data->len);

    if (0 != app_data_decoding((unsigned char *)ul_data->data, ul_data->len, &decode_data, &decode_data_len, &ps_header))
    {
        rt_kprintf("%s, %d decoding failed\n", __FUNCTION__, __LINE__);
        rt_free(ul_data->data);
        ul_data->data = RT_NULL;
        return;
    }

    rt_kprintf("recv iote 0x%x cmd %d. response_flag %d, segment_flag %d\n", ul_data->id, ps_header.cmd_type, ps_header.property.response_flag, ps_header.property.segment_flag);

    /* if it is a data segment */
    if (ps_header.property.segment_flag)
    {
        /* find data packet by id and packet_num */
        packet = recv_data_packet_find(ul_data->id, ps_header.packet_num);
        /* add a new data packet, if not found */
        if (RT_NULL == packet)
            packet = recv_data_packet_append(ul_data->id, ps_header.packet_num, ps_header.segment_info.total_num);
        /* add data segment */
        recv_data_packet_add_segment(packet, ps_header.segment_info.current_num, decode_data, decode_data_len);
        /* if a complete packet is received */
        if (0 == recv_data_packet_reader(packet, &decode_data, &decode_data_len))
        {
            /* remove the data packet */
            recv_data_packet_remove(packet);

            /* print debug information */
            rt_kprintf("recv iote[%d]:\n", decode_data_len);
            for (int i = 0; i < decode_data_len; ++i)
                rt_kprintf("%02x ", decode_data[i]);
            rt_kprintf("\n");
        }
        else
        {
            rt_free(ul_data->data);
            ul_data->data = RT_NULL;
            return;
        }
    }

    rt_kprintf("recv iote page,run cmd %d\n", ps_header.cmd_type);
    if (!ps_header.property.response_flag)
    {
        switch (ps_header.cmd_type)
        {
        case APP_CMD_IOTE_PROPERTY_REPORT:
        case APP_CMD_IOTE_STATE_REPORT:
        case APP_CMD_IOTE_PAIR_REQUEST:
        case APP_CMD_IOTE_MUTICAST_REQUEST:
            if (0 == manager_send_data_logic_to_net(ps_header.cmd_type, decode_data, decode_data_len, ps_header.addr.src_addr/*ul_data->id*/))
                decode_data = RT_NULL;
            break;
        case APP_CMD_NET_CTRL_IOTE:
        {
            if (SERVER_DEFAULT_ADDRESS != ps_header.addr.dest_addr && ps_header.property.is_dest_addr)
            {
                rt_kprintf("Iote ctrol dest address 0x%x\n", ps_header.addr.dest_addr);
                // the ap do not response to message forwarding.
                if (!ps_header.property.is_need_res || AP_DEFAULT_ADDRESS != ps_header.addr.dest_addr)
                    ps_header.addr.src_addr = 0;
                manager_logic_send_to_operater(MANAGER_OPERATION_MSG,
                                               ps_header.packet_num,
                                               ps_header.addr.src_addr,
                                               APP_CMD_NET_CTRL_IOTE,
                                               manager_query_wiotaid(ps_header.addr.dest_addr),
                                               ul_data->len,
                                               ul_data->data,
                                               RT_NULL,
                                               manager_control_message_result);
                ul_data->data = RT_NULL;
            }
            break;
        }
        case APP_CMD_UPDATE_VERSION_REQUEST:
        {
            rt_kprintf("APP_CMD_UPDATE_VERSION_REQUEST state %d,decode_data = %s\n", manager_get_ota_state(), decode_data);
            if (MANAGER_UPDATE_STOP == manager_get_ota_state() || \
                MANAGER_UPDATE_DEFAULT == manager_get_ota_state())
            {
                if (0 == manager_send_data_logic_to_net(ps_header.cmd_type, decode_data, decode_data_len, ps_header.addr.src_addr))
                    decode_data = RT_NULL;
            }
            else
            {
                manager_ota_send_state(MANAGER_OTA_STOP);
            }
            break;
        }
        case APP_CMD_GET_SPECIFIC_DATA_REQUEST:
        {
            if (MANAGER_UPDATE_STOP != manager_get_ota_state() && \
                MANAGER_UPDATE_DEFAULT != manager_get_ota_state())
            {
                manager_ota_send_state(MANAGER_OTA_GONGING);
            }
            else
            {
                if (MANAGER_UPDATE_STOP == manager_get_ota_state() )
                {
                    // send ota bin data
                    manager_ota_resend_data(decode_data);
                }
                else
                {
                    // send ota bin data
                    manager_ota_send_state(MANAGER_OTA_NODATA);
                }
            }
            break;
        }
        case APP_CMD_AP_REQUEST_IOTE_REPLACE_ADDR:
            manager_reallocated_wiotaid(ps_header.addr.src_addr, ul_data->id, manager_send_replace_response);
            break;
        }
        if (RT_NULL != ul_data->data)
        {
            rt_free(ul_data->data);
            ul_data->data = RT_NULL;
        }
        if (RT_NULL != decode_data)
        {
            rt_free(decode_data);
            decode_data = RT_NULL;
        }
    }
}

#endif
