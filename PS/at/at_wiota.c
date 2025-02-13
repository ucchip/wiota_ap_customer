#include <rtthread.h>
#ifdef RT_USING_AT
#ifdef AT_USING_SERVER
#include <rtdevice.h>
#include <stdio.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "at.h"
#include "ati_prs.h"
#include "quick_connect.h"

#define WIOTA_WAIT_DATA_TIMEOUT 10000
#define WIOTA_SEND_TIMEOUT 60000

const unsigned char CRC_SEND[4][8] = {{1, 1, 4, 4, 4, 0, 0, 0},
                                      {1, 1, 1, 4, 4, 4, 4, 0},
                                      {1, 1, 1, 1, 4, 4, 4, 4},
                                      {1, 1, 1, 1, 4, 4, 4, 4}};

const unsigned int U_FRAME_LEN[4][4] = {{73120, 145696, 291232, 582176},
                                        {73392, 145968, 291504, 582448},
                                        {105888, 211232, 422304, 844320},
                                        {106160, 211504, 422576, 844592}};

typedef enum
{
    AT_WIOTA_ADD_BLACKLIST = 0,
    AT_WIOTA_REMOVE_BLACKLIST = 1,
} at_blacklist_mode_e;

typedef enum
{
    AT_WIOTA_HOPPING_SET_FREQ = 0,
    AT_WIOTA_HOPPING_SET_MODE = 1,
} at_hopping_type_e;

enum at_wiota_log
{
    AT_LOG_CLOSE = 0,
    AT_LOG_OPEN,
    AT_LOG_UART0,
    AT_LOG_UART1,
    AT_LOG_SPI_CLOSE,
    AT_LOG_SPI_OPEN,
    AT_LOG_ISR_CLOSE,
    AT_LOG_ISR_OPEN,
};

enum at_test_communication_command
{
    AT_TEST_COMMAND_DEFAULT = 0,
    AT_TEST_COMMAND_UP_TEST,
    AT_TEST_COMMAND_DOWN_TEST,
    AT_TEST_COMMAND_LOOP_TEST,
    AT_TEST_COMMAND_DATA_MODE,
    AT_TEST_COMMAND_DATA_DOWN,
    AT_TEST_COMMAND_STOP,
};

typedef struct at_test_statistical_data
{
    int type;
    int dev;

    int upcurrentrate;
    int upaverate;
    int upminirate;
    int upmaxrate;

    int downcurrentrate;
    int downaverate;
    int downminirate;
    int downmaxrate;

    int send_fail;
    int recv_fail;
    int msc;
    int power;
    int rssi;
    int snr;
} t_at_test_statistical_data;

#define AT_TEST_COMMUNICATION_HEAD "testMode"
#define AT_TEST_COMMUNICATION_HEAD_LEN 9

typedef struct at_test_communication
{
    char head[AT_TEST_COMMUNICATION_HEAD_LEN];
    //char num; /*1 -- */
    char command;
    //char report; //reserved
    // iote  statistical time
    char timeout;
    char mcs_num;
    short test_len;
    short all_len;
    //short data_len;
    //t_at_test_statistical_data data;
} t_at_test_communication;

typedef enum
{
    AT_TEST_FIRST_COMMAND_SU = 0,
    AT_TEST_FIRST_COMMAND_WAIT_SEND = 1,
} e_at_test_first_succe_flag;

typedef struct iote_info_managerlist
{
    // iote of user id
    unsigned int user_id;
    // recv number data to determine whether it is duplicate data.
    // sending sequence number starts form 1.
    // now default is 0.
    unsigned int send_num;
    // record the number of times it was sent.
    // prevent sending full or timeout, always send data memory is not enough.
    unsigned int send_all_counter;
    // start test ap send the start command to iote.
    // send a successful flag.
    unsigned int send_manager_flag;
    struct iote_info_managerlist *next;
} t_iote_info_managerlist;

typedef struct at_test_data
{
    int type;
    int time;
    int general_report;
    int num;
    int flag;
    short test_data_len;
    int mcs_num;
    unsigned int send_num;
    unsigned int time_delay;
    rt_timer_t test_mode_timer;
    rt_thread_t test_mode_task;
    rt_mq_t test_queue;
    rt_sem_t test_sem;
    char tast_state;
    char clean_parenm_flag;
    t_iote_info_managerlist iote_info_list;
    t_at_test_statistical_data statistical;
} t_at_test_data;

enum at_test_mode_data_type
{
    AT_TEST_MODE_RECVDATA,
    AT_TEST_MODE_QUEUE_EXIT,
};

typedef struct at_test_queue_data
{
    enum at_test_mode_data_type type;
    unsigned int user_id;
    void *data;
} t_at_test_queue_data;

#define AT_TEST_GET_RATE(TIME, NUM, LEN, CURRENT, AVER, MIN, MAX) \
    {                                                             \
        CURRENT = LEN * 1000 / TIME;                              \
        if (AVER == 0)                                            \
        {                                                         \
            AVER = CURRENT;                                       \
        }                                                         \
        else                                                      \
        {                                                         \
            AVER = (AVER * NUM + CURRENT) / (NUM + 1);            \
        }                                                         \
        if (MIN > CURRENT || MIN == 0)                            \
        {                                                         \
            MIN = CURRENT;                                        \
        }                                                         \
        if (MAX < CURRENT || MAX == 0)                            \
        {                                                         \
            MAX = CURRENT;                                        \
        }                                                         \
    }

#define AT_TEST_CALCUTLATE(RESULT, ALL, BASE)                \
    {                                                        \
        if (0 != ALL)                                        \
        {                                                    \
            float get_result = ((float)BASE) / ((float)ALL); \
            get_result = get_result * 100.0;                 \
            RESULT = (int)get_result;                        \
        }                                                    \
        else                                                 \
        {                                                    \
            RESULT = 0;                                      \
        }                                                    \
    }

t_at_test_data g_t_test_data = {0};

static at_result_t at_freq_query(void)
{
    at_server_printfln("+WIOTAFREQ:%u", uc_wiota_get_freq_info());

    return AT_RESULT_OK;
}

static at_result_t at_freq_setup(const char *args)
{
    unsigned int freq = 0;

    args = parse((char *)(++args), "d", &freq);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_freq_info(freq))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

#ifdef ZCRD_CUSTOMER
static unsigned int system_id = 0x11223344;
#endif

static at_result_t at_system_config_query(void)
{
    sub_system_config_t config = {0};
    uc_wiota_get_system_config(&config);
#ifdef ZCRD_CUSTOMER
    at_server_printfln("+WIOTACONFIG:%d,%d,%d,%d,%d,%d,%d,0x%x,0x%x",
                       config.id_len, config.symbol_length, config.dlul_ratio,
                       config.bt_value, config.group_number, config.ap_tx_power,
                       config.spectrum_idx, system_id, config.subsystem_id);
#else
    at_server_printfln("+WIOTACONFIG:%d,%d,%d,%d,%d,%d,%d,%d,%d,0x%x",
                       config.ap_tx_power, config.id_len, config.symbol_length,
                       config.dlul_ratio, config.bt_value, config.group_number, config.spectrum_idx,
                       config.old_subsys_v, config.bitscb, config.subsystem_id);
#endif
    return AT_RESULT_OK;
}

static at_result_t at_system_config_setup(const char *args)
{
    int temp[9] = {0};
    sub_system_config_t config;

    if (uc_wiota_get_state() != WIOTA_STATE_INIT && uc_wiota_get_state() != WIOTA_STATE_RUN)
    {
        rt_kprintf("please init wiota first\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }
    uc_wiota_get_system_config(&config);
#ifdef ZCRD_CUSTOMER
    args = parse((char *)(++args), "d,d,d,d,d,d,d,y,y",
                 &temp[0], &temp[1], &temp[2],
                 &temp[3], &temp[4], &temp[5],
                 &temp[6], &system_id, &config.subsystem_id);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    config.id_len = (unsigned char)temp[0];
    config.symbol_length = (unsigned char)temp[1];
    config.dlul_ratio = (unsigned char)temp[2];
    config.bt_value = (unsigned char)temp[3];
    config.group_number = (unsigned char)temp[4];
    config.ap_tx_power = (unsigned char)temp[5] - 20;
    config.spectrum_idx = (unsigned char)temp[6];
    config.old_subsys_v = 1;
    config.bitscb = 0;
#else
    args = parse((char *)(++args), "d,d,d,d,d,d,d,d,d,y",
                 &temp[0], &temp[1], &temp[2],
                 &temp[3], &temp[4], &temp[5],
                 &temp[6], &temp[7], &temp[8],
                 &config.subsystem_id);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    config.ap_tx_power = (unsigned char)temp[0] - 20;
    config.id_len = (unsigned char)temp[1];
    config.symbol_length = (unsigned char)temp[2];
    config.dlul_ratio = (unsigned char)temp[3];
    config.bt_value = (unsigned char)temp[4];
    config.group_number = (unsigned char)temp[5];
    config.spectrum_idx = (unsigned char)temp[6];
    config.old_subsys_v = (unsigned char)temp[7];
    config.bitscb = (unsigned char)temp[8];
#endif

    config.pp = 1;
    if (0 != uc_wiota_set_system_config(&config))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

void uc_wiota_time_service_state_cb(time_service_state_e state);
void uc_wiota_time_service_info_cb(uc_ts_info_t *ts_info);
static at_result_t at_wiota_init_exec(void)
{
#ifdef ZCRD_CUSTOMER
    uc_wiota_set_frame_boundary_align_func(1);
    uc_wiota_set_time_service_func(TIME_SERVICE_1588_PS, 1);
#endif

    if (uc_wiota_get_state() == WIOTA_STATE_DEFAULT || uc_wiota_get_state() == WIOTA_STATE_EXIT)
    {
        uc_wiota_init();

        return AT_RESULT_OK;
    }

    return AT_RESULT_REPETITIVE_FAILE;
}

void uc_wiota_show_drop_func(unsigned int user_id)
{
    rt_kprintf("0x%08x droped, fn %u\n", user_id, uc_wiota_get_frame_num());
}

static int at_fitering_test_data(unsigned int user_id, unsigned char *recv_data, unsigned short data_len)
{
    t_at_test_communication *communication = (t_at_test_communication *)recv_data;
    unsigned int send_data_address = 0;
    t_at_test_queue_data *queue_data;
    char *copy_recv_data;
    rt_err_t res;

    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;
    while (temp_all_list != RT_NULL)
    {
        if (temp_all_list->user_id == user_id)
        {
            if (temp_all_list->send_num != 0 && rt_strcmp(communication->head, AT_TEST_COMMUNICATION_HEAD))
            {
                if (g_t_test_data.type == AT_TEST_COMMAND_UP_TEST)
                {
                    return 1;
                }
            }

            break;
        }
        temp_all_list = temp_all_list->next;
    }
    // rt_kprintf("%s line %d ap recv id 0x%x head %s\n", __FUNCTION__, __LINE__, user_id, communication->head);
    // check head
    if (0 != rt_strcmp(communication->head, AT_TEST_COMMUNICATION_HEAD))
        return 0;
    // rt_kprintf("%s line %d ap recv id 0x%x command %d\n", __FUNCTION__, __LINE__, user_id, communication->command);

    //send queue data
    copy_recv_data = rt_malloc(data_len);
    if (RT_NULL == copy_recv_data)
    {
        rt_kprintf("%s line %d rt_malloc\n", __FUNCTION__, __LINE__);
        return 1;
    }
    rt_memcpy(copy_recv_data, recv_data, data_len);

    queue_data = rt_malloc(sizeof(t_at_test_queue_data));
    if (RT_NULL == queue_data)
    {
        rt_kprintf("%s line %d rt_malloc\n", __FUNCTION__, __LINE__);
        return 1;
    }
    queue_data->type = AT_TEST_MODE_RECVDATA;
    queue_data->user_id = user_id;
    queue_data->data = copy_recv_data;

    send_data_address = (unsigned int)queue_data;

    res = rt_mq_send(g_t_test_data.test_queue, &send_data_address, 4);
    if (RT_EOK != res)
    {
        rt_kprintf("%s line %d rt_mq_send_wait err %d\n", __FUNCTION__, __LINE__, res);
    }

    return 1;
}

void uc_wiota_show_recv_data(unsigned int user_id, uc_dev_pos_t dev_pos, unsigned char *recv_data, unsigned short data_len, uc_recv_data_type_e type)
{
    if (type == DATA_TYPE_ACCESS)
    {
        if (g_t_test_data.time > 0)
        {
            rt_kprintf("0x%08x accessed time %d\n", user_id, g_t_test_data.time);

            t_iote_info_managerlist *temp_all_list = &g_t_test_data.iote_info_list;
            while (temp_all_list != RT_NULL)
            {
                if (temp_all_list->user_id == user_id)
                    return;

                if (RT_NULL == temp_all_list->next)
                {
                    t_iote_info_managerlist *node = rt_malloc(sizeof(t_iote_info_managerlist));
                    if (node == RT_NULL)
                        return;
                    node->send_num = 1;
                    node->send_all_counter = 0;
                    node->send_manager_flag = AT_TEST_FIRST_COMMAND_WAIT_SEND;
                    node->user_id = user_id;
                    node->next = RT_NULL;
                    temp_all_list->next = node;
                    // rt_kprintf("add new node\n");
                    return;
                }
                else
                {
                    temp_all_list = temp_all_list->next;
                }
            }
        }
        else
        {
            rt_kprintf("0x%08x accessed, fn %u\n", user_id, uc_wiota_get_frame_num());
        }
    }

    if (g_t_test_data.time > 0 && at_fitering_test_data(user_id, recv_data, data_len))
    {
        return;
    }

    at_server_printfln("+WIOTARECV:0x%x,%d,%d:", user_id, type, data_len);
    if (type != DATA_TYPE_SUBF_DATA)
    {
        at_send_data(recv_data, data_len);
        at_server_printf("\r\n");
    }
}

static void uc_wiota_show_recv_data_detail(uc_recv_detail_t *recv_detail)
{
    if (recv_detail->data_type == DATA_TYPE_ACCESS)
    {
        rt_kprintf("0x%08x accessed, fn %u\n", recv_detail->user_id, uc_wiota_get_frame_num());
    }

    at_server_printfln("+WIOTARECV:0x%x,%d,%d,%d,%d,%d,%d,%d,%d,%u:",
                       recv_detail->user_id, recv_detail->data_type, recv_detail->data_len,
                       recv_detail->rssi, recv_detail->delay, recv_detail->fn_cnt,
                       recv_detail->group_idx, recv_detail->burst_idx, recv_detail->slot_idx,
                       recv_detail->frame_num);
    if (recv_detail->data_type != DATA_TYPE_SUBF_DATA)
    {
        at_send_data(recv_detail->data, recv_detail->data_len);
        at_server_printf("\r\n");
    }
}

static void uc_wiota_sync_paging_cb(unsigned int user_id, unsigned char burst_idx, unsigned int fn_index)
{
    at_server_printfln("+WIOTASPCTRL:0x%x,%d,%u", user_id, burst_idx, fn_index);
}

void uc_wiota_register_callback(void)
{
    uc_wiota_register_iote_dropped_callback(uc_wiota_show_drop_func);
    uc_wiota_register_recv_data_callback(uc_wiota_show_recv_data);
    uc_wiota_register_sync_paging_callback(uc_wiota_sync_paging_cb);
}

static at_result_t at_wiota_func_setup(const char *args)
{
    int state = 0;

    args = parse((char *)(++args), "d", &state);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (1 == state && uc_wiota_get_state() == WIOTA_STATE_INIT)
    {
#ifdef ZCRD_CUSTOMER
        uc_wiota_time_service_start();
#else
        if (uc_wiota_get_time_service_state() == TIME_SERVICE_START)
        {
            rt_kprintf("cannot manually run after the time service func is enable\n");
            return AT_RESULT_REPETITIVE_FAILE;
        }
        uc_wiota_run();
        at_server_printfln("+WIOTARUN:OK");
        uc_wiota_register_callback();
#endif
    }
    else if (0 == state && uc_wiota_get_state() == WIOTA_STATE_RUN)
    {
        uc_wiota_exit();
    }
    else
    {
        return AT_RESULT_REPETITIVE_FAILE;
    }
    return AT_RESULT_OK;
}

static at_result_t at_wiota_new_callback_setup(const char *args)
{
    unsigned int new_recv_cb = 0;
    unsigned int new_ts_cb = 0;

    args = parse((char *)(++args), "d,d", &new_recv_cb, &new_ts_cb);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (new_recv_cb)
    {
        uc_wiota_register_recv_data_detail_callback(uc_wiota_show_recv_data_detail);
    }

    if (new_ts_cb)
    {
        uc_wiota_register_time_service_info_callback(uc_wiota_time_service_info_cb);
    }

    return AT_RESULT_OK;
}

static at_result_t at_blacklist_query(void)
{
    uc_blacklist_t *head_node = NULL;
    uc_blacklist_t *curr_node = NULL;
    unsigned short blacklist_num = 0;

    if (uc_wiota_get_state() < WIOTA_STATE_INIT)
    {
        rt_kprintf("please init wiota first\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    head_node = uc_wiota_get_blacklist(&blacklist_num);

    at_server_printfln("+WIOTABLACKLIST:%d", blacklist_num);
    rt_slist_for_each_entry(curr_node, &head_node->node, node)
    {
        at_server_printfln("+WIOTABLACKLIST:0x%x", curr_node->user_id);
    }

    return AT_RESULT_OK;
}

static at_result_t at_blacklist_setup(const char *args)
{
    unsigned int user_id = 0;
    unsigned int mode = 0;

    if (uc_wiota_get_state() < WIOTA_STATE_INIT)
    {
        rt_kprintf("please init wiota first\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "y,d", &user_id, &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode == AT_WIOTA_ADD_BLACKLIST)
    {
        if (0 != uc_wiota_add_iote_to_blacklist(&user_id, 1))
        {
            return AT_RESULT_FAILE;
        }
    }
    else if (mode == AT_WIOTA_REMOVE_BLACKLIST)
    {
        if (0 != uc_wiota_remove_iote_from_blacklist(&user_id, 1))
        {
            return AT_RESULT_FAILE;
        }
    }
    else
    {
        return AT_RESULT_CHECK_FAILE;
    }

    return AT_RESULT_OK;
}

#ifdef WIOTA_IOTE_INFO
static at_result_t at_iote_info_query(void)
{
    uc_iote_info_t *head_node = NULL;
    uc_iote_info_t *curr_node = NULL;
    unsigned short con_num, discon_num;

    if (uc_wiota_get_state() < WIOTA_STATE_INIT)
    {
        rt_kprintf("please init wiota first\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    head_node = uc_wiota_get_iote_info(&con_num, &discon_num);

    at_server_printfln("+WIOTAIOTEINFO:%d,%d", con_num, discon_num);
    rt_slist_for_each_entry(curr_node, &head_node->node, node)
    {
        at_server_printfln("+WIOTAIOTEINFO:0x%x,%d,%d/%d", curr_node->user_id, curr_node->iote_status, curr_node->group_idx, curr_node->subf_idx);
    }
    return AT_RESULT_OK;
}
#endif

static at_result_t at_active_time_query(void)
{
    at_server_printfln("+WIOTAACTIVETIME:%u", uc_wiota_get_active_time());

    return AT_RESULT_OK;
}

static at_result_t at_active_time_setup(const char *args)
{
    unsigned int active_time = 0;

    if (uc_wiota_get_state() < WIOTA_STATE_INIT)
    {
        rt_kprintf("please init wiota first\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "d", &active_time);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_active_time(active_time))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static rt_err_t get_char_timeout(rt_tick_t timeout, char *chr)
{
    char ch;
    rt_err_t result;
    at_server_t at_server = at_get_server();

    while (rt_device_read(at_server->device, 0, &ch, 1) == 0)
    {
        rt_sem_control(at_server->rx_notice, RT_IPC_CMD_RESET, RT_NULL);
        if ((result = rt_sem_take(at_server->rx_notice, timeout)) != RT_EOK)
        {
            return result;
        }
    }

    if (at_server->echo_mode)
    {
        at_server_printf("%c", ch);
    }

    *chr = ch;
    return RT_EOK;
}

static void uc_wiota_send_mc_callback(uc_send_recv_t *send_result)
{
    at_server_printfln("+WIOTAMC:%d,0x%x,0x%x", send_result->result, send_result->user_id, send_result->data_id);
}

static at_result_t at_multicast_setup(const char *args)
{
    unsigned int data_len = 0;
    unsigned int mc_id = 0;
    unsigned int data_id = 0;
    unsigned int is_block = 1;
    signed int timeout = 0;
    unsigned char *send_buf = NULL;
    uc_result_e result = UC_OP_FAIL;
    unsigned char ret = AT_RESULT_OK;

#ifdef ZCRD_CUSTOMER
    args = parse((char *)(++args), "d,y,d", &data_len, &mc_id, &timeout);
#else
    args = parse((char *)(++args), "y,d,y,d,d", &data_id, &data_len, &mc_id, &timeout, &is_block);
#endif
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (data_len > 0)
    {
        send_buf = (unsigned char *)rt_malloc(data_len);
        if (send_buf == NULL)
        {
            rt_kprintf("rt_malloc failed!\n");
            return AT_RESULT_FAILE;
        }
        rt_memset(send_buf, 0, data_len);

        at_server_printfln(">");
        if (at_server_recv(at_get_server(), (char *)send_buf, data_len, rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT)) != data_len)
        {
            rt_kprintf("at_server_recv fail\n");
            rt_free(send_buf);
            return AT_RESULT_FAILE;
        }

        result = uc_wiota_send_multicast_data(send_buf,
                                              data_len,
                                              mc_id,
                                              timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT,
                                              is_block ? RT_NULL : uc_wiota_send_mc_callback,
                                              (void *)data_id);
        rt_free(send_buf);
        send_buf = NULL;
        if (is_block)
        {
            if (UC_OP_SUCC == result)
            {
#ifdef ZCRD_CUSTOMER
                at_server_printf("send mc suc\n");
#else
                at_server_printfln("+WIOTAMC:%d,0x%x,0x%x", result, mc_id, data_id);
#endif
                ret = AT_RESULT_OK;
            }
            else
            {
#ifdef ZCRD_CUSTOMER
                at_server_printf("send mc failed or timeout %d\n", result);
#else
                at_server_printfln("+WIOTAMC:%d,0x%x,0x%x", result, mc_id, data_id);
#endif
                ret = AT_RESULT_FAILE;
            }
        }
    }

    return ret;
}

static void uc_wiota_send_bc_callback(uc_send_recv_t *send_result)
{
    at_server_printfln("+WIOTABC:%d,0x%x", send_result->result, send_result->data_id);
}

static at_result_t at_broadcast_setup(const char *args)
{
    unsigned int data_len = 0;
    unsigned int data_id = 0;
    unsigned int is_block = 1;
    unsigned int mode = 0;
    signed int timeout = 0;
    unsigned char *send_buf = NULL;
    unsigned char ret = AT_RESULT_OK;

#ifdef ZCRD_CUSTOMER
    args = parse((char *)(++args), "d,d,d", &data_len, &mode, &timeout);
#else
    args = parse((char *)(++args), "y,d,d,d,d", &data_id, &data_len, &mode, &timeout, &is_block);
#endif
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (data_len > 0)
    {
        send_buf = (unsigned char *)rt_malloc(data_len);
        if (send_buf == NULL)
        {
            rt_kprintf("rt_malloc failed!\n");
            return AT_RESULT_FAILE;
        }
        rt_memset(send_buf, 0, data_len);

        at_server_printfln(">");
        if (at_server_recv(at_get_server(), (char *)send_buf, data_len, rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT)) != data_len)
        {
            rt_kprintf("at_server_recv fail\n");
            rt_free(send_buf);
            return AT_RESULT_FAILE;
        }

        unsigned char res = uc_wiota_send_broadcast_data(send_buf,
                                                         data_len,
                                                         mode,
                                                         timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT,
                                                         is_block ? RT_NULL : uc_wiota_send_bc_callback,
                                                         (void *)data_id);
        rt_free(send_buf);
        send_buf = NULL;
        if (is_block)
        {
            if (UC_OP_SUCC == res)
            {
#ifdef ZCRD_CUSTOMER
                at_server_printf("send bc suc\n");
#else
                at_server_printfln("+WIOTABC:%d,0x%x", res, data_id);
#endif
                ret = AT_RESULT_OK;
            }
            else
            {
                rt_kprintf("send bc failed or timeout %d\n", res);
#ifdef ZCRD_CUSTOMER
                at_server_printf("send bc failed or timeout %d\n", res);
#else
                at_server_printfln("+WIOTABC:%d,0x%x", res, data_id);
#endif
                ret = AT_RESULT_FAILE;
            }
        }
    }

    return ret;
}

static void uc_wiota_send_callback(uc_send_recv_t *send_result)
{
    at_server_printfln("+WIOTASEND:%d,0x%x,0x%x", send_result->result, send_result->user_id, send_result->data_id);
}

static at_result_t at_send_data_setup(const char *args)
{
    unsigned int data_len = 0;
    unsigned int user_id = 0;
    unsigned int data_id = 0;
    unsigned int is_block = 1;
    signed int timeout = 0;
    unsigned char *send_buf = NULL;
    uc_result_e result = UC_OP_FAIL;
    unsigned char ret = AT_RESULT_OK;

#ifdef ZCRD_CUSTOMER
    args = parse((char *)(++args), "d,y,d", &data_len, &user_id, &timeout);
#else
    args = parse((char *)(++args), "y,d,y,d,d", &data_id, &data_len, &user_id, &timeout, &is_block);
#endif
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (data_len > 0)
    {
        send_buf = (unsigned char *)rt_malloc(data_len);
        if (send_buf == NULL)
        {
            rt_kprintf("rt_malloc failed!\n");
            return AT_RESULT_FAILE;
        }
        rt_memset(send_buf, 0, data_len);

        at_server_printfln(">");
        if (at_server_recv(at_get_server(), (char *)send_buf, data_len, rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT)) != data_len)
        {
            rt_kprintf("at_server_recv fail\n");
            rt_free(send_buf);
            return AT_RESULT_FAILE;
        }

        result = uc_wiota_send_data(send_buf,
                                    data_len,
                                    user_id,
                                    timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT,
                                    is_block ? RT_NULL : uc_wiota_send_callback,
                                    (void *)data_id);
        rt_free(send_buf);
        send_buf = NULL;
        if (is_block)
        {
            if (UC_OP_SUCC == result)
            {
#ifdef ZCRD_CUSTOMER
                at_server_printf("send pdu suc\n");
#else
                at_server_printfln("+WIOTASEND:%d,0x%x,0x%x", result, user_id, data_id);
#endif
                ret = AT_RESULT_OK;
            }
            else
            {
#ifdef ZCRD_CUSTOMER
                at_server_printf("send pdu failed or timeout %d\n", result);
#else
                at_server_printfln("+WIOTASEND:%d,0x%x,0x%x", result, user_id, data_id);
#endif
                ret = AT_RESULT_FAILE;
            }
        }
    }

    return ret;
}

static at_result_t at_send_data_order_setup(const char *args)
{
    unsigned int data_len = 0;
    unsigned int user_id = 0;
    unsigned int data_id = 0;
    unsigned int is_block = 1;
    signed int timeout = 0;
    signed int order_business = 0;
    unsigned char *send_buf = NULL;
    uc_result_e result = UC_OP_FAIL;
    unsigned char ret = AT_RESULT_OK;

    args = parse((char *)(++args), "y,d,y,d,d,d", &data_id, &data_len, &user_id, &timeout, &order_business, &is_block);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (data_len > 0)
    {
        send_buf = (unsigned char *)rt_malloc(data_len);
        if (send_buf == NULL)
        {
            rt_kprintf("rt_malloc failed!\n");
            return AT_RESULT_FAILE;
        }
        rt_memset(send_buf, 0, data_len);

        at_server_printfln(">");
        if (at_server_recv(at_get_server(), (char *)send_buf, data_len, rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT)) != data_len)
        {
            rt_kprintf("at_server_recv fail\n");
            rt_free(send_buf);
            return AT_RESULT_FAILE;
        }

        result = uc_wiota_send_data_order(send_buf,
                                          data_len,
                                          user_id,
                                          timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT,
                                          order_business,
                                          is_block ? RT_NULL : uc_wiota_send_callback,
                                          (void *)data_id);
        rt_free(send_buf);
        send_buf = NULL;
        if (is_block)
        {
            if (UC_OP_SUCC == result)
            {
                at_server_printfln("+WIOTASEND:%d,0x%x,0x%x", result, user_id, data_id);
                ret = AT_RESULT_OK;
            }
            else
            {
                at_server_printfln("+WIOTASEND:%d,0x%x,0x%x", result, user_id, data_id);
                ret = AT_RESULT_FAILE;
            }
        }
    }

    return ret;
}

unsigned int uc_nth_power(unsigned int num, unsigned int n)
{
    unsigned int s = 1;

    for (unsigned int i = 0; i < n; i++)
    {
        s *= num;
    }
    return s;
}

static void uc_string_to_int(unsigned int num_len, unsigned int num, const unsigned char *p_start, unsigned char *array)
{
    unsigned char *temp = NULL;
    unsigned int len = 0;
    unsigned int nth = num_len;

    temp = (unsigned char *)rt_malloc(num_len);
    if (temp == NULL)
    {
        rt_kprintf("uc_string_to_int malloc failed\n");
        return;
    }

    for (len = 0; len < num_len; len++)
    {
        temp[len] = p_start[len] - '0';
        array[num] += uc_nth_power(10, nth - 1) * temp[len];
        nth--;
    }
    rt_free(temp);
    temp = NULL;
}

static unsigned int uc_string_to_array(unsigned char *string, unsigned char *array)
{
    unsigned char *p_start = string;
    unsigned char *p_end = string;
    unsigned int num = 0;
    unsigned int num_len = 0;

    while (*p_start != '\0')
    {
        while (*p_end != '\0')
        {
            if (*p_end == ',')
            {
                uc_string_to_int(num_len, num, p_start, array);
                num++;
                p_end++;
                p_start = p_end;
                num_len = 0;
            }
            num_len++;
            p_end++;
        }

        uc_string_to_int(num_len, num, p_start, array);
        num++;
        p_start = p_end;
    }
    return num;
}

static char *uc_str_reverse(char *str)
{
    char temp;
    char *p_start = str;
    char *p_end = str;
    while (*p_end)
        ++p_end;
    p_end--;

    while (p_end > p_start)
    {
        temp = *p_start;
        *p_start++ = *p_end;
        *p_end-- = temp;
    }
    return str;
}

static char *uc_itoa(int num)
{
    int i = 0, is_ng = 0;
    static char str[100];
    if ((is_ng = num) < 0)
    {
        num = -num;
    }
    do
    {
        str[i++] = num % 10 + '0';
        num = num / 10;
    } while (num > 0);

    if (is_ng < 0)
    {
        str[i++] = '-';
    }
    str[i] = '\0';

    return uc_str_reverse(str);
}

static void uc_array_to_string(unsigned char *array, int array_len, unsigned char *string)
{
    if (array == RT_NULL || string == RT_NULL)
    {
        return;
    }

    int str_temp_len = 0;
    int offset = 0;
    unsigned char str_temp[10] = {0};

    for (int i = 0; i < array_len; i++)
    {
        rt_strncpy((char *)str_temp, uc_itoa(array[i]), 10);
        str_temp_len = rt_strlen((char *)str_temp);
        rt_strncpy((char *)string + offset, (char *)str_temp, str_temp_len);
        offset += str_temp_len;
        if (i < array_len - 1)
        {
            rt_strncpy((char *)string + offset, ",", 1);
            offset += 1;
        }
    }
}

static at_result_t at_scan_freq_setup(const char *args)
{
    unsigned int freq_num = 0;
    unsigned int scan_type = 0;
    unsigned char convert_num = 0;
    signed int timeout = 0;
    unsigned char *freq_string = NULL;
    unsigned char *freq_arry = NULL;
    unsigned int data_len = 0;
    unsigned int result_num = 0;
    int conver_buf_len = 0;
    uc_scan_recv_t scan_info = {0};
    unsigned char ret = AT_RESULT_OK;
    unsigned int is_gateway = 0;

#ifdef ZCRD_CUSTOMER
    args = parse((char *)(++args), "d,d,d", &timeout, &data_len, &freq_num);
#else
    args = parse((char *)(++args), "d,d,d,d,d", &timeout, &data_len, &freq_num, &scan_type, &is_gateway);
#endif
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (freq_num > 0)
    {
        freq_string = (unsigned char *)rt_malloc(data_len);
        if (freq_string == NULL)
        {
            rt_kprintf("rt_malloc freq_string failed!\n");
            return AT_RESULT_FAILE;
        }
        rt_memset(freq_string, 0, data_len);

        at_server_printfln(">");
        if (at_server_recv(at_get_server(), (char *)freq_string, data_len, rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT)) != data_len)
        {
            rt_kprintf("at_server_recv fail\n");
            rt_free(freq_string);
            return AT_RESULT_FAILE;
        }

        freq_arry = (unsigned char *)rt_malloc(freq_num * sizeof(unsigned char));
        if (freq_arry == NULL)
        {
            rt_kprintf("rt_malloc freq_arry failed!\n");
            rt_free(freq_string);
            freq_string = NULL;
            return AT_RESULT_FAILE;
        }
        rt_memset(freq_arry, 0, freq_num * sizeof(unsigned char));

        freq_string[data_len - 2] = '\0';

        convert_num = uc_string_to_array(freq_string, freq_arry);
        if (convert_num != freq_num)
        {
            rt_kprintf("convert_num error!\n");
            rt_free(freq_string);
            freq_string = NULL;
            rt_free(freq_arry);
            freq_arry = NULL;
            return AT_RESULT_FAILE;
        }
        rt_free(freq_string);
        freq_string = NULL;

        uc_wiota_scan_freq(freq_arry, freq_num, scan_type, timeout, NULL, &scan_info);

        rt_free(freq_arry);
        freq_arry = NULL;
    }
    else
    {
        uc_wiota_scan_freq(NULL, 0, scan_type, timeout, NULL, &scan_info);
    }

    if (UC_OP_SUCC == scan_info.result)
    {
        uc_scan_freq_t *freq_list = (uc_scan_freq_t *)scan_info.data;
        result_num = scan_info.data_len / sizeof(uc_scan_freq_t);

        if (is_gateway)
        {
            unsigned char *conver_buf = rt_malloc(4096);

            if (conver_buf == NULL)
            {
                rt_free(scan_info.data);
                scan_info.data = NULL;
                rt_kprintf("%s line %d malloc fail\n");
                return AT_RESULT_FAILE;
            }
            rt_memset(conver_buf, 0, 4096);
            uc_array_to_string((unsigned char *)freq_list, scan_info.data_len, conver_buf);
            conver_buf_len = rt_strlen((const char *)conver_buf);
            conver_buf[conver_buf_len] = '\0';

            at_server_printf("+WIOTASCANFREQ,%d,%d,%d:", UC_OP_SUCC, result_num, conver_buf_len + 1);
            at_send_data(conver_buf, conver_buf_len + 1);
            at_server_printf("\r\n");

            rt_free(conver_buf);
            conver_buf = NULL;
        }
        else
        {
            for (unsigned char idx = 0; idx < result_num; idx++)
            {
                at_server_printfln("+WIOTASCANFREQ:%d,%d,%d,%d", freq_list->freq_idx, freq_list->rssi, freq_list->snr, freq_list->is_synced);
                freq_list++;
            }
        }
        rt_free(scan_info.data);
        scan_info.data = NULL;

        ret = AT_RESULT_OK;
    }
    else
    {
        at_server_printfln("+WIOTASCANFREQ:%d,%d,%d", scan_info.result, result_num, conver_buf_len);
        ret = AT_RESULT_FAILE;
    }

    return ret;
}

static at_result_t at_multicast_id_setup(const char *args)
{
    unsigned int mc_id[4] = {0};
    unsigned int mc_id_array[4] = {0};
    unsigned int type = 0;
    unsigned int id_num = 0;

    args = parse((char *)(++args), "d,y,y,y,y", &type, &mc_id[0], &mc_id[1], &mc_id[2], &mc_id[3]);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (type > 1)
    {
        return AT_RESULT_CMD_ERR;
    }

    for (int i = 0; i < 4; i++)
    {
        if (mc_id[i] != 0x0)
        {
            mc_id_array[i] = mc_id[i];
            id_num++;
        }
    }

    if (id_num == 0)
    {
        return AT_RESULT_CMD_ERR;
    }
    if (type == 0)
    {
        if (0 != uc_wiota_set_multicast_id(mc_id_array, id_num))
        {
            return AT_RESULT_FAILE;
        }
    }
    else if (type == 1)
    {
        if (0 != uc_wiota_del_multicast_id(mc_id_array, id_num))
        {
            return AT_RESULT_FAILE;
        }
    }
    else
    {
        return AT_RESULT_CMD_ERR;
    }

    return AT_RESULT_OK;
}

static at_result_t at_freq_list_query(void)
{
    unsigned char freq_list[16] = {0};
    unsigned char freq_list_str[sizeof(unsigned char) * 64] = {0};
    uc_wiota_get_freq_list(freq_list);
    int num = 0;
    for (; num < 16 && 255 != freq_list[num]; num++)
        ;
    uc_array_to_string(freq_list, num, freq_list_str);
    at_server_printfln("+WIOTAFREQLIST:%s\n", freq_list_str);
    return AT_RESULT_OK;
}

static at_result_t at_freq_list_exec(void)
{
    unsigned int data_len = 65;
    unsigned int freq_num = 0;
    unsigned char *freq_string = NULL;
    unsigned char *temp_freq = NULL;
    unsigned char *freq_arry = NULL;
    unsigned int str_len = 0;

    str_len = data_len;
    freq_string = (unsigned char *)rt_malloc(data_len);
    if (freq_string == NULL)
    {
        rt_kprintf("rt_malloc freq_string failed!\n");
        return AT_RESULT_PARSE_FAILE;
    }
    rt_memset(freq_string, 0, data_len);
    temp_freq = freq_string;
    // at_server_printfln("OK");
    at_server_printfln(">");
    while (data_len)
    {
        if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)temp_freq) != RT_EOK)
        {
            rt_kprintf("get char failed!\n");
            rt_free(freq_string);
            freq_string = NULL;
            return AT_RESULT_PARSE_FAILE;
        }

        if (*temp_freq == '\n')
        {
            data_len = 255;
            break;
        }
        data_len--;
        temp_freq++;
    }

    if (data_len != 255)
        return AT_RESULT_PARSE_FAILE;

    data_len = str_len;
    temp_freq = freq_string;
    while (data_len)
    {
        if ((*temp_freq < '0' || *temp_freq > '9') && (*temp_freq != ',') && *temp_freq != '\r' && *temp_freq != '\n')
            return AT_RESULT_PARSE_FAILE;
        if (*temp_freq == '\n')
        {
            temp_freq++;
            break;
        }
        data_len--;
        temp_freq++;
    }

    if (*(temp_freq - 3) == ',')
        return AT_RESULT_PARSE_FAILE;
    *(temp_freq - 2) = '\0';
    *(temp_freq - 1) = '\0';

    freq_arry = (unsigned char *)rt_malloc(32 * sizeof(unsigned char));
    if (freq_arry == NULL)
    {
        rt_kprintf("rt_malloc freq_arry failed!\n");
        rt_free(freq_string);
        freq_string = NULL;
        return AT_RESULT_PARSE_FAILE;
    }
    rt_memset(freq_arry, 0, 32 * sizeof(unsigned char));

    freq_num = uc_string_to_array(freq_string, freq_arry);
    if (freq_num > 16)
    {
        rt_kprintf("freq_num= %d,is greater than 16\n", freq_num);
        rt_free(freq_string);
        rt_free(freq_arry);
        return AT_RESULT_PARSE_FAILE;
    }

    rt_free(freq_string);
    freq_string = NULL;

    uc_wiota_set_freq_list(freq_arry, freq_num);
    uc_wiota_save_static_info(0);
    rt_free(freq_arry);

    return AT_RESULT_OK;
}

static at_result_t at_read_temp_query(void)
{
    uc_temp_recv_t read_temp = {0};

    if (uc_wiota_get_state() != WIOTA_STATE_RUN)
    {
        rt_kprintf("please run wiota first\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    if (UC_OP_SUCC == uc_wiota_read_temperature(NULL, &read_temp, 10000))
    {
        at_server_printfln("+WIOTATEMP:%d", read_temp.temp);
        return AT_RESULT_OK;
    }
    else
    {
        rt_kprintf("read failed or timeout %d\n", read_temp.result);
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_ap_tx_power_setup(const char *args)
{
    signed int rf_power = 0;

    args = parse((char *)(++args), "d", &rf_power);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_ap_tx_power(rf_power - 20))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_ap_tx_power_query(void)
{
    if (uc_wiota_get_state() != WIOTA_STATE_RUN)
    {
        rt_kprintf("please run wiota first\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }
    sub_system_config_t config = {0};
    uc_wiota_get_system_config(&config);

    at_server_printfln("+WIOTAPOW:%d", config.ap_tx_power);
    return AT_RESULT_OK;
}

static at_result_t at_version_query(void)
{
    unsigned char wiota_version_8088[15] = {0};
    unsigned char git_info_8088[36] = {0};
    unsigned char make_time_8088[36] = {0};
    unsigned char wiota_version_8288[15] = {0};
    unsigned char git_info_8288[36] = {0};
    unsigned char make_time_8288[36] = {0};
    unsigned int cce_version = 0;

    uc_wiota_get_version(wiota_version_8088, git_info_8088, make_time_8088, wiota_version_8288, git_info_8288, make_time_8288, &cce_version);
    at_server_printfln("+WIOTAVERSION:%s,%s", wiota_version_8088, wiota_version_8288);
    at_server_printfln("+GITINFO:%s,%s", git_info_8088, git_info_8288);
    at_server_printfln("+TIME:%s,%s", make_time_8088, make_time_8288);
    at_server_printfln("+CCEVERSION:%x", cce_version);

    return AT_RESULT_OK;
}

static at_result_t at_hopping_setup(const char *args)
{
    unsigned int type = 0;
    unsigned int value = 0;
    unsigned int value1 = 0;
    at_result_t result = AT_RESULT_OK;

    args = parse((char *)(++args), "d,d,d", &type, &value, &value1);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    switch (type)
    {
    case AT_WIOTA_HOPPING_SET_FREQ:
        if (0 != uc_wiota_set_hopping_freq(value))
        {
            result = AT_RESULT_FAILE;
        }
        break;

    case AT_WIOTA_HOPPING_SET_MODE:
        if (0 != uc_wiota_set_hopping_mode(value, value1))
        {
            result = AT_RESULT_FAILE;
        }
        break;

    default:
        return AT_RESULT_CMD_ERR;
    }

    return result;
}

static at_result_t at_max_iote_num_query(void)
{
    at_server_printfln("+WIOTAIOTENUM:%d", uc_wiota_get_max_num_of_active_iote());
    return AT_RESULT_OK;
}

static at_result_t at_max_iote_num_setup(const char *args)
{
    unsigned int max_iote_num = 0;

    args = parse((char *)(++args), "d", &max_iote_num);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_max_num_of_active_iote(max_iote_num))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_bc_mcs_query(void)
{
    at_server_printfln("+WIOTABCMCS:%d", uc_wiota_get_broadcast_mcs());
    return AT_RESULT_OK;
}

static at_result_t at_bc_mcs_setup(const char *args)
{
    unsigned int bc_mcs = 0;

    args = parse((char *)(++args), "d", &bc_mcs);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_broadcast_mcs(bc_mcs))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
void at_handle_log_uart(int uart_number)
{
    rt_device_t device = NULL;

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT; /*init default parment*/

    device = rt_device_find(AT_SERVER_DEVICE);

    if (device)
    {
        rt_device_close(device);
    }

    if (0 == uart_number)
    {
        config.baud_rate = BAUD_RATE_460800;
        rt_console_set_device(AT_SERVER_DEVICE);
    }
    else if (1 == uart_number)
    {
        config.baud_rate = BAUD_RATE_115200;
        rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
    }

    if (device)
    {
        rt_device_open(device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
        rt_device_control(device, RT_DEVICE_CTRL_CONFIG, &config);
    }
}
#endif

static at_result_t at_wiotalog_setup(const char *args)
{
    int mode = 0;
    int result = 0;

    args = parse((char *)(++args), "d", &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    switch (mode)
    {
    case AT_LOG_CLOSE:
    case AT_LOG_OPEN:
        result = uc_wiota_log_switch(UC_LOG_UART, mode - AT_LOG_CLOSE);
        break;

    case AT_LOG_UART0:
    case AT_LOG_UART1:
#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
        at_handle_log_uart(mode - AT_LOG_UART0);
#endif
        break;

    case AT_LOG_SPI_CLOSE:
    case AT_LOG_SPI_OPEN:
        result = uc_wiota_log_switch(UC_LOG_SPI, mode - AT_LOG_SPI_CLOSE);
        break;

    case AT_LOG_ISR_CLOSE:
    case AT_LOG_ISR_OPEN:
        result = uc_wiota_log_switch(UC_LOG_DEBUG, mode - AT_LOG_ISR_CLOSE);
        break;

    default:
        return AT_RESULT_FAILE;
    }

    if (result != 0)
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiotacrc_query(void)
{
    at_server_printfln("+WIOTACRC:%d", uc_wiota_get_crc());
    return AT_RESULT_OK;
}

static at_result_t at_wiotacrc_setup(const char *args)
{
    int crc_limit = 0;

    args = parse((char *)(++args), "d", &crc_limit);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_crc(crc_limit))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_read_mem_setup(const char *args)
{
    unsigned int addr = 0;
    unsigned int type = 0;
    unsigned int len = 0;

    args = parse((char *)(++args), "d,y,d", &type, &addr, &len);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    unsigned char *out_buf = rt_malloc(len);
    if (out_buf == RT_NULL)
    {
        return AT_RESULT_FAILE;
    }
    if (0 != uc_wiota_read_value_from_mem(type, addr, len, out_buf))
    {
        rt_free(out_buf);
        out_buf = RT_NULL;
        return AT_RESULT_CMD_ERR;
    }
    at_server_printfln("+WIOTAREADMEM:0x%x", *(unsigned int *)&out_buf[0]);

    for (int i = 0; i < len; i++)
    {
        if (i != 0 && i % 16 == 0)
            rt_kprintf("\n");
        rt_kprintf("0x%02x ", out_buf[i]);
    }
    rt_kprintf("\n");

    rt_free(out_buf);
    out_buf = RT_NULL;

    return AT_RESULT_OK;
}

static at_result_t at_wiota_rate_setup(const char *args)
{
    unsigned int rate_mode = 0;
    unsigned int rate_value = 0;

    args = parse((char *)(++args), "d,d", &rate_mode, &rate_value);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_data_rate(rate_mode, rate_value))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_rate_query_setup(const char *args)
{
    unsigned int rate_mode = 0;

    args = parse((char *)(++args), "d", &rate_mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    unsigned int rate_value = uc_wiota_get_data_rate_value(rate_mode);
    if (rate_value == 0xff) // input rate mode erorr
    {
        return AT_RESULT_CMD_ERR;
    }
    at_server_printfln("+WIOTARATEQUERY:%d", rate_value);

    return AT_RESULT_OK;
}

static void uc_wiota_paging_callback(uc_paging_recv_t *result)
{
    at_server_printfln("+WIOTASP:%d,0x%x", result->result, result->user_id);
}

static at_result_t at_wiota_sync_paging_setup(const char *args)
{
    unsigned int user_id = 0;
    unsigned int fn_index = 0;
    unsigned int detection_period = 0;
    unsigned int send_round = 0;
    unsigned int continue_fn = 0;
    unsigned int is_block = 0;
    uc_paging_info_t paging_info = {0};

    args = parse((char *)(++args), "y,d,d,d,d,d", &user_id, &fn_index, &detection_period, &send_round, &continue_fn, &is_block);
    if (!args)
    {
        rt_kprintf("AT_RESULT_PARSE_FAILE\n");
        return AT_RESULT_PARSE_FAILE;
    }

    paging_info.user_id = user_id;
    paging_info.fn_index = fn_index;
    paging_info.detection_period = detection_period;
    paging_info.send_round = send_round;
    paging_info.continue_fn = continue_fn;

    uc_result_e result = uc_wiota_sync_paging(&paging_info, is_block ? RT_NULL : uc_wiota_paging_callback);
    if (is_block && result != UC_OP_SUCC)
    {
        return AT_RESULT_FAILE;
    }
    return AT_RESULT_OK;
}

static at_result_t at_sync_paging_num_setup(const char *args)
{
    unsigned int group_idx = 0;
    unsigned int subf_idx = 0;

    args = parse((char *)(++args), "d,d", &group_idx, &subf_idx);
    if (!args)
    {
        rt_kprintf("AT_RESULT_PARSE_FAILE\n");
        return AT_RESULT_PARSE_FAILE;
    }

    unsigned char sp_num = uc_wiota_get_sync_paging_num(group_idx, subf_idx);
    if (sp_num == 0xff)
    {
        return AT_RESULT_CMD_ERR;
    }
    at_server_printfln("+WIOTASPNUM:%d", sp_num);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_pos_query_setup(const char *args)
{
    uc_dev_pos_t *dev_pos = RT_NULL;
    unsigned int start_addr = 0;
    unsigned int addr_cnt = 0;
    unsigned int *id_array = RT_NULL;
    unsigned int id_array_len = 0;

    args = parse((char *)(++args), "y,d", &start_addr, &addr_cnt);
    if (!args)
    {
        rt_kprintf("AT_RESULT_PARSE_FAILE\n");
        return AT_RESULT_PARSE_FAILE;
    }
    if (addr_cnt == 0 || start_addr == 0)
    {
        return AT_RESULT_CMD_ERR;
    }
    rt_kprintf("pos_query start_addr 0x%x, addr_cnt %d\n", start_addr, addr_cnt);
    id_array_len = sizeof(unsigned int) * addr_cnt;
    id_array = rt_malloc(id_array_len);
    RT_ASSERT(id_array);
    rt_memset(id_array, 0, id_array_len);

    for (int i = 0; i < addr_cnt; i++)
    {
        id_array[i] = start_addr + i;
    }

    dev_pos = uc_wiota_query_dev_pos_by_userid(id_array, addr_cnt);
    if (dev_pos)
    {
        for (int i = 0; i < addr_cnt; i++)
        {
            at_server_printfln("+POS:%d,%d,%d", dev_pos[i].group_idx, dev_pos[i].burst_idx, dev_pos[i].slot_idx);
        }

        rt_free(id_array);
        id_array = RT_NULL;
        rt_free(dev_pos);
        dev_pos = RT_NULL;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_single_tone_setup(const char *args)
{
    unsigned int is_open = 0;

    args = parse((char *)(++args), "d", &is_open);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_single_tone(is_open))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_bc_fn_cycle_query(void)
{
    at_server_printfln("+WIOTABCFNCYCLE:%d", uc_wiota_get_broadcast_fn_cycle());

    return AT_RESULT_OK;
}

static at_result_t at_wiota_bc_fn_cycle_setup(const char *args)
{
    unsigned int bc_fn_cycle = 0;

    args = parse((char *)(++args), "d", &bc_fn_cycle);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    uc_wiota_set_broadcast_fn_cycle(bc_fn_cycle);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_bc_send_round_query(void)
{
    at_server_printfln("+WIOTABCROUND:%d", uc_wiota_get_broadcast_send_round());

    return AT_RESULT_OK;
}

static at_result_t at_wiota_bc_send_round_setup(const char *args)
{
    unsigned int bc_send_round = 0;

    args = parse((char *)(++args), "d", &bc_send_round);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if (0 != uc_wiota_set_broadcast_send_round(bc_send_round))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_frame_len_query(void)
{
    at_server_printfln("+WIOTAFRAMELEN:%d", uc_wiota_get_frame_len());

    return AT_RESULT_OK;
}

static at_result_t at_wiota_frame_num_query(void)
{
    at_server_printfln("+WIOTAFRAMENUM:%d", uc_wiota_get_frame_num());

    return AT_RESULT_OK;
}

static at_result_t at_wiota_sync_assistant_pps_query(void)
{
    at_server_printfln("+WIOTAPPS:%d", uc_wiota_get_sync_assistant_pps());

    return AT_RESULT_OK;
}

static at_result_t at_wiota_sync_assistant_pps_setup(const char *args)
{
    unsigned int is_open = 0;

    args = parse((char *)(++args), "d", &is_open);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_sync_assistant_pps(is_open))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_iote_leaving_setup(const char *args)
{
    unsigned int user_id = 0;

    args = parse((char *)(++args), "y", &user_id);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_iote_leaving_active_state(&user_id, 1))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_module_id_query(void)
{
    unsigned char module_id[20] = {0};

    uc_wiota_get_module_id(module_id);
    at_server_printfln("+WIOTAMODULEID:%s", module_id);

    return AT_RESULT_OK;
}

static at_result_t at_send_subf_mode_cfg_query(void)
{
    uc_subf_cfg_t subf_cfg = {0};

    uc_wiota_get_subframe_mode_cfg(&subf_cfg);

    at_server_printfln("+WIOTASUBFCFG:%d,%d", subf_cfg.block_size, subf_cfg.send_round);

    return AT_RESULT_OK;
}

static at_result_t at_send_subf_mode_cfg_setup(const char *args)
{
    unsigned int block_size = 0;
    unsigned int send_round = 0;
    uc_subf_cfg_t subf_cfg = {0};

    args = parse((char *)(++args), "d,d", &block_size, &send_round);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    subf_cfg.block_size = block_size;
    subf_cfg.send_round = send_round;

    uc_wiota_set_subframe_mode_cfg(&subf_cfg);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_ul_subf_mode_setup(const char *args)
{
    unsigned int rach_delay = 0;
    unsigned int subf_mode = 0;
    unsigned int user_id = 0;

    args = parse((char *)(++args), "d,y,d", &subf_mode, &user_id, &rach_delay);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_ul_subframe_mode(subf_mode, user_id, rach_delay))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_send_dl_subf_data_setup(const char *args)
{
    unsigned int data_len = 0;
    unsigned int fn = 0;
    unsigned char *subf_data = NULL;
    at_result_t result = AT_RESULT_OK;

    args = parse((char *)(++args), "d,d", &data_len, &fn);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (data_len > 0)
    {
        subf_data = (unsigned char *)rt_malloc(data_len);
        if (subf_data == NULL)
        {
            rt_kprintf("rt_malloc failed!\n");
            return AT_RESULT_FAILE;
        }
        rt_memset(subf_data, 0, data_len);

        at_server_printfln(">");
        if (at_server_recv(at_get_server(), (char *)subf_data, data_len, rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT)) != data_len)
        {
            rt_kprintf("at_server_recv fail\n");
            rt_free(subf_data);
            return AT_RESULT_FAILE;
        }

        if (0 != uc_wiota_add_dl_subframe_data(subf_data, data_len, fn))
        {
            result = AT_RESULT_FAILE;
        }

        rt_free(subf_data);
    }

    return result;
}

static at_result_t at_send_subf_mode_test_setup(const char *args)
{
    unsigned int mode = 0;
    at_result_t result = AT_RESULT_OK;

    args = parse((char *)(++args), "d", &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode >= 0 && mode <= 2)
    {
        if (0 != uc_wiota_set_subframe_test(mode))
        {
            result = AT_RESULT_FAILE;
        }
    }
    else if (mode == 3)
    {
        uc_subf_test_t *test_info = uc_wiota_get_subframe_test();
        if (test_info)
        {
            at_server_printfln("+WIOTASUBFSENDCNT:%u", test_info->send_cnt);
            at_server_printfln("+WIOTASUBFNODENUM:%u", test_info->subf_node_num);
            if (test_info->subf_node && test_info->subf_node_num > 0)
            {
                for (unsigned char i = 0; i < test_info->subf_node_num; i++)
                {
                    at_server_printfln("+WIOTASUBFTEST:0x%x,%u", test_info->subf_node[i].user_id, test_info->subf_node[i].recv_cnt);
                }
                rt_free(test_info->subf_node);
            }
            rt_free(test_info);
        }
    }
    else
    {
        return AT_RESULT_CMD_ERR;
    }

    return result;
}

static at_result_t at_mem_info_query(void)
{
    unsigned int total = 0;
    unsigned int used = 0;
    unsigned int max_used = 0;

    rt_memory_info(&total, &used, &max_used);

    at_server_printfln("+MEMINFO:%d", total - used);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_boost_level05_setup(const char *args)
{
    unsigned int is_open = 0;

    args = parse((char *)(++args), "d", &is_open);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_boost_level_0_5(is_open))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_resend_query(void)
{
    at_server_printfln("+WIOTARESEND:%d", uc_wiota_get_sm_resend_times());

    return AT_RESULT_OK;
}

static at_result_t at_wiota_resend_setup(const char *args)
{
    unsigned int resend_times = 0;

    args = parse((char *)(++args), "d", &resend_times);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_sm_resend_times(resend_times);

    return AT_RESULT_OK;
}

static void uc_wiota_fn_refresh(unsigned int frame_num)
{
    at_server_printfln("+WIOTAFN:%u", frame_num);
}

static at_result_t at_wiota_fn_refresh_cb_exec(void)
{
    uc_wiota_register_fn_refresh_callback(uc_wiota_fn_refresh);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_recv_send_sm_by_fn_setup(const char *args)
{
    unsigned int user_id = 0;
    unsigned int start_recv_fn = 0;
    unsigned int recv_fns = 0;
    unsigned int send_fns = 0;
    unsigned int data_len = 0;
    unsigned int data_id = 0;
    unsigned char *send_buf = NULL;
    recv_send_by_fn_t rs_fn = {0};

    args = parse((char *)(++args), "y,d,y,d,d,d", &data_id, &data_len, &user_id, &start_recv_fn, &recv_fns, &send_fns);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    rs_fn.user_id = user_id;
    rs_fn.start_recv_fn = start_recv_fn;
    rs_fn.recv_fns = recv_fns;

    if (data_len > 0 && send_fns > 0)
    {
        send_buf = (unsigned char *)rt_malloc(data_len);
        if (send_buf == NULL)
        {
            rt_kprintf("rt_malloc failed!\n");
            return AT_RESULT_FAILE;
        }
        rt_memset(send_buf, 0, data_len);

        at_server_printfln(">");
        if (at_server_recv(at_get_server(), (char *)send_buf, data_len, rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT)) != data_len)
        {
            rt_kprintf("at_server_recv fail\n");
            rt_free(send_buf);
            return AT_RESULT_FAILE;
        }

        rs_fn.send_fns = send_fns;
        rs_fn.data_len = data_len;
        rs_fn.data = send_buf;
        rs_fn.callback = uc_wiota_send_callback;
        rs_fn.para = (void *)data_id;
    }
    else
    {
        rs_fn.send_fns = 0;
        rs_fn.data_len = 0;
        rs_fn.data = RT_NULL;
        rs_fn.callback = RT_NULL;
        rs_fn.para = RT_NULL;
    }

    uc_wiota_recv_send_sm_by_fn(&rs_fn);

    if (send_buf)
    {
        rt_free(send_buf);
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_ramp_type_setup(const char *args)
{
    unsigned int ramp_type = 0;

    args = parse((char *)(++args), "d", &ramp_type);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_ramp_type(ramp_type))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_ramp_value_setup(const char *args)
{
    unsigned int ramp_value = 0;

    args = parse((char *)(++args), "d", &ramp_value);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_ramp_value(ramp_value))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_rf_setup(const char *args)
{
    unsigned int rf_ctrl_idx = 0;

    args = parse((char *)(++args), "d", &rf_ctrl_idx);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_rf_ctrl_idx(rf_ctrl_idx))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_aagc_idx_setup(const char *args)
{
    unsigned int aagc_idx = 0;

    args = parse((char *)(++args), "d", &aagc_idx);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_aagc_idx(aagc_idx))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_bnack_query(void)
{
    at_server_printfln("+WIOTABNACK:%d", uc_wiota_get_bnack_func());

    return AT_RESULT_OK;
}

static at_result_t at_wiota_bnack_setup(const char *args)
{
    unsigned int is_open = 0;

    args = parse((char *)(++args), "d", &is_open);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_bnack_func(is_open);

    return AT_RESULT_OK;
}

#ifdef WIOTA_AP_STATE_INFO
static at_result_t at_wiota_state_setup(const char *args)
{
    int get_or_reset = 0;
    int user_id = 0;
    int state_type = 0;

    if (uc_wiota_get_state() < WIOTA_STATE_INIT)
    {
        rt_kprintf("please init wiota first\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "d,y,d", &get_or_reset, &user_id, &state_type);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (get_or_reset == 0) // get
    {
        if (state_type == 0 && user_id == 0x0) // get all state of all iote
        {
            uc_state_info_t *head_node = uc_wiota_get_all_state_info();
            uc_state_info_t *curr_node;

            at_server_printfln("+WIOTASTATE:%d", rt_slist_len((rt_slist_t *)head_node));
            rt_slist_for_each_entry(curr_node, &head_node->node, node)
            {
                at_server_printfln("+WIOTASTATE:0x%x,%d,%d,%d,%d,%d",
                                   curr_node->user_id, curr_node->ul_recv_len, curr_node->ul_recv_suc,
                                   curr_node->dl_send_len, curr_node->dl_send_suc, curr_node->dl_send_fail);
            }
        }
        else if (state_type == 0 && user_id != 0x0) // get all state of single iote
        {
            uc_state_info_t *head_node = uc_wiota_get_all_state_info_of_iote(user_id);
            if (head_node != NULL)
            {
                at_server_printfln("+WIOTASTATE:0x%x,%d,%d,%d,%d,%d",
                                   head_node->user_id, head_node->ul_recv_len, head_node->ul_recv_suc,
                                   head_node->dl_send_len, head_node->dl_send_suc, head_node->dl_send_fail);
            }
        }
        else if (state_type != 0 && user_id != 0x0) // get single state of single iote
        {
            if (state_type < TYPE_UL_RECV_LEN || state_type >= UC_STATE_TYPE_MAX)
            {
                return AT_RESULT_FAILE;
            }
            unsigned int state = uc_wiota_get_single_state_info_of_iote(user_id, state_type);
            at_server_printfln("+WIOTASTATE:0x%x,%d", user_id, state);
        }
        else
        {
            return AT_RESULT_FAILE;
        }
    }
    else if (get_or_reset == 1) // reset
    {
        if (state_type == 0 && user_id == 0) // reset all state of all iote
        {
            uc_wiota_reset_all_state_info();
        }
        else if (state_type == 0 && user_id != 0) // reset all state of single iote
        {
            uc_wiota_reset_all_state_info_of_iote(user_id);
        }
        else if (state_type != 0 && user_id != 0) // reset single state of single iote
        {
            if (state_type < TYPE_UL_RECV_LEN || state_type >= UC_STATE_TYPE_MAX)
            {
                return AT_RESULT_FAILE;
            }
            uc_wiota_reset_single_state_info_of_iote(user_id, state_type);
        }
        else
        {
            return AT_RESULT_FAILE;
        }
    }
    else
    {
        return AT_RESULT_FAILE;
    }
    return AT_RESULT_OK;
}

static void at_test_clean_list(void)
{
    t_iote_info_managerlist *temp_all_list = g_t_test_data.iote_info_list.next;

    while (temp_all_list != RT_NULL)
    {
        rt_free(temp_all_list);
        temp_all_list = temp_all_list->next;
    }
}

static void at_test_get_devid_list(void)
{
    uc_iote_info_t *head_node = RT_NULL;
    t_iote_info_managerlist *temp_all_list = &g_t_test_data.iote_info_list;
    uc_iote_info_t *curr_node = RT_NULL;
    unsigned short con_num, discon_num;

    head_node = uc_wiota_get_iote_info(&con_num, &discon_num);
    rt_slist_for_each_entry(curr_node, &head_node->node, node)
    {
        if (curr_node->iote_status == STATUS_ONLINE)
        {
            t_iote_info_managerlist *node = rt_malloc(sizeof(t_iote_info_managerlist));
            if (node == RT_NULL)
                return;
            // rt_kprintf("disconnect user_id=%x\n", curr_node->user_id);
            //get user_id
            node->send_num = 1;
            node->send_all_counter = 0;
            node->send_manager_flag = AT_TEST_FIRST_COMMAND_WAIT_SEND;
            node->user_id = curr_node->user_id;
            node->next = RT_NULL;
            // add list
            temp_all_list->next = node;
            temp_all_list = temp_all_list->next;
        }
    }

    //rt_free(read_iote_info);
}
void at_test_down_callback(uc_send_recv_t *result)
{
    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;
    // rt_kprintf("======> id %x res %d\n", result->user_id, result->result);

    while (temp_all_list != RT_NULL)
    {
        if (temp_all_list->user_id == result->user_id)
        {
            if (UC_OP_SUCC == result->result)
                temp_all_list->send_manager_flag = AT_TEST_FIRST_COMMAND_SU;
            if (temp_all_list->send_all_counter > 0)
                temp_all_list->send_all_counter--;
            //rt_kprintf("======> id %x res %d have send counter %d\n", result->user_id, result->result, temp_all_list->send_all_counter);
            return;
        }
        temp_all_list = temp_all_list->next;
    }
}

void at_test_loop_callback(uc_send_recv_t *result)
{
    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;
    // rt_kprintf("======> id %x res %d\n", result->user_id, result->result);

    while (temp_all_list != RT_NULL)
    {
        if (temp_all_list->user_id == result->user_id)
        {
            if (UC_OP_SUCC == result->result)
            {
                temp_all_list->send_manager_flag = AT_TEST_FIRST_COMMAND_SU;
            }
            else
            {
                temp_all_list->send_all_counter = 1; // send fail ,resend data.
            }
            // rt_kprintf("%s line %d id %x res %d have send counter %d\n", __FUNCTION__, __LINE__, result->user_id, result->result, temp_all_list->send_all_counter);
            return;
        }
        temp_all_list = temp_all_list->next;
    }
}

static void at_test_report_statistical_fun(unsigned int type, unsigned int userid, t_at_test_statistical_data statistical)
{
    switch (type)
    {

    case AT_TEST_COMMAND_DEFAULT:
        at_server_printfln("+STATISTICS:0x%x,  %dbps,  %dbps,  %dbps,  %dbps,  %d%", userid,
                           statistical.upcurrentrate / 1000 * 8, statistical.upmaxrate / 1000 * 8,
                           statistical.downcurrentrate / 1000 * 8, statistical.downmaxrate / 1000 * 8, statistical.send_fail);
        break;
    case AT_TEST_COMMAND_UP_TEST:
        at_server_printfln("+UP:0x%x,  %dbps,  %dbps  %d%", userid,
                           statistical.upcurrentrate / 1000 * 8, statistical.upmaxrate / 1000 * 8, statistical.send_fail);
        break;
    case AT_TEST_COMMAND_DOWN_TEST:
        at_server_printfln("+DOWN:0x%x,  %dbps,  %dbps  %d%", userid,
                           statistical.downcurrentrate / 1000 * 8, statistical.downmaxrate / 1000 * 8, statistical.send_fail);
        break;
    case AT_TEST_COMMAND_LOOP_TEST:
        at_server_printfln("+LOOP:0x%x,  %dbps,  %dbps,  %dbps,  %dbps  %d%", userid,
                           statistical.upcurrentrate / 1000 * 8, statistical.upmaxrate / 1000 * 8,
                           statistical.downcurrentrate / 1000 * 8, statistical.downmaxrate / 1000 * 8, statistical.send_fail);
        break;
    default:
        break;
    }
}

static void at_test_get_statistical(void)
{
    t_at_test_statistical_data statistical = {0};
    uc_state_info_t *head_node = uc_wiota_get_all_state_info();
    uc_state_info_t *curr_node;

    int send_flag = 0;
    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;

    while (temp_all_list != RT_NULL)
    {
        if (temp_all_list->send_num > 0)
        {
            send_flag = 1;
            break;
        }
        temp_all_list = temp_all_list->next;
    }

    rt_slist_for_each_entry(curr_node, &head_node->node, node)
    {
        // statistical data
        AT_TEST_GET_RATE(g_t_test_data.time, 1, curr_node->ul_recv_len,
                         statistical.upcurrentrate, statistical.upaverate, statistical.upminirate, curr_node->ul_recv_max);
        statistical.upmaxrate = curr_node->ul_recv_max;
        AT_TEST_GET_RATE(g_t_test_data.time, 1, curr_node->dl_send_len,
                         statistical.downcurrentrate, statistical.downaverate, statistical.downminirate, curr_node->dl_send_max);
        statistical.downmaxrate = curr_node->dl_send_max;
        // rt_kprintf("statistical.send_fail = %d\n", statistical.send_fail);
        AT_TEST_CALCUTLATE(statistical.send_fail, curr_node->dl_send_suc + curr_node->dl_send_fail, curr_node->dl_send_fail);
        // rt_kprintf("statistical.send_fail = %d\n", statistical.recv_fail);
        if (send_flag)
            at_test_report_statistical_fun(g_t_test_data.type, curr_node->user_id, statistical);
    }
}

static void at_test_get_statistical_general_report(void)
{
    int send_flag = 0;
    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;

    while (temp_all_list != RT_NULL)
    {
        if (temp_all_list->send_num > 0)
        {
            send_flag = 1;
            break;
        }
        temp_all_list = temp_all_list->next;
    }

    int flag = 0;
    uc_state_info_t state_info = {0};
    uc_state_info_t *head_node = uc_wiota_get_all_state_info();
    uc_state_info_t *curr_node;

    rt_slist_for_each_entry(curr_node, &head_node->node, node)
    {
        if (1 == send_flag)
        {
            flag = 1;
            state_info.ul_recv_len += curr_node->ul_recv_len;
            state_info.dl_send_len += curr_node->dl_send_len;
            state_info.ul_recv_suc += curr_node->ul_recv_suc;
            state_info.dl_send_suc += curr_node->dl_send_suc;
            state_info.dl_send_fail += curr_node->dl_send_fail;
        }
    }

    if (1 == flag)
    {
        AT_TEST_GET_RATE(g_t_test_data.time, g_t_test_data.num, state_info.ul_recv_len,
                         g_t_test_data.statistical.upcurrentrate, g_t_test_data.statistical.upaverate,
                         g_t_test_data.statistical.upminirate, g_t_test_data.statistical.upmaxrate)

        AT_TEST_GET_RATE(g_t_test_data.time, g_t_test_data.num, state_info.dl_send_len,
                         g_t_test_data.statistical.downcurrentrate, g_t_test_data.statistical.downaverate,
                         g_t_test_data.statistical.downminirate, g_t_test_data.statistical.downmaxrate)

        AT_TEST_CALCUTLATE(g_t_test_data.statistical.send_fail, state_info.dl_send_suc + state_info.dl_send_fail, state_info.dl_send_fail)

        AT_TEST_CALCUTLATE(g_t_test_data.statistical.recv_fail, state_info.dl_send_suc, state_info.dl_send_suc - state_info.ul_recv_suc)

        at_test_report_statistical_fun(g_t_test_data.type, 0, g_t_test_data.statistical);

        if (0 != g_t_test_data.statistical.upaverate)
            g_t_test_data.num++;
    }
}

static void at_test_get_statistical_data_loop()
{
    sub_system_config_t config;
    uc_wiota_get_system_config(&config);
    unsigned int only_time = 128 * (1 << config.symbol_length) * 8 * 4 * g_t_test_data.send_num / 1000;
    unsigned int m_time = U_FRAME_LEN[(1 << config.dlul_ratio) - 1][config.symbol_length] * g_t_test_data.send_num / 8 / 1000;

    int send_flag = 0;
    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;

    while (temp_all_list != RT_NULL)
    {
        if (temp_all_list->send_num > 0)
        {
            send_flag = 1;
            break;
        }
        temp_all_list = temp_all_list->next;
    }

    int flag = 0;
    uc_state_info_t state_info = {0};
    uc_state_info_t *head_node = uc_wiota_get_all_state_info();
    uc_state_info_t *curr_node;

    rt_slist_for_each_entry(curr_node, &head_node->node, node)
    {
        if (1 == send_flag)
        {
            flag = 1;
            state_info.ul_recv_len += curr_node->ul_recv_len;
            state_info.dl_send_len += curr_node->dl_send_len;
            state_info.ul_recv_suc += curr_node->ul_recv_suc;
            state_info.dl_send_suc += curr_node->dl_send_suc;
            // statistical data
        }
    }

    uc_wiota_reset_all_state_info();

    if (1 == flag)
    {
        state_info.dl_send_len += (CRC_SEND[config.symbol_length][g_t_test_data.mcs_num]) * g_t_test_data.send_num,
            state_info.ul_recv_len += (CRC_SEND[config.symbol_length][g_t_test_data.mcs_num]) * g_t_test_data.send_num;

        unsigned int thoeretical_rate = state_info.ul_recv_len * 8 * 1000 / only_time;

        AT_TEST_GET_RATE(m_time, g_t_test_data.num, state_info.ul_recv_len,
                         g_t_test_data.statistical.upcurrentrate, g_t_test_data.statistical.upaverate,
                         g_t_test_data.statistical.upminirate, g_t_test_data.statistical.upmaxrate)

        AT_TEST_GET_RATE(m_time, g_t_test_data.num, state_info.dl_send_len,
                         g_t_test_data.statistical.downcurrentrate, g_t_test_data.statistical.downaverate,
                         g_t_test_data.statistical.downminirate, g_t_test_data.statistical.downmaxrate)

        AT_TEST_CALCUTLATE(g_t_test_data.statistical.recv_fail, state_info.dl_send_suc, state_info.dl_send_suc - state_info.ul_recv_suc)

        at_server_printfln("+DATA:0x%x,  %dbps,  %dbps,  %dbps,  %dbps,  %dbps,  %d%", 0,
                           g_t_test_data.statistical.upcurrentrate * 8, g_t_test_data.statistical.upmaxrate * 8,
                           g_t_test_data.statistical.downcurrentrate * 8, g_t_test_data.statistical.downmaxrate * 8, thoeretical_rate, g_t_test_data.statistical.recv_fail);
    }
}

static void at_test_mode_time_fun(void *parameter)
{
    if (1 == g_t_test_data.general_report)
        at_test_get_statistical_general_report();
    else
        at_test_get_statistical();
    g_t_test_data.clean_parenm_flag = 1;
}

static void at_test_send_to_iote(t_at_test_communication *data)
{
    if (1 == g_t_test_data.flag || g_t_test_data.type == AT_TEST_COMMAND_DATA_DOWN)
    {
        // rt_kprintf("%s line %d data->all_len: %d  g_t_test_data.mcs_num: %d\n", __FUNCTION__, __LINE__, data->all_len, g_t_test_data.mcs_num);
        //rt_tick_t data_reserved;
        //data_reserved = rt_tick_get();
        //rt_memcpy(&data->reserved + 4, &data_reserved, sizeof(rt_tick_t));
        factory_test_handle_loop_msg(g_t_test_data.mcs_num, g_t_test_data.send_num, g_t_test_data.test_data_len);
        if (g_t_test_data.type == 4)
            at_test_get_statistical_data_loop();
        // rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
        return;
    }

    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;
    while (temp_all_list != RT_NULL)
    {
        if (0 == temp_all_list->send_num)
        {
            temp_all_list = temp_all_list->next;
            continue;
        }

        if (4 == g_t_test_data.type)
        {
            if (0 == factory_test_get_loop_is_rach())
            {
                continue;
            }
        }
        if (temp_all_list->send_manager_flag == AT_TEST_FIRST_COMMAND_WAIT_SEND)
        {
            uc_result_e res;
            // rt_kprintf("%s line %d command %d timeout %d id 0x%x\n", __FUNCTION__, __LINE__, data->command, data->timeout, temp_all_list->user_id);
            res = uc_wiota_send_data((unsigned char *)data, sizeof(t_at_test_communication), temp_all_list->user_id, 60000, RT_NULL, RT_NULL);
            // rt_kprintf("%s line %d ap send command result %d\n", __FUNCTION__, __LINE__, res);
            if (UC_OP_SUCC == res)
            {
                temp_all_list->send_manager_flag = AT_TEST_FIRST_COMMAND_SU;
                if (AT_TEST_COMMAND_DATA_MODE == g_t_test_data.type)
                {
                    uc_wiota_reset_all_state_info();
                    // rt_kprintf("%s line %d ap send command result\n", __FUNCTION__, __LINE__);
                    g_t_test_data.flag = 1;
                    return;
                }
            }
        }
        else if (g_t_test_data.type == AT_TEST_COMMAND_DOWN_TEST)
        {
            // rt_kprintf("%s line %d command %d timeout %d id 0x%x send_all_counter %d\n", __FUNCTION__, __LINE__, data->command, data->timeout, temp_all_list->user_id, temp_all_list->send_all_counter);
            if (temp_all_list->send_all_counter < 3)
            {
                temp_all_list->send_num = 1;
                unsigned char *rand_data = factory_test_gen_rand_data(data->test_len);
                uc_wiota_send_data((unsigned char *)rand_data, data->test_len, temp_all_list->user_id, 60000, at_test_down_callback, RT_NULL);
                rt_free(rand_data);
                temp_all_list->send_all_counter++;
            }
            else
            {
                temp_all_list->send_num++;
            }

            if (temp_all_list->send_num > 50000)
                temp_all_list->send_num = 0;
        }
        else if (g_t_test_data.type == AT_TEST_COMMAND_LOOP_TEST)
        {
            if (temp_all_list->send_all_counter)
            {
                //unsigned char *rand_data = factory_test_gen_rand_data(data->all_len);
                //uc_wiota_send_data((unsigned char *)rand_data, data->all_len, temp_all_list->user_id, 60000, at_test_loop_callback);
                //rt_free(rand_data);
                uc_wiota_send_data((unsigned char *)data, sizeof(t_at_test_communication), temp_all_list->user_id, 60000, at_test_loop_callback, RT_NULL);
                temp_all_list->send_all_counter = 0;
            }
        }
        temp_all_list = temp_all_list->next;
    }
}
void at_test_clean_loop_flag(unsigned int user_id)
{
    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;
    while (temp_all_list != RT_NULL)
    {
        if (user_id == temp_all_list->user_id)
        {
            temp_all_list->send_all_counter = 1;
            break;
        }
        temp_all_list = temp_all_list->next;
    }
}

static void at_test_mode_task_fun(void *parameter)
{
    unsigned int queue_data = 0;
    t_at_test_queue_data *pqueue_data = RT_NULL;

    t_at_test_communication *communication = rt_malloc(sizeof(t_at_test_communication));

    if (4 == g_t_test_data.type)
    {
        factory_test_set_save_loop_id_flag(TRUE);
    }

    uc_wiota_reset_all_state_info();

    at_test_get_devid_list();
    rt_memset(communication->head, 0, AT_TEST_COMMUNICATION_HEAD_LEN);
    rt_memcpy(communication->head, AT_TEST_COMMUNICATION_HEAD, rt_strlen(AT_TEST_COMMUNICATION_HEAD));
    communication->command = g_t_test_data.type;
    communication->timeout = g_t_test_data.time;
    communication->mcs_num = g_t_test_data.mcs_num;

    communication->test_len = g_t_test_data.test_data_len;
    communication->all_len = sizeof(t_at_test_communication) > communication->test_len ? sizeof(t_at_test_communication) : communication->test_len;

    if (g_t_test_data.type != AT_TEST_COMMAND_DATA_MODE && g_t_test_data.type != AT_TEST_COMMAND_DATA_DOWN)
    {
        g_t_test_data.test_mode_timer = rt_timer_create("teMode",
                                                        at_test_mode_time_fun,
                                                        RT_NULL,
                                                        g_t_test_data.time * 1000,
                                                        RT_TIMER_FLAG_PERIODIC);
        if (RT_NULL == g_t_test_data.test_mode_timer)
        {
            rt_kprintf("%s line %d rt_timer_create error\n", __FUNCTION__, __LINE__);
            return;
        }
        rt_timer_start(g_t_test_data.test_mode_timer);
    }

    while (1)
    {
        //rt_kprintf("%s line %d heap size %d\n", __FUNCTION__, __LINE__, uc_heap_size());
        //recv queue data from timer or wiota callback
        if (RT_EOK == rt_mq_recv(g_t_test_data.test_queue, &queue_data, 4, g_t_test_data.flag == 1 ? 0 : 200)) // RT_WAITING_NO
        {
            pqueue_data = (t_at_test_queue_data *)queue_data;
            rt_kprintf("queue recv data type = %d\n", pqueue_data->type);
            switch ((int)pqueue_data->type)
            {
            case AT_TEST_MODE_RECVDATA:
            {
                t_at_test_communication *recv_communication = pqueue_data->data;

                if (AT_TEST_COMMAND_LOOP_TEST == recv_communication->command)
                {
                    at_test_clean_loop_flag(pqueue_data->user_id);
                }
                else if (AT_TEST_COMMAND_DEFAULT == recv_communication->command)
                {
                    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;
                    while (temp_all_list != RT_NULL)
                    {
                        if (pqueue_data->user_id == temp_all_list->user_id)
                        {
                            temp_all_list->send_manager_flag = AT_TEST_FIRST_COMMAND_WAIT_SEND;
                            temp_all_list->send_num = 1;
                            break;
                        }
                        temp_all_list = temp_all_list->next;
                    }
                }
                else if (AT_TEST_COMMAND_STOP == recv_communication->command)
                {
                    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;
                    while (temp_all_list != RT_NULL)
                    {
                        if (pqueue_data->user_id == temp_all_list->user_id)
                        {
                            temp_all_list->send_num = 0;
                            break;
                        }
                        temp_all_list = temp_all_list->next;
                    }
                }
                // free buffer
                rt_free(recv_communication);
                rt_free(pqueue_data);
                break;
            }
            case AT_TEST_MODE_QUEUE_EXIT:
            {
                rt_free(pqueue_data);
                rt_free(communication);
                rt_sem_release(g_t_test_data.test_sem);
                return;
            }
            }
        }

        if (g_t_test_data.clean_parenm_flag)
        {
            uc_wiota_reset_all_state_info();
            g_t_test_data.clean_parenm_flag = 0;
        }

        // send data
        if (AT_TEST_COMMAND_DEFAULT != g_t_test_data.type)
        {
            at_test_send_to_iote(communication);
        }
    }
}

static at_result_t at_test_mode_start(const char *args)
{
    g_t_test_data.flag = 0;
    if (g_t_test_data.time > 0)
        return AT_RESULT_PARSE_FAILE;

    if (uc_wiota_get_state() != WIOTA_STATE_RUN)
    {
        // rt_kprintf("at_test_mode_start wiota_state error\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }
    // parse at parament
    args = parse((char *)(++args), "d,d,d,d,d,d", &g_t_test_data.type, &g_t_test_data.time,
                 &g_t_test_data.general_report, &g_t_test_data.test_data_len, &g_t_test_data.mcs_num, &g_t_test_data.send_num);
    if (!args)
    {
        // rt_kprintf("at_req_parse_args wiota_state error\n");
        return AT_RESULT_PARSE_FAILE;
    }

    if (g_t_test_data.mcs_num < 0)
        g_t_test_data.mcs_num = 0;
    if (g_t_test_data.mcs_num > 8)
        g_t_test_data.mcs_num = 8;

    if (g_t_test_data.send_num < 10)
        g_t_test_data.send_num = 10;

    if (g_t_test_data.type < 4)
    {
        if (g_t_test_data.test_data_len > 300)
            g_t_test_data.test_data_len = 300;
        if (g_t_test_data.test_data_len < 26)
            g_t_test_data.test_data_len = 26;
    }

    // rt_kprintf("type = %d, timeout = %d\n", g_t_test_data.type, g_t_test_data.time);

    if (g_t_test_data.type > 5)
    {
        // rt_kprintf("at_test_mode_start type>4 is error\n");
        return AT_RESULT_PARSE_FAILE;
    }

    if (g_t_test_data.time < 1)
        g_t_test_data.time = 3;

    // create sem
    g_t_test_data.test_sem = rt_sem_create("teMode", 0, RT_IPC_FLAG_PRIO);
    if (RT_NULL == g_t_test_data.test_sem)
    {
        rt_kprintf("%s line %d rt_sem_create error\n", __FUNCTION__, __LINE__);
        rt_memset(&g_t_test_data, 0, sizeof(g_t_test_data));
        return AT_RESULT_PARSE_FAILE;
    }

    // create queue
    g_t_test_data.test_queue = rt_mq_create("teMode", 4, 8, RT_IPC_FLAG_PRIO);
    if (RT_NULL == g_t_test_data.test_queue)
    {
        rt_kprintf("%s line %d at_create_queue error\n", __FUNCTION__, __LINE__);
        rt_sem_delete(g_t_test_data.test_sem);
        rt_memset(&g_t_test_data, 0, sizeof(g_t_test_data));
        return AT_RESULT_PARSE_FAILE;
    }

    //create task
    g_t_test_data.test_mode_task = rt_thread_create("teMode",
                                                    at_test_mode_task_fun,
                                                    RT_NULL,
                                                    2048,
                                                    RT_THREAD_PRIORITY_MAX / 3 - 1,
                                                    3);
    if (RT_NULL == g_t_test_data.test_mode_task)
    {
        rt_kprintf("%s line %d rt_thread_create error\n", __FUNCTION__, __LINE__);
        rt_sem_delete(g_t_test_data.test_sem);
        rt_mq_delete(g_t_test_data.test_queue);
        rt_memset(&g_t_test_data, 0, sizeof(g_t_test_data));
        return AT_RESULT_PARSE_FAILE;
    }
    rt_thread_startup(g_t_test_data.test_mode_task);

    return AT_RESULT_OK;
}

static at_result_t at_test_mode_stop_exec(void)
{
    unsigned int send_data_address;
    t_at_test_queue_data *data;
    if (g_t_test_data.time == 0) //|| g_t_test_data.test_mode_timer == RT_NULL
    {
        // rt_kprintf("%s line %d no start\n", __FUNCTION__, __LINE__);
        return AT_RESULT_FAILE;
    }

    data = rt_malloc(sizeof(t_at_test_queue_data));
    if (RT_NULL == data)
    {
        rt_kprintf("%s line %d rt_malloc error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }
    send_data_address = (unsigned int)data;

    data->type = AT_TEST_MODE_QUEUE_EXIT;

    // send queue data
    rt_mq_send_wait(g_t_test_data.test_queue, &send_data_address, 4, 1000);
    // wait sem 1s
    if (RT_EOK != rt_sem_take(g_t_test_data.test_sem, 190000))
    {
        rt_kprintf("%s line %d rt_sem_take error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }
    //del sem
    if (RT_NULL != g_t_test_data.test_sem)
        rt_sem_delete(g_t_test_data.test_sem);
    // delet timer
    if (g_t_test_data.test_mode_timer != RT_NULL)
    {
        rt_timer_stop(g_t_test_data.test_mode_timer);
        rt_timer_delete(g_t_test_data.test_mode_timer);
    }
    // rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    // del queue
    if (RT_NULL != g_t_test_data.test_queue)
        rt_mq_delete(g_t_test_data.test_queue);
    // del task
    if (RT_NULL != g_t_test_data.test_mode_task)
        rt_thread_delete(g_t_test_data.test_mode_task);
    at_test_clean_list();

    //clean test parament
    rt_memset(&g_t_test_data, 0, sizeof(g_t_test_data));

    return AT_RESULT_OK;
}
#endif

#ifdef WIOTA_BC_MODE_TEST
extern void mng_set_bc_mode(unsigned char mode);
static at_result_t at_wiota_bcmode_setup(const char *args)
{
    unsigned int mode = 0;

    args = parse((char *)(++args), "d", &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    mng_set_bc_mode(mode);

    return AT_RESULT_OK;
}
#endif

static at_result_t at_wiota_frame_boundary_align_setup(const char *args)
{
    unsigned int state = 0;

    args = parse((char *)(++args), "d", &state);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_frame_boundary_align_func(state);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_time_service_cycle_setup(const char *args)
{
    unsigned int cycle_min = 0;

    args = parse((char *)(++args), "d", &cycle_min);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_time_service_cycle(cycle_min);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_time_service_cycle_query(void)
{
    at_server_printfln("+WIOTATSCYCLE:%d", uc_wiota_get_time_service_cycle());

    return AT_RESULT_OK;
}

void uc_wiota_time_service_state_cb(time_service_state_e state)
{
#ifndef ZCRD_CUSTOMER
    const char *str[7] = {"TS NULL",
                          "TS START",
                          "TS SUC",
                          "TS FAIL",
                          "TS INIT END",
                          "TS ALIGN END",
                          "TS STOP"};

    at_server_printfln("%s", str[state]);
#endif
}

void uc_wiota_time_service_info_cb(uc_ts_info_t *ts_info)
{
    char longitude_str[20] = {0};
    char latitude_str[20] = {0};
    char altitude_str[20] = {0};

    // float to string
    snprintf(longitude_str, sizeof(longitude_str), "%.*f", 4, ts_info->longitude);
    snprintf(latitude_str, sizeof(latitude_str), "%.*f", 4, ts_info->latitude);
    snprintf(altitude_str, sizeof(altitude_str), "%.*f", 4, ts_info->altitude);

    at_server_printfln("+WIOTATSINFO:%d,%u,%d,%u,%u,%d,%d,%d,%s,%s,%s",
                       ts_info->ts_state,
                       ts_info->frame_head, ts_info->frame_head_offset,
                       ts_info->cur_time_s, ts_info->cur_time_us,
                       ts_info->pos_x, ts_info->pos_y, ts_info->pos_z,
                       longitude_str, latitude_str, altitude_str);
}

static at_result_t at_wiota_time_service_setup(const char *args)
{
    unsigned int state = 0;
    unsigned char func_gnss = uc_wiota_get_time_service_func(TIME_SERVICE_GNSS);
    unsigned char func_1588 = uc_wiota_get_time_service_func(TIME_SERVICE_1588_PS);
    unsigned char func_sync = uc_wiota_get_time_service_func(TIME_SERVICE_SYNC_ASSISTANT);

    args = parse((char *)(++args), "d", &state);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (func_gnss != 1 && func_1588 != 1 && func_sync != 1)
    {
        return AT_RESULT_FAILE;
    }

    if (state == 1 && ((uc_wiota_get_state() == WIOTA_STATE_INIT) || (uc_wiota_get_state() == WIOTA_STATE_RUN)))
    {
        if (func_gnss || func_1588)
        {
            uc_wiota_register_time_service_state_callback(uc_wiota_time_service_state_cb);
        }
        uc_wiota_time_service_start();
    }
    else if (state == 0 && uc_wiota_get_time_service_state() != TIME_SERVICE_NULL)
    {
        uc_wiota_time_service_stop();
    }
    else
    {
        return AT_RESULT_FAILE;
    }
    return AT_RESULT_OK;
}

static at_result_t at_wiota_time_setup(const char *args)
{
    unsigned int sec = 0;
    unsigned int usec = 0;

    args = parse((char *)(++args), "d,d", &sec, &usec);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if ((sec == 0) && (usec == 0))
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_1588_protocol_rtc(sec, usec);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_bc_utc_setup(const char *args)
{
    unsigned int is_bc_utc = 0;

    args = parse((char *)(++args), "d", &is_bc_utc);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_broadcast_utc_func(is_bc_utc);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_time_service_mode_setup(const char *args)
{
    unsigned int state = 0;
    unsigned int type = 0;

    args = parse((char *)(++args), "d,d", &type, &state);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 != uc_wiota_set_time_service_func(type, state))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_time_service_mode_query(void)
{
    unsigned char func_gnss = uc_wiota_get_time_service_func(TIME_SERVICE_GNSS);
    unsigned char func_1588 = uc_wiota_get_time_service_func(TIME_SERVICE_1588_PS);
    unsigned char func_sync = uc_wiota_get_time_service_func(TIME_SERVICE_SYNC_ASSISTANT);

    at_server_printfln("+WIOTATSMODE:%d,%d,%d", func_gnss, func_1588, func_sync);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_time_service_state_query(void)
{
    at_server_printfln("+WIOTATSSTATE:%d", uc_wiota_get_time_service_state());
    return AT_RESULT_OK;
}

static at_result_t at_wiota_gnss_pos_query(const char *args)
{
    unsigned int type = 0;

    args = parse((char *)(++args), "d", &type);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (type == 0)
    {
        int pos_x = 0, pos_y = 0, pos_z = 0;

        uc_wiota_gnss_query_coordinate_xyz(&pos_x, &pos_y, &pos_z);
        at_server_printfln("+GNSSPOSQUERY:%d,%d,%d", pos_x, pos_y, pos_z);
    }
    else if (type == 1)
    {
        float longitude = 0, latitude = 0, altitude = 0;
        char longitude_str[20] = {0};
        char latitude_str[20] = {0};
        char altitude_str[20] = {0};

        uc_wiota_gnss_query_coordinate_lla(&longitude, &latitude, &altitude);
        // float to string
        snprintf(longitude_str, sizeof(longitude_str), "%.*f", 4, longitude);
        snprintf(latitude_str, sizeof(latitude_str), "%.*f", 4, latitude);
        snprintf(altitude_str, sizeof(altitude_str), "%.*f", 4, altitude);

        at_server_printfln("+GNSSPOSQUERY:%s,%s,%s", longitude_str, latitude_str, altitude_str);
    }
    else
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiota_gnss_relocation_setup(const char *args)
{
    unsigned int state = 0;

    args = parse((char *)(++args), "d", &state);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_gnss_relocation(state);

    return AT_RESULT_OK;
}

static at_result_t at_paging_tx_config_setup(const char *args)
{
    uc_lpm_tx_cfg_t config = {0};
    unsigned int temp[6];

    args = parse((char *)(++args), "d,d,d,d,d,d",
                 &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_tx_cfg(&config);

    config.freq = (unsigned char)temp[0];
    config.spectrum_idx = (unsigned char)temp[1];
    config.bandwidth = (unsigned char)temp[2];
    config.symbol_length = (unsigned char)temp[3];
    config.awaken_id = (unsigned short)temp[4];
    config.send_time = (unsigned int)temp[5];

    if (0 != uc_wiota_set_paging_tx_cfg(&config))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_paging_tx_config_query(void)
{
    uc_lpm_tx_cfg_t config = {0};

    uc_wiota_get_paging_tx_cfg(&config);

    at_server_printfln("+WIOTAPAGINGTX:%d,%d,%d,%d,%d,%d",
                       config.freq, config.spectrum_idx, config.bandwidth, config.symbol_length, config.awaken_id, config.send_time);

    return AT_RESULT_OK;
}

static at_result_t at_paging_tx_send_exec(void)
{
    if (0 != uc_wiota_paging_tx_start())
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_paging_tx_frame_send_exec(void)
{
    if (0 != uc_wiota_paging_tx_frame_start())
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_paging_config_mode_query(void)
{
    uc_lpm_rx_cfg_t config_rx;
    uc_lpm_tx_cfg_t config_tx;
    uc_wiota_get_paging_rx_cfg(&config_rx);
    uc_wiota_get_paging_tx_cfg(&config_tx);
    at_server_printfln("+WIOTAPAGINGMODE=%d,%d", config_rx.mode, config_tx.mode);
    return AT_RESULT_OK;
}

static at_result_t at_paging_config_mode_setup(const char *args)
{
    uc_lpm_rx_cfg_t config_rx = {0};
    uc_lpm_tx_cfg_t config_tx = {0};
    unsigned int rx_mode, tx_mode;

    args = parse((char *)(++args), "d,d", &rx_mode, &tx_mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_rx_cfg(&config_rx);
    uc_wiota_get_paging_tx_cfg(&config_tx);

    config_rx.mode = rx_mode;
    config_tx.mode = tx_mode;

    if (0 != uc_wiota_set_paging_rx_cfg(&config_rx))
    {
        return AT_RESULT_FAILE;
    }

    if (0 != uc_wiota_set_paging_tx_cfg(&config_tx))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_paging_rx_config_query(void)
{
    uc_lpm_rx_cfg_t config;
    uc_wiota_get_paging_rx_cfg(&config);
    at_server_printfln("+WIOTAPAGINGRX=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                       config.freq, config.spectrum_idx, config.bandwidth,
                       config.symbol_length, config.awaken_id, config.detect_period,
                       config.lpm_nlen, config.lpm_utimes, config.threshold,
                       config.extra_flag, config.extra_period, config.period_multiple, config.awaken_id_another);

    return AT_RESULT_OK;
}

static at_result_t at_paging_rx_config_setup(const char *args)
{
    uc_lpm_rx_cfg_t config = {0};
    unsigned int temp[13];

    args = parse((char *)(++args), "d,d,d,d,d,d,d,d,d,d,d,d,d",
                 &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5], &temp[6],
                 &temp[7], &temp[8], &temp[9], &temp[10], &temp[11], &temp[12]);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_rx_cfg(&config);

    config.freq = (unsigned char)temp[0];
    config.spectrum_idx = (unsigned char)temp[1];
    config.bandwidth = (unsigned char)temp[2];
    config.symbol_length = (unsigned char)temp[3];
    config.awaken_id = (unsigned short)temp[4];
    config.detect_period = (unsigned int)temp[5];
    config.lpm_nlen = (unsigned char)temp[6];
    config.lpm_utimes = (unsigned char)temp[7];
    config.threshold = (unsigned short)temp[8];
    config.extra_flag = (unsigned short)temp[9];
    config.extra_period = (unsigned int)temp[10];
    config.period_multiple = (unsigned int)temp[11];
    config.awaken_id_another = (unsigned int)temp[12];

    if (0 != uc_wiota_set_paging_rx_cfg(&config))
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_paging_rx_enter_setup(const char *args)
{
    unsigned int is_need_32k_div = 0;
    unsigned int timeout_max = 0;

    args = parse((char *)(++args), "d,d", &is_need_32k_div, &timeout_max);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    at_server_printfln("OK");

    uc_wiota_paging_rx_enter(is_need_32k_div, timeout_max);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_awaken_query(void)
{
    uc_awaken_cause_t awaken_cause = {0};

    uc_wiota_get_awakened_cause(&awaken_cause);

    at_server_printfln("+WIOTAAWAKEN:%d,%d,%d,%u,%d",
                       awaken_cause.awaken_cause,
                       awaken_cause.is_cs_awakened,
                       awaken_cause.lpm_last_wakeup_cause,
                       awaken_cause.lpm_detected_times,
                       awaken_cause.lpm_last_wakeup_idx);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_hardware_query(void)
{
    at_server_printfln("+WIOTAHW:%d", uc_wiota_get_is_new_hardware());

    return AT_RESULT_OK;
}

static at_result_t at_wiota_ap8288_state_query(void)
{
    if (uc_wiota_get_state() != WIOTA_STATE_RUN)
    {
        rt_kprintf("please run wiota first\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    at_server_printfln("+WIOTAAPSTATE:%d", uc_wiota_get_ap8288_state());

    return AT_RESULT_OK;
}

static at_result_t at_wiota_quick_start_setup(const char *args)
{
    at_result_t result = AT_RESULT_FAILE;
    unsigned int mode = 0;
    unsigned int freq = 0;
    unsigned char onoff = 0;

    args = parse((char *)(++args), "d,d,d", &onoff, &freq, &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode < QC_MODE_MAX)
    {
        if (onoff == 1)
        {
            if (0 == wiota_quick_connect_start((unsigned char)freq, (qc_config_e)mode))
            {
                result = AT_RESULT_OK;
            }
        }
        else if (onoff == 0)
        {
            wiota_quick_connect_stop();

            result = AT_RESULT_OK;
        }
    }

    return result;
}

AT_CMD_EXPORT("AT+WIOTAINIT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_init_exec);
AT_CMD_EXPORT("AT+WIOTAFREQ", "=<freq_idx>", RT_NULL, at_freq_query, at_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAACTIVETIME", "=<active_time>", RT_NULL, at_active_time_query, at_active_time_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARUN", "=<state>", RT_NULL, RT_NULL, at_wiota_func_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAREGNEWCB", "=<new_recv_cb>,<new_ts_cb>", RT_NULL, RT_NULL, at_wiota_new_callback_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABLACKLIST", "=<user_id>,<mode>", RT_NULL, at_blacklist_query, at_blacklist_setup, RT_NULL);
#ifdef WIOTA_IOTE_INFO
AT_CMD_EXPORT("AT+WIOTAIOTEINFO", RT_NULL, RT_NULL, at_iote_info_query, RT_NULL, RT_NULL);
#endif
AT_CMD_EXPORT("AT+WIOTAMCID", "=<type>,<id0>,<id1>,<id2>,<id3>", RT_NULL, RT_NULL, at_multicast_id_setup, RT_NULL);
#ifdef ZCRD_CUSTOMER
AT_CMD_EXPORT("AT+WIOTACONFIG", "=<id_len>,<symbol_len>,<dlul_ratio>,<bt_value>,<group_num>,<ap_max_pow>,<spectrum_idx>,<system_id>,<subsystem_id>",
              RT_NULL, at_system_config_query, at_system_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAMC", "=<len>,<mc_id>,<timeout>", RT_NULL, RT_NULL, at_multicast_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABC", "=<len>,<mode>,<timeout>", RT_NULL, RT_NULL, at_broadcast_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASEND", "=<len>,<user_id>,<timeout>", RT_NULL, RT_NULL, at_send_data_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASCANFREQ", "=<timeout>,<data_len>,<freq_num>", RT_NULL, RT_NULL, at_scan_freq_setup, RT_NULL);
#else
AT_CMD_EXPORT("AT+WIOTACONFIG", "=<ap_max_pow>,<id_len>,<symbol_len>,<dlul_ratio>,<bt_value>,<group_num>,<spectrum_idx>,<old_subsys_v>,<bitscb>,<subsystem_id>",
              RT_NULL, at_system_config_query, at_system_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAMC", "=<data_id>,<len>,<mc_id>,<timeout>,<is_block>", RT_NULL, RT_NULL, at_multicast_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABC", "=<data_id>,<len>,<mode>,<timeout>,<is_block>", RT_NULL, RT_NULL, at_broadcast_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASEND", "=<data_id>,<len>,<user_id>,<timeout>,<is_block>", RT_NULL, RT_NULL, at_send_data_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASENDORDER", "=<data_id>,<len>,<user_id>,<timeout>,<order_business>,<is_block>", RT_NULL, RT_NULL, at_send_data_order_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASCANFREQ", "=<timeout>,<data_len>,<freq_num>,<scan_type>,<is_gwmode>", RT_NULL, RT_NULL, at_scan_freq_setup, RT_NULL);
#endif
AT_CMD_EXPORT("AT+WIOTASP", "=<user_id>,<fn_index>,<period>,<send_round>,<continue_fn>,<is_block>", RT_NULL, RT_NULL, at_wiota_sync_paging_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASPNUM", "=<group_idx>,<subf_idx>", RT_NULL, RT_NULL, at_sync_paging_num_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPOSQUERY", "=<start_addr>,<addr_cnt>", RT_NULL, RT_NULL, at_wiota_pos_query_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATEMP", RT_NULL, RT_NULL, at_read_temp_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPOW", "=<power>", RT_NULL, at_ap_tx_power_query, at_ap_tx_power_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAVERSION", RT_NULL, RT_NULL, at_version_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAHOPPING", "=<type>,<value>,<value1>", RT_NULL, RT_NULL, at_hopping_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAIOTENUM", "=<max_num>", RT_NULL, at_max_iote_num_query, at_max_iote_num_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABCMCS", "=<bc_mcs>", RT_NULL, at_bc_mcs_query, at_bc_mcs_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTALOG", "=<mode>", RT_NULL, RT_NULL, at_wiotalog_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACRC", "=<crc_limit>", RT_NULL, at_wiotacrc_query, at_wiotacrc_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAREADMEM", "=<type>,<addr>,<len>", RT_NULL, RT_NULL, at_wiota_read_mem_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARATE", "=<rate_mode>,<rate_value>", RT_NULL, RT_NULL, at_wiota_rate_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARATEQUERY", "=<rate_mode>", RT_NULL, RT_NULL, at_wiota_rate_query_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAFREQLIST", RT_NULL, RT_NULL, at_freq_list_query, RT_NULL, at_freq_list_exec);
AT_CMD_EXPORT("AT+WIOTAFBALIGN", "=<state>", RT_NULL, RT_NULL, at_wiota_frame_boundary_align_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATSMODE", "=<type>,<state>", RT_NULL, at_wiota_time_service_mode_query, at_wiota_time_service_mode_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATIME", "=<sec>,<usec>", RT_NULL, RT_NULL, at_wiota_time_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABCUTC", "=<is_bc_utc>", RT_NULL, RT_NULL, at_wiota_bc_utc_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATSCYCLE", "=<cycle_min>", RT_NULL, at_wiota_time_service_cycle_query, at_wiota_time_service_cycle_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATSRUN", "=<state>", RT_NULL, RT_NULL, at_wiota_time_service_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATSSTATE", RT_NULL, RT_NULL, at_wiota_time_service_state_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+GNSSPOSQUERY", "=<type>", RT_NULL, RT_NULL, at_wiota_gnss_pos_query, RT_NULL);
AT_CMD_EXPORT("AT+GNSSRELOCATION", "=<state>", RT_NULL, RT_NULL, at_wiota_gnss_relocation_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPAGINGTX", "=<freq>,<spec_idx>,<band>,<symbol>,<awaken_id>,<send_time>", RT_NULL, at_paging_tx_config_query, at_paging_tx_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASENDPT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_paging_tx_send_exec);
AT_CMD_EXPORT("AT+WIOTASENDPTF", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_paging_tx_frame_send_exec);
AT_CMD_EXPORT("AT+WIOTAPAGINGRX", "=<freq>,<spec_i>,<band>,<symbol>,<aw_id>,<d_period>,<nlen>,<ut>,<thres>,<ex_flag>,<ex_period>,<period_mult>,<aw_id_ano>",
              RT_NULL, at_paging_rx_config_query, at_paging_rx_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPAGINGMODE", "=<rx_mode>,<tx_mode>", RT_NULL, at_paging_config_mode_query, at_paging_config_mode_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASENDPR", "=<is_need_32k_div>,<timeout_max>", RT_NULL, RT_NULL, at_paging_rx_enter_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAAWAKEN", RT_NULL, RT_NULL, at_wiota_awaken_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAHW", RT_NULL, RT_NULL, at_wiota_hardware_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAAPSTATE", RT_NULL, RT_NULL, at_wiota_ap8288_state_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASINGLETONE", "=<is_open>", RT_NULL, RT_NULL, at_wiota_single_tone_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABCFNCYCLE", "=<bc_fn_cycle>", RT_NULL, at_wiota_bc_fn_cycle_query, at_wiota_bc_fn_cycle_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABCROUND", "=<bc_send_round>", RT_NULL, at_wiota_bc_send_round_query, at_wiota_bc_send_round_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAFRAMELEN", RT_NULL, RT_NULL, at_wiota_frame_len_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAFRAMENUM", RT_NULL, RT_NULL, at_wiota_frame_num_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPPS", "=<is_pps>", RT_NULL, at_wiota_sync_assistant_pps_query, at_wiota_sync_assistant_pps_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAIOTELEAV", "=<user_id>", RT_NULL, RT_NULL, at_wiota_iote_leaving_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAMODULEID", RT_NULL, RT_NULL, at_wiota_module_id_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASUBFCFG", "=<block_size>,<send_round>", RT_NULL, at_send_subf_mode_cfg_query, at_send_subf_mode_cfg_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAULSUBF", "=<subf_mode>,<user_id>,<rach_delay>", RT_NULL, RT_NULL, at_wiota_ul_subf_mode_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTADLSUBF", "=<data_len>,<fn>", RT_NULL, RT_NULL, at_send_dl_subf_data_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASUBFTEST", "=<mode>", RT_NULL, RT_NULL, at_send_subf_mode_test_setup, RT_NULL);
AT_CMD_EXPORT("AT+MEMINFO", RT_NULL, RT_NULL, at_mem_info_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABL05", "=<is_open>", RT_NULL, RT_NULL, at_wiota_boost_level05_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARESEND", "=<times>", RT_NULL, at_wiota_resend_query, at_wiota_resend_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAFN", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_fn_refresh_cb_exec);
AT_CMD_EXPORT("AT+WIOTARSBYFN", "=<data_id>,<len>,<user_id>,<start_recv_fn>,<recv_fns>,<send_fns>", RT_NULL, RT_NULL, at_wiota_recv_send_sm_by_fn_setup, RT_NULL);

AT_CMD_EXPORT("AT+WIOTARAMPTYPE", "=<ramp_type>", RT_NULL, RT_NULL, at_wiota_ramp_type_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARAMPVALUE", "=<ramp_value>", RT_NULL, RT_NULL, at_wiota_ramp_value_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASETRF", "=<rf_ctrl_idx>", RT_NULL, RT_NULL, at_wiota_rf_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASETAAGC", "=<aagc_idx>", RT_NULL, RT_NULL, at_wiota_aagc_idx_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABNACK", "=<is_open>", RT_NULL, at_wiota_bnack_query, at_wiota_bnack_setup, RT_NULL);

#ifdef WIOTA_BC_MODE_TEST
AT_CMD_EXPORT("AT+WIOTABCMODE", "=<mode>", RT_NULL, RT_NULL, at_wiota_bcmode_setup, RT_NULL);
#endif
#ifdef WIOTA_AP_STATE_INFO
AT_CMD_EXPORT("AT+WIOTASTATE", "=<get_or_reset>,<user_id>,<state_type>", RT_NULL, RT_NULL, at_wiota_state_setup, RT_NULL);
AT_CMD_EXPORT("AT+THROUGHTSTART", "=<type>,<time>,<general_report>,<test_data_len>,<mcs_num>,<send_num>", RT_NULL, RT_NULL, at_test_mode_start, RT_NULL);
AT_CMD_EXPORT("AT+THROUGHTSTOP", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_test_mode_stop_exec);
#endif

AT_CMD_EXPORT("AT+WIOTAQC", "=<onoff>,<freq>,<mode>", RT_NULL, RT_NULL, at_wiota_quick_start_setup, RT_NULL);

#endif // AT_USING_SERVER
#endif // RT_USING_AT