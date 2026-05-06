#include <rtthread.h>
#include <rtdevice.h>
#include <at.h>
#include <stdio.h>
#include "ati_prs.h"
#include "uc_uart.h"
#include "gpio.h"
#include "fastlz.h"
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"

typedef enum
{
    RATE_MODE_1 = 1, // 128+mcs3, ul 6.23kbps, dl 3.12kbps
    RATE_MODE_2,     // 128+mcs2, ul 5.33kbps, dl 2.66kbps
    RATE_MODE_3,     // 256+mcs3, ul 2.66kbps, dl 1.33kbps
    RATE_MODE_4,     // 256+mcs2, ul 1.32kbps, dl 0.66kbps
    RATE_MODE_5,     // 256+mcs1, ul 0.92kbps, dl 0.46kbps
    RATE_MODE_6,     // 256+mcs0, ul 0.42kbps, dl 0.21kbps
    RATE_MODE_7,     // 512+mcs0, ul 0.21kbps, dl 0.10kbps
    RATE_MODE_8,     // 1024+mcs0, ul 0.10kbps, dl 0.05kbps
    RATE_MODE_MAX,
} rate_mode_e;

typedef struct
{
    unsigned char symbol_length;
    unsigned char dlul_ratio;
    unsigned char mcs;
} rate_mode_t;

typedef struct
{
    unsigned char is_compress : 1; // 是否压缩
    unsigned char reserved : 7;    // 保留位
} app_header_t;

typedef struct
{
    unsigned int addr;                            // AP地址，用作唤醒ID，只读
    unsigned int subsystemid;                     // AP子系统id，支持热配置
    unsigned int base_freq;                       // 频谱基础频率
    unsigned int cur_freq;                        // 当前频率，支持热配置
    unsigned char spectrum_idx;                   // 频谱索引，不支持热配置
    unsigned char power;                          // 功率，支持热配置
    unsigned char rate_mode;                      // rate_mode_e，不支持热配置
    rate_mode_t rate_mode_map[RATE_MODE_MAX - 1]; // 速率模式映射表
} at_simplified_t;

#define MIN_SLEEP_TIME 100   // ms
#define MAX_SLEEP_TIME 44000 // ms
#define SERIAL_NUM_LEN 20
#define SOFTWARE_VER_LEN 15

#define MAX_SUBSYSTEMID 16777215      // 同步字最大值
#define DEFAULT_SUBSYSTEMID 354689    // 默认子系统id
#define DEFAULE_FREQ 502000000        // 默认频率502000000Hz
#define DEFAULE_BW_HZ 200000          // 默认带宽200000Hz
#define DEFAULE_POWER 20              // 默认功率20dbm
#define DEFAULE_RATE_MODE RATE_MODE_4 // 256+mcs2, ul 1.32kbps, dl 0.66kbps
#define INDEX_TO_FREQ(index, base_freq, bw) ((index) * bw + base_freq)
#define FREQ_TO_INDEX(freq, base_freq, bw) ((freq - base_freq) / bw)

#define RUN_STATE_PIN GPIO_PIN_5
#define TX_STATE_PIN GPIO_PIN_28
#define RX_STATE_PIN GPIO_PIN_7

#define FREQ_LIST_NUM 16
#define SCAN_SINGLE_TIMEOUT (10000) // 扫描单个频点超时时间
#define RATE_MODE_OFFSET 1          // 速率模式偏移量，模式从1开始，下标从0开始

#define RATE_MODE_MAP                            \
    {                                            \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_128,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_3,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_128,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_2,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_3,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_2,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_1,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_0,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_512,  \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_0,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_1024, \
            .dlul_ratio = 1,                     \
            .mcs = UC_MCS_LEVEL_0,               \
        },                                       \
    };

#define BASE_FREQ 223000000
#define FREQ_SPECTRUM_COUNT 8 // 频率频谱总数
#define POWER_MIN 0           // 最小功率，单位dBm
#define POWER_MAX 27          // 最大功率，单位dBm
#define PIN_INIT_MDELAY 1000  // 引脚初始化延迟
#define RX_COMPLETE_MDELAY 1  // RX完成延迟
#define DECOMPRESS_MAX_LEN 1024
#define COMPRESS_BUFFER_FACTOR 2 // 压缩缓冲区大小倍数
#define RECV_DATA_TIMEOUT 10000  // 接收数据超时时间

const unsigned int FREQ_RANGES[FREQ_SPECTRUM_COUNT][2] = {
    {BASE_FREQ, 235000000}, // 223MHz~235MHz，sepctrum_idx 0
    {400000000, 440000000}, // 400MHz~440MHz，sepctrum_idx 1
    {433050000, 434790000}, // 433.05MHz~434.79MHz，sepctrum_idx 2
    {470000000, 510000000}, // 470MHz~510MHz，sepctrum_idx 3
    {779000000, 787000000}, // 779MHz~787MHz，sepctrum_idx 4
    {840000000, 845000000}, // 840MHz~845MHz，sepctrum_idx 5
    {863000000, 870000000}, // 863MHz~870MHz，sepctrum_idx 6
    {902000000, 928000000}, // 902MHz~928MHz，sepctrum_idx 7
};

// 各频谱下的最大频率数
static const unsigned char g_max_freq_num_table[FREQ_SPECTRUM_COUNT] = {60, 200, 8, 200, 40, 25, 35, 130};

// 各符号长度对应的最大唤醒ID
static const unsigned short g_max_awaken_id_table[SYMBOL_LENGTH_1024 + 1] = {1023, 4095, 16383, 65535};

extern at_server_t at_get_server(void);
static at_simplified_t at_simplified_local = {0};

// GPIO引脚定义表
static const unsigned int g_state_pins[] = {RUN_STATE_PIN, TX_STATE_PIN, RX_STATE_PIN};
#define STATE_PINS_COUNT (sizeof(g_state_pins) / sizeof(g_state_pins[0]))

static void at_state_pin_init(void)
{
    for (int i = 0; i < STATE_PINS_COUNT; i++)
    {
        gpio_set_pin_mux(UC_GPIO_CFG, g_state_pins[i], GPIO_FUNC_0);
        rt_pin_mode(g_state_pins[i], PIN_MODE_OUTPUT);
        rt_pin_write(g_state_pins[i], PIN_LOW);
    }
}

void at_simplified_running(void)
{
    rt_pin_write(RUN_STATE_PIN, PIN_HIGH);
    rt_thread_mdelay(PIN_INIT_MDELAY);
    rt_pin_write(RUN_STATE_PIN, PIN_LOW);
    rt_thread_mdelay(PIN_INIT_MDELAY);
}

static inline void at_tx_start(void)
{
    rt_pin_write(TX_STATE_PIN, PIN_HIGH);
}

static inline void at_tx_end(void)
{
    rt_pin_write(TX_STATE_PIN, PIN_LOW);
}

static inline void at_rx_notity(void)
{
    rt_pin_write(RX_STATE_PIN, PIN_HIGH);
}

static inline void at_rx_complete(void)
{
    rt_thread_mdelay(RX_COMPLETE_MDELAY);
    rt_pin_write(RX_STATE_PIN, PIN_LOW);
}

static inline int is_space(char c)
{
    // 匹配空格、制表符、换行符等空白字符，核心只处理空格即可
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

static inline int str_to_int(const char *str, int len)
{
    int num = 0;
    for (int i = 0; i < len; i++)
    {
        if (str[i] < '0' || str[i] > '9')
        {
            return -1;
        }
        num = num * 10 + (str[i] - '0');
    }
    return num;
}

static int str_to_int_array(const char *input_str, unsigned int *out_array, int *out_count)
{
    if (input_str == NULL || out_array == NULL || out_count == NULL)
    {
        return -1;
    }
    *out_count = 0;

    int input_len = rt_strlen(input_str);
    int start = 0, end = input_len - 1;
    while (start <= end && is_space(input_str[start]))
    {
        start++;
    }
    while (end >= start && is_space(input_str[end]))
    {
        end--;
    }
    if (start > end)
    {
        return -1;
    }

    int num_start = start;
    for (int i = start; i <= end + 1; i++)
    {
        if (i == end + 1 || input_str[i] == ',')
        {
            int seg_start = num_start;
            int seg_end = i - 1;
            while (seg_start <= seg_end && is_space(input_str[seg_start]))
            {
                seg_start++;
            }
            while (seg_end >= seg_start && is_space(input_str[seg_end]))
            {
                seg_end--;
            }
            if (seg_start > seg_end)
            {
                num_start = i + 1;
                continue;
            }

            if (*out_count >= FREQ_LIST_NUM)
            {
                return -2;
            }

            int num_len = seg_end - seg_start + 1;
            int num = str_to_int(input_str + seg_start, num_len);
            if (num == -1)
            {
                return -3;
            }

            out_array[*out_count] = num;
            (*out_count)++;

            num_start = i + 1;
        }
    }

    return 0;
}

static unsigned int at_get_base_freq_and_spectrum_idx(unsigned int freq, unsigned char *spectrum_idx, unsigned int *actual_freq)
{
    for (int i = FREQ_SPECTRUM_COUNT - 1; i >= 0; i--)
    {
        if (freq >= FREQ_RANGES[i][0] && freq <= FREQ_RANGES[i][1])
        {
            unsigned int base_freq = FREQ_RANGES[i][0];
            int diff_freq_hz = (int)(freq - base_freq);

            if (actual_freq != RT_NULL)
            {
                if (diff_freq_hz % DEFAULE_BW_HZ != 0)
                {
                    // 不为带宽的整数倍，向下取整修正
                    *actual_freq = base_freq + (diff_freq_hz / DEFAULE_BW_HZ) * DEFAULE_BW_HZ;
                }
                else
                {
                    *actual_freq = freq;
                }
            }
            if (spectrum_idx != RT_NULL)
            {
                *spectrum_idx = i;
            }
            return base_freq; // 返回base freq
        }
    }

    return 1; // 不在范围
}

static inline unsigned short at_get_max_awaken_id(void)
{
    rate_mode_t *rate_mode = &at_simplified_local.rate_mode_map[at_simplified_local.rate_mode];
    unsigned char symbol_length = rate_mode->symbol_length;
    return g_max_awaken_id_table[symbol_length];
}

static inline unsigned char at_get_max_freq_num(unsigned char spectrum_idx)
{
    if (spectrum_idx >= FREQ_SPECTRUM_COUNT)
    {
        return 0;
    }
    return g_max_freq_num_table[spectrum_idx];
}

static inline unsigned int at_get_address(void)
{
    unsigned char serial[SERIAL_NUM_LEN] = {0};

    uc_wiota_get_module_id(serial);

    unsigned char *addr_s = &serial[8];

    return (addr_s[0] << 24) | (addr_s[1] << 16) | (addr_s[2] << 8) | addr_s[3];
}

static inline int at_set_subsystemid(unsigned int subsystemid)
{
    if (at_simplified_local.subsystemid != subsystemid)
    {
        sub_system_config_t config;
        uc_wiota_get_system_config(&config);
        config.subsystem_id = subsystemid;
        if (uc_wiota_set_system_config(&config) == 0)
        {
            at_simplified_local.subsystemid = subsystemid;
            return 0;
        }
        return 1;
    }
    return 0;
}

static inline int at_set_power(unsigned char power)
{
    if (at_simplified_local.power != power)
    {
        if (uc_wiota_set_ap_tx_power(power) == 0)
        {
            at_simplified_local.power = power;
            return 0;
        }
        return 1;
    }
    return 0;
}

static inline unsigned int at_get_send_timeout(unsigned char type)
{
    static const unsigned short send_timeout[2][4] = {{20, 40, 80, 160}, {40, 80, 160, 320}}; // unit sec
    rate_mode_t *rate_mode = &at_simplified_local.rate_mode_map[at_simplified_local.rate_mode - 1];

    return (send_timeout[type][rate_mode->symbol_length] * 1000);
}

static int at_get_freq_list(const char *freq_str, unsigned char *freq_list, int *freq_num, unsigned char *spectrum_idx, unsigned int *base_freq)
{
    unsigned int freq_array[FREQ_LIST_NUM] = {0};
    int freq_num_temp = 0;

    if (0 != str_to_int_array(freq_str, freq_array, &freq_num_temp) || freq_num_temp > FREQ_LIST_NUM)
    {
        rt_kprintf("invalid freq list\n");
        return 1;
    }

    // 检查该组频率是否属于同一频谱，如果不属于，则返回错误
    unsigned int base_freq_temp = 0;
    unsigned int last_base_freq = 0;
    unsigned char spectrum_idx_temp = 0;
    unsigned int actual_freq_temp = 0;
    for (int i = 0; i < freq_num_temp; i++)
    {
        base_freq_temp = at_get_base_freq_and_spectrum_idx(freq_array[i], &spectrum_idx_temp, &actual_freq_temp);
        if (last_base_freq > 0 && base_freq_temp != last_base_freq)
        {
            rt_kprintf("freq not in same spectrum\n");
            return 2;
        }
        last_base_freq = base_freq_temp;
        freq_list[i] = FREQ_TO_INDEX(actual_freq_temp, base_freq_temp, DEFAULE_BW_HZ);
        if (freq_list[i] > at_get_max_freq_num(spectrum_idx_temp))
        {
            rt_kprintf("freq out of range\n");
            return 3;
        }
    }

    *spectrum_idx = spectrum_idx_temp;
    *freq_num = freq_num_temp;
    *base_freq = base_freq_temp;

    return 0;
}

void at_wiota_recv_callback(uc_recv_detail_t *recv_detail)
{
    if (recv_detail == RT_NULL || recv_detail->data == RT_NULL || recv_detail->data_len < sizeof(app_header_t))
    {
        rt_kprintf("invalid recv_detail\n");
        return;
    }

    app_header_t *app_header = (app_header_t *)recv_detail->data;
    int data_len = recv_detail->data_len - sizeof(app_header_t);
    unsigned char *recv_data = &recv_detail->data[sizeof(app_header_t)];
    unsigned char *decompress_data = RT_NULL;

    if (app_header->is_compress)
    {
        // 解压数据
        decompress_data = (unsigned char *)rt_malloc(DECOMPRESS_MAX_LEN);
        if (decompress_data == RT_NULL)
        {
            rt_kprintf("decompress data malloc failed\n");
            return;
        }
        rt_memset(decompress_data, 0, DECOMPRESS_MAX_LEN);

        int decompress_len = fastlz_decompress(recv_data, data_len, decompress_data, DECOMPRESS_MAX_LEN);
        if (decompress_len > 0)
        {
            recv_data = decompress_data;
            data_len = decompress_len;
        }
        else
        {
            rt_kprintf("decompress data failed\n");
            rt_free(decompress_data);
            return;
        }
    }

    at_rx_notity();
    at_server_printf("+RECV:%u,%d,%d,", recv_detail->user_id, recv_detail->rssi, data_len);
    at_send_data(recv_data, data_len);
    at_server_printfln("");
    at_rx_complete();

    if (decompress_data != RT_NULL)
    {
        rt_free(decompress_data);
    }
}

static void at_wiota_run(void)
{
    uc_wiota_exit();

    uc_wiota_set_frame_boundary_align_func(1);            // 帧边界校准
    uc_wiota_set_time_service_func(TIME_SERVICE_GNSS, 1); // GPS授时
    // uc_wiota_set_broadcast_utc_func(1);                   // 同步世界时间功能

    // 启动wiota协议栈
    uc_wiota_init();

    rate_mode_t *rate_mode = &at_simplified_local.rate_mode_map[at_simplified_local.rate_mode - 1]; // -1，模式从1开始，下标从0开始
    sub_system_config_t config = {0};

    uc_wiota_get_system_config(&config);
    config.ap_tx_power = at_simplified_local.power;
    config.symbol_length = rate_mode->symbol_length;
    config.dlul_ratio = rate_mode->dlul_ratio;
    config.group_number = rate_mode->dlul_ratio;
    config.spectrum_idx = at_simplified_local.spectrum_idx;
    config.freq_idx = FREQ_TO_INDEX(at_simplified_local.cur_freq, at_simplified_local.base_freq, DEFAULE_BW_HZ);
    config.subsystem_id = at_simplified_local.subsystemid;
    rt_kprintf("pow %d, sym_len %d, du_ratio %d, spec_idx %d, freq_idx %d, subsystemid %u(0x%x), mcs %d\n",
               config.ap_tx_power, config.symbol_length, config.dlul_ratio, config.spectrum_idx,
               config.freq_idx, config.subsystem_id, config.subsystem_id, rate_mode->mcs);
    uc_wiota_set_system_config(&config);

    uc_wiota_run();
    uc_wiota_time_service_start(); // 启动GPS授时

    uc_wiota_set_data_rate(0, rate_mode->mcs);  // 单播mcs
    uc_wiota_set_broadcast_mcs(rate_mode->mcs); // 广播mcs
    uc_wiota_set_crc(1);                        // crc校验常开
    uc_wiota_register_recv_data_detail_callback(at_wiota_recv_callback);

    rt_kprintf("at_wiota_run ok\n");
}

void at_simplified_init(void)
{
    at_state_pin_init();

    // 初始化默认配置
    // -1，模式从1开始，下标从0开始
    const rate_mode_t rate_mode_map[RATE_MODE_MAX - 1] = RATE_MODE_MAP;

    at_simplified_local.addr = at_get_address();
    at_simplified_local.subsystemid = DEFAULT_SUBSYSTEMID;
    // cur_freq 5020000, base_freq 470000000, spectrum_idx 3
    at_simplified_local.base_freq = at_get_base_freq_and_spectrum_idx(DEFAULE_FREQ, &at_simplified_local.spectrum_idx, &at_simplified_local.cur_freq);
    RT_ASSERT(at_simplified_local.base_freq >= BASE_FREQ);
    at_simplified_local.power = DEFAULE_POWER;
    // rate_mode 4, symbol_length 256, mcs 2
    at_simplified_local.rate_mode = DEFAULE_RATE_MODE;
    rt_memcpy(at_simplified_local.rate_mode_map, rate_mode_map, sizeof(rate_mode_map));
    rt_kprintf("at_simplified_init, addr %u(0x%x), freq %uHz-%uHz, power %ddBm, rate mode %d(sym_len 1, mcs 2)\n",
               at_simplified_local.addr, at_simplified_local.addr,
               at_simplified_local.base_freq, at_simplified_local.cur_freq,
               at_simplified_local.power, at_simplified_local.rate_mode);

    // 启动wiota协议栈
    at_wiota_run();
    at_server_printfln("+WIOTA:OK");
}

static at_result_t at_wiota_sn_query(void)
{
    unsigned char serial[SERIAL_NUM_LEN] = {0};

    uc_wiota_get_module_id(serial);
    at_server_printfln("+SN:%s", serial);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_version_query(void)
{
    unsigned char version[SOFTWARE_VER_LEN] = {0};

    uc_wiota_get_version(version, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL);
    at_server_printfln("+VER:%s", version);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_scan_freq(unsigned char *freq_list, int freq_num, unsigned char spectrum_idx, unsigned int base_freq)
{
    uc_scan_recv_t scan_result = {0};
    unsigned int timeout = SCAN_SINGLE_TIMEOUT * (freq_num == 0 ? at_get_max_freq_num(spectrum_idx) : freq_num);

    uc_wiota_scan_freq(freq_list, freq_num, 0, timeout, RT_NULL, &scan_result);
    if (UC_OP_SUCC == scan_result.result && scan_result.data != RT_NULL)
    {
        uc_scan_freq_t *freq_scan_result = (uc_scan_freq_t *)scan_result.data;
        for (int i = 0; i < scan_result.data_len / sizeof(uc_scan_freq_t); i++)
        {
            at_server_printfln("+RSSI:%d,%d,%d",
                               INDEX_TO_FREQ(freq_scan_result[i].freq_idx, base_freq, DEFAULE_BW_HZ),
                               freq_scan_result[i].rssi,
                               freq_scan_result[i].is_synced);
        }
        rt_free(scan_result.data);
        return AT_RESULT_OK;
    }
    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_rssi_test(void)
{
    at_server_printfln("AT+RSSI=<freq1>[,<freq2>[,...,<freq16>]]");
    return AT_RESULT_OK;
}

static at_result_t at_wiota_rssi_query(void)
{
    // 查询当前频率的环境信号
    unsigned char freq_idx = uc_wiota_get_freq_info();

    return at_wiota_scan_freq(&freq_idx, 1, at_simplified_local.spectrum_idx, at_simplified_local.base_freq);
}

static at_result_t at_wiota_rssi_setup(const char *args)
{
    // 查询指定频率列表的环境信号
    unsigned char freq_list[FREQ_LIST_NUM] = {0};
    int freq_num = 0;
    unsigned char spectrum_idx = 0;
    unsigned int base_freq = 0;

    if (0 != at_get_freq_list(args + 1, freq_list, &freq_num, &spectrum_idx, &base_freq))
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return at_wiota_scan_freq(freq_list, freq_num, spectrum_idx, base_freq);
}

static at_result_t at_wiota_rssi_exec(void)
{
    // 查询当前频谱的所有频率的环境信号
    return at_wiota_scan_freq(RT_NULL, 0, at_simplified_local.spectrum_idx, at_simplified_local.base_freq);
}

static at_result_t at_wiota_sync_test(void)
{
    at_server_printfln("AT+SYNC=<sync[1-%d]>", MAX_SUBSYSTEMID);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_sync_query(void)
{
    at_server_printfln("+SYNC:%u", at_simplified_local.subsystemid);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_sync_setup(const char *args)
{
    unsigned int subsystemid = 0;

    args = parse((char *)(++args), "d", &subsystemid);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (subsystemid <= MAX_SUBSYSTEMID)
    {
        if (0 == at_set_subsystemid(subsystemid))
        {
            return AT_RESULT_OK;
        }
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_address_query(void)
{
    at_server_printfln("+ADDR:%u", at_simplified_local.addr);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_power_test(void)
{
    at_server_printfln("AT+POWER=<power[%d-%d]>", POWER_MIN, POWER_MAX);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_power_query(void)
{
    at_server_printfln("+POWER:%d", at_simplified_local.power);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_power_setup(const char *args)
{
    int power = 0;

    args = parse((char *)(++args), "d", &power);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (power >= POWER_MIN && power <= POWER_MAX)
    {
        if (0 == at_set_power(power))
        {
            return AT_RESULT_OK;
        }
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_freq_test(void)
{
    at_server_printfln("AT+FREQ=<freq1>[,<freq2>[,...,<freq16>]]");
    return AT_RESULT_OK;
}

static at_result_t at_wiota_freq_query(void)
{
    at_server_printfln("+FREQ:%u", at_simplified_local.cur_freq);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_freq_setup(const char *args)
{
    unsigned char freq_list[FREQ_LIST_NUM] = {0};
    int freq_num = 0;
    unsigned char spectrum_idx = 0;
    unsigned char freq_idx = 0xff;

    if (0 != at_get_freq_list(args + 1, freq_list, &freq_num, &spectrum_idx, &at_simplified_local.base_freq))
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (spectrum_idx != at_simplified_local.spectrum_idx)
    {
        at_simplified_local.spectrum_idx = spectrum_idx;
        at_wiota_run();
    }

    if (freq_num == 1)
    {
        freq_idx = freq_list[0];
    }
    else
    {
        uc_scan_recv_t scan_result = {0};
        unsigned int timeout = SCAN_SINGLE_TIMEOUT * freq_num;

        uc_wiota_scan_freq(freq_list, freq_num, 0, timeout, RT_NULL, &scan_result);
        if (UC_OP_SUCC == scan_result.result && scan_result.data != RT_NULL)
        {
            uc_scan_freq_t *freq_scan_result = (uc_scan_freq_t *)scan_result.data;
            unsigned char min_rssi_idx = 0xff;

            // 选出rssi最小的频率
            for (int i = 0; i < scan_result.data_len / sizeof(uc_scan_freq_t); i++)
            {
                if (freq_scan_result[i].is_synced == 0)
                {
                    if (min_rssi_idx == 0 || freq_scan_result[i].rssi < freq_scan_result[min_rssi_idx].rssi)
                    {
                        min_rssi_idx = i;
                    }
                }
            }

            if (min_rssi_idx != 0xff)
            {
                freq_idx = freq_scan_result[min_rssi_idx].freq_idx;
            }
            else
            {
                // 未找到合适的频率，使用第一个频率
                freq_idx = freq_list[0];
            }

            rt_free(scan_result.data);
        }
    }

    if (uc_wiota_set_freq_info(freq_idx) == 0)
    {
        at_simplified_local.cur_freq = INDEX_TO_FREQ(freq_idx, at_simplified_local.base_freq, DEFAULE_BW_HZ);
        at_server_printfln("+FREQ:%u", at_simplified_local.cur_freq);
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_rate_test(void)
{
    at_server_printfln("AT+RATE=<rate[%d-%d]>", RATE_MODE_1, RATE_MODE_MAX - 1);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_rate_query(void)
{
    at_server_printfln("+RATE:%d", at_simplified_local.rate_mode);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_rate_setup(const char *args)
{
    unsigned int rate = 0;

    args = parse((char *)(++args), "d", &rate);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (rate <= RATE_MODE_8 && rate >= RATE_MODE_1)
    {
        unsigned char last_rate = at_simplified_local.rate_mode;
        rate_mode_t *last_rate_mode = &at_simplified_local.rate_mode_map[last_rate - RATE_MODE_OFFSET];
        rate_mode_t *cur_rate_mode = &at_simplified_local.rate_mode_map[rate - RATE_MODE_OFFSET];

        if (last_rate != rate)
        {
            if ((last_rate_mode->symbol_length != cur_rate_mode->symbol_length) ||
                last_rate_mode->dlul_ratio != cur_rate_mode->dlul_ratio)
            {
                at_simplified_local.rate_mode = rate;
                at_wiota_run();
            }
            else
            {
                uc_wiota_set_data_rate(0, cur_rate_mode->mcs); // mcs
            }
        }
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

// 数据压缩辅助函数
// 返回值：0 成功, 非0 失败
static int at_wiota_compress_and_pack(unsigned char *data_buf, unsigned int data_len,
                                      unsigned char *header, int header_len,
                                      int *output_len)
{
    if (data_buf == NULL || header == NULL || output_len == NULL)
    {
        return -1;
    }

    int comp_data_len = data_len * COMPRESS_BUFFER_FACTOR;
    unsigned char *comp_data = rt_malloc(comp_data_len);
    if (comp_data == NULL)
    {
        return -2;
    }
    rt_memset(comp_data, 0, comp_data_len);

    // 压缩数据部分
    int compressed_len = fastlz_compress(data_buf, data_len, comp_data);
    if (compressed_len < data_len)
    {
        // 压缩成功，使用压缩数据
        app_header_t *app_header = (app_header_t *)header;
        app_header->is_compress = 1;
        rt_memcpy(data_buf, comp_data, compressed_len);
        *output_len = header_len + compressed_len;
    }
    else
    {
        // 压缩无效，使用原始数据
        app_header_t *app_header = (app_header_t *)header;
        app_header->is_compress = 0;
        *output_len = header_len + data_len;
    }

    rt_free(comp_data);
    return 0;
}

static at_result_t at_wiota_send_test(void)
{
    at_server_printfln("AT+SEND=<addr[1-4294967295]>,<len[1-%d]>", UC_WIOTA_MAX_SEND_NORMAL_DATA_LEN - sizeof(app_header_t));
    at_server_printfln("AT+SEND=<addr[0]>,<len[1-%d]>", UC_WIOTA_MAX_SEND_BROADCAST_DATA_LEN - sizeof(app_header_t));
    return AT_RESULT_OK;
}

static at_result_t at_wiota_send_setup(const char *args)
{
    unsigned char *recv_buf = RT_NULL;
    unsigned int addr = 0, recv_len = 0;

    args = parse((char *)(++args), "d,d", &addr, &recv_len);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    int header_len = sizeof(app_header_t);
    int max_data_len = addr ? (UC_WIOTA_MAX_SEND_NORMAL_DATA_LEN - header_len) : (UC_WIOTA_MAX_SEND_BROADCAST_DATA_LEN - header_len);

    if (recv_len < 1 || recv_len > max_data_len)
    {
        return AT_RESULT_FAILE;
    }

    // 分配缓冲区：报头 + 数据
    int malloc_len = header_len + recv_len;
    recv_buf = rt_malloc(malloc_len);
    if (recv_buf == RT_NULL)
    {
        return AT_RESULT_FAILE;
    }
    rt_memset(recv_buf, 0, malloc_len);

    at_server_printfln(">");
    if (at_server_recv(at_get_server(), (char *)&recv_buf[header_len], recv_len, rt_tick_from_millisecond(RECV_DATA_TIMEOUT)) != recv_len)
    {
        rt_kprintf("at_server_recv timeout\n");
        rt_free(recv_buf);
        return AT_RESULT_FAILE;
    }

    // 初始化报头
    app_header_t app_header = {0};
    rt_memcpy(&recv_buf[0], &app_header, header_len);

    // 数据压缩和打包
    int data_len = 0;
    if (at_wiota_compress_and_pack(&recv_buf[header_len], recv_len, recv_buf, header_len, &data_len) != 0)
    {
        rt_free(recv_buf);
        return AT_RESULT_FAILE;
    }

    uc_result_e result = UC_OP_FAIL;
    at_tx_start();
    if (addr > 0)
    {
        result = uc_wiota_send_data(recv_buf, data_len, addr, at_get_send_timeout(0), RT_NULL, recv_buf);
    }
    else
    {
        result = uc_wiota_send_broadcast_data(recv_buf, data_len, OTA_BROADCAST, at_get_send_timeout(1), RT_NULL, recv_buf);
    }
    at_tx_end();

    rt_free(recv_buf);

    if (result == UC_OP_SUCC)
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_sleep_test(void)
{
    at_server_printfln("AT+SLEEP=<time[%d-%d]>", MIN_SLEEP_TIME, MAX_SLEEP_TIME);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_sleep_setup(const char *args)
{
    unsigned int time = 0; // ms

    args = parse((char *)(++args), "d", &time);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (time < MIN_SLEEP_TIME || time > MAX_SLEEP_TIME)
    {
        return AT_RESULT_FAILE;
    }

    at_server_printfln("OK"); // 睡眠之后无法再输出，需提前输出OK
    uart_wait_tx_done();

    uc_lpm_rx_cfg_t pr_cfg = {0};
    sub_system_config_t config = {0};
    unsigned short max_awaken_id = at_get_max_awaken_id();

    uc_wiota_get_system_config(&config);
    uc_wiota_get_paging_rx_cfg(&pr_cfg);

    pr_cfg.mode = 1;
    pr_cfg.spectrum_idx = config.spectrum_idx;
    pr_cfg.symbol_length = config.symbol_length;
    pr_cfg.freq = config.freq_idx;
    pr_cfg.detect_period = time;
    pr_cfg.period_multiple = 1;
    pr_cfg.awaken_id = at_simplified_local.addr % (max_awaken_id - 1); // 单播唤醒ID
    pr_cfg.awaken_id_another = max_awaken_id;                          // 取最后一个为广播唤醒ID

    uc_wiota_set_paging_rx_cfg(&pr_cfg);
    uc_wiota_paging_rx_enter(0, 0);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_wakeup_test(void)
{
    at_server_printfln("AT+WAKEUP=<addr[0-4294967295]>,<time[%d-%d]>", MIN_SLEEP_TIME, MAX_SLEEP_TIME);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_wakeup_setup(const char *args)
{
    unsigned int addr = 0, time = 0; // ms

    args = parse((char *)(++args), "d,d", &addr, &time);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (time >= MIN_SLEEP_TIME && time <= MAX_SLEEP_TIME)
    {
        uc_lpm_tx_cfg_t pt_cfg = {0};
        sub_system_config_t config = {0};
        unsigned short max_awaken_id = at_get_max_awaken_id();

        uc_wiota_get_system_config(&config);
        uc_wiota_get_paging_tx_cfg(&pt_cfg);

        pt_cfg.mode = 1;
        pt_cfg.spectrum_idx = config.spectrum_idx;
        pt_cfg.symbol_length = config.symbol_length;
        pt_cfg.freq = config.freq_idx;
        pt_cfg.send_time = time + 100;
        if (addr == 0)
        {
            // 广播唤醒
            pt_cfg.awaken_id = max_awaken_id;
        }
        else
        {
            // 单播唤醒
            pt_cfg.awaken_id = addr % (max_awaken_id - 1);
        }
        uc_wiota_set_paging_tx_cfg(&pt_cfg);
        if (uc_wiota_paging_tx_start() == 0)
        {
            return AT_RESULT_OK;
        }
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_gnss_query(void)
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

    at_server_printfln("+GNSS:%s,%s,%s", longitude_str, latitude_str, altitude_str);

    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+SN", RT_NULL, RT_NULL, at_wiota_sn_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+VER", RT_NULL, RT_NULL, at_wiota_version_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+RSSI", "=<freq>", at_wiota_rssi_test, at_wiota_rssi_query, at_wiota_rssi_setup, at_wiota_rssi_exec);
AT_CMD_EXPORT("AT+SYNC", "=<sync>", at_wiota_sync_test, at_wiota_sync_query, at_wiota_sync_setup, RT_NULL);
AT_CMD_EXPORT("AT+ADDR", RT_NULL, RT_NULL, at_wiota_address_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+POWER", "=<power>", at_wiota_power_test, at_wiota_power_query, at_wiota_power_setup, RT_NULL);
AT_CMD_EXPORT("AT+FREQ", "=<freq>", at_wiota_freq_test, at_wiota_freq_query, at_wiota_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+RATE", "=<rate>", at_wiota_rate_test, at_wiota_rate_query, at_wiota_rate_setup, RT_NULL);
AT_CMD_EXPORT("AT+SEND", "=<addr>,<data>", at_wiota_send_test, RT_NULL, at_wiota_send_setup, RT_NULL);
AT_CMD_EXPORT("AT+SLEEP", "=<time>", at_wiota_sleep_test, RT_NULL, at_wiota_sleep_setup, RT_NULL);
AT_CMD_EXPORT("AT+WAKEUP", "=<addr>,<time>", at_wiota_wakeup_test, RT_NULL, at_wiota_wakeup_setup, RT_NULL);
AT_CMD_EXPORT("AT+GNSS", RT_NULL, RT_NULL, at_wiota_gnss_query, RT_NULL, RT_NULL);