#include "manager_operation_data.h"
#include "manager_wiota_respons.h"
#include "manger_net_messager.h"
#include "manager_module.h"
#include "manager_logic_cmd.h"
#include "manager_userid.h"
#include "uc_coding.h"
#include "string.h"
#include "cJSON.h"

int manager_pasing_address(unsigned char *data, t_address_period *address_info)
{
    cJSON* cjson = cJSON_Parse((const char *)data);
    MEMORY_ASSERT(cjson);

    address_info->start_address = cJSON_GetObjectItem(cjson, "start_address")->valueint;
    address_info->end_address = cJSON_GetObjectItem(cjson, "end_address")->valueint;

    return 0;
}



int manager_net_data_logic(t_from_network_message *page)
{
    rt_kprintf("manager_net_data_logic page->dest_len = %d,page->dest_address = 0x%x, cmd = %d, json_data(0x%x) = %s\n", page->dest_len, *page->dest_address, page->cmd, (unsigned int)page->json_data, (char *)page->json_data);

    switch(page->cmd)
    {
        case APP_CMD_NET_PAIR_RESPONSE:
        case APP_CMD_NET_PAIR_CANCEL:
        case APP_CMD_NET_CTRL_IOTE:
        case APP_CMD_NET_MUTICAST_ASSIGNMENT:
        {
             app_ps_header_t ps_header = {0};
             unsigned char *output_data;
             unsigned int output_len;
             
            app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
            app_set_header_property(PRO_NEED_RES, 1, &ps_header.property);
            
            ps_header.addr.src_addr = SERVER_DEFAULT_ADDRESS;
            ps_header.packet_num = page->request_number;
            
            ps_header.cmd_type = page->cmd;

            if(0 == app_data_coding(&ps_header, page->json_data, (unsigned int)rt_strlen((const char *)page->json_data), &output_data, &output_len))
            {
                #if 0
                {
                       rt_kprintf("app_data_coding 0\n");
                       int n = 0;
                       for(;n < output_len; n++)
                        {
                            rt_kprintf("0x%x\t", output_data[n]);
                        }
                       rt_kprintf("\n");
                 }
                #endif
                int num = 0;
                int dest_num = (page->dest_len>> 2);
                for(; num < dest_num; num++)
                {
                    unsigned char * data_page = RT_NULL;
                    if (num + 1  == dest_num)
                    {
                        data_page = output_data;
                    }
                    else
                    {
                        data_page = rt_malloc(output_len);
                        rt_memcpy(data_page, output_data, output_len);
                    }
                    #if 0
                   {
                       rt_kprintf("app_data_coding num %d. output_len %d\n", num, output_len);
                       int n = 0;
                       for(;n < output_len; n++)
                        {
                            rt_kprintf("0x%x\t", data_page[n]);
                        }
                       rt_kprintf("\n");
                    }
                    #endif
                   
                    manager_logic_send_to_operater(page->request_number, ps_header.addr.src_addr, ps_header.cmd_type, (page->dest_address)[num], output_len, data_page, manager_network_to_wiota_result);
                }
             }
            break;
        }
        case MANAGER_LOGIC_RESPONSE_RESERVE_ADDR:
        {
            t_address_period address_info;
            manager_pasing_address(page->json_data, &address_info);
            manager_get_reserved_address(address_info.start_address, address_info.end_address);
            break;
        }
        default:
        {
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
