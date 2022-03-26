#include <rtthread.h>
#include "at.h"
#include "uc_wiota_api.h"
#include "string.h"
#ifdef WIOTA_RECORD_TEST
#include "wiota_record_test.h"
#endif
#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
#include <rtdevice.h>
#endif
#include "ati_prs.h"

#ifdef UC8088_MODULE

#define WIOTA_WAIT_DATA_TIMEOUT 10000
#define WIOTA_SEND_TIMEOUT 60000

#define WIOTA_TEST_AUTO_SEND_SM 0

typedef enum
{
    AT_WIOTA_DEFAULT = 0,
    AT_WIOTA_INIT = 1,
    AT_WIOTA_RUN = 2,
    AT_WIOTA_EXIT = 3,
} at_wiota_state_e;

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
};

enum at_test_communication_command
{
    AT_TEST_COMMAND_DEFAULT = 0,
    AT_TEST_COMMAND_UP_TEST,
    AT_TEST_COMMAND_DOWN_TEST,
    AT_TEST_COMMAND_LOOP_TEST,
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

#define AT_TEST_COMMUNICATION_HEAD "The test mode."
#define AT_TEST_COMMUNICATION_HEAD_LEN 18
#define AT_TEST_COMMUNICATION_RESERVED_LEN 206

typedef struct at_test_communication
{
    char head[AT_TEST_COMMUNICATION_HEAD_LEN];
    char num; /*1 -- */
    char command;
    // 1: iote report statistical_data to ap
    char report;
    // iote  statistical time
    char timeout;
    short all_len;
    short data_len;
    t_at_test_statistical_data data;
    char reserved[AT_TEST_COMMUNICATION_RESERVED_LEN];
} t_at_test_communication;

typedef enum
{
    AT_TEST_FIRST_COMMAND_SU = 0,
    AT_TEST_FIRST_COMMAND_WAIT_SEND = 1,
} e_at_test_first_succe_flag;

typedef struct iote_info_managerlist
{
    // iote of user id
    u32_t user_id;
    // recv number data to determine whether it is duplicate data.
    // sending sequence number starts form 1.
    // now default is 0.
    u32_t recv_num;
    // record the number of times it was sent.
    // prevent sending full or timeout, always send data memory is not enough.
    u32_t send_all_counter;
    // start test ap send the start command to iote.
    // send a successful flag.
    u32_t send_manager_flag;

    struct iote_info_managerlist *next;
} t_iote_info_managerlist;

typedef struct at_test_data
{
    int type;
    int time;
    int iote_reprot;
    rt_timer_t test_mode_timer;
    rt_thread_t test_mode_task;
    rt_mq_t test_queue;
    rt_sem_t test_sem;
    char tast_state;
    t_iote_info_managerlist iote_info_list;
} t_at_test_data;

enum at_test_mode_data_type
{
    AT_TEST_MODE_TIMEOUT,
    AT_TEST_MODE_RECVDATA,
    AT_TEST_MODE_QUEUE_EXIT,
};

typedef struct at_test_queue_data
{
    enum at_test_mode_data_type type;
    u32_t usrid;
    void *data;
} t_at_test_queue_data;

#define AT_TEST_GET_RATE(TIME, LEN, CURRENT, AVER, MIN, MAX) \
    {                                                        \
        CURRENT = LEN / TIME;                                \
        if (AVER == 0)                                       \
            AVER = CURRENT;                                  \
        else                                                 \
            AVER = (AVER + CURRENT) / 2;                     \
        if (MIN > CURRENT || MIN == 0)                       \
            MIN = CURRENT;                                   \
        if (MAX < CURRENT || MAX == 0)                       \
            MAX = CURRENT;                                   \
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

static int wiota_state = AT_WIOTA_DEFAULT;

static at_result_t at_freq_query(void)
{
    at_server_printfln("+WIOTAFREQ=%u", uc_wiota_get_freq_info());

    return AT_RESULT_OK;
}

static at_result_t at_freq_setup(const char *args)
{
    u32_t freq = 0;

    args = parse((char *)(++args), "d", &freq);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_freq_info(freq);

    return AT_RESULT_OK;
}

static at_result_t at_system_config_query(void)
{
    sub_system_config_t config = {0};
    uc_wiota_get_system_config(&config);

    at_server_printfln("+WIOTACONFIG=%d,%d,%d,%d,%d,%d,%d,0x%x,0x%x",
                       config.id_len, config.symbol_length, config.dlul_ratio, config.bt_value,
                       config.group_number, config.ap_max_power, config.spectrum_idx, config.system_id, config.subsystem_id);
    return AT_RESULT_OK;
}

static at_result_t at_system_config_setup(const char *args)
{
    int temp[7] = {0};
    sub_system_config_t config;

    if (wiota_state < AT_WIOTA_INIT && wiota_state != AT_WIOTA_EXIT)
    {
        at_server_printfln("please init wiota first");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "d,d,d,d,d,d,d,y,y",
                 &temp[0], &temp[1], &temp[2],
                 &temp[3], &temp[4], &temp[5],
                 &temp[6], &config.system_id, &config.subsystem_id);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    config.id_len = (u8_t)temp[0];
    config.symbol_length = (u8_t)temp[1];
    config.dlul_ratio = (u8_t)temp[2];
    config.bt_value = (u8_t)temp[3];
    config.group_number = (u8_t)temp[4];
    config.ap_max_power = (u8_t)temp[5];
    config.spectrum_idx = (u8_t)temp[6];
    config.pn_num = 1;

    uc_wiota_set_system_config(&config);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_init_exec(void)
{
    if (wiota_state == AT_WIOTA_DEFAULT || wiota_state == AT_WIOTA_EXIT)
    {
        uc_wiota_init();
        wiota_state = AT_WIOTA_INIT;

        return AT_RESULT_OK;
    }

    return AT_RESULT_REPETITIVE_FAILE;
}

#if 1
void uc_wiota_show_access_func(u32_t user_id)
{
    rt_kprintf("user_id 0x%x accessed  time %d\n", user_id, g_t_test_data.time);

    if (g_t_test_data.time > 0)
    {
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
                node->recv_num = 0;
                node->send_all_counter = 0;
                node->send_manager_flag = AT_TEST_FIRST_COMMAND_WAIT_SEND;
                node->user_id = user_id;
                node->next = RT_NULL;
                temp_all_list->next = node;
                rt_kprintf("add new node\n");
                return;
            }
            else
            {
                temp_all_list = temp_all_list->next;
            }
        }
    }
}

void uc_wiota_show_drop_func(u32_t user_id)
{
    at_server_printfln("user_id 0x%x dropped\n", user_id);
}

#if WIOTA_TEST_AUTO_SEND_SM
extern u8_t *generate_fake_data(u32_t data_len, u8_t repeat_num);
void uc_wiota_show_result(uc_send_recv_t *result)
{
    if (result->result == 0)
    {
        rt_kprintf("send dl to 0x%x succ!\n", result->user_id);
    }
    else
    {
        rt_kprintf("send dl to 0x%x failed or timeout! %d\n", result->user_id, result->result);
    }
}
#endif //WIOTA_TEST_AUTO_SEND_SM

static int at_fitering_test_data(u32_t user_id, u8_t *recv_data, u32_t data_len)
{
    t_at_test_communication *communication = (t_at_test_communication *)recv_data;
    unsigned int send_data_address = 0;
    t_at_test_queue_data *queue_data;
    char *copy_recv_data;

    // check head
    if (data_len < AT_TEST_COMMUNICATION_RESERVED_LEN || 0 != strcmp(communication->head, AT_TEST_COMMUNICATION_HEAD))
        return 0;

    //send queue data
    copy_recv_data = rt_malloc(data_len);
    memcpy(copy_recv_data, recv_data, data_len);

    queue_data = rt_malloc(sizeof(t_at_test_queue_data));
    queue_data->type = AT_TEST_MODE_RECVDATA;
    queue_data->usrid = user_id;
    queue_data->data = recv_data;

    rt_kprintf("%s line %d ap recv id 0x%x\n", __FUNCTION__, __LINE__, user_id);
    send_data_address = (unsigned int)queue_data;

    //at_send_queue(g_test_data.test_queue, data,  2000);
    rt_mq_send_wait(g_t_test_data.test_queue, &send_data_address, 4, 1000);

    return 1;
}

void uc_wiota_show_recv_data(u32_t user_id, u8_t *recv_data, u32_t data_len)
{
#if WIOTA_TEST_AUTO_SEND_SM
    if (!ap_pgw_get_grant_mode())
    {
        u8_t *fake_data = NULL;

        fake_data = generate_fake_data(8, 2);
        // for (u8_t i = 0; i < 16; i++)
        uc_wiota_send_data(fake_data, 8, &user_id, 1, 10000, uc_wiota_show_result);
        rt_free(fake_data);
        fake_data = NULL;
        //for test send two
        // rt_thread_mdelay(100);
        // fake_data = generate_fake_data(80, 10);
        // uc_wiota_paging_and_send_normal_data(fake_data, 80, &user_id, 1, 100, uc_wiota_show_result);
        // rt_free(fake_data);
        // fake_data = NULL;
    }
#endif //WIOTA_TEST_AUTO_SEND_SM
    if (g_t_test_data.time > 0 && at_fitering_test_data(user_id, recv_data, data_len))
    {
        return;
    }
    at_server_printf("+WIOTARECV,0x%x,%d,", user_id, data_len);
    at_send_data(recv_data, data_len);
    at_server_printf("\r\n");
}

void uc_wiota_register_callback(void)
{
    uc_wiota_register_iote_access_callback(uc_wiota_show_access_func);
    uc_wiota_register_iote_dropped_callback(uc_wiota_show_drop_func);
    uc_wiota_register_recv_data_callback(uc_wiota_show_recv_data);
}
#endif

static at_result_t at_wiota_func_setup(const char *args)
{
    int state = 0;

    args = parse((char *)(++args), "d", &state);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (1 == state && wiota_state == AT_WIOTA_INIT)
    {
        uc_wiota_run();

        uc_wiota_register_callback();

        wiota_state = AT_WIOTA_RUN;
    }
    else if (0 == state && wiota_state == AT_WIOTA_RUN)
    {
        uc_wiota_exit();
        wiota_state = AT_WIOTA_EXIT;
    }
    else
    {
        return AT_RESULT_REPETITIVE_FAILE;
    }
    return AT_RESULT_OK;
}

static at_result_t at_blacklist_query(void)
{
    blacklist_t *head_node = NULL;
    u16_t blacklist_num = 0;

    if (wiota_state < AT_WIOTA_INIT && wiota_state != AT_WIOTA_EXIT)
    {
        at_server_printfln("please init wiota first");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    head_node = uc_wiota_get_blacklist(&blacklist_num);

    while (head_node != NULL)
    {
        at_server_printfln("+WIOTABLACKLIST=0x%x,%d", head_node->user_id, blacklist_num);
        head_node = head_node->next;
    }

    return AT_RESULT_OK;
}

static at_result_t at_blacklist_setup(const char *args)
{
    u32_t user_id = 0;
    u32_t mode = 0;

    if (wiota_state < AT_WIOTA_INIT && wiota_state != AT_WIOTA_EXIT)
    {
        at_server_printfln("please init wiota first");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "y,d", &user_id, &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode == AT_WIOTA_ADD_BLACKLIST)
    {
        uc_wiota_add_iote_to_blacklist(&user_id, 1);
    }
    else if (mode == AT_WIOTA_REMOVE_BLACKLIST)
    {
        uc_wiota_remove_iote_from_blacklist(&user_id, 1);
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
    return AT_RESULT_OK;
}

static at_result_t at_iote_info_setup(const char *args)
{
    u32_t type = 0;
    u16_t iote_num = 0;
    iote_info_t *iote_info = NULL;

    if (wiota_state < AT_WIOTA_INIT && wiota_state != AT_WIOTA_EXIT)
    {
        at_server_printfln("please init wiota first");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "d", &type);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (type == 0)
    {
        iote_info = uc_wiota_get_connected_iotes(&iote_num);
    }
    else if (type == 1)
    {
        iote_info = uc_wiota_get_disconnected_iotes(&iote_num);
    }
    else
    {
        // type error
        return AT_RESULT_FAILE;
    }

    while (iote_info != NULL)
    {
        at_server_printfln("+WIOTAIOTEINFO=%d,0x%x,%d", type, iote_info->user_id, iote_num);
        iote_info = iote_info->next;
    }
    return AT_RESULT_OK;
}

static at_result_t at_active_time_query(void)
{
    at_server_printfln("+WIOTAACTIVETIME=%u", uc_wiota_get_active_time());

    return AT_RESULT_OK;
}

static at_result_t at_active_time_setup(const char *args)
{
    u32_t active_time = 0;

    if (wiota_state < AT_WIOTA_INIT && wiota_state != AT_WIOTA_EXIT)
    {
        at_server_printfln("please init wiota first");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "d", &active_time);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_active_time(active_time);
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

static at_result_t at_broadcast_setup(const char *args)
{
    u16_t data_len = 0;
    u16_t mode = 0;
    s32_t timeout = 0;
    u8_t *send_buf = NULL;
    u8_t *p_send_buf = NULL;
    u8_t ret = 0;

    args = parse((char *)(++args), "d,d,d", &data_len, &mode, &timeout);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (data_len > 0)
    {
        send_buf = (u8_t *)rt_malloc(data_len);
        if (send_buf == NULL)
        {
            at_server_printfln("rt_malloc failed!");
            return AT_RESULT_NULL;
        }
        rt_memset(send_buf, 0, data_len);

        p_send_buf = send_buf;
        at_server_printfln("OK");
        at_server_printf(">");
        while (data_len)
        {
            if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)p_send_buf) != RT_EOK)
            {
                at_server_printfln("get char failed");
                rt_free(send_buf);
                send_buf = NULL;
                return AT_RESULT_NULL;
            }
            data_len--;
            p_send_buf++;
        }
        u8_t res = uc_wiota_send_broadcast_data(send_buf, p_send_buf - send_buf, mode, timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT, RT_NULL);
        if (UC_OP_SUCC == res)
        {
            at_server_printfln("send bc suc");
            ret = AT_RESULT_OK;
        }
        else
        {
            at_server_printfln("send bc failed or timeout %d", res);
            ret = AT_RESULT_NULL;
        }
    }
    rt_free(send_buf);
    send_buf = NULL;
    return ret;
}

static at_result_t at_send_data_setup(const char *args)
{
    u32_t data_len = 0;
    u32_t user_id = 0;
    u32_t user_id_num = 0;
    s32_t timeout = 0;
    u8_t *send_buf = NULL;
    u8_t *p_send_buf = NULL;
    uc_result_e result = UC_OP_FAIL;
    u8_t ret = 0;

    args = parse((char *)(++args), "d,y,d,d", &data_len, &user_id, &user_id_num, &timeout);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (data_len > 0)
    {
        send_buf = (u8_t *)rt_malloc(data_len);
        if (send_buf == NULL)
        {
            at_server_printfln("rt_malloc failed!");
            return AT_RESULT_NULL;
        }
        rt_memset(send_buf, 0, data_len);

        p_send_buf = send_buf;
        at_server_printfln("OK");
        at_server_printf(">");
        while (data_len)
        {
            if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)p_send_buf) != RT_EOK)
            {
                at_server_printfln("get char failed!");
                rt_free(send_buf);
                send_buf = NULL;
                return AT_RESULT_NULL;
            }
            data_len--;
            p_send_buf++;
        }

        result = uc_wiota_send_data(send_buf, p_send_buf - send_buf, &user_id, user_id_num, timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT, RT_NULL);
        if (UC_OP_SUCC == result)
        {
            at_server_printfln("send pdu suc");
            ret = AT_RESULT_OK;
        }
        else
        {
            at_server_printfln("send pdu failed or timeout %d", result);
            ret = AT_RESULT_NULL;
        }
    }
    rt_free(send_buf);
    send_buf = NULL;
    return ret;
}

u32_t nth_power(u32_t num, u32_t n)
{
    u32_t s = 1;

    for (u32_t i = 0; i < n; i++)
    {
        s *= num;
    }
    return s;
}

void convert_string_to_int(u8_t num_len, u8_t num, const u8_t *p_start, u8_t *array)
{
    u8_t *temp = NULL;
    u8_t len = 0;
    u8_t nth = num_len;

    temp = (u8_t *)rt_malloc(num_len);
    if (temp == NULL)
    {
        rt_kprintf("convert_string_to_int malloc failed\n");
        return;
    }

    for (len = 0; len < num_len; len++)
    {
        temp[len] = p_start[len] - '0';
        array[num] += nth_power(10, nth - 1) * temp[len];
        nth--;
    }
    rt_free(temp);
    temp = NULL;
}

u8_t convert_string_to_array(u8_t *string, u8_t *array)
{
    u8_t *p_start = string;
    u8_t *p_end = string;
    u8_t num = 0;
    u8_t num_len = 0;

    while (*p_start != '\0')
    {
        while (*p_end != '\0')
        {
            if (*p_end == ',')
            {
                convert_string_to_int(num_len, num, p_start, array);
                num++;
                p_end++;
                p_start = p_end;
                num_len = 0;
            }
            num_len++;
            p_end++;
        }

        convert_string_to_int(num_len, num, p_start, array);
        num++;
        p_start = p_end;
    }
    return num;
}

static at_result_t at_scan_freq_setup(const char *args)
{
    u32_t freq_num = 0;
    u8_t convert_num = 0;
    s32_t timeout = 0;
    u8_t *freq_string = NULL;
    u8_t *freq_arry = NULL;
    u8_t *temp_freq = NULL;
    u32_t data_len = 0;
    u32_t str_len = 0;
    uc_scan_recv_t scan_info = {0};
    u8_t ret = AT_RESULT_OK;

    args = parse((char *)(++args), "d,d,d", &timeout, &data_len, &freq_num);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    str_len = data_len;

    if (freq_num > 0)
    {
        freq_string = (u8_t *)rt_malloc(data_len);
        if (freq_string == NULL)
        {
            at_server_printfln("rt_malloc freq_string failed!");
            return AT_RESULT_NULL;
        }
        rt_memset(freq_string, 0, data_len);
        temp_freq = freq_string;
        at_server_printfln("OK");
        at_server_printf(">");
        while (data_len)
        {
            if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)temp_freq) != RT_EOK)
            {
                at_server_printfln("get char failed!");
                rt_free(freq_string);
                freq_string = NULL;
                return AT_RESULT_NULL;
            }
            data_len--;
            temp_freq++;
        }

        freq_arry = (u8_t *)rt_malloc(freq_num * sizeof(u8_t));
        if (freq_arry == NULL)
        {
            at_server_printfln("rt_malloc freq_arry failed!");
            rt_free(freq_string);
            freq_string = NULL;
            return AT_RESULT_NULL;
        }
        rt_memset(freq_arry, 0, freq_num * sizeof(u8_t));

        freq_string[str_len - 2] = '\0';

        convert_num = convert_string_to_array(freq_string, freq_arry);
        if (convert_num != freq_num)
        {
            at_server_printfln("convert_num error!");
            rt_free(freq_string);
            freq_string = NULL;
            rt_free(freq_arry);
            freq_arry = NULL;
            return AT_RESULT_FAILE;
        }
        rt_free(freq_string);
        freq_string = NULL;

        uc_wiota_scan_freq(freq_arry, freq_num, timeout, NULL, &scan_info);
        if (UC_OP_SUCC == scan_info.result)
        {
            uc_scan_freq_t *freqList = (uc_scan_freq_t *)scan_info.data;
            at_server_printfln("+WIOTASCANFREQ:");
            for (u8_t idx = 0; idx < (scan_info.data_len / sizeof(uc_scan_freq_t)); idx++)
            {
#ifdef WIOTA_RECORD_TEST
                // rt_kprintf(";$;%d,%u,%d,%d,%d;$;\n", RECORD_SCANFREQ, freqList->freq_idx, freqList->snr, freqList->rssi, freqList->is_synced);
#endif
                at_server_printfln("%u,%d,%d,%d", freqList->freq_idx, freqList->rssi, freqList->snr, freqList->is_synced);
                freqList++;
            }

            ret = AT_RESULT_OK;
        }
        else
        {
            at_server_printfln("scan timeout or failed result %d", scan_info.result);
            ret = AT_RESULT_FAILE;
        }
    }

    rt_free(freq_arry);
    freq_arry = NULL;
    rt_free(scan_info.data);
    scan_info.data = NULL;

    return ret;
}

static at_result_t at_scan_freq_exec(void)
{
    uc_scan_recv_t scan_info = {0};
    u8_t ret = AT_RESULT_NULL;

    if (wiota_state != AT_WIOTA_RUN)
    {
        at_server_printfln("please run wiota first");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    uc_wiota_scan_freq(NULL, 0, -1, NULL, &scan_info);
    if (UC_OP_SUCC == scan_info.result)
    {
        uc_scan_freq_t *freq_list = (uc_scan_freq_t *)scan_info.data;
        if ((scan_info.data_len / sizeof(uc_scan_freq_t)) == (UC_WIOTA_MAX_FREQUENCE_POINT))
        {
            at_server_printfln("+WIOTASCANFREQ: ");
            for (u8_t idx = 0; idx < (scan_info.data_len / sizeof(uc_scan_freq_t)); idx++)
            {
#ifdef WIOTA_RECORD_TEST
                rt_kprintf(";$;%d,%u,%d,%d,%d;$;\n", RECORD_SCANFREQ, freq_list->freq_idx, freq_list->rssi, freq_list->snr, freq_list->is_synced);
#else
                at_server_printfln("freq_idx=%u, snr=%d, rssi=%d, is_synced=%d", freq_list->freq_idx, freq_list->snr, freq_list->rssi, freq_list->is_synced);
#endif
                freq_list++;
            }
        }
        else
        {
            at_server_printfln("scan_info->data_len not match");
            ret = AT_RESULT_FAILE;
        }
        ret = AT_RESULT_OK;
    }
    else
    {
        at_server_printfln("scan failed or timeout %d", scan_info.result);
        ret = AT_RESULT_NULL;
    }

    rt_free(scan_info.data);
    scan_info.data = NULL;

    return ret;
}

static at_result_t at_read_temp_exec(void)
{
    uc_temp_recv_t read_temp = {0};

    if (wiota_state != AT_WIOTA_RUN)
    {
        at_server_printfln("please run wiota first");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    if (UC_OP_SUCC == uc_wiota_read_temperature(NULL, &read_temp, 10000))
    {
        at_server_printfln("+WIOTATEMP:%d", read_temp.temp);
        return AT_RESULT_OK;
    }
    else
    {
        at_server_printfln("read failed or timeout %d", read_temp.result);
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_rf_power_setup(const char *args)
{
    s32_t rf_power = 0;

    if (wiota_state != AT_WIOTA_RUN)
    {
        at_server_printfln("please run wiota first");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "d", &rf_power);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_ap_max_power(rf_power);

    return AT_RESULT_OK;
}

static at_result_t at_version_exec(void)
{
    u8_t wiota_version[8] = {0};
    u8_t git_info[36] = {0};
    u8_t make_time[36] = {0};

    uc_wiota_get_version(wiota_version, git_info, make_time);
    at_server_printfln("+WIOTAVERSION:%s", wiota_version);
    at_server_printfln("+GITINFO:%s", git_info);
    at_server_printfln("+TIME:%s", make_time);

    return AT_RESULT_OK;
}

static at_result_t at_hopping_setup(const char *args)
{
    u32_t type = 0;
    u32_t value = 0;

    args = parse((char *)(++args), "d,d", &type, &value);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    switch (type)
    {
    case AT_WIOTA_HOPPING_SET_FREQ:
        uc_wiota_set_hopping_freq(value);
        break;

    case AT_WIOTA_HOPPING_SET_MODE:
        uc_wiota_set_hopping_mode(value);
        break;

    default:
        break;
    }

    return AT_RESULT_OK;
}

static at_result_t at_max_iote_num_setup(const char *args)
{
    u32_t max_iote_num = 0;

    args = parse((char *)(++args), "d", &max_iote_num);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_max_active_iote_num_in_the_same_subframe(max_iote_num);

    return AT_RESULT_OK;
}

static at_result_t at_bc_mcs_setup(const char *args)
{
    u32_t bc_mcs = 0;

    args = parse((char *)(++args), "d", &bc_mcs);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_broadcast_mcs(bc_mcs);

    return AT_RESULT_OK;
}

static at_result_t at_muti_sm_mode_setup(const char *args)
{
    u32_t is_open = 0;

    args = parse((char *)(++args), "d", &is_open);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_muti_sm_mode(is_open);

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
        rt_device_control(device, RT_DEVICE_CTRL_CONFIG, &config);
        rt_device_open(device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    }
}
#endif

static at_result_t at_wiotalog_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    switch (mode)
    {
    case AT_LOG_CLOSE:
    case AT_LOG_OPEN:
        uc_wiota_log_switch(UC_LOG_UART, mode - AT_LOG_CLOSE);
        break;

    case AT_LOG_UART0:
    case AT_LOG_UART1:
#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
        at_handle_log_uart(mode - AT_LOG_UART0);
#endif
        break;

    case AT_LOG_SPI_CLOSE:
    case AT_LOG_SPI_OPEN:
        uc_wiota_log_switch(UC_LOG_SPI, mode - AT_LOG_SPI_CLOSE);
        break;

    default:
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiotacrc_query(void)
{
    at_server_printfln("+WIOTACRC=%d", uc_wiota_get_crc());
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

    uc_wiota_set_crc(crc_limit);

    return AT_RESULT_OK;
}

static at_result_t at_wiotagrant_query(void)
{
    at_server_printfln("+WIOTAGRANT=%d", uc_wiota_get_grant_limit());
    return AT_RESULT_OK;
}

static at_result_t at_wiotagrant_setup(const char *args)
{
    int grant_limit = 0;

    args = parse((char *)(++args), "d", &grant_limit);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_grant_limit(grant_limit);

    return AT_RESULT_OK;
}

#ifdef WIOTA_AP_STATE_INFO
static at_result_t at_wiota_state_setup(const char *args)
{
    int get_or_reset = 0;
    int user_id = 0;
    int state_type = 0;

    if (wiota_state < AT_WIOTA_INIT && wiota_state != AT_WIOTA_EXIT)
    {
        at_server_printfln("please init wiota first");
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
            uc_state_info_t *p_node = uc_wiota_get_all_state_info();

            while (p_node != NULL)
            {
                at_server_printfln("+WIOTASTATE=0x%x,%d,%d,%d,%d,%d",
                                   p_node->user_id, p_node->ul_recv_len, p_node->ul_recv_suc,
                                   p_node->dl_send_len, p_node->dl_send_suc, p_node->dl_send_fail);
                p_node = p_node->next;
            }
        }
        else if (state_type == 0 && user_id != 0x0) // get all state of single iote
        {
            uc_state_info_t *p_node = uc_wiota_get_all_state_info_of_iote(user_id);
            if (p_node != NULL)
            {
                at_server_printfln("+WIOTASTATE=0x%x,%d,%d,%d,%d,%d",
                                   p_node->user_id, p_node->ul_recv_len, p_node->ul_recv_suc,
                                   p_node->dl_send_len, p_node->dl_send_suc, p_node->dl_send_fail);
            }
        }
        else if (state_type != 0 && user_id != 0x0) // get single state of single iote
        {
            if (state_type < TYPE_UL_RECV_LEN || state_type >= UC_STATE_TYPE_MAX)
            {
                return AT_RESULT_FAILE;
            }
            u32_t state = uc_wiota_get_single_state_info_of_iote(user_id, state_type);
            at_server_printfln("+WIOTASTATE=0x%x,%d", user_id, state);
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
#endif

static void at_test_get_devid_list(void)
{
    iote_info_t *read_iote_info = RT_NULL;
    t_iote_info_managerlist *temp_all_list = &g_t_test_data.iote_info_list;
    iote_info_t *tem_iote_info = RT_NULL;
    u16_t num;

    read_iote_info = uc_wiota_get_disconnected_iotes(&num);
    tem_iote_info = read_iote_info;

    while (tem_iote_info != RT_NULL)
    {
        t_iote_info_managerlist *node = rt_malloc(sizeof(t_iote_info_managerlist));
        rt_kprintf("connect user_id=%x\n", tem_iote_info->user_id);
        if (node == RT_NULL)
            return;
        node->recv_num = 0;
        node->send_all_counter = 0;
        node->send_manager_flag = AT_TEST_FIRST_COMMAND_WAIT_SEND;
        node->user_id = tem_iote_info->user_id;
        node->next = RT_NULL;
        // add list
        temp_all_list->next = node;

        temp_all_list = temp_all_list->next;
        tem_iote_info = tem_iote_info->next;
    }
    //rt_free(read_iote_info);

    read_iote_info = uc_wiota_get_connected_iotes(&num);
    tem_iote_info = read_iote_info;
    while (tem_iote_info != RT_NULL)
    {
        t_iote_info_managerlist *node = rt_malloc(sizeof(t_iote_info_managerlist));
        if (node == RT_NULL)
            return;
        rt_kprintf("disconnect user_id=%x\n", tem_iote_info->user_id);
        //get usrid
        node->recv_num = 0;
        node->send_all_counter = 0;
        node->send_manager_flag = AT_TEST_FIRST_COMMAND_WAIT_SEND;
        node->user_id = tem_iote_info->user_id;
        node->next = RT_NULL;
        // add list
        temp_all_list->next = node;

        temp_all_list = temp_all_list->next;
        tem_iote_info = tem_iote_info->next;
    }

    //rt_free(read_iote_info);
}
void at_test_send_callback(uc_send_recv_t *result)
{
    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;
    //rt_kprintf("======> id %x res %d\n", result->user_id, result->result);

    while (temp_all_list != RT_NULL)
    {
        if (temp_all_list->user_id == result->user_id)
        {
            if (UC_OP_SUCC == result->result)
                temp_all_list->send_manager_flag = AT_TEST_FIRST_COMMAND_SU;
            temp_all_list->send_all_counter--;
            rt_kprintf("======> id %x res %d have send counter %d\n", result->user_id, result->result, temp_all_list->send_all_counter);
            return;
        }
    }
}

static int at_test_filter_page(t_at_test_queue_data *page)
{
    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;
    t_at_test_communication *communication = page->data;

    while (temp_all_list != RT_NULL)
    {
        if (temp_all_list->user_id == page->usrid &&
            communication->num != temp_all_list->recv_num)
        {
            temp_all_list->recv_num = communication->num;
            return 1;
        }
        temp_all_list = temp_all_list->next;
    }

    return 0;
}

static void at_test_report_statistical(u32_t type, u32_t userid, u32_t dev, t_at_test_statistical_data statistical)
{
    at_server_printfln("+THROUGHT=%d,%d,0x%x,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", type, dev, userid,
                       statistical.upcurrentrate, statistical.upaverate, statistical.upminirate, statistical.upmaxrate,
                       statistical.downcurrentrate, statistical.downaverate, statistical.downminirate, statistical.downmaxrate,
                       statistical.send_fail, statistical.recv_fail,
                       statistical.msc, statistical.power, statistical.rssi, statistical.snr);
}

#ifdef WIOTA_AP_STATE_INFO
static void at_test_get_statistical(void)
{
    t_at_test_statistical_data statistical;
    uc_state_info_t *tmp_statistical_list = uc_wiota_get_all_state_info();

    while (tmp_statistical_list != RT_NULL)
    {
        rt_kprintf("id %x ul_recv_len %d dl_send_len %d ul_recv_suc %d dl_send_suc %d dl_send_fail %d\n",
                   tmp_statistical_list->user_id,
                   tmp_statistical_list->ul_recv_len, tmp_statistical_list->dl_send_len,
                   tmp_statistical_list->ul_recv_suc, tmp_statistical_list->dl_send_suc,
                   tmp_statistical_list->dl_send_fail);

        memset(&statistical, 0, sizeof(t_at_test_statistical_data));
        // statistical data
        AT_TEST_GET_RATE(g_t_test_data.time, tmp_statistical_list->ul_recv_len,
                         statistical.upcurrentrate, statistical.upaverate, statistical.upminirate, statistical.upmaxrate)

        AT_TEST_GET_RATE(g_t_test_data.time, tmp_statistical_list->dl_send_len,
                         statistical.downcurrentrate, statistical.downaverate, statistical.downminirate, statistical.downmaxrate)

        AT_TEST_CALCUTLATE(statistical.send_fail, tmp_statistical_list->dl_send_suc + tmp_statistical_list->dl_send_fail, tmp_statistical_list->dl_send_fail)

        at_test_report_statistical(g_t_test_data.type, tmp_statistical_list->user_id, 0, statistical);
        tmp_statistical_list = tmp_statistical_list->next;
    }
    // rt_kprintf("%s line %d \n", __FUNCTION__, __LINE__);
    //uc_wiota_reset_all_state_info();
    //rt_kprintf("%s line %d \n", __FUNCTION__, __LINE__);
}

static void at_test_mode_time_fun(void *parameter)
{
    at_test_get_statistical();
}
#endif

static void at_test_send_to_iote(t_at_test_communication *data)
{
    t_iote_info_managerlist *temp_all_list = (&g_t_test_data.iote_info_list)->next;

    data->num++;
    //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

    // send test data to iote. down and loopback tests.
    if (g_t_test_data.type == AT_TEST_COMMAND_DOWN_TEST ||
        g_t_test_data.type == AT_TEST_COMMAND_LOOP_TEST)
    {
        while (temp_all_list != RT_NULL)
        {
            // send data
            uc_wiota_send_data((u8_t *)data, data->all_len, &(temp_all_list->user_id), 1, 0, at_test_send_callback);
            temp_all_list = temp_all_list->next;
        }
    }
    else
    {
        while (temp_all_list != RT_NULL)
        {
            if (temp_all_list->send_manager_flag == AT_TEST_FIRST_COMMAND_WAIT_SEND)
            {
                // send data
                if (temp_all_list->send_all_counter < 5)
                {
                    uc_result_e res = uc_wiota_send_data((unsigned char *)data, data->all_len, &(temp_all_list->user_id), 1, 0, at_test_send_callback);
                    temp_all_list->send_all_counter++;
                    rt_kprintf("%s line %d send %x result %d\n", __FUNCTION__, __LINE__, temp_all_list->user_id, res);
                }
                temp_all_list = temp_all_list->next;
            }
        }
        rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    }
}

static void at_test_mode_task_fun(void *parameter)
{
    //t_at_test_queue_data time_queue_data;
    unsigned int queue_data = 0;
    t_at_test_queue_data *pqueue_data = RT_NULL;
    t_at_test_communication communication = {0};

    //time_queue_data.type = AT_TEST_MODE_TIMEOUT;
#ifdef WIOTA_AP_STATE_INFO
    uc_wiota_reset_all_state_info();
#endif
    // get iote userid list
    at_test_get_devid_list();

    // send first page to iote
    memcpy(communication.head, AT_TEST_COMMUNICATION_HEAD, strlen(AT_TEST_COMMUNICATION_HEAD));
    communication.command = g_t_test_data.type;
    communication.report = g_t_test_data.iote_reprot;
    communication.timeout = g_t_test_data.time;
    communication.all_len = sizeof(t_at_test_communication);

    at_test_send_to_iote(&communication);

    // create timer
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

    while (1)
    {
        //recv queue data from timer or wiota callback
        if (RT_EOK == rt_mq_recv(g_t_test_data.test_queue, &queue_data, 4, 100)) // RT_WAITING_NO
        {
            pqueue_data = (t_at_test_queue_data *)queue_data;
            //rt_kprintf("pqueue_data->type = %d\n", pqueue_data->type);

            switch ((int)pqueue_data->type)
            {
            case AT_TEST_MODE_TIMEOUT:
            {
                // get wiota ap state
#ifdef WIOTA_AP_STATE_INFO
                at_test_get_statistical();
#endif
                break;
            }
            case AT_TEST_MODE_RECVDATA:
            {
                t_at_test_communication *communication = pqueue_data->data;
                // pasre page
                if (at_test_filter_page(pqueue_data))
                {
                    // report iote data to uart
                    at_test_report_statistical(communication->command, pqueue_data->usrid, 1, communication->data);
                }

                // free buffer
                rt_free(communication);
                rt_free(pqueue_data);
                break;
            }
            case AT_TEST_MODE_QUEUE_EXIT:
            {
                rt_free(pqueue_data);
                rt_sem_release(g_t_test_data.test_sem);
                return;
            }
            }
        }
        //rt_kprintf("%s line %d \n", __FUNCTION__, __LINE__);
        // send data
        at_test_send_to_iote(&communication);
    }
}

static at_result_t at_test_mode_start(const char *args)
{
    rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

    if (g_t_test_data.time > 0)
        return AT_RESULT_PARSE_FAILE;

    if (wiota_state != AT_WIOTA_RUN)
    {
        rt_kprintf("at_test_mode_start wiota_state error\n");
        return AT_RESULT_REPETITIVE_FAILE;
    }

    // parse at parament
    args = parse((char *)(++args), "d,d", &g_t_test_data.type, &g_t_test_data.time);
    if (!args)
    {
        rt_kprintf("at_req_parse_args wiota_state error\n");
        return AT_RESULT_PARSE_FAILE;
    }

    if (g_t_test_data.time < 1)
        g_t_test_data.time = 3;

    // create sem
    g_t_test_data.test_sem = rt_sem_create("teMode", 0, RT_IPC_FLAG_PRIO);
    if (RT_NULL == g_t_test_data.test_sem)
    {
        rt_kprintf("%s line %d rt_sem_create error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }

    // create queue
    g_t_test_data.test_queue = rt_mq_create("teMode", 4, 8, RT_IPC_FLAG_PRIO);
    if (RT_NULL == g_t_test_data.test_queue)
    {
        rt_kprintf("%s line %d at_create_queue error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }

    //create task
    g_t_test_data.test_mode_task = rt_thread_create("test_mode",
                                                    at_test_mode_task_fun,
                                                    RT_NULL,
                                                    2048,
                                                    RT_THREAD_PRIORITY_MAX / 3 - 1,
                                                    3);
    if (RT_NULL == g_t_test_data.test_mode_task)
    {
        rt_kprintf("%s line %d rt_thread_create error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }
    rt_thread_startup(g_t_test_data.test_mode_task);

    return AT_RESULT_OK;
}

static at_result_t at_test_mode_stop_exec(void)
{
    unsigned int send_data_address;
    t_at_test_queue_data *data;

    if (g_t_test_data.time == 0 || g_t_test_data.test_mode_timer == RT_NULL)
    {
        rt_kprintf("%s line %d no start\n", __FUNCTION__, __LINE__);
        return AT_RESULT_FAILE;
    }

    data = rt_malloc(sizeof(t_at_test_queue_data));
    if (RT_NULL == data)
    {
        rt_kprintf("%s line %d rt_malloc error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }
    send_data_address = (unsigned int)data;

    rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

    data->type = AT_TEST_MODE_QUEUE_EXIT;

    // send queue data
    rt_mq_send_wait(g_t_test_data.test_queue, &send_data_address, 4, 1000);

    // wait sem 1s
    if (RT_EOK != rt_sem_take(g_t_test_data.test_sem, 10000))
    {
        rt_kprintf("%s line %d rt_sem_take error\n", __FUNCTION__, __LINE__);
        return AT_RESULT_PARSE_FAILE;
    }

    //del sem
    if (RT_NULL != g_t_test_data.test_sem)
        rt_sem_delete(g_t_test_data.test_sem);

    // delet timer
    rt_timer_stop(g_t_test_data.test_mode_timer);
    rt_timer_delete(g_t_test_data.test_mode_timer);

    // del queue
    if (RT_NULL != g_t_test_data.test_queue)
        rt_mq_delete(g_t_test_data.test_queue);

    // del task
    if (RT_NULL != g_t_test_data.test_mode_task)
        rt_thread_delete(g_t_test_data.test_mode_task);

    //clean test parament
    memset(&g_t_test_data, 0, sizeof(g_t_test_data));

    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+WIOTAINIT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_init_exec);
AT_CMD_EXPORT("AT+WIOTAFREQ", "=<freq_idx>", RT_NULL, at_freq_query, at_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAACTIVETIME", "=<active_time>", RT_NULL, at_active_time_query, at_active_time_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACONFIG", "=<id_len>,<symbol_len>,<dlul_ratio>,<bt_value>,<group_num>,<ap_max_pow>,<spectrum_idx>,<system_id>,<subsystem_id>", RT_NULL, at_system_config_query, at_system_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARUN", "=<state>", RT_NULL, RT_NULL, at_wiota_func_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABLACKLIST", "=<user_id>,<mode>", RT_NULL, at_blacklist_query, at_blacklist_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAIOTEINFO", "=<type>", RT_NULL, RT_NULL, at_iote_info_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABC", "=<len>,<mode>,<timeout>", RT_NULL, RT_NULL, at_broadcast_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASEND", "=<len>,<user_id>,<user_id_num>,<timeout>", RT_NULL, RT_NULL, at_send_data_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASCANFREQ", "=<timeout>,<data_len>,<freq_num>", RT_NULL, RT_NULL, at_scan_freq_setup, at_scan_freq_exec);
AT_CMD_EXPORT("AT+WIOTATEMP", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_read_temp_exec);
AT_CMD_EXPORT("AT+WIOTAPOW", "=<power>", RT_NULL, RT_NULL, at_rf_power_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAVERSION", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_version_exec);
AT_CMD_EXPORT("AT+WIOTAHOPPING", "=<type>,<value>", RT_NULL, RT_NULL, at_hopping_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAIOTENUM", "=<max_num>", RT_NULL, RT_NULL, at_max_iote_num_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABCMCS", "=<bc_mcs>", RT_NULL, RT_NULL, at_bc_mcs_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAMUTISMMODE", "=<is_open>", RT_NULL, RT_NULL, at_muti_sm_mode_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTALOG", "=<mode>", RT_NULL, RT_NULL, at_wiotalog_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACRC", "=<crc_limit>", RT_NULL, at_wiotacrc_query, at_wiotacrc_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAGRANT", "=<grant_limit>", RT_NULL, at_wiotagrant_query, at_wiotagrant_setup, RT_NULL);
#ifdef WIOTA_AP_STATE_INFO
AT_CMD_EXPORT("AT+WIOTASTATE", "=<get_or_reset>,<user_id>,<state_type>", RT_NULL, RT_NULL, at_wiota_state_setup, RT_NULL);
#endif
AT_CMD_EXPORT("AT+THROUGHTSTART", "=<type>,<time>,<iote_report>", RT_NULL, RT_NULL, at_test_mode_start, RT_NULL);
AT_CMD_EXPORT("AT+THROUGHTSTOP", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_test_mode_stop_exec);

#endif