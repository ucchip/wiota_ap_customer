#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "string.h"
#include "uc_wiota_api.h"
#include "manager_list.h"
#include "manager_queue.h"
#include "manager_module.h"
#include "manager_logic.h"
#include "manager_logic_cmd.h"
#include "manager_wiota_freq.h"
#include "manager_wiota_frame.h"

static void *operation_queue_handle;
static t_manager_list wiota_send_manager_list;
static rt_sem_t manager_list_sem;
unsigned int recv_msg_count = 0;

int manager_create_operation_queue(void)
{
    // create wiota app manager queue.
    operation_queue_handle = manager_create_queue("operation_manager", 4, 32, UC_SYSTEM_IPC_FLAG_PRIO);
    if (operation_queue_handle == RT_NULL)
    {
        rt_kprintf("manager_create_queue error\n");
        return 1;
    }
    manager_list_sem = rt_sem_create("operation_sem", 1, RT_IPC_FLAG_PRIO);
    if (RT_NULL == manager_list_sem)
    {
        rt_kprintf("rt_sem_create error\n");
        return 1;
    }

    return 0;
}

void to_operation_wiota_data(int cmd, void *data)
{
    t_app_logic_message *message = rt_malloc(sizeof(t_app_logic_message));
    if (RT_NULL == message)
    {
        rt_kprintf("wiota_recv_wiota_data malloc error\n");
        return;
    }
    message->cmd = cmd;
    message->data = data;
    //rt_kprintf("from logic task to operation queue\n");
    manager_send_page(operation_queue_handle, MANAGER_LOGIC_INDENTIFICATION, message);
}

void manager_logic_send_to_operater(int msg_type, unsigned int request_number, unsigned int src_address, unsigned int cmd, unsigned int id, int len, void *data, unsigned char *segment, manager_message_exe_result func)
{
    t_app_operation_data *page = rt_malloc(sizeof(t_app_operation_data));

    MEMORY_ASSERT(page);

    //rt_kprintf("manager_logic_send_to_operater id 0x%x\n", id);
    page->head.id = id; /*dest dev userid*/
    page->head.state = MANAGER_SEND_DEFAULT;
    page->head.result_function = func;
    page->head.cmd = cmd;
    page->head.request_number = request_number;
    page->head.src_address = src_address;

    page->len = len;
    page->pload = data;
    page->segment = segment;
    // send data to option tasks
    to_operation_wiota_data(msg_type, page);
}

/*
 * Wiota receives data,it to the managerment module,through multiple queues.
 * Wiota data can not be usered, only copy.
 * */
void manager_recv_data(unsigned int user_id, unsigned char *recv_data, unsigned int data_len, unsigned char type)
{
    t_app_recv_wiota_info *wiota_info = rt_malloc(sizeof(t_app_recv_wiota_info));
    void *wiota_data = rt_malloc(data_len);

    rt_kprintf("manager recv data from 0x%x wiota\n", user_id);

    if (RT_NULL == wiota_info || RT_NULL == wiota_data)
    {
        if (RT_NULL != wiota_info)
            rt_free(wiota_info);
        else
            rt_free(wiota_data);

        rt_kprintf("wiota recv data, rt_malloc error\n");

        return;
    }
    // now must copy data
    memcpy(wiota_data, recv_data, data_len);

    wiota_info->id = user_id;
    wiota_info->len = data_len;
    wiota_info->data = wiota_data;

    to_queue_logic_data(MANAGER_OPERATION_INDENTIFICATION, MANAGER_LOGIC_RECV_WIOTA_DATA, (void *)wiota_info);
    recv_msg_count++;
}

void manager_send_result(uc_send_recv_t *result)
{
    uc_send_recv_t *get_res;

#if 1
    if (UC_OP_SUCC == result->result)
    {
        rt_kprintf("manager_send_result  to 0x%x succ!\n", result->user_id);
    }
    else
    {
        rt_kprintf("manager_send_result to 0x%x failed or timeout! %d\n", result->user_id, result->result);
    }
#endif

    get_res = rt_malloc(sizeof(uc_send_recv_t));
    if (RT_NULL == get_res)
    {
        rt_kprintf("manager_send_result malloc error.now result %d\n", result->result);
        return;
    }
    memcpy(get_res, result, sizeof(uc_send_recv_t));

    to_operation_wiota_data(MANAGER_OPERATION_MSG_RESULT, (void *)get_res);
}

static int query_result_callback(t_manager_list *node, void *target)
{
    t_app_operation_data *data = (t_app_operation_data *)node->data;
    if (data->head.id != *(unsigned int *)target)
        return 1;
    return 0;
}

static int del_result_callback(t_manager_list *node, void *target)
{
    if (node == target)
        return 0;
    return 1;
}

//static int test_result_callback(t_manager_list *node, void *target)
//{
//    t_app_operation_data *data = (t_app_operation_data *)node->data;
//    rt_kprintf("%s test list node info: id 0x%x cmd %d.node 0x%x\n", (char *)target, data->head.id, data->head.cmd, node);
//    return 0;
//}

static void manager_operate_result(uc_send_recv_t *res)
{
    t_app_operation_data *process;
    t_manager_list *node;

    rt_sem_take(manager_list_sem, RT_WAITING_FOREVER);
    //test_head_list(&wiota_send_manager_list, "result 1", test_result_callback);
    //find wiota_send_manager_list.
    node = query_head_list(&wiota_send_manager_list, (void *)&(res->user_id), query_result_callback);
    rt_sem_release(manager_list_sem);
    if (RT_NULL == node)
    {
        rt_kprintf("manager_operate_result query1 error.res->result %d,id 0x%x\n", res->result, res->user_id);
        return;
    }
    process = node->data;

    // send data suceess
    if (UC_OP_SUCC == res->result)
    {
        rt_kprintf("======>> ap send data(cmd %d) to 0x%x succ\n", process->head.cmd, process->head.id);
        // remove iote info
        rt_sem_take(manager_list_sem, RT_WAITING_FOREVER);
        remove_manager_node(&wiota_send_manager_list, (void *)node, del_result_callback);
        rt_sem_release(manager_list_sem);
        // send data suc state
        process->head.state = MANAGER_SEND_SUCCESS;
        to_queue_logic_data(MANAGER_OPERATION_INDENTIFICATION, MANAGER_LOGIC_SEND_RESULT, process);
    }
    else
    {
        if (process->head.state > MANAGER_MAX_SEND_NUM - 2)
        {
            rt_sem_take(manager_list_sem, RT_WAITING_FOREVER);
            remove_manager_node(&wiota_send_manager_list, (void *)node, del_result_callback);
            rt_sem_release(manager_list_sem);

            rt_kprintf("======>> ap send data(cmd %d) to 0x%x fail. result %d.counter %d\n", process->head.cmd, process->head.id, res->result, process->head.state);
            //test_memory(process, __FUNCTION__, __LINE__);
            // send data fail state
            process->head.state = MANAGER_SEND_FAIL;
            to_queue_logic_data(MANAGER_OPERATION_INDENTIFICATION, MANAGER_LOGIC_SEND_RESULT, process);

            //test_memory(process, __FUNCTION__, __LINE__);
        }
        else
        {
            rt_kprintf("======>> ap send data(cmd %d) to 0x%x fail. result %d.now resend.counter %d\n", process->head.cmd, process->head.id, res->result, process->head.state);
            // resend data
            uc_wiota_send_data(process->pload, process->len, process->head.id, 10000, manager_send_result);
            process->head.state++;
            rt_sem_take(manager_list_sem, RT_WAITING_FOREVER);
            remove_head_manager_node(&wiota_send_manager_list, (void *)node);
            rt_sem_release(manager_list_sem);
        }
    }

    //rt_sem_take(manager_list_sem, RT_WAITING_FOREVER);
    //test_head_list(&wiota_send_manager_list, "result 2", test_result_callback);
    //rt_sem_release(manager_list_sem);

    //if (31 == process->head.cmd)
    //{
    //  memcheck();
    //}
}
static void manager_recv_timer_func(void *parameter)
{
    unsigned int *current_recv_msg_count = parameter;
    static unsigned int old_recv_msg_count = 0;

    if (*current_recv_msg_count == old_recv_msg_count)
    {
        // send exit cmd to operation queue
        to_operation_wiota_data(MANAGER_OPERATION_EXIT, RT_NULL);
    }
    else
    {
        old_recv_msg_count = *current_recv_msg_count;
    }
}

void uc_send_radio_callback(uc_result_e result)
{
    rt_kprintf("uc_send_radio_callback %d\n", result);
}

static void uc_send_radio_ota_callback(uc_result_e result)
{
    rt_kprintf("manager_send_result malloc error.now result %d\n", result);
    to_operation_wiota_data(MANAGER_OPERATION_OTA_RADIO_RESULT, RT_NULL);
}

int manager_ota_remove_cb(t_manager_list *node, void *parament)
{
    t_app_operation_data *process = (t_app_operation_data *)node->data; // TODO:

    if (MANAGER_OPERATION_OTA_RADIO == process->head.cmd)
    {
        to_queue_logic_data(MANAGER_OPERATION_INDENTIFICATION, MANAGER_LOGIC_SEND_RESULT, process);
        return 0;
    }
    return 1;
}

static void manager_operation(void)
{
    /*
    relationship between page and message:
        typedef struct app_manager_message
        {
            int src_task;
            void *message; -----------------> typedef struct app_logic_message
                                                                         {
                                                                           int cmd;
                                                                           void *data;  -------------------------------> diffrent command have different data type.
                                                                         }t_app_logic_message;
        }t_app_manager_message;
    */

    t_app_manager_message *page;
    t_app_logic_message *message;
    t_app_operation_data *data;
    rt_timer_t timer_recv_manager;
    char current_task_state = 1;

    to_queue_logic_data(MANAGER_OPERATION_INDENTIFICATION, MANAGER_LOGIC_WIOTA_START, RT_NULL);

    init_manager_list(&wiota_send_manager_list);

    uc_wiota_register_recv_data_callback(manager_recv_data);

    timer_recv_manager = rt_timer_create("recv_manager", manager_recv_timer_func, &recv_msg_count, MANAGER_CHECKOUT_AP_STATE_TIMEOUT, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    MEMORY_ASSERT(timer_recv_manager);
    rt_timer_start(timer_recv_manager);

    while (0 != current_task_state)
    {
        if (QUEUE_EOK != manager_recv_queue(operation_queue_handle, (void *)&page, UC_QUEUE_WAITING_FOREVER))
            continue;

        message = page->message;
        data = message->data;

        rt_kprintf("manager_operation cmd 0x%x\n", message->cmd);
        switch (message->cmd)
        {
        case MANAGER_OPERATION_MSG:
        {
            rt_sem_take(manager_list_sem, RT_WAITING_FOREVER);

            //test_head_list(&wiota_send_manager_list, "send 1", test_result_callback);
            //insert iote info to wiota_send_manager_list
            insert_tail_manager_list(&wiota_send_manager_list, data);
            //test_head_list(&wiota_send_manager_list, "send 2", test_result_callback);
            rt_sem_release(manager_list_sem);
            rt_kprintf("send data to iote(0x%x). data type %d,data len %d\n", data->head.id, data->head.cmd, data->len);
            //send data to wiota
            uc_wiota_send_data(data->pload, data->len, data->head.id, 10000, manager_send_result);

            break;
        }
        case MANAGER_OPERATION_MSG_RESULT:
        {
            manager_operate_result((uc_send_recv_t *)data);
            rt_free(data);
            break;
        }
        case MANAGER_OPERATION_RADIO:
        {
            if (UC_OP_SUCC != uc_wiota_send_broadcast_data(data->pload, data->len, NORMAL_BROADCAST, 0, uc_send_radio_callback))
            {
                rt_kprintf("MANAGER_OPERATION_RADIO is error\n");
            }
            if (RT_NULL != data->pload)
                rt_free(data->pload);

            if (RT_NULL != data)
                rt_free(data);
            break;
        }
        case MANAGER_OPERATION_OTA_RADIO:
        {
            rt_sem_take(manager_list_sem, RT_WAITING_FOREVER);
            insert_tail_manager_list(&wiota_send_manager_list, data);
            rt_sem_release(manager_list_sem);
            uc_wiota_send_broadcast_data(data->pload, data->len, OTA_BROADCAST, 0, uc_send_radio_ota_callback);
            break;
        }
        case MANAGER_OPERATION_OTA_RADIO_RESULT:
        {
            rt_sem_take(manager_list_sem, RT_WAITING_FOREVER);
            remove_manager_node(&wiota_send_manager_list, RT_NULL, manager_ota_remove_cb);
            rt_sem_release(manager_list_sem);
            if (RT_NULL != data)
                rt_free(data);
            break;
        }
        case MANAGER_OPERATION_EXIT:
        {
            current_task_state = 0;
            break;
        }
        default:
            if (RT_NULL != data)
                rt_free(data);
            break;
        }

        if (RT_NULL != message)
            rt_free(message);
        if (RT_NULL != page)
            rt_free(page);
    }
    rt_timer_stop(timer_recv_manager);
    rt_timer_delete(timer_recv_manager);
    clean_manager_list(&wiota_send_manager_list);
}

void manager_operation_task(void *pPara)
{
    while (1)
    {
#if 1
        // wiota freq, frequency point strategy.
        manager_wiota();
#else
        uc_wiota_init();
        uc_wiota_set_freq_info(108);
        uc_wiota_run();
#endif

        manager_operation();

        // exit wiota
        manager_wiota_exit();
    }
}

#endif
