#include <rtthread.h>
#ifdef UC_SPI_COM_SUPPORT
#include "board.h"
#include "gpio.h"
#include "rtdevice.h"
#include "uc_wiota_api.h"

typedef struct
{
    unsigned short read_index : 15;
    unsigned short read_mirror : 1;
    unsigned char channel;
    unsigned char read_crc;
} rb_read_state_t;

typedef struct
{
    unsigned short write_index : 15;
    unsigned short write_mirror : 1;
    unsigned char channel;
    unsigned char write_crc;
} rb_write_state_t;

typedef struct
{
    unsigned int isr_cnt;
    unsigned short crc_limit : 12;
    unsigned short ap8288_state : 1;
    unsigned short channel : 3;
    unsigned short heart_crc;
} heart_info_t;

typedef struct
{
    unsigned short state;
    unsigned short channel;
} hand_shake_t;

#define MAX_CHECK_CNT (2)
#define RB_STATE_SIZE (4)
#define DL_RING_BUFFER_SIZE (7152)
#define UL_RING_BUFFER_SIZE (7152)

/*|start_addr 0x00336800                                            total_size 14kb                                          end_addr 0x0033A000|*/
/*|   heart_info  |dl_rb_state_buf|             dl_buf              |dl_buf_state|ul_rb_state_buf|             ul_buf              |ul_buf_state|*/
/*|     8 byte    |     8 byte    |            7152 byte            |   4 byte   |    8 byte     |            7152 byte            |   4 byte   |*/

__attribute__((section(".spiulheartinfo"))) heart_info_t heart_info;
__attribute__((section(".spidlrbrstate"))) rb_read_state_t dl_rb_read_state;
__attribute__((section(".spidlrbwstate"))) rb_write_state_t dl_rb_write_state;
__attribute__((section(".spidlbuf"))) unsigned char dl_buf[DL_RING_BUFFER_SIZE];
__attribute__((section(".spidlbufstat"))) hand_shake_t dl_buf_state;

__attribute__((section(".spiulrbrstate"))) rb_read_state_t ul_rb_read_state;
__attribute__((section(".spiulrbwstate"))) rb_write_state_t ul_rb_write_state;
__attribute__((section(".spiulbuf"))) unsigned char ul_buf[UL_RING_BUFFER_SIZE];
__attribute__((section(".spiulbufstat"))) hand_shake_t ul_buf_state;

#define WIOTA_HEART_PERIOD (20000) // 20s
#define WIOTA_CMD_MAX 0xff
#define RING_BUFFER_ALIGN_SIZE 4

void uc_wiota_recv_master_ready_msg(void);

typedef enum
{
    WIOTA_INIT = 0,
    WIOTA_RUN,
    WIOTA_EXIT,
    WIOTA_REBOOT,
    WIOTA_DFE_COUNTER_FLAG_SET,
    WIOTA_DFE_COUNTER_SEND = 5,
    WIOTA_PAGING_TX_START,
    WIOTA_MASTER_READY,
    WIOTA_TS_START,
    WIOTA_TS_STOP,
    WIOTA_REG_FN_REFRESH_CB,
} uc_wiota_exec_cmd_e;

typedef enum
{
    WIOTA_ACT_SET = 0,
    WIOTA_CONFIG_SET,
    WIOTA_POWER_SET,
    WIOTA_FREQ_SET,
    WIOTA_HOPPING_FREQ_SET,
    WIOTA_HOPPING_MODE_SET = 5,
    WIOTA_MAX_IOTE_NUM_SET,
    WIOTA_DATA_RATE_SET,
    WIOTA_BC_MCS_SET,
    WIOTA_CRC_SET,
    WIOTA_LOG_SET = 10,
    WIOTA_DEFAULT_DFE_SET,
    WIOTA_FB_ALIGN_SET,
    WIOTA_PAGING_TX_SET,
    WIOTA_BC_FN_CYCLE_SET,
    WIOTA_BC_SEND_ROUND_SET = 15,
    WIOTA_TS_FUNC_SET,
    WIOTA_TS_CYCLE_SET,
    WIOTA_1588_SYNC_SET,
    WIOTA_PPS_SET,
    WIOTA_SINGLE_TONE_SET = 20,
    WIOTA_UL_SUBF_MODE_SET,
    WIOTA_SUBF_MODE_CFG_SET,
    WIOTA_SM_RESEND_TIMES_SET,
    WIOTA_PAGING_RX_SET,
    WIOTA_AAGC_IDX_SET = 25,
    WIOTA_BNACK_FUNC_SET,
} uc_wiota_setup_cmd_e;

typedef enum
{
    WIOTA_ACT_GET = 0,
    WIOTA_CONFIG_GET,
    WIOTA_FREQ_GET,
    WIOTA_MAX_IOTE_NUM_GET,
    WIOTA_DATA_RATE_GET,
    WIOTA_BC_MCS_GET = 5,
    WIOTA_CRC_GET,
    WIOTA_FRAME_HEAD_DFE_GET,
    WIOTA_DFE_COUNTER_GET,
    WIOTA_AP_STATE_GET,
    WIOTA_PAGING_TX_GET = 10,
    WIOTA_BC_FN_CYCLE_GET,
    WIOTA_BC_SEND_ROUND_GET,
    WIOTA_FRAME_LEN_GET,
    WIOTA_FRAME_NUM_GET,
    WIOTA_TS_FUNC_GET = 15,
    WIOTA_PPS_GET
} uc_wiota_query_cmd_e;

typedef enum
{
    // send cmd
    WIOTA_SEND_DATA = 0,
    WIOTA_SEND_BC,
    WIOTA_SEND_MC,
    WIOTA_SCAN_FREQ,
    WIOTA_TEMP_QUERY,
    WIOTA_SYNC_PAGING = 5,
    WIOTA_RECV_SM_BY_FN,
} uc_wiota_send_cmd_e;

typedef enum
{
    WIOTA_BL_ADD = 0,
    WIOTA_BL_REMOVE,
    WIOTA_BL_GET,
    WIOTA_MC_ID_ADD,
    WIOTA_MC_ID_DEL,
    WIOTA_VERSION_GET,
    WIOTA_ADD_DL_SUBF_DATA,
} uc_other_cmd_e;

typedef enum
{
    // recv cmd
    WIOTA_RECV_DATA = 0,
    WIOTA_PAGING_CTRL,
    WIOTA_DROP,
    WIOTA_TS_STATE,
    WIOTA_FN_REFRESH,
    // WIOTA_MP_FULL,
    // WIOTA_MP_EMPTY,
} uc_wiota_recv_cmd_e;

typedef enum
{
    CMD_TYPE_EXEC = 0, // exec cmd
    CMD_TYPE_SETUP,    // setup cmd
    CMD_TYPE_QUERY,    // all query cmd
    CMD_TYPE_OTHER,
    CMD_TYPE_SEND,     // send data
    CMD_TYPE_RECV = 5, // recv data, only ul used
} uc_wiota_cmd_type_e;

typedef enum
{
    WIOTA_CMD_EXEC_SUC = 0,
    WIOTA_CMD_EXEC_FAIL = 1
} uc_wiota_cmd_exec_res_e;

#pragma pack(4)
typedef struct
{
    unsigned char cmd_type : 3; // max 5
    unsigned char cmd : 5;      // max 13
    unsigned char channel : 3;  // max 7
    unsigned char result : 1;
    unsigned char reserved : 4;
    unsigned short data_len;
    unsigned int header_crc;
} cmd_header_t;

typedef struct
{
    cmd_header_t cmd_header;
} exec_cmd_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int in_value[11];
} setup_cmd_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int in_value[2];
} query_cmd_info_t;

typedef struct
{
    cmd_header_t cmd_header;
} exec_result_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int out_value[11];
} query_result_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int user_id;
    unsigned int data_type;
    unsigned short data_len;
    signed char rssi;
    unsigned char delay;
    unsigned char fn_cnt;
    unsigned char group_idx;
    unsigned char burst_idx;
    unsigned char slot_idx;
    unsigned int frame_num;
    unsigned char data[0];
} recv_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int user_id;
    unsigned int burst_idx;
    unsigned int fn_index;
} paging_ctrl_recv_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int user_id;
    signed int timeout;
    unsigned int data_id;
    unsigned char order_business;
    unsigned char is_block;
    unsigned short data_len;
    unsigned char data[0];
} send_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int user_id;
    unsigned int data_id;
    unsigned int result;
} send_res_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int timeout;
    unsigned short is_block;
    unsigned char freq_num;
    unsigned char scan_type;
    unsigned char freq[0];
} scan_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned short result;
    unsigned short data_len;
    uc_scan_freq_t scan_freq[0];
} scan_res_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int timeout;
    int is_block;
} temp_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned short temp;
    unsigned short result;
} temp_res_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int user_id;
    unsigned int fn_index;
    unsigned int detection_period;
    unsigned short send_round;
    unsigned short continue_fn;
    unsigned int is_block;
} paging_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int user_id;
    unsigned int result;
} paging_res_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int user_id;
} drop_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    uc_ts_info_t ts_info;
} ts_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned char wiota_version_8088[15];
    unsigned char git_info_8088[36];
    unsigned char make_time_8088[36];
    unsigned char wiota_version_8288[15];
    unsigned char git_info_8288[36];
    unsigned char make_time_8288[36];
    unsigned int cce_version;
} version_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int id_num;
    unsigned int id[0];
} id_info_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned char na[2];
    unsigned char data_len;
    unsigned char fn;
    unsigned char data[0];
} subf_data_ex_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int user_id;
    unsigned int data_id;
    unsigned int start_recv_fn;
    unsigned char recv_fns;
    unsigned char send_fns;
    unsigned short data_len;
    unsigned char data[0];
} rs_sm_by_fn_ex_t;

typedef struct
{
    cmd_header_t cmd_header;
    unsigned int frame_num;
} fn_refresh_t;

struct cmd_send_para
{
    int para_num;
    cmd_header_t *cmd_header;
    void *para_ptr[2];
    unsigned short para_len[2];
};
#pragma pack()

static struct rt_ringbuffer *dl_rb = RT_NULL;
static struct rt_ringbuffer *ul_rb = RT_NULL;
static rt_sem_t cmd_notice = RT_NULL;
static rt_mutex_t cmd_lock = RT_NULL;

// ap channel
static unsigned char ap_channel_id = 0xff;

static unsigned int dl_irq_cnt = 0;
static unsigned int ul_irq_cnt = 0;
static unsigned int g_add_cnt = 0;
static unsigned int g_del_cnt = 0;

static unsigned char g_report_fn_to_gw = 0;

void uc_wiota_print_data_cnt(void)
{
    rt_kprintf("data_cnt: add_cnt %u, del_cnt %u\n", g_add_cnt, g_del_cnt);
}

void uc_wiota_store_frame_num(unsigned int frame_num)
{
    rt_memcpy(&ul_buf[7148], &frame_num, 4);
}

static void uc_wiota_enable_ul_irq(void)
{
    ul_irq_cnt++;
    // rt_kprintf("channel %d, ul_irq cnt %d\n", ap_channel_id, ul_irq_cnt);
    gpio_set_pin_value(GPIO_PIN_5, GPIO_VALUE_HIGH);
    rt_thread_mdelay(3);
    gpio_set_pin_value(GPIO_PIN_5, GPIO_VALUE_LOW);
}

static void uc_wiota_handle_dl_irq(void *para)
{
    if (cmd_notice)
    {
        rt_sem_release(cmd_notice);
    }
}

static void uc_wiota_hand_shake_with_gateway(void)
{
    // ap hand shake with gateway
    ul_buf_state.state = 1;
    rt_kprintf("hand shake state, dl:%d, ul:%d\n", dl_buf_state.state, ul_buf_state.state);
    while (1)
    {
        if (dl_buf_state.state == 1)
        {
            ap_channel_id = dl_buf_state.channel; // get ap channel id
            ul_rb_read_state.channel = ap_channel_id;
            dl_rb_write_state.channel = ap_channel_id;
            ul_rb_write_state.channel = ap_channel_id;
            dl_rb_read_state.channel = ap_channel_id;
            ul_rb_read_state.read_crc = uc_wiota_crc8_calc((unsigned char *)&ul_rb_read_state, sizeof(rb_read_state_t) - UC_CRC8_LEN);
            dl_rb_write_state.write_crc = uc_wiota_crc8_calc((unsigned char *)&dl_rb_write_state, sizeof(rb_write_state_t) - UC_CRC8_LEN);
            ul_rb_write_state.write_crc = uc_wiota_crc8_calc((unsigned char *)&ul_rb_write_state, sizeof(rb_write_state_t) - UC_CRC8_LEN);
            dl_rb_read_state.read_crc = uc_wiota_crc8_calc((unsigned char *)&dl_rb_read_state, sizeof(rb_read_state_t) - UC_CRC8_LEN);
            rt_kprintf("channel %d, hand shake with gateway suc\n", ap_channel_id);
            break;
        }
        else
        {
            // rt_kprintf("channel %d, hand shake with gw fail. retry!\n", ap_channel_id);
            rt_thread_mdelay(1);
        }
    }
}

static unsigned char uc_wiota_get_channel_id(void)
{
    return ap_channel_id;
}

static struct rt_ringbuffer *uc_wiota_ringbuffer_init(unsigned int rb_size, unsigned char *buffer_ptr)
{
    RT_ASSERT(rb_size > 0 && buffer_ptr != RT_NULL);

    struct rt_ringbuffer *new_rb = (struct rt_ringbuffer *)rt_malloc(sizeof(struct rt_ringbuffer));
    RT_ASSERT(new_rb);
    rt_memset(new_rb, 0, sizeof(struct rt_ringbuffer));

    /* initialize read and write index */
    new_rb->read_mirror = new_rb->read_index = 0;
    new_rb->write_mirror = new_rb->write_index = 0;

    /* set buffer pool and size */
    new_rb->buffer_ptr = buffer_ptr;
    new_rb->buffer_size = RT_ALIGN_DOWN(rb_size, RING_BUFFER_ALIGN_SIZE);
    rt_memset(new_rb->buffer_ptr, 0, new_rb->buffer_size);

    return new_rb;
}

// static void uc_wiota_ringbuffer_deinit(struct rt_ringbuffer *rb)
// {
//     RT_ASSERT(rb != RT_NULL);

//     rt_free(rb);
//     rb = RT_NULL;
// }

static int uc_wiota_ringbuffer_get_dl_write_idx(void)
{
    int result = 1;
    unsigned short write_crc = 0;

    if (dl_rb_write_state.channel != ap_channel_id)
    {
        rt_kprintf("channel %d, read dl_rb_read_state from other channel %d\n", ap_channel_id, dl_rb_write_state.channel);
        return result;
    }

    write_crc = uc_wiota_crc8_calc((unsigned char *)&dl_rb_write_state, sizeof(rb_write_state_t) - UC_CRC8_LEN);
    if (dl_rb_write_state.write_crc == write_crc)
    {
        dl_rb->write_index = dl_rb_write_state.write_index;
        dl_rb->write_mirror = dl_rb_write_state.write_mirror;
        result = 0;
    }
    else
    {
        rt_kprintf("channel %d, dl_rb_read_state crc_check error\n", ap_channel_id);
    }

    return result;
}

static void uc_wiota_ringbuffer_set_dl_read_idx(void)
{
    dl_rb_read_state.read_index = dl_rb->read_index;
    dl_rb_read_state.read_mirror = dl_rb->read_mirror;
    dl_rb_read_state.channel = ap_channel_id;
    dl_rb_read_state.read_crc = uc_wiota_crc8_calc((unsigned char *)&dl_rb_read_state, sizeof(rb_read_state_t) - UC_CRC8_LEN);
}

static int uc_wiota_ringbuffer_get_ul_read_idx(void)
{
    int result = 1;
    unsigned short read_crc = 0;

    if (ul_rb_read_state.channel != ap_channel_id)
    {
        rt_kprintf("channel %d, read ul_rb_read_state from other channel %d\n", ap_channel_id, ul_rb_read_state.channel);
        return result;
    }

    for (int check_cnt = 0; check_cnt < MAX_CHECK_CNT; check_cnt++)
    {
        read_crc = uc_wiota_crc8_calc((unsigned char *)&ul_rb_read_state, sizeof(rb_read_state_t) - UC_CRC8_LEN);
        if (ul_rb_read_state.read_crc == read_crc)
        {
            ul_rb->read_index = ul_rb_read_state.read_index;
            ul_rb->read_mirror = ul_rb_read_state.read_mirror;
            result = 0;
            break;
        }
        else
        {
            rt_kprintf("channel %d, ul_rb_read_state crc_check error, cnt %d\n", ap_channel_id, check_cnt);
            rt_thread_mdelay(4);
        }
    }

    return result;
}

static void uc_wiota_ringbuffer_set_ul_write_idx(void)
{
    ul_rb_write_state.write_index = ul_rb->write_index;
    ul_rb_write_state.write_mirror = ul_rb->write_mirror;
    ul_rb_write_state.channel = ap_channel_id;
    ul_rb_write_state.write_crc = uc_wiota_crc8_calc((unsigned char *)&ul_rb_write_state, sizeof(rb_write_state_t) - UC_CRC8_LEN);
}

static unsigned short uc_wiota_get_dl_data_len(void)
{
    unsigned short dl_data_len = rt_ringbuffer_data_len(dl_rb);

    if (dl_data_len == 0)
    {
        // rt_kprintf("channel %d, no data, dl_rb:%04d,%d,%04d,%d, cnt %u\n",
        //            ap_channel_id,
        //            dl_rb->write_index,
        //            dl_rb->write_mirror,
        //            dl_rb->read_index,
        //            dl_rb->read_mirror,
        //            dl_irq_cnt);
        return 0;
    }

    if ((dl_data_len < sizeof(cmd_header_t)) || (dl_data_len > dl_rb->buffer_size))
    {
        rt_kprintf("channel %d, dl_data_len %d invalid\n", ap_channel_id, dl_data_len);
        return 0;
    }

    return dl_data_len;
}

static unsigned short uc_wiota_ringbuffer_get(struct rt_ringbuffer *rb, unsigned char *ptr, unsigned short data_len)
{
    RT_ASSERT(rb != RT_NULL);

    unsigned short read_len = RT_ALIGN(data_len, RING_BUFFER_ALIGN_SIZE);

    if (rb->buffer_size - rb->read_index > read_len)
    {
        /* copy all of data */
        rt_memcpy(ptr, &rb->buffer_ptr[rb->read_index], read_len);
        rb->read_index += read_len;
    }
    else
    {
        rt_memcpy(&ptr[0],
                  &rb->buffer_ptr[rb->read_index],
                  rb->buffer_size - rb->read_index);
        rt_memcpy(&ptr[rb->buffer_size - rb->read_index],
                  &rb->buffer_ptr[0],
                  read_len - (rb->buffer_size - rb->read_index));

        rb->read_mirror = ~rb->read_mirror;
        rb->read_index = read_len - (rb->buffer_size - rb->read_index);
    }

    return read_len;
}

static unsigned short uc_wiota_ringbuffer_put(struct rt_ringbuffer *rb, unsigned char *data, unsigned short length)
{
    RT_ASSERT(rb != RT_NULL);

    unsigned short write_len = RT_ALIGN(length, RING_BUFFER_ALIGN_SIZE);
    unsigned short space_length = 0;

    space_length = rt_ringbuffer_space_len(rb);

    /* no space */
    if (space_length == 0)
    {
        rt_kprintf("channel %d, no space, ul_rb:%04d,%d,%04d,%d, len %04d, cnt %u\n",
                   ap_channel_id, rb->write_index, rb->write_mirror, rb->read_index, rb->read_mirror, write_len, ul_irq_cnt);
        return 0;
    }

    /* space not enough */
    if (space_length < write_len)
    {
        rt_kprintf("channel %d, space not enough, ul_rb:%04d,%d,%04d,%d, len %04d, cnt %u\n",
                   ap_channel_id, rb->write_index, rb->write_mirror, rb->read_index, rb->read_mirror, write_len, ul_irq_cnt);
        return 0;
    }

    if (rb->buffer_size - rb->write_index > write_len)
    {
        /* read_index - write_index = empty space */
        rt_memcpy(&rb->buffer_ptr[rb->write_index], data, write_len);
        /* this should not cause overflow because there is enough space for
         * write_len of data in current mirror */
        rb->write_index += write_len;
    }
    else
    {
        /* we are going into the other side of the mirror */
        rt_memcpy(&rb->buffer_ptr[rb->write_index],
                  &data[0],
                  rb->buffer_size - rb->write_index);
        rt_memcpy(&rb->buffer_ptr[0],
                  &data[rb->buffer_size - rb->write_index],
                  write_len - (rb->buffer_size - rb->write_index));

        /* we are going into the other side of the mirror */
        rb->write_mirror = ~rb->write_mirror;
        rb->write_index = write_len - (rb->buffer_size - rb->write_index);
    }

    return write_len;
}

static void uc_wiota_sub_data_free(void *sub_data)
{
    RT_ASSERT(sub_data);

    rt_free(sub_data);
    sub_data = RT_NULL;
}

static int uc_wiota_send_data_to_gateway(struct cmd_send_para *cmd_info)
{
    unsigned short total_len = 0; // include head + (data + crc32)
    unsigned short data_len = 0;
    unsigned short align_data_len = 0;
    unsigned char *write_buf = RT_NULL;
    unsigned short write_len = 0;
    int result = 0;

    rt_mutex_take(cmd_lock, RT_WAITING_FOREVER);
    // add header
    total_len += sizeof(cmd_header_t);

    // add data
    for (int i = 0; i < cmd_info->para_num; i++)
    {
        data_len += cmd_info->para_len[i];
        align_data_len = RT_ALIGN(data_len, RING_BUFFER_ALIGN_SIZE);
    }
    total_len += align_data_len;

    // write cmd head
    cmd_info->cmd_header->data_len = data_len;
    cmd_info->cmd_header->channel = uc_wiota_get_channel_id();
    cmd_info->cmd_header->result = WIOTA_CMD_EXEC_SUC;
    cmd_info->cmd_header->header_crc = uc_wiota_crc32_calc((unsigned char *)cmd_info->cmd_header, sizeof(cmd_header_t) - UC_CRC32_LEN);

    if (align_data_len > 0)
    {
        // add crc32 len
        total_len += UC_CRC32_LEN;

        // malloc heap mem
        write_buf = rt_malloc(total_len);
        if (RT_NULL == write_buf)
        {
            rt_kprintf("channel %d, parse malloc fail\n", ap_channel_id);
            rt_mutex_release(cmd_lock);
            return -RT_EFULL;
        }
        rt_memset(write_buf, 0, total_len);
        rt_memcpy(write_buf, (unsigned char *)cmd_info->cmd_header, sizeof(cmd_header_t));
        write_len += sizeof(cmd_header_t);

        // copy data
        for (int i = 0; i < cmd_info->para_num; i++)
        {
            rt_memcpy(&write_buf[write_len], cmd_info->para_ptr[i], cmd_info->para_len[i]);
            write_len += cmd_info->para_len[i];
        }
        // data align 4byte
        write_len += (align_data_len - data_len);

        // add crc
        *(unsigned int *)&write_buf[write_len] = uc_wiota_crc32_calc(write_buf, write_len);
        write_len += UC_CRC32_LEN;
    }
    else
    {
        // only header, not add crc
        write_buf = (unsigned char *)cmd_info->cmd_header;
        write_len += sizeof(cmd_header_t);
    }

    // get ul_rb read_index from gateway
    if (0 == uc_wiota_ringbuffer_get_ul_read_idx())
    {
        // put data to ul_rb
        // note:update write_len of whether rb has space or not and enable irq
        uc_wiota_ringbuffer_put(ul_rb, write_buf, write_len);
        // update ul_rb write_index to gateway
        uc_wiota_ringbuffer_set_ul_write_idx();
        // enable irq
        uc_wiota_enable_ul_irq();

        // rt_kprintf("channel %d, ul_rb:%04d,%d,%04d,%d, len %04d, cnt %u\n",
        //            ap_channel_id,
        //            ul_rb->write_index,
        //            ul_rb->write_mirror,
        //            ul_rb->read_index,
        //            ul_rb->read_mirror,
        //            write_len,
        //            ul_irq_cnt);
    }
    else
    {
        result = 1;
    }

    if (align_data_len > 0)
    {
        rt_free(write_buf);
    }
    rt_mutex_release(cmd_lock);

    return result;
}

// static void uc_wiota_printf_data(unsigned char *data, int data_len)
// {
//     for (unsigned int i = 0; i < data_len; i++)
//     {
//         if (i != 0 && i % 16 == 0)
//         {
//             rt_kprintf("\n");
//         }
//         rt_kprintf("%02x ", data[i]);
//     }
//     rt_kprintf("\n");
// }

void uc_wiota_send_heart_msg(unsigned int isr_cnt)
{
    heart_info.isr_cnt = isr_cnt;
    heart_info.crc_limit = uc_wiota_get_crc();
    heart_info.ap8288_state = uc_wiota_get_ap8288_state();
    heart_info.channel = ap_channel_id;
    heart_info.heart_crc = uc_wiota_crc16_calc((unsigned char *)&heart_info, sizeof(heart_info_t) - UC_CRC16_LEN);

    if (heart_info.ap8288_state != 1)
    {
        rt_kprintf("channel %d, hert_msg, cnt %d, crc %d, state %d, h_crc 0x%x\n",
                   ap_channel_id, heart_info.isr_cnt, heart_info.crc_limit, heart_info.ap8288_state, heart_info.heart_crc);
    }
}

void uc_wiota_recv_master_ready_msg(void)
{
}

// static void uc_wiota_drop_callback(unsigned int user_id)
// {
//     drop_info_t drop_info = {0};

//     drop_info.cmd_header.cmd_type = CMD_TYPE_RECV;
//     drop_info.cmd_header.cmd = WIOTA_DROP;
//     drop_info.user_id = user_id;

//     uc_wiota_ringbuffer_put(ul_rb, (unsigned char *)&drop_info, sizeof(drop_info_t));
// }

static void uc_wiota_recv_detail_callback(uc_recv_detail_t *recv_detail)
{
    recv_info_t recv_info = {0};
    struct cmd_send_para cmd_info = {0};

    recv_info.cmd_header.cmd_type = CMD_TYPE_RECV;
    recv_info.cmd_header.cmd = WIOTA_RECV_DATA;
    recv_info.user_id = recv_detail->user_id;
    recv_info.data_type = recv_detail->data_type;
    recv_info.data_len = recv_detail->data_len;
    recv_info.rssi = recv_detail->rssi;
    recv_info.delay = recv_detail->delay;
    recv_info.fn_cnt = recv_detail->fn_cnt;
    recv_info.group_idx = recv_detail->group_idx;
    recv_info.burst_idx = recv_detail->burst_idx;
    recv_info.slot_idx = recv_detail->slot_idx;
    recv_info.frame_num = recv_detail->frame_num;

    // cmd
    cmd_info.cmd_header = &recv_info.cmd_header,
    cmd_info.para_num = 2;
    // para 1
    cmd_info.para_ptr[0] = ((unsigned char *)&recv_info) + sizeof(cmd_header_t);
    cmd_info.para_len[0] = sizeof(recv_info_t) - sizeof(cmd_header_t);
    //para2
    cmd_info.para_ptr[1] = recv_detail->data;
    cmd_info.para_len[1] = recv_detail->data_len;

    if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
    {
        rt_kprintf("recv detail send fail\n");
    }
    // uc_wiota_printf_data(recv_data, data_len);
}

static void uc_wiota_paging_ctrl_callback(unsigned int user_id, unsigned char burst_idx, unsigned int fn_index)
{
    paging_ctrl_recv_t paging_recv = {0};
    struct cmd_send_para cmd_info = {0};

    paging_recv.cmd_header.cmd_type = CMD_TYPE_RECV;
    paging_recv.cmd_header.cmd = WIOTA_PAGING_CTRL;
    paging_recv.user_id = user_id;
    paging_recv.burst_idx = burst_idx;
    paging_recv.fn_index = fn_index;

    // cmd
    cmd_info.cmd_header = &paging_recv.cmd_header,
    cmd_info.para_num = 1;
    // para 1
    cmd_info.para_ptr[0] = ((unsigned char *)&paging_recv) + sizeof(cmd_header_t);
    cmd_info.para_len[0] = sizeof(paging_ctrl_recv_t) - sizeof(cmd_header_t);

    if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
    {
        rt_kprintf("pg ctrl send fail\n");
    }
}

static void uc_wiota_temp_callback(uc_temp_recv_t *temp_result)
{
    temp_res_t temp_res = {0};
    struct cmd_send_para cmd_info = {0};

    temp_res.cmd_header.cmd_type = CMD_TYPE_SEND;
    temp_res.cmd_header.cmd = WIOTA_TEMP_QUERY;
    temp_res.cmd_header.result = WIOTA_CMD_EXEC_SUC;
    temp_res.temp = temp_result->temp;
    temp_res.result = temp_result->result;

    // cmd
    cmd_info.cmd_header = &temp_res.cmd_header,
    cmd_info.para_num = 1;
    // para 1
    cmd_info.para_ptr[0] = ((unsigned char *)&temp_res) + sizeof(cmd_header_t);
    cmd_info.para_len[0] = sizeof(temp_res_t) - sizeof(cmd_header_t);

    if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
    {
        rt_kprintf("read temp send fail\n");
    }
}

static void uc_wiota_handle_send_result(uc_send_recv_t *send_result, unsigned char cmd)
{
    send_res_t send_res = {0};
    struct cmd_send_para cmd_info = {0};

    send_res.user_id = send_result->user_id;
    send_res.result = send_result->result;
    send_res.data_id = send_result->data_id;
    send_res.cmd_header.cmd_type = CMD_TYPE_SEND;
    send_res.cmd_header.cmd = cmd;
    send_res.cmd_header.result = WIOTA_CMD_EXEC_SUC;

    // cmd
    cmd_info.cmd_header = &send_res.cmd_header,
    cmd_info.para_num = 1;
    // para 1
    cmd_info.para_ptr[0] = ((unsigned char *)&send_res) + sizeof(cmd_header_t);
    cmd_info.para_len[0] = sizeof(send_res_t) - sizeof(cmd_header_t);

    if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
    {
        rt_kprintf("send res send fail\n");
    }
    else
    {
        if (cmd == WIOTA_SEND_DATA)
        {
            g_del_cnt++;
        }
    }
}

static void uc_wiota_send_callback(uc_send_recv_t *send_result)
{
    uc_wiota_handle_send_result(send_result, WIOTA_SEND_DATA);
}

static void uc_wiota_send_bc_callback(uc_send_recv_t *send_result)
{
    uc_wiota_handle_send_result(send_result, WIOTA_SEND_BC);
}

static void uc_wiota_send_mc_callback(uc_send_recv_t *send_result)
{
    uc_wiota_handle_send_result(send_result, WIOTA_SEND_MC);
}

static void uc_wiota_scan_callback(uc_scan_recv_t *scan_result)
{
    scan_res_t scan_res = {0};
    struct cmd_send_para cmd_info = {0};

    scan_res.cmd_header.cmd_type = CMD_TYPE_SEND;
    scan_res.cmd_header.cmd = WIOTA_SCAN_FREQ;
    scan_res.cmd_header.result = WIOTA_CMD_EXEC_SUC;
    scan_res.data_len = scan_result->data_len;
    scan_res.result = scan_result->result;

    // cmd
    cmd_info.cmd_header = &scan_res.cmd_header,
    cmd_info.para_num = 1;
    // para 1
    cmd_info.para_ptr[0] = ((unsigned char *)&scan_res) + sizeof(cmd_header_t);
    cmd_info.para_len[0] = sizeof(scan_res_t) - sizeof(cmd_header_t);
    // para 2
    if (scan_result->result == UC_OP_SUCC)
    {
        cmd_info.para_num += 1;
        cmd_info.para_ptr[1] = scan_result->data;
        cmd_info.para_len[1] = scan_result->data_len;
    }
    if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
    {
        rt_kprintf("scan res send fail\n");
    }

    if (scan_result->result == UC_OP_SUCC)
    {
        rt_free(scan_result->data);
        scan_result->data = RT_NULL;
    }
}

static void uc_wiota_sync_paging_callback(uc_paging_recv_t *paging_result)
{
    paging_res_t paging_res = {0};
    struct cmd_send_para cmd_info = {0};

    paging_res.cmd_header.cmd_type = CMD_TYPE_SEND;
    paging_res.cmd_header.cmd = WIOTA_SYNC_PAGING;
    paging_res.cmd_header.result = WIOTA_CMD_EXEC_SUC;
    paging_res.result = paging_result->result;
    paging_res.user_id = paging_result->user_id;

    // cmd
    cmd_info.cmd_header = &paging_res.cmd_header,
    cmd_info.para_num = 1;
    // para 1
    cmd_info.para_ptr[0] = ((unsigned char *)&paging_res) + sizeof(cmd_header_t);
    cmd_info.para_len[0] = sizeof(paging_res_t) - sizeof(cmd_header_t);

    if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
    {
        rt_kprintf("sp res send fail\n");
    }
}

static void uc_wiota_time_service_info_cb(uc_ts_info_t *ts_info)
{
    ts_info_t ts_info_ex = {0};
    struct cmd_send_para cmd_info = {0};

    if (ts_info->ts_state == TIME_SERVICE_INIT_END ||
        ts_info->ts_state == TIME_SERVICE_ALIGN_END)
    {
        ts_info_ex.cmd_header.cmd_type = CMD_TYPE_RECV;
        ts_info_ex.cmd_header.cmd = WIOTA_TS_STATE;
        ts_info_ex.cmd_header.result = WIOTA_CMD_EXEC_SUC;
        rt_memcpy(&ts_info_ex.ts_info, ts_info, sizeof(uc_ts_info_t));

        // cmd
        cmd_info.cmd_header = &ts_info_ex.cmd_header,
        cmd_info.para_num = 1;
        // para 1
        cmd_info.para_ptr[0] = ((unsigned char *)&ts_info_ex) + sizeof(cmd_header_t);
        cmd_info.para_len[0] = sizeof(ts_info_t) - sizeof(cmd_header_t);

        if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
        {
            rt_kprintf("ts res send fail\n");
        }
    }
}

static void uc_wiota_fn_refresh(unsigned int frame_num)
{
    // save the frame num to the last 4 bytes of shared memory
    uc_wiota_store_frame_num(frame_num);

    // update the heart detection info every 30 frames
    if (frame_num % 5 == 0)
    {
        uc_wiota_send_heart_msg(frame_num);
    }

    if (frame_num % 30 == 0)
    {
        uc_wiota_print_data_cnt();
    }

    if (g_report_fn_to_gw)
    {
        fn_refresh_t fn_info = {0};
        struct cmd_send_para cmd_info = {0};

        fn_info.cmd_header.cmd_type = CMD_TYPE_RECV;
        fn_info.cmd_header.cmd = WIOTA_FN_REFRESH;
        fn_info.cmd_header.result = WIOTA_CMD_EXEC_SUC;
        fn_info.frame_num = frame_num;

        // cmd
        cmd_info.cmd_header = &fn_info.cmd_header,
        cmd_info.para_num = 1;
        // para 1
        cmd_info.para_ptr[0] = ((unsigned char *)&fn_info) + sizeof(cmd_header_t);
        cmd_info.para_len[0] = sizeof(fn_refresh_t) - sizeof(cmd_header_t);

        if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
        {
            rt_kprintf("fn refresh send fail\n");
        }
    }
}

static void uc_wiota_handle_exec_msg(unsigned char *sub_data)
{
    exec_cmd_info_t *exec_cmd = (exec_cmd_info_t *)sub_data;
    exec_result_info_t result = {0};
    struct cmd_send_para cmd_info = {0};

    switch (exec_cmd->cmd_header.cmd)
    {
    case WIOTA_INIT:
    {
        unsigned char func_gnss = uc_wiota_get_time_service_func(TIME_SERVICE_GNSS);
        unsigned char func_1588 = uc_wiota_get_time_service_func(TIME_SERVICE_1588_PS);
        unsigned char func_sync = uc_wiota_get_time_service_func(TIME_SERVICE_SYNC_ASSISTANT);

        if (func_gnss || func_1588 || func_sync)
        {
            uc_wiota_register_time_service_info_callback(uc_wiota_time_service_info_cb);
        }
        uc_wiota_init();
        break;
    }

    case WIOTA_RUN:
        uc_wiota_run();
        uc_wiota_register_recv_data_detail_callback(uc_wiota_recv_detail_callback);
        uc_wiota_register_sync_paging_callback(uc_wiota_paging_ctrl_callback);
        uc_wiota_register_fn_refresh_callback(uc_wiota_fn_refresh);
        break;

    case WIOTA_EXIT:
        uc_wiota_exit();
        break;

    case WIOTA_REBOOT:
        result.cmd_header.result = WIOTA_CMD_EXEC_SUC;
        result.cmd_header.cmd_type = exec_cmd->cmd_header.cmd_type;
        result.cmd_header.cmd = exec_cmd->cmd_header.cmd;

        // cmd res
        cmd_info.cmd_header = &result.cmd_header,
        cmd_info.para_num = 0;

        if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
        {
            rt_kprintf("reboot send fail\n");
        }
        rt_thread_mdelay(1000);
        uc8088_chip_reset();
        break;

    case WIOTA_DFE_COUNTER_FLAG_SET:
        uc_wiota_set_dfe_counter_flag();
        break;

    case WIOTA_DFE_COUNTER_SEND:
        uc_wiota_master_control_bc_dfe_counter();
        break;

    case WIOTA_PAGING_TX_START:
        uc_wiota_paging_tx_frame_start();
        break;

    case WIOTA_MASTER_READY:
        uc_wiota_recv_master_ready_msg();
        break;

    case WIOTA_TS_START:
        uc_wiota_time_service_start();
        break;

    case WIOTA_TS_STOP:
        uc_wiota_time_service_stop();
        break;

    case WIOTA_REG_FN_REFRESH_CB:
        // uc_wiota_register_fn_refresh_callback(uc_wiota_fn_refresh);
        // default register, but only user register reports frame num to gateway
        g_report_fn_to_gw = 1;
        break;

    default:
        break;
    }

    result.cmd_header.result = WIOTA_CMD_EXEC_SUC;
    result.cmd_header.cmd_type = exec_cmd->cmd_header.cmd_type;
    result.cmd_header.cmd = exec_cmd->cmd_header.cmd;

    // cmd
    cmd_info.cmd_header = &result.cmd_header,
    cmd_info.para_num = 0;

    if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
    {
        rt_kprintf("exec res send fail\n");
    }
}

static void uc_wiota_handle_setup_msg(unsigned char *sub_data)
{
    setup_cmd_info_t *setup_cmd = (setup_cmd_info_t *)sub_data;
    exec_result_info_t result = {0};
    struct cmd_send_para cmd_info = {0};
    unsigned int *in_value = setup_cmd->in_value;

    // for (int i = 0; i < 11; i++)
    //     rt_kprintf("setup_cmd in_value[%d] %d\n", i, in_value[i]);

    switch (setup_cmd->cmd_header.cmd)
    {
    case WIOTA_ACT_SET:
        uc_wiota_set_active_time(in_value[0]);
        break;

    case WIOTA_CONFIG_SET:
    {
        sub_system_config_t config = {0};

        config.ap_tx_power = in_value[0];
        config.id_len = in_value[1];
        config.pp = in_value[2];
        config.symbol_length = in_value[3];
        config.dlul_ratio = in_value[4];
        config.bt_value = in_value[5];
        config.group_number = in_value[6];
        config.spectrum_idx = in_value[7];
        config.old_subsys_v = in_value[8];
        config.bitscb = in_value[9];
        config.subsystem_id = in_value[10];
        uc_wiota_set_system_config(&config);
        break;
    }

    case WIOTA_POWER_SET:
        uc_wiota_set_ap_tx_power(in_value[0]);
        break;

    case WIOTA_FREQ_SET:
        uc_wiota_set_freq_info(in_value[0]);
        break;

    case WIOTA_HOPPING_FREQ_SET:
        uc_wiota_set_hopping_freq(in_value[0]);
        break;

    case WIOTA_HOPPING_MODE_SET:
        uc_wiota_set_hopping_mode(in_value[0], in_value[1]);
        break;

    case WIOTA_MAX_IOTE_NUM_SET:
        uc_wiota_set_max_num_of_active_iote(in_value[0]);
        break;

    case WIOTA_DATA_RATE_SET:
        uc_wiota_set_data_rate(in_value[0], in_value[1]);
        break;

    case WIOTA_BC_MCS_SET:
        uc_wiota_set_broadcast_mcs(in_value[0]);
        break;

    case WIOTA_CRC_SET:
        uc_wiota_set_crc(in_value[0]);
        break;

    case WIOTA_LOG_SET:
        uc_wiota_log_switch(in_value[0], in_value[1]);
        break;

    case WIOTA_DEFAULT_DFE_SET:
        uc_wiota_set_default_dfe(in_value[0]);
        break;

    case WIOTA_FB_ALIGN_SET:
        uc_wiota_set_frame_boundary_align_func(in_value[0]);
        break;

    case WIOTA_PAGING_TX_SET:
    {
        uc_lpm_tx_cfg_t config = {0};

        config.freq = in_value[0];
        config.spectrum_idx = in_value[1];
        config.bandwidth = in_value[2];
        config.symbol_length = in_value[3];
        config.awaken_id = in_value[4];
        config.send_time = in_value[5];
        config.mode = in_value[6];

        uc_wiota_set_paging_tx_cfg(&config);
        break;
    }

    case WIOTA_BC_FN_CYCLE_SET:
        uc_wiota_set_broadcast_fn_cycle(in_value[0]);
        break;

    case WIOTA_BC_SEND_ROUND_SET:
        uc_wiota_set_broadcast_send_round(in_value[0]);
        break;

    case WIOTA_TS_FUNC_SET:
        uc_wiota_set_time_service_func(in_value[0], in_value[1]);
        break;

    case WIOTA_TS_CYCLE_SET:
        uc_wiota_set_time_service_cycle(in_value[0]);
        break;

    case WIOTA_1588_SYNC_SET:
        uc_wiota_set_1588_protocol_rtc(in_value[0], in_value[1]);
        break;

    case WIOTA_PPS_SET:
        uc_wiota_set_sync_assistant_pps(in_value[0]);
        break;

    case WIOTA_SINGLE_TONE_SET:
        uc_wiota_set_single_tone(in_value[0]);
        break;

    case WIOTA_UL_SUBF_MODE_SET:
        uc_wiota_set_ul_subframe_mode(in_value[0], in_value[1], in_value[2]);
        break;

    case WIOTA_SUBF_MODE_CFG_SET:
    {
        uc_subf_cfg_t subf_cfg = {0};

        subf_cfg.block_size = in_value[0];
        subf_cfg.send_round = in_value[1];
        uc_wiota_set_subframe_mode_cfg(&subf_cfg);
        break;
    }

    case WIOTA_SM_RESEND_TIMES_SET:
        uc_wiota_set_sm_resend_times(in_value[0]);
        break;

    case WIOTA_PAGING_RX_SET:
    {
        uc_lpm_rx_cfg_t config = {0};

        config.freq = in_value[0];
        config.spectrum_idx = in_value[1];
        config.bandwidth = in_value[2];
        config.symbol_length = in_value[3];
        config.lpm_nlen = in_value[4];
        config.lpm_utimes = in_value[5];
        config.threshold = in_value[6];
        config.extra_flag = in_value[7];
        config.awaken_id = in_value[8];
        config.detect_period = in_value[9];
        config.extra_period = in_value[10];
        config.mode = in_value[11];
        config.period_multiple = in_value[12];
        config.awaken_id_another = in_value[13];

        uc_wiota_set_paging_rx_cfg(&config);
        break;
    }

    case WIOTA_AAGC_IDX_SET:
        uc_wiota_set_aagc_idx(in_value[0]);
        break;

    case WIOTA_BNACK_FUNC_SET:
        uc_wiota_set_bnack_func(in_value[0]);
        break;

    default:
        break;
    }

    result.cmd_header.result = WIOTA_CMD_EXEC_SUC;
    result.cmd_header.cmd_type = setup_cmd->cmd_header.cmd_type;
    result.cmd_header.cmd = setup_cmd->cmd_header.cmd;

    //cmd res
    cmd_info.cmd_header = &result.cmd_header,
    cmd_info.para_num = 0;

    if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
    {
        rt_kprintf("setup res send fail\n");
    }
}

static void uc_wiota_handle_query_msg(unsigned char *sub_data)
{
    query_cmd_info_t *query_cmd = (query_cmd_info_t *)sub_data;
    query_result_info_t query_result = {0};
    struct cmd_send_para cmd_info = {0};
    unsigned int *in_value = query_cmd->in_value;
    unsigned int *out_value = query_result.out_value;
    unsigned int out_value_num = 1;

    // for (int i = 0; i < 2; i++)
    //     rt_kprintf("query_cmd in_value[%d] %d\n", i, in_value[i]);

    switch (query_cmd->cmd_header.cmd)
    {
    case WIOTA_ACT_GET:
        out_value[0] = uc_wiota_get_active_time();
        break;

    case WIOTA_CONFIG_GET:
    {
        sub_system_config_t config = {0};

        uc_wiota_get_system_config(&config);
        out_value[0] = config.ap_tx_power;
        out_value[1] = config.id_len;
        out_value[2] = config.pp;
        out_value[3] = config.symbol_length;
        out_value[4] = config.dlul_ratio;
        out_value[5] = config.bt_value;
        out_value[6] = config.group_number;
        out_value[7] = config.spectrum_idx;
        out_value[8] = config.old_subsys_v;
        out_value[9] = config.bitscb;
        out_value[10] = config.subsystem_id;
        out_value_num = 11;
        break;
    }

    case WIOTA_FREQ_GET:
        out_value[0] = uc_wiota_get_freq_info();
        break;

    case WIOTA_MAX_IOTE_NUM_GET:
        out_value[0] = uc_wiota_get_max_num_of_active_iote();
        break;

    case WIOTA_DATA_RATE_GET:
        out_value[0] = uc_wiota_get_data_rate_value(in_value[0]);
        break;

    case WIOTA_BC_MCS_GET:
        out_value[0] = uc_wiota_get_broadcast_mcs();
        break;

    case WIOTA_CRC_GET:
        out_value[0] = uc_wiota_get_crc();
        rt_kprintf("crc_limit %d\n", out_value[0]);
        break;

    case WIOTA_FRAME_HEAD_DFE_GET:
        out_value[0] = uc_wiota_get_frame_head_dfe();
        break;

    case WIOTA_DFE_COUNTER_GET:
        out_value[0] = uc_wiota_read_dfe_counter(in_value[0]);
        break;

    case WIOTA_AP_STATE_GET:
        out_value[0] = uc_wiota_get_ap8288_state();
        rt_kprintf("ap8288_state %d\n", out_value[0]);
        break;

    case WIOTA_PAGING_TX_GET:
    {
        uc_lpm_tx_cfg_t config = {0};

        uc_wiota_get_paging_tx_cfg(&config);

        out_value[0] = config.freq;
        out_value[1] = config.spectrum_idx;
        out_value[2] = config.bandwidth;
        out_value[3] = config.symbol_length;
        out_value[4] = config.awaken_id;
        out_value[5] = config.send_time;
        out_value_num = 6;
        break;
    }

    case WIOTA_BC_FN_CYCLE_GET:
        out_value[0] = uc_wiota_get_broadcast_fn_cycle();
        break;

    case WIOTA_BC_SEND_ROUND_GET:
        out_value[0] = uc_wiota_get_broadcast_send_round();
        break;

    case WIOTA_FRAME_LEN_GET:
        out_value[0] = uc_wiota_get_frame_len();
        break;

    case WIOTA_FRAME_NUM_GET:
        out_value[0] = uc_wiota_get_frame_num();
        break;

    case WIOTA_TS_FUNC_GET:
        out_value[0] = uc_wiota_get_time_service_func(TIME_SERVICE_GNSS);
        out_value[1] = uc_wiota_get_time_service_func(TIME_SERVICE_1588_PS);
        out_value[2] = uc_wiota_get_time_service_func(TIME_SERVICE_SYNC_ASSISTANT);
        break;

    case WIOTA_PPS_GET:
        out_value[0] = uc_wiota_get_sync_assistant_pps();
        break;

    default:
        break;
    }
    // for (int i = 0; i < 11; i++)
    //     rt_kprintf("query_cmd out_value[%d] %d\n", i, out_value[i]);

    query_result.cmd_header.result = WIOTA_CMD_EXEC_SUC;
    query_result.cmd_header.cmd_type = query_cmd->cmd_header.cmd_type;
    query_result.cmd_header.cmd = query_cmd->cmd_header.cmd;

    // cmd res
    cmd_info.cmd_header = &query_result.cmd_header,
    cmd_info.para_num = 1;
    cmd_info.para_ptr[0] = query_result.out_value;
    cmd_info.para_len[0] = out_value_num * sizeof(unsigned int);

    if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
    {
        rt_kprintf("query res send fail\n");
    }
}

static void uc_wiota_handle_send_msg(unsigned char *sub_data, unsigned char cmd)
{
    switch (cmd)
    {
    case WIOTA_SEND_DATA:
    case WIOTA_SEND_MC:
    case WIOTA_SEND_BC:
    {
        send_info_t *send_info = (send_info_t *)sub_data;
        uc_send_recv_t send_result = {0};

        if (cmd == WIOTA_SEND_DATA)
        {
            // rt_kprintf("send_uc data_id 0x%x, data_len %d, user_id 0x%x, timeout %d, is_block %d\n",
            //              send_info->data_id, send_info->data_len, send_info->user_id, send_info->timeout, send_info->is_block);
            g_add_cnt++;
            send_result.result = uc_wiota_send_data_order(send_info->data,
                                                          send_info->data_len,
                                                          send_info->user_id,
                                                          send_info->timeout,
                                                          send_info->order_business,
                                                          send_info->is_block ? RT_NULL : uc_wiota_send_callback,
                                                          (void *)send_info->data_id);
        }
        else if (cmd == WIOTA_SEND_MC)
        {
            // rt_kprintf("send_mc data_id 0x%x, data_len %d, mc_id 0x%x, timeout %d, is_block %d\n",
            //            send_info->data_id, send_info->data_len, send_info->user_id, send_info->timeout, send_info->is_block);
            send_result.result = uc_wiota_send_multicast_data(send_info->data,
                                                              send_info->data_len,
                                                              send_info->user_id,
                                                              send_info->timeout,
                                                              send_info->is_block ? RT_NULL : uc_wiota_send_mc_callback,
                                                              (void *)send_info->data_id);
        }
        else
        {
            // rt_kprintf("send_bc data_id 0x%x, data_len %d, mode %d, timeout %d, is_block %d\n",
            //            send_info->data_id, send_info->data_len, send_info->user_id, send_info->timeout, send_info->is_block);
            send_result.result = uc_wiota_send_broadcast_data(send_info->data,
                                                              send_info->data_len,
                                                              send_info->user_id,
                                                              send_info->timeout,
                                                              send_info->is_block ? RT_NULL : uc_wiota_send_bc_callback,
                                                              (void *)send_info->data_id);
        }

        if (send_info->is_block)
        {
            send_result.user_id = send_info->user_id;
            send_result.data_id = send_info->data_id;
            uc_wiota_handle_send_result(&send_result, cmd);
        }
        break;
    }

    case WIOTA_SCAN_FREQ:
    {
        scan_info_t *scan_info = (scan_info_t *)sub_data;
        uc_scan_recv_t scan_result = {0};

        // rt_kprintf("scan_freq freq_num %d, scan_type %d, timeout %d, is_block %d\n",
        //            scan_info->freq_num, scan_info->scan_type, scan_info->timeout, scan_info->is_block);

        scan_result.result = uc_wiota_scan_freq(scan_info->freq_num == 0 ? RT_NULL : scan_info->freq,
                                                scan_info->freq_num,
                                                scan_info->scan_type,
                                                scan_info->timeout,
                                                scan_info->is_block ? RT_NULL : uc_wiota_scan_callback,
                                                &scan_result);
        if (scan_info->is_block)
        {
            uc_wiota_scan_callback(&scan_result);
        }
        break;
    }

    case WIOTA_TEMP_QUERY:
    {
        temp_info_t *temp_info = (temp_info_t *)sub_data;
        uc_temp_recv_t temp_recv = {0};

        // rt_kprintf("query temp timeout %d, is_block %d\n", temp_info->timeout, temp_info->is_block);
        temp_recv.result = uc_wiota_read_temperature(temp_info->is_block ? RT_NULL : uc_wiota_temp_callback, &temp_recv, temp_info->timeout);
        if (temp_info->is_block)
        {
            uc_wiota_temp_callback(&temp_recv);
        }
        break;
    }

    case WIOTA_SYNC_PAGING:
    {
        paging_info_t *paging_info = (paging_info_t *)sub_data;
        uc_paging_info_t paging = {0};
        uc_paging_recv_t paging_recv = {0};

        paging.user_id = paging_info->user_id;
        paging.fn_index = paging_info->fn_index;
        paging.detection_period = paging_info->detection_period;
        paging.send_round = paging_info->send_round;
        paging.continue_fn = paging_info->continue_fn;

        paging_recv.result = uc_wiota_sync_paging(&paging, paging_info->is_block ? RT_NULL : uc_wiota_sync_paging_callback);
        if (paging_info->is_block)
        {
            paging_recv.user_id = paging_info->user_id;
            uc_wiota_sync_paging_callback(&paging_recv);
        }
        break;
    }

    case WIOTA_RECV_SM_BY_FN:
    {
        rs_sm_by_fn_ex_t *sf_rs = (rs_sm_by_fn_ex_t *)sub_data;
        recv_send_by_fn_t rs_fn = {0};

        rs_fn.user_id = sf_rs->user_id;
        rs_fn.start_recv_fn = sf_rs->start_recv_fn;
        rs_fn.recv_fns = sf_rs->recv_fns;

        if (sf_rs->data_len && sf_rs->send_fns > 0)
        {
            rs_fn.send_fns = sf_rs->send_fns;
            rs_fn.data_len = sf_rs->data_len;
            rs_fn.data = sf_rs->data;
            rs_fn.callback = uc_wiota_send_callback;
            rs_fn.para = (void *)sf_rs->data_id;
            g_add_cnt++;
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
        break;
    }

    default:
    {
        RT_ASSERT(0);
        break;
    }
    }
}

static void uc_wiota_handle_other_msg(unsigned char *sub_data, unsigned char cmd)
{
    switch (cmd)
    {
    case WIOTA_BL_ADD:
    case WIOTA_BL_REMOVE:
    case WIOTA_MC_ID_ADD:
    case WIOTA_MC_ID_DEL:
    {
        id_info_t *id_info = (id_info_t *)sub_data;
        exec_result_info_t result = {0};
        struct cmd_send_para cmd_info = {0};
        unsigned int *id = RT_NULL;
        id = id_info->id;

        // for (int i = 0; i < id_info->id_num; i++)
        // {
        //     rt_kprintf("id 0x%x\n", id[i]);
        // }

        if (cmd == WIOTA_BL_ADD)
        {
            uc_wiota_add_iote_to_blacklist(id, id_info->id_num);
        }
        else if (cmd == WIOTA_BL_REMOVE)
        {
            uc_wiota_remove_iote_from_blacklist(id, id_info->id_num);
        }
        else if (cmd == WIOTA_MC_ID_ADD)
        {
            uc_wiota_set_multicast_id(id, id_info->id_num);
        }
        else
        {
            uc_wiota_del_multicast_id(id, id_info->id_num);
        }
        result.cmd_header.result = WIOTA_CMD_EXEC_SUC;
        result.cmd_header.cmd_type = CMD_TYPE_OTHER;
        result.cmd_header.cmd = cmd;

        // cmd
        cmd_info.cmd_header = &result.cmd_header,
        cmd_info.para_num = 0;

        if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
        {
            rt_kprintf("id_set res send fail\n");
        }
        break;
    }

    case WIOTA_BL_GET:
    {
        id_info_t *bl_info = RT_NULL;
        uc_blacklist_t *curr_node = RT_NULL;
        uc_blacklist_t *head_node = RT_NULL;
        struct cmd_send_para cmd_info = {0};
        unsigned short bl_num = 0;
        unsigned int total_len = 0;
        int i = 0;

        head_node = uc_wiota_get_blacklist(&bl_num);

        total_len = sizeof(id_info_t) + bl_num * sizeof(int);
        bl_info = rt_malloc(total_len);
        RT_ASSERT(bl_info);
        rt_memset(bl_info, 0, total_len);
        bl_info->cmd_header.result = WIOTA_CMD_EXEC_SUC;
        bl_info->cmd_header.cmd_type = CMD_TYPE_OTHER;
        bl_info->cmd_header.cmd = cmd;
        bl_info->id_num = bl_num;

        rt_slist_for_each_entry(curr_node, &head_node->node, node)
        {
            rt_kprintf("id:0x%x\n", curr_node->user_id);
            bl_info->id[i] = curr_node->user_id;
            i++;
        }

        // cmd
        cmd_info.cmd_header = &bl_info->cmd_header,
        cmd_info.para_num = 2;
        // para 1
        cmd_info.para_ptr[0] = ((unsigned char *)bl_info) + sizeof(cmd_header_t);
        cmd_info.para_len[0] = sizeof(id_info_t) - sizeof(cmd_header_t);
        // para
        cmd_info.para_ptr[1] = bl_info->id;
        cmd_info.para_len[1] = bl_num * sizeof(int);

        if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
        {
            rt_kprintf("bl_get res send fail\n");
        }

        rt_free(bl_info);
        bl_info = RT_NULL;

        break;
    }

    case WIOTA_VERSION_GET:
    {
        version_info_t version_info = {0};
        struct cmd_send_para cmd_info = {0};

        uc_wiota_get_version(version_info.wiota_version_8088,
                             version_info.git_info_8088,
                             version_info.make_time_8088,
                             version_info.wiota_version_8288,
                             version_info.git_info_8288,
                             version_info.make_time_8288,
                             &version_info.cce_version);

        version_info.cmd_header.result = WIOTA_CMD_EXEC_SUC;
        version_info.cmd_header.cmd_type = CMD_TYPE_OTHER;
        version_info.cmd_header.cmd = cmd;

        // cmd
        cmd_info.cmd_header = &version_info.cmd_header,
        cmd_info.para_num = 1;
        // para 1
        cmd_info.para_ptr[0] = ((unsigned char *)&version_info) + sizeof(cmd_header_t);
        cmd_info.para_len[0] = sizeof(version_info_t) - sizeof(cmd_header_t);

        if (uc_wiota_send_data_to_gateway(&cmd_info) != 0)
        {
            rt_kprintf("ver get res send fail\n");
        }
        break;
    }

    case WIOTA_ADD_DL_SUBF_DATA:
    {
        subf_data_ex_t *subf_data = (subf_data_ex_t *)sub_data;

        uc_wiota_add_dl_subframe_data(subf_data->data, subf_data->data_len, subf_data->fn);
        break;
    }

    default:
        break;
    }
}

static unsigned short uc_wiota_dl_data_parse(void)
{
    unsigned short dl_data_len = 0;
    unsigned char *dl_data = RT_NULL;
    cmd_header_t *cmd_header = RT_NULL;
    unsigned short sub_data_len = 0;
    unsigned char *sub_data = RT_NULL;
    unsigned short align_data_len = 0;
    unsigned int calc_crc = 0;
    unsigned int ori_crc = 0;
    unsigned short sub_data_num = 0;
    unsigned short sub_data_offset = 0;
    unsigned int header_crc = 0;
    int check_cnt = 0;
    // unsigned char cmd_header_error_cnt = 0;

    // get dl write_idx
    if (0 != uc_wiota_ringbuffer_get_dl_write_idx())
    {
        return 0;
    }

    // get dl_data_len
    dl_data_len = uc_wiota_get_dl_data_len();
    if (0 == dl_data_len)
    {
        return 0;
    }

    // malloc heap
    dl_data = rt_malloc(dl_data_len);
    if (RT_NULL == dl_data)
    {
        rt_kprintf("channel %d, parse malloc fail\n", ap_channel_id);
        return 0;
    }
    rt_memset(dl_data, 0, dl_data_len);

    // get dl_data and dl_data backup
    uc_wiota_ringbuffer_get(dl_rb, dl_data, dl_data_len);

    // update dl_rb read_index to gateway
    uc_wiota_ringbuffer_set_dl_read_idx();

    // parse dl_data
    while (1)
    {
        if (sub_data_offset >= dl_data_len) // all sub_data parse complete, exit loop
        {
            break;
        }
        sub_data = &dl_data[sub_data_offset];

        // cmd_header crc check
        for (check_cnt = 0; check_cnt < MAX_CHECK_CNT; check_cnt++)
        {
            cmd_header = (cmd_header_t *)sub_data;

            header_crc = uc_wiota_crc32_calc((unsigned char *)cmd_header, sizeof(cmd_header_t) - UC_CRC32_LEN);
            if (cmd_header->header_crc == header_crc)
            {
                if (check_cnt == MAX_CHECK_CNT - 1) // reset sub_data
                {
                    // cp rigth header to ori data
                    rt_memcpy(&dl_data[sub_data_offset], cmd_header, sizeof(cmd_header_t));
                    sub_data = &dl_data[sub_data_offset];
                    rt_kprintf("channel %d, use bk cmd_header\n", ap_channel_id);
                }
                break;
            }
            else
            {
                rt_kprintf("channel %d, cmd_header crc_check error, cnt %d\n", ap_channel_id, check_cnt);
                if (check_cnt == 0)
                {
                    sub_data = &dl_data[dl_data_len / 2];
                }
            }
        }

        if (check_cnt == MAX_CHECK_CNT) // cmd header check error
        {
            break;
        }

        check_cnt = 0;
        sub_data_len = 0;   // new cmd req
        align_data_len = 0; // clear align_data_len when loop start
        // cmd_header_error_cnt = 0; // reset error cnt

        // cmd_header len
        sub_data_len += sizeof(cmd_header_t);

        // sub_data and cmd_tail_len
        if (cmd_header->data_len > 0)
        {
            align_data_len = RT_ALIGN(cmd_header->data_len, RING_BUFFER_ALIGN_SIZE);
            sub_data_len += align_data_len;
            // crc32 len
            sub_data_len += UC_CRC32_LEN;

            // sub_data crc check
            for (check_cnt = 0; check_cnt < MAX_CHECK_CNT; check_cnt++)
            {
                calc_crc = uc_wiota_crc32_calc(sub_data, sub_data_len - UC_CRC32_LEN);
                ori_crc = *(unsigned int *)&sub_data[sub_data_len - UC_CRC32_LEN];
                if (calc_crc == ori_crc)
                {
                    if (check_cnt == MAX_CHECK_CNT - 1)
                    {
                        rt_kprintf("channel %d, use bk sub_data\n", ap_channel_id);
                    }
                    sub_data_len *= 2;
                    break;
                }
                else
                {
                    rt_kprintf("channel %d, sub_data crc_check error, len %d %d, cnt %d\n",
                               ap_channel_id, sub_data_len, dl_data_len, check_cnt);
                    // ctx_print_data_by_u8(sub_data, sub_data_len);
                    if (check_cnt == 0)
                    {
                        // cp right cmd header to backup data
                        rt_memcpy(&dl_data[sub_data_len], cmd_header, sizeof(cmd_header_t));
                        sub_data = &dl_data[sub_data_len];
                    }
                }
            }
        }
        else
        {
            sub_data_len += sizeof(cmd_header_t);
        }

        if (check_cnt == MAX_CHECK_CNT) // sub_data check error
        {
            break;
        }

        // dl_rb write_idx read error
        if (sub_data_len > dl_data_len)
        {
            rt_kprintf("channel %d, dl_data_len error, %d,%d\n", ap_channel_id, sub_data_len, dl_data_len);
            break;
        }

        // next sub_data begin pos
        sub_data_offset += sub_data_len;
        sub_data_num++; // sub_data_num +1 when sub_data check ok
        check_cnt = 0;

        // rt_kprintf("channel %d, dl_rb:%04d,%d,%04d,%d, len %03d/%04d/%04d, num %02d, cnt %u\n",
        //            ap_channel_id,
        //            dl_rb->write_index,
        //            dl_rb->write_mirror,
        //            dl_rb->read_index,
        //            dl_rb->read_mirror,
        //            sub_data_len, sub_data_offset, dl_data_len, sub_data_num, dl_irq_cnt);

        switch (cmd_header->cmd_type)
        {
        case CMD_TYPE_EXEC:
            uc_wiota_handle_exec_msg(sub_data);
            break;

        case CMD_TYPE_SETUP:
            uc_wiota_handle_setup_msg(sub_data);
            break;

        case CMD_TYPE_QUERY:
            uc_wiota_handle_query_msg(sub_data);
            break;

        case CMD_TYPE_SEND:
            uc_wiota_handle_send_msg(sub_data, cmd_header->cmd);
            break;

        case CMD_TYPE_OTHER:
            uc_wiota_handle_other_msg(sub_data, cmd_header->cmd);
            break;

        default:
            rt_kprintf("channel %d, recv error cmd_type %d\n", ap_channel_id, cmd_header->cmd_type);
            RT_ASSERT(0);
        }
    }

    // rt_kprintf("channel %d, dl_data parse complete, sub_data_num %d, dl_data_len %d\n", ap_channel_id, sub_data_num, dl_data_len);
    uc_wiota_sub_data_free(dl_data);

    return sub_data_offset;
}

static void uc_wiota_handle_dl_irq_msg(void *para)
{
    while (1)
    {
        if (RT_EOK != rt_sem_take(cmd_notice, RT_WAITING_FOREVER))
        {
            continue;
        }
        dl_irq_cnt++;
        // rt_kprintf("channel %d, dl_irq cnt %d\n", ap_channel_id, dl_irq_cnt);

        if (0 == uc_wiota_dl_data_parse())
        {
            continue;
        }
    }
}

int uc_wiota_spi_com_init(void)
{
    // ring_buffer init
    dl_rb = uc_wiota_ringbuffer_init(sizeof(dl_buf), dl_buf);
    ul_rb = uc_wiota_ringbuffer_init(sizeof(ul_buf) - 4, ul_buf); // last 4byte store frame num

    // rt_memset ul/dl rb w/r state
    rt_memset(&heart_info, 0, sizeof(heart_info_t));
    rt_memset(&ul_rb_read_state, 0, sizeof(rb_read_state_t));
    rt_memset(&ul_rb_write_state, 0, sizeof(rb_write_state_t));
    rt_memset(&dl_rb_read_state, 0, sizeof(rb_read_state_t));
    rt_memset(&dl_rb_write_state, 0, sizeof(rb_write_state_t));
    rt_memset(&dl_buf_state, 0, sizeof(hand_shake_t));
    rt_memset(&ul_buf_state, 0, sizeof(hand_shake_t));

    // init ul irq gpio
    rt_pin_mode(GPIO_PIN_5, PIN_MODE_OUTPUT);
    rt_pin_write(GPIO_PIN_5, PIN_LOW);

    // attach dl irq recv callback
    gpio_set_pin_mux(UC_GPIO_CFG, GPIO_PIN_4, GPIO_FUNC_0);
    rt_pin_mode(GPIO_PIN_4, PIN_MODE_INPUT);
    rt_pin_attach_irq(GPIO_PIN_4, PIN_IRQ_MODE_RISING, uc_wiota_handle_dl_irq, RT_NULL);
    rt_pin_irq_enable(GPIO_PIN_4, PIN_IRQ_ENABLE);

    // create dl irq notice sem
    cmd_notice = rt_sem_create("cmd_sem", 0, RT_IPC_FLAG_PRIO);
    RT_ASSERT(cmd_notice);

    cmd_lock = rt_mutex_create("cmd_lock", RT_IPC_FLAG_PRIO);
    RT_ASSERT(cmd_lock);

    // create dl cmd request handle thread
    rt_thread_t dl_irq_handler = rt_thread_create("dl_irq",
                                                  uc_wiota_handle_dl_irq_msg,
                                                  RT_NULL,
                                                  2048,
                                                  3,
                                                  3);
    RT_ASSERT(dl_irq_handler);

    // hand shake with gateway
    uc_wiota_hand_shake_with_gateway();

    rt_thread_startup(dl_irq_handler);

    rt_kprintf("uc_wiota_spi_com_init suc\n");

    return 0;
}
#endif