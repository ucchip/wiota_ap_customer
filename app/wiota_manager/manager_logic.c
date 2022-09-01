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
#include "manager_request_reserve_addr.h"
#include "manager_data_packet.h"
#include "manager_net_messager.h"
#include "manager_iote_message.h"
#include "manager_logic_cmd.h"
#include "manager_addrss.h"
#include "uc_coding.h"
#include "manager_update.h"
#include "manager_request_version.h"
#include "cJSON.h"

static void *manager_queue_handle;

/*
 * Linked lists of business logic records.
 * For multi-task parallel processing.(Task of concurrent)
 * Example:
 *         The cloud server is controlling the lamp 1. Switch 2 controls lamp 3.
 */
//static t_manager_list manager_wiota_list;

int manager_create_logic_queue(void)
{
    // create wiota app manager queue.
    manager_queue_handle = manager_create_queue("app_manager", 4, 16, UC_SYSTEM_IPC_FLAG_PRIO);
    if (manager_queue_handle == RT_NULL)
    {
        rt_kprintf("manager_create_queue error\n");
        return 1;
    }

    return 0;
}

void to_queue_logic_data(int src_task, int cmd, void *data)
{
    t_app_logic_message *message = rt_malloc(sizeof(t_app_logic_message));
    MEMORY_ASSERT(message);

    message->cmd = cmd;
    message->data = data;

    //if(RT_NULL != data)
    //{
    //    rt_kprintf("to_queue_logic_data--> 0x%x\n", data);
    //    test_memory(data, __FUNCTION__, __LINE__);
    //}

    if (manager_send_page(manager_queue_handle, src_task, message))
    {
        rt_free(message);
        rt_kprintf("%s line %d send data to manager_queue_handle error\n", __FUNCTION__, __LINE__);
    }
}

//static void manager_message_def_exe_result(void *buf)
//{
//    t_app_operation_data *operation_data = buf;
//    rt_kprintf("manager_message_def_exe_result page->pload = 0x%x\n", operation_data->pload);
//    if (RT_NULL != operation_data->pload)
//        rt_free(operation_data->pload);
//}

//static void manager_send_replace_address_message(unsigned int old_address, unsigned int new_address)
//{
//    app_ps_header_t ps_header = {0};
//    cJSON *root;
//    unsigned char *string_data;
//    unsigned char *output_data;
//    unsigned int output_len;
//
//    // coding head
//    app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
//    app_set_header_property(PRO_DEST_ADDR, 1, &ps_header.property);
//    app_set_header_property(PRO_PACKET_NUM, 1, &ps_header.property);
//    app_set_header_property(PRO_NEED_RES, 1, &ps_header.property);
//
//    ps_header.addr.src_addr = AP_DEFAULT_ADDRESS;
//    ps_header.addr.dest_addr = old_address;
//    ps_header.packet_num = app_packet_num();
//    ps_header.cmd_type = APP_CMD_AP_REQUEST_IOTE_REPLACE_ADDR;
//
//    // dcoding json data
//    root = cJSON_CreateObject();
//    MEMORY_ASSERT(root);
//    cJSON_AddNumberToObject(root, "new_address", new_address);
//    string_data = (unsigned char *)cJSON_Print(root);
//    rt_kprintf("string_data:%s\n", string_data);
//
//    app_data_coding(&ps_header, string_data, (unsigned int)rt_strlen((const char *)string_data), &output_data, &output_len);
//
//    // send MANAGER_OPERATION_MSG to operation task.
//    //manager_logic_send_data_to_opreation(output_data, output_len, old_address);
//    manager_logic_send_to_operater(MANAGER_OPERATION_MSG,
//                                   ps_header.packet_num,
//                                   ps_header.addr.src_addr,
//                                   ps_header.cmd_type,
//                                   old_address,
//                                   output_len,
//                                   output_data,
//                                   manager_message_def_exe_result);
//    rt_free(string_data);
//    cJSON_Delete(root);
//}

void manager_wiota_task(void *pPara)
{
    /*
    relationship between page and message:
        typedef struct app_manager_message
        {
            int src_task;
            void *message;  typedef struct app_logic_message
                            {
                                int cmd;
                                void *data;   //diffrent command have different data type.
                            }t_app_logic_message;
        }t_app_manager_message;
    */
    static t_app_manager_message *page;
    static t_app_logic_message *message;
    int result = 0;

    manager_address_init(manager_request_reserve_addr);

    // initialize receive data packet
    recv_data_packet_init();
    //report attribute to serve
    manager_input_attribute();

    // startup trigger version check.
    // manager_set_ota_state(MANAGER_UPDATE_REQUEST_MSG);
    // manager_check_ap_version();

    while (1)
    {
        //rt_kprintf("wait manager_recv_queue\n");
        result = manager_recv_queue(manager_queue_handle, (void **)&page, UC_QUEUE_WAITING_FOREVER);
        if (QUEUE_EOK != result)
        {
            rt_kprintf("manager_wiota_task recv queue data error, result %d\n", result);
            continue;
        }

        //rt_kprintf("%s line %d page address 0x%x\n",__FUNCTION__, __LINE__, page);

        message = page->message;
        //rt_kprintf("manager_logic message->cmd 0x%x(%d)\n", message->cmd, message->cmd);

        switch (message->cmd)
        {
        case MANAGER_LOGIC_WIOTA_START:
        {
            manager_wiotaid_start();
            break;
        }
        case MANAGER_LOGIC_SEND_RESULT:
        {
            t_app_operation_data *result_page = message->data;

            if (RT_NULL != result_page->head.result_function)
                result_page->head.result_function(result_page);
            break;
        }
        case MANAGER_LOGIC_RECV_WIOTA_DATA:
        {
            manager_recv_wiota_msg((t_app_recv_wiota_info *)message->data);
            break;
        }
        case MANAGER_LOGIC_FROM_NETWORK_MESSAGE:
        {
            manager_net_data_logic(message->data);
            break;
        }
        case MANAGER_LOGIC_FROM_HTTP_MESSAGE:
        {
            rt_kprintf("MANAGER_LOGIC_FROM_HTTP_MESSAGE %d\n", ((t_http_message *)message->data)->state);
            manager_start_update(message->data);
            break;
        }
        default:
        {
            break;
        }
        }

        //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

        if (RT_NULL != message->data)
            rt_free(message->data);

        //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

        rt_free(message);
        message = RT_NULL;

        //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

        rt_free(page);
        page = RT_NULL;

        //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    }
    manager_address_exit();
    // clear receive data packet
    recv_data_packet_clear();

    manager_dele_queue(manager_queue_handle);

    //clean_manager_list(&manager_wiota_list);
}

#endif
