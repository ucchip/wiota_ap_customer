#ifdef UC8088_MODULE
#include <rtthread.h>
#include "at.h"
#include "uc_wiota_interface.h"
#include "string.h"
#ifdef WIOTA_RECORD_TEST
#include "wiota_record_test.h"
#endif

#define WIOTA_WAIT_DATA_TIMEOUT 10000
#define WIOTA_SEND_TIMEOUT 60000

#define WIOTA_TEST_AUTO_SEND_SM 1

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

static int wiota_state = AT_WIOTA_DEFAULT;

static at_result_t at_freq_query(void)
{
    at_server_printfln("+WIOTAFREQ=%u", uc_wiota_get_frequency_point());

    return AT_RESULT_OK;
}

static at_result_t at_freq_setup(const char *args)
{
    int argc = 0;
    u32_t freq = 0;
    const char *req_expr = "=%u";

    argc = at_req_parse_args(args, req_expr, &freq);
    if (argc != 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_frequency_point(freq);

    return AT_RESULT_OK;
}

static at_result_t at_dcxo_query(void)
{
    at_server_printfln("+WIOTADCXO=0x%x", uc_wiota_get_dcxo());

    return AT_RESULT_OK;
}

static at_result_t at_dcxo_setup(const char *args)
{
    int argc = 0;
    u32_t dcxo = 0;
    const char *req_expr = "=%x";

    argc = at_req_parse_args(args, req_expr, &dcxo);
    if (argc != 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_dcxo(dcxo);

    return AT_RESULT_OK;
}

static at_result_t at_system_config_query(void)
{
    dynamic_para_t *config;
    config = uc_wiota_get_all_dynamic_parameter();

    at_server_printfln("+WIOTACONFIG=%d,%d,%d,%d,%d,%d,%d,0x%x,0x%x",
                       config->id_len, config->symbol_length, config->dlul_ratio, config->bt_value,
                       config->group_number, config->ap_max_power, config->spectrum_idx, config->system_id, config->subsystem_id);
    if (config != NULL)
    {
        rt_free(config);
        config = NULL;
    }
    return AT_RESULT_OK;
}

static at_result_t at_system_config_setup(const char *args)
{
    int argc = 0;
    dynamic_para_t config;
    const char *req_expr = "=%d,%d,%d,%d,%d,%d,%d,0x%x,0x%x";

    argc = at_req_parse_args(args, req_expr,
                             &config.id_len, &config.symbol_length, &config.dlul_ratio,
                             &config.bt_value, &config.group_number, &config.ap_max_power,
                             &config.spectrum_idx, &config.system_id, &config.subsystem_id);

    if (argc != 9)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    config.pn_num = 1;

    uc_wiota_set_all_dynamic_parameter(&config);

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
    rt_kprintf("uc_wiota_show_access_func user_id 0x%x accessed\n", user_id);
}

void uc_wiota_show_drop_func(u32_t user_id)
{
    rt_kprintf("uc_wiota_show_drop_func user_id 0x%x dropped\n", user_id);
}

#if WIOTA_TEST_AUTO_SEND_SM
extern u8_t *generate_fake_data(u32_t data_len, u8_t repeat_num);
void uc_wiota_show_result(uc_send_recv_t *result)
{
    rt_kprintf("uc_wiota_show_result result %d\n", result->result);
}
#endif

void uc_wiota_show_report_data(u32_t user_id, u8_t *report_data, u32_t report_data_len)
{
#if WIOTA_TEST_AUTO_SEND_SM
    u8_t *fake_data = NULL;

    fake_data = generate_fake_data(120, 10);
    uc_wiota_paging_and_send_normal_data(fake_data, 120, &user_id, 1, 100, uc_wiota_show_result);
    rt_free(fake_data);
    fake_data = NULL;
    //for test send two
    // rt_thread_mdelay(100);
    // fake_data = generate_fake_data(80, 10);
    // uc_wiota_paging_and_send_normal_data(fake_data, 80, &user_id, 1, 100, uc_wiota_show_result);
    // rt_free(fake_data);
    // fake_data = NULL;
#endif
#ifdef WIOTA_RECORD_TEST
#else
    at_server_printf("+WIOTARECV,0x%x,%d,", user_id, report_data_len);
    at_send_data(report_data, report_data_len);
    at_server_printf("\n");
#endif
}

void uc_wiota_register_callback(void)
{
    uc_wiota_register_iote_access_callback(uc_wiota_show_access_func);
    uc_wiota_register_iote_dropped_callback(uc_wiota_show_drop_func);
    uc_wiota_register_proactively_report_data_callback(uc_wiota_show_report_data);
}
#endif

static at_result_t at_wiota_func_setup(const char *args)
{
    int argc = 0, state = 0;
    const char *req_expr = "=%d";

    argc = at_req_parse_args(args, req_expr, &state);
    if (argc != 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (1 == state && wiota_state == AT_WIOTA_INIT)
    {
        uc_wiota_start();

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
    blacklist_t *headNode = NULL;
    blacklist_t *tempNode = NULL;
    u16_t blacklistNum = 0;

    headNode = uc_wiota_get_blacklist(&blacklistNum);
    tempNode = headNode->next;

    while (tempNode != NULL)
    {
        at_server_printfln("+WIOTABLACKLIST=0x%x,%d", tempNode->user_id, blacklistNum);
        tempNode = tempNode->next;
    }

    if (headNode != NULL)
    {
        rt_free(headNode);
        headNode = NULL;
    }
    return AT_RESULT_OK;
}

static at_result_t at_blacklist_setup(const char *args)
{
    int argc = 0;
    u32_t userId = 0;
    u32_t mode = 0;
    const char *req_expr = "=0x%x,%d";

    argc = at_req_parse_args(args, req_expr, &userId, &mode);
    if (argc != 2)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode == AT_WIOTA_ADD_BLACKLIST)
    {
        uc_wiota_add_iote_to_blacklist(&userId, 1);
    }
    else if (mode == AT_WIOTA_REMOVE_BLACKLIST)
    {
        uc_wiota_remove_iote_from_blacklist(&userId, 1);
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
    return AT_RESULT_OK;
}

static at_result_t at_iote_info_exec(void)
{
    u16_t ioteNum = 0;
    iote_info_t *ioteInfo = NULL;
    iote_info_t *tempNode = NULL;

    ioteInfo = uc_wiota_query_info_of_currently_connected_iote(&ioteNum);
    tempNode = ioteInfo->next;

    while (tempNode != NULL)
    {
        at_server_printfln("+WIOTAIOTEINFO=0x%x,%d", tempNode->user_id, ioteNum);
        tempNode = tempNode->next;
    }
    if (ioteInfo != NULL)
    {
        rt_free(ioteInfo);
        ioteInfo = NULL;
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
    int argc = 0;
    u32_t activeTime = 0;
    const char *req_expr = "=%u";

    argc = at_req_parse_args(args, req_expr, &activeTime);
    if (argc != 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_active_time(activeTime);
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
    int argc = 0;
    u16_t dataLen = 0;
    u16_t mode = 0;
    s32_t timeout = 0;
    u8_t *sendBuf = NULL;
    u8_t *pSendBuf = NULL;
    u8_t ret = 0;
    const char *req_expr = "=%u,%d,%d";

    argc = at_req_parse_args(args, req_expr, &dataLen, &mode, &timeout);
    if (argc != 3)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (wiota_state != AT_WIOTA_RUN)
    {
        return AT_RESULT_REPETITIVE_FAILE;
    }

    if (dataLen > 0)
    {
        sendBuf = (u8_t *)rt_malloc(dataLen);
        if (sendBuf == NULL)
        {
            at_server_printfln("rt_malloc failed!");
            return AT_RESULT_NULL;
        }
        rt_memset(sendBuf, 0, dataLen);

        pSendBuf = sendBuf;
        at_server_printfln("OK");
        at_server_printf(">");
        while (dataLen)
        {
            if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)pSendBuf) != RT_EOK)
            {
                at_server_printfln("get char failed");
                rt_free(sendBuf);
                sendBuf = NULL;
                return AT_RESULT_NULL;
            }
            dataLen--;
            pSendBuf++;
        }

        if (UC_SUCCESS == uc_wiota_send_broadcast_data(sendBuf, pSendBuf - sendBuf, mode, timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT, RT_NULL))
        {
            at_server_printfln("send data suc");
            ret = AT_RESULT_OK;
        }
        else
        {
            at_server_printfln("send data failed");
            ret = AT_RESULT_NULL;
        }
    }
    rt_free(sendBuf);
    sendBuf = NULL;
    return ret;
}

static at_result_t at_paging_setup(const char *args)
{
    int argc = 0;
    u32_t dataLen = 0;
    u32_t userId = 0;
    u32_t userIdNum = 0;
    s32_t timeout = 0;
    u8_t *sendBuf = NULL;
    u8_t *pSendBuf = NULL;
    uc_result_e result = UC_FAILED;
    u8_t ret = 0;
    const char *req_expr = "=%u,0x%x,%u,%d";

    argc = at_req_parse_args(args, req_expr, &dataLen, &userId, &userIdNum, &timeout);
    if (argc != 4)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (wiota_state != AT_WIOTA_RUN)
    {
        return AT_RESULT_REPETITIVE_FAILE;
    }

    if (dataLen > 0)
    {
        sendBuf = (u8_t *)rt_malloc(dataLen);
        if (sendBuf == NULL)
        {
            at_server_printfln("rt_malloc failed!");
            return AT_RESULT_NULL;
        }
        rt_memset(sendBuf, 0, dataLen);

        pSendBuf = sendBuf;
        at_server_printfln("OK");
        at_server_printf(">");
        while (dataLen)
        {
            if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)pSendBuf) != RT_EOK)
            {
                at_server_printfln("get char failed!");
                rt_free(sendBuf);
                sendBuf = NULL;
                return AT_RESULT_NULL;
            }
            dataLen--;
            pSendBuf++;
        }

        result = uc_wiota_paging_and_send_normal_data(sendBuf, pSendBuf - sendBuf, &userId, userIdNum, timeout > 0 ? timeout : WIOTA_SEND_TIMEOUT, RT_NULL);
        if (UC_SUCCESS == result)
        {
            at_server_printfln("send data suc");
            ret = AT_RESULT_OK;
        }
        else if (UC_PAGING == result)
        {
            at_server_printfln("wait 0x%x response paging request\n", userId);
            ret = AT_RESULT_OK;
        }
        else
        {
            at_server_printfln("send data failed");
            ret = AT_RESULT_NULL;
        }
    }
    rt_free(sendBuf);
    sendBuf = NULL;
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

void convert_string_to_int(u8_t numLen, u8_t num, const u8_t *pStart, u8_t *array)
{
    u8_t *temp = NULL;
    u8_t len = 0;
    u8_t nth = numLen;

    temp = (u8_t *)rt_malloc(numLen);
    if (temp == NULL)
    {
        rt_kprintf("convert_string_to_int malloc failed\n");
        return;
    }

    for (len = 0; len < numLen; len++)
    {
        temp[len] = pStart[len] - '0';
        array[num] += nth_power(10, nth - 1) * temp[len];
        nth--;
    }
    rt_free(temp);
    temp = NULL;
}

u8_t convert_string_to_array(u8_t *string, u8_t *array)
{
    u8_t *pStart = string;
    u8_t *pEnd = string;
    u8_t num = 0;
    u8_t numLen = 0;

    while (*pStart != '\0')
    {
        while (*pEnd != '\0')
        {
            if (*pEnd == ',')
            {
                convert_string_to_int(numLen, num, pStart, array);
                num++;
                pEnd++;
                pStart = pEnd;
                numLen = 0;
            }
            numLen++;
            pEnd++;
        }

        convert_string_to_int(numLen, num, pStart, array);
        num++;
        pStart = pEnd;
    }
    return num;
}

static at_result_t at_scan_freq_setup(const char *args)
{
    int argc = 0;
    u32_t freqNum = 0;
    u8_t convertNum = 0;
    s32_t timeout = 0;
    u8_t *freqString = NULL;
    u8_t *freqArry = NULL;
    u8_t *tempFreq = NULL;
    u32_t dataLen = 0;
    u32_t strLen = 0;
    uc_scan_recv_t scan_info = {0};
    u8_t ret = AT_RESULT_OK;

    const char *req_expr = "=%d,%u,%u";

    argc = at_req_parse_args(args, req_expr, &timeout, &dataLen, &freqNum);
    if (argc != 3)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (wiota_state != AT_WIOTA_RUN)
    {
        return AT_RESULT_REPETITIVE_FAILE;
    }
    strLen = dataLen;

    if (freqNum > 0)
    {
        freqString = (u8_t *)rt_malloc(dataLen);
        if (freqString == NULL)
        {
            at_server_printfln("rt_malloc freqString failed!");
            return AT_RESULT_NULL;
        }
        rt_memset(freqString, 0, dataLen);
        tempFreq = freqString;
        at_server_printfln("OK");
        at_server_printf(">");
        while (dataLen)
        {
            if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)tempFreq) != RT_EOK)
            {
                at_server_printfln("get char failed!");
                rt_free(freqString);
                freqString = NULL;
                return AT_RESULT_NULL;
            }
            dataLen--;
            tempFreq++;
        }

        freqArry = (u8_t *)rt_malloc(freqNum * sizeof(u8_t));
        if (freqArry == NULL)
        {
            at_server_printfln("rt_malloc freqArry failed!");
            rt_free(freqString);
            freqString = NULL;
            return AT_RESULT_NULL;
        }
        rt_memset(freqArry, 0, freqNum * sizeof(u8_t));

        freqString[strLen - 2] = '\0';

        convertNum = convert_string_to_array(freqString, freqArry);
        if (convertNum != freqNum)
        {
            at_server_printfln("converNum error!");
            rt_free(freqString);
            freqString = NULL;
            rt_free(freqArry);
            freqArry = NULL;
            return AT_RESULT_FAILE;
        }
        rt_free(freqString);
        freqString = NULL;

        uc_wiota_scan_freq(freqArry, freqNum, timeout, NULL, &scan_info);
        if (UC_SUCCESS == scan_info.result)
        {
            uc_scan_freq_t *freqList = (uc_scan_freq_t *)scan_info.data;
            at_server_printfln("+WIOTASCANFREQ: ");
            for (u8_t idx = 0; idx < (scan_info.data_len / sizeof(uc_scan_freq_t)); idx++)
            {
#ifdef WIOTA_RECORD_TEST
                rt_kprintf(";$;%d,freq_idx=%u,snr=%d,rssi=%d,is_synced=%d;$;\n", RECORD_SCANFREQ, freqList->freq_idx, freqList->snr, freqList->rssi, freqList->is_synced);
#else
                at_server_printfln("freq_idx=%u, snr=%d, rssi=%d, is_synced=%d", freqList->freq_idx, freqList->snr, freqList->rssi, freqList->is_synced);
#endif
                freqList++;
            }
#ifdef WIOTA_RECORD_TEST
            rt_kprintf(";$;%d,WIOTASCANFREQ=end;$;\n", RECORD_SCANFREQ_END);
#endif
            ret = AT_RESULT_OK;
        }
        else
        {
            at_server_printfln("scan timeout or failed result %d", scan_info.result);
            ret = AT_RESULT_FAILE;
        }
    }

    rt_free(freqArry);
    freqArry = NULL;
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
        return AT_RESULT_REPETITIVE_FAILE;
    }

    uc_wiota_scan_freq(NULL, 0, 0, NULL, &scan_info);
    if (UC_SUCCESS == scan_info.result)
    {
        uc_scan_freq_t *freqList = (uc_scan_freq_t *)scan_info.data;
        if ((scan_info.data_len / sizeof(uc_scan_freq_t)) == (UC_WIOTA_MAX_FREQUENCE_POINT))
        {
            at_server_printfln("+WIOTASCANFREQ: ");
            for (u8_t idx = 0; idx < (scan_info.data_len / sizeof(uc_scan_freq_t)); idx++)
            {
#ifdef WIOTA_RECORD_TEST
                rt_kprintf(";$;%d,freq_idx=%u,snr=%d,rssi=%d,is_synced=%d;$;\n", RECORD_SCANFREQ, freqList->freq_idx, freqList->snr, freqList->rssi, freqList->is_synced);
#else
                at_server_printfln("freq_idx=%u, snr=%d, rssi=%d, is_synced=%d", freqList->freq_idx, freqList->snr, freqList->rssi, freqList->is_synced);
#endif
                freqList++;
            }
#ifdef WIOTA_RECORD_TEST
            rt_kprintf(";$;%d,WIOTASCANFREQ=end;$;\n", RECORD_SCANFREQ_END);
#endif
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
        at_server_printfln("scan timeout or failed!");
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
        return AT_RESULT_REPETITIVE_FAILE;
    }

    if (UC_SUCCESS == uc_wiota_read_temperature(NULL, &read_temp, 10000))
    {
        at_server_printfln("+WIOTATEMP:%d", read_temp.temp);
        return AT_RESULT_OK;
    }
    else
    {
        at_server_printfln("read failed!");
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_rf_power_setup(const char *args)
{
    int argc = 0;
    s32_t rfPower = 0;

    const char *req_expr = "=%d";

    argc = at_req_parse_args(args, req_expr, &rfPower);
    if (argc != 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    uc_wiota_set_rf_power(rfPower);

    return AT_RESULT_OK;
}

static at_result_t at_version_exec(void)
{
    uc_wiota_get_version(NULL, NULL);
    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+WIOTAINIT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_init_exec);
AT_CMD_EXPORT("AT+WIOTAFREQ", "=<freqpoint>", RT_NULL, at_freq_query, at_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTADCXO", "=<dcxo>", RT_NULL, at_dcxo_query, at_dcxo_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAACTIVETIME", "=<activetime>", RT_NULL, at_active_time_query, at_active_time_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACONFIG", "=<idlen>,<symbollen>,<dlul>,<bt>,<groupnum>,<apmaxpow>,<spectrumidx>,<systemid>,<subsystemid>", RT_NULL, at_system_config_query, at_system_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARUN", "=<state>", RT_NULL, RT_NULL, at_wiota_func_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABLACKLIST", "=<userid>,<mode>", RT_NULL, at_blacklist_query, at_blacklist_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAIOTEINFO", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_iote_info_exec);
AT_CMD_EXPORT("AT+WIOTABROADCAST", "=<len>,<mode>,<timeout>", RT_NULL, RT_NULL, at_broadcast_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPAGING", "=<len>,<userid>,<useridnum>,<timeout>", RT_NULL, RT_NULL, at_paging_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASCANFREQ", "=<timeout>,<dataLen>,<freqnum>", RT_NULL, RT_NULL, at_scan_freq_setup, at_scan_freq_exec);
AT_CMD_EXPORT("AT+WIOTATEMP", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_read_temp_exec);
AT_CMD_EXPORT("AT+WIOTAPOWER", "=<power>", RT_NULL, RT_NULL, at_rf_power_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAVERSION", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_version_exec);
#endif