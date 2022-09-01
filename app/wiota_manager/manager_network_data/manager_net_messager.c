#include "manager_wiota_frame.h"
#include "manager_wiota_respons.h"
#include "manager_net_messager.h"
#include "manager_module.h"
#include "manager_logic_cmd.h"
#include "manager_userid.h"
#include "manager_addrss.h"
#include "manager_update.h"
#include "string.h"
#include "uc_wiota_api.h"
#include "uc_coding.h"
#include "cJSON.h"

int manager_pasing_address(unsigned char *data, t_address_period *address_info)
{
    cJSON *cjson = cJSON_Parse((const char *)data);
    MEMORY_ASSERT(cjson);

    //rt_kprintf("data = %s\n", data);

    address_info->start_address = cJSON_GetObjectItem(cjson, "start_address")->valuedouble;
    address_info->end_address = cJSON_GetObjectItem(cjson, "end_address")->valuedouble;

    cJSON_Delete(cjson);

    return 0;
}

static void manager_send_packet(app_ps_header_t *ps_header, unsigned char *segment, unsigned char is_broadcast, unsigned int request_number, unsigned char *input_data, unsigned int input_data_len)
{
    unsigned char *output_data = RT_NULL;
    unsigned int output_len = 0;

    if (0 == app_data_coding(ps_header, input_data, input_data_len, &output_data, &output_len))
    {
        if (is_broadcast == 0)
        {
            manager_logic_send_to_operater(
                MANAGER_OPERATION_MSG,
                request_number,
                ps_header->addr.src_addr,
                ps_header->cmd_type,
                manager_query_wiotaid(ps_header->addr.dest_addr),
                output_len,
                output_data,
                segment,
                manager_network_to_wiota_result);
        }
        else
        {
            manager_logic_send_to_operater(
                MANAGER_OPERATION_RADIO,
                request_number,
                ps_header->addr.src_addr,
                ps_header->cmd_type,
                manager_query_wiotaid(ps_header->addr.dest_addr),
                output_len,
                output_data,
                segment,
                RT_NULL);
            manager_response_message_result(MANAGER_SEND_SUCCESS, request_number);
        }
    }
}

static void manager_send_msg_to_wiota(t_from_network_message *page)
{
    int num = 0;
    int dest_num = (page->dest_len >> 2);
    for (; num < dest_num; num++)
    {
        app_ps_header_t ps_header = {0};
        unsigned char *segment = RT_NULL;
        unsigned char *input_data = page->json_data;
        unsigned int input_data_len = (unsigned int)rt_strlen((const char *)input_data);
        unsigned int max_len, segment_len;
        unsigned char is_broadcast =
            (page->dest_address[num] & AP_DEFAULT_ADDRESS) == AP_DEFAULT_ADDRESS ||
            (page->dest_address[num] & AP_DEFAULT_RADIO_ADDRESS) == AP_DEFAULT_RADIO_ADDRESS ||
            (page->dest_address[num] & AP_MULTICAST_ADDRESS) == AP_MULTICAST_ADDRESS;

        app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
        app_set_header_property(PRO_DEST_ADDR, 1, &ps_header.property);
        /* broadcast does not require a response. */
        if (is_broadcast == 0)
        {
            app_set_header_property(PRO_NEED_RES, 1, &ps_header.property);
            max_len = UC_WIOTA_MAX_SEND_NORMAL_DATA_LEN - sizeof(app_ps_header_t);
        }
        else
            max_len = UC_WIOTA_MAX_SEND_BROADCAST_DATA_LEN - sizeof(app_ps_header_t);

        ps_header.addr.dest_addr = /*manager_query_wiotaid */(page->dest_address[num]);
        ps_header.addr.src_addr = SERVER_DEFAULT_ADDRESS;
        ps_header.packet_num = page->request_number;
        ps_header.cmd_type = page->cmd;

        // char big_data[] = "{\"control\": \"on\",                                                                                                                                                                                                                                                                                                            \"type\": \"server\"}";
        // input_data = (unsigned char*)big_data;
        // input_data_len = (unsigned int)rt_strlen(big_data);
        if (input_data_len > max_len)
        {
            /* print debug information */
            rt_kprintf("%s line %d[%d>%d]:\n", __FUNCTION__, __LINE__, input_data_len, max_len);
            for (int i = 0; i < input_data_len; ++i)
                rt_kprintf("%02x ", input_data[i]);
            rt_kprintf("\n");

            /* need to split packet */
            app_set_header_property(PRO_SEGMENT_FLAG, 1, &ps_header.property);
            app_set_header_property(PRO_PACKET_NUM, 1, &ps_header.property);
            ps_header.segment_info.total_num = (input_data_len + max_len - 1) / max_len;
            ps_header.segment_info.current_num = 0;
            segment = rt_malloc(4);
            segment[0] = MANAGER_SEND_SUCCESS; /* state */
            segment[1] = ps_header.segment_info.total_num;
            while (input_data_len > 0)
            {
                segment_len = input_data_len > max_len ? max_len : input_data_len;
                manager_send_packet(&ps_header, segment, is_broadcast, page->request_number, input_data, segment_len);
                ps_header.segment_info.current_num++;
                input_data += segment_len;
                input_data_len -= segment_len;
            }
        }
        else
        {
            /* print debug information */
            rt_kprintf("%s line %d[%d<=%d]:\n", __FUNCTION__, __LINE__, input_data_len, max_len);
            for (int i = 0; i < input_data_len; ++i)
                rt_kprintf("%02x ", input_data[i]);
            rt_kprintf("\n");

            manager_send_packet(&ps_header, segment, is_broadcast, page->request_number, input_data, input_data_len);
        }
    }
}

//void send_big_data_to_wiota()
//{
//    t_from_network_message page = {0};
//    page.dest_len = 4;
//    page.dest_address = rt_malloc(sizeof(unsigned int));
//    page.dest_address[0] = 0x123459;
//    page.request_number = 0;
//    page.cmd = 40;
//    manager_send_msg_to_wiota(&page);
//    rt_free(page.dest_address);
//}

static void manager_send_config_to_wioata(t_from_network_message *page)
{
    manager_response_message_result(MANAGER_SEND_SUCCESS, page->request_number);
}

int manager_net_data_logic(t_from_network_message *page)
{
    rt_kprintf("manager_net_data_logic page->dest_len = %d,page->dest_address = 0x%x, num = %d, cmd = %d, json_data(0x%x) = %s\n",
               page->dest_len, *page->dest_address, page->request_number, page->cmd, (unsigned int)page->json_data, (char *)page->json_data);

    switch (page->cmd)
    {
    case APP_CMD_NET_PAIR_RESPONSE:
    case APP_CMD_NET_PAIR_CANCEL:
    case APP_CMD_NET_CTRL_IOTE:
    case APP_CMD_NET_MUTICAST_ASSIGNMENT:
    {
        manager_send_msg_to_wiota(page);
        break;
    }
    case APP_CMD_UPDATE_VERSION_SPONSE:
    {
        manager_set_ota_state(MANAGER_UPDATE_RESPONSE_MSG);
        manager_update_response(page, RT_NULL, RT_NULL);
        break;
    }
    case APP_CMD_NET_DELIVER_IOTE_CONFIG:
    {
        manager_send_config_to_wioata(page);
        break;
    }
    case MANAGER_LOGIC_RESPONSE_RESERVE_ADDR:
    {
        t_address_period address_info;
        manager_pasing_address(page->json_data, &address_info);
        /*user id of hight bit is reserver flag. default is 0. user manager set 1*/
        manager_get_reserved_address(address_info.start_address | (1 << 31), address_info.end_address | (1 << 31));
        break;
    }

    default:
    {
        rt_kprintf("manager_net_data_logic default\n");
        break;
    }
    }

    if (RT_NULL != page->json_data)
    {
        rt_free(page->json_data);
        page->json_data = RT_NULL;
    }

    if (RT_NULL != page->dest_address)
    {
        rt_free(page->dest_address);
        page->dest_address = RT_NULL;
    }
    return 0;
}
