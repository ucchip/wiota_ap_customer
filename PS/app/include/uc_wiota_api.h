/*
 * uc_wiota_api.h
 *
 *  Created on: 2021.07.20
 *  Author: jpwang
 */

#ifndef __UC_WIOTA_API_H__
#define __UC_WIOTA_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

//period of time writing to flash
#define UC_WRITE_FLASH_PERIOD ((1000) * (60) * (60))

//max length of broadcast data sent
#define UC_WIOTA_MAX_SEND_BROADCAST_DATA_LEN 1024

#define UC_WIOTA_MAX_SEND_NORMAL_DATA_LEN 310

//max num of frequency point
#define UC_WIOTA_MAX_FREQUENCE_POINT 201

/* 470M */
#define UC_WIOTA_BASE_FREQUENCE 47000

/* 200 k */
#define UC_WIOTA_FREQUENCE_STEP 20

/* 470M ~ 510M */
#define UC_WIOTA_FREQUENCE(frequency_point) (UC_WIOTA_BASE_FREQUENCE + frequency_point * UC_WIOTA_FREQUENCE_STEP)

/* 0 - 200 */
#define UC_WIOTA_FREQUENCE_POINT(frequency) ((frequency - UC_WIOTA_BASE_FREQUENCE) / UC_WIOTA_FREQUENCE_STEP)

#define UC_WIOTA_MAX_MULTICAST_ID_NUM (8)

#define uc_container_of(ptr, type, member) \
    (type *)((char *)(ptr) - ((unsigned long)(&((type *)NULL)->member)))

/**                single list                   **/
#define uc_slist_entry(node, type, member) \
    uc_container_of(node, type, member)

#define uc_slist_for_each_entry(pos, head, member) \
    for (pos = uc_slist_entry((head)->next, typeof(*pos), member); &pos->member != NULL; pos = uc_slist_entry(pos->member.next, typeof(*pos), member))

typedef struct slist
{
    struct slist *next;
} slist_t;

typedef enum
{
    UC_LOG_UART = 0,
    UC_LOG_SPI = 1,
} uc_log_type_e;

typedef enum
{
    UC_OP_SUCC = 0,
    UC_OP_TIMEOUT = 1,
    UC_OP_FAIL = 2,
    UC_OP_MP_POOL = 3,
    UC_OP_DROP_CLEAR = 4,
} uc_result_e;

typedef struct
{
    void *semaphore;
    unsigned int result;
    unsigned int user_id;
    unsigned int data_id;
} uc_send_result_t;

typedef struct
{
    unsigned int user_id;
    unsigned int data_id;
    unsigned int result;
} uc_send_recv_t;

typedef struct
{
    void *semaphore;
    unsigned short data_len;
    unsigned short result;
    unsigned char *data;
} uc_scan_result_t;

typedef struct
{
    unsigned short data_len;
    unsigned short result;
    unsigned char *data;
} uc_scan_recv_t;

typedef struct
{
    unsigned char freq_idx;
    signed char snr;
    signed char rssi;
    unsigned char is_synced;
} uc_scan_freq_t;

typedef struct
{
    void *semaphore;
    signed char temp;
    unsigned char result;
} uc_temp_result_t;

typedef struct
{
    signed char temp;
    unsigned char result;
} uc_temp_recv_t;

typedef struct
{
    void *semaphore;
    unsigned int user_id;
    unsigned int result;
} uc_paging_result_t;

typedef struct
{
    unsigned int user_id;
    unsigned int result;
} uc_paging_recv_t;

typedef struct
{
    unsigned int user_id;                // iote user id
    unsigned int fn_index;               // the frame number of iote's first sleep
    unsigned int detection_period;       // iote detection period
    unsigned short send_round;           // number of rounds to send sync paging
    unsigned short continue_fn;          // the number of frames sent in one round
} uc_paging_info_t;

typedef struct
{
    unsigned char group_idx;
    unsigned char burst_idx;
    unsigned char slot_idx;
    unsigned char reserved;
} uc_dev_pos_t;

typedef struct
{
    unsigned int user_id;
    unsigned short data_len;
    unsigned char type;
    unsigned char *data;
} uc_recv_ul_data_t;

typedef struct
{
    signed char ap_tx_power;     //21, 30
    unsigned char id_len;        //id len
    unsigned char pp;            //0: 1, 1: 2, 2: 4, 3: not use
    unsigned char symbol_length; //128,256,512,1024
    unsigned char dlul_ratio;    //0 1:1,  1 1:2
    unsigned char bt_value;      //bt from rf 1: 0.3, 0: 1.2
    unsigned char group_number;  //frame ul group number: 1,2,4,8
    unsigned char spectrum_idx;  //default value:3(470M-510M)
    unsigned char old_subsys_v;  //default 0, if set 1, match old version(v2.3_ap8088) subsystem id
    unsigned char bitscb;        //bit scrambling flag bit, default 1, if set 0, match old version(v2.3_ap8288)
    unsigned char freq_idx;      // freq idx
    unsigned char reserved;      //for 4bytes alain
    unsigned int subsystem_id;
} sub_system_config_t;

typedef struct blacklist
{
    slist_t node;
    unsigned int user_id;
} uc_blacklist_t;

#ifdef WIOTA_IOTE_INFO
typedef struct iote_info
{
    slist_t node;
    unsigned int user_id;
    unsigned char iote_status;
    unsigned char group_idx;
    unsigned char subf_idx;
    unsigned char reserved;
} uc_iote_info_t;

typedef enum
{
    STATUS_OFFLINE = 0,
    STATUS_ONLINE = 1,
    STATUS_MAX
} uc_iote_status_e;
#endif

#ifdef WIOTA_AP_STATE_INFO
typedef struct uc_state_info
{
    slist_t node;
    unsigned int user_id;
    unsigned int ul_recv_len;
    unsigned int dl_send_len;
    unsigned int ul_recv_suc;
    unsigned int dl_send_suc;
    unsigned int dl_send_fail;
    unsigned short ul_recv_max;
    unsigned short dl_send_max;
} uc_state_info_t;


typedef enum
{
    //not be 0
    TYPE_UL_RECV_LEN = 1,
    TYPE_UL_RECV_SUC = 2,
    TYPE_DL_SEND_LEN = 3,
    TYPE_DL_SEND_SUC = 4,
    TYPE_DL_SEND_FAIL = 5,
    UC_STATE_TYPE_MAX
} uc_state_type_e;
#endif

typedef struct
{
    unsigned char freq;
    unsigned char spectrum_idx;
    unsigned char bandwidth;
    unsigned char symbol_length;
    unsigned short awaken_id; // indicate which id should send
    unsigned char mode;       // 0: old id range(narrow), 1: extend id range(wide)
    unsigned char reserved;   // re
    unsigned int send_time;   // ms, at least rx detect period
} uc_lpm_tx_cfg_t, *uc_lpm_tx_cfg_p;

typedef struct
{
    unsigned char freq;
    unsigned char spectrum_idx;
    unsigned char bandwidth;
    unsigned char symbol_length;
    unsigned char lpm_nlen;   // 1,2,3,4, default 4
    unsigned char lpm_utimes; // 1,2,3, default 2
    unsigned char threshold;  // detect threshold, 1~15, default 10
    unsigned char extra_flag; // defalut, if set 1, last period will use extra_period, then wake up
    unsigned short awaken_id; // indicate which id should detect
    unsigned short reserved;
    unsigned int detect_period; // ms, like 1000 ms
    unsigned int extra_period;  // ms, extra new period before wake up
    unsigned char mode;         // 0: old id range(narrow), 1: extend id range(wide)
    unsigned char period_multiple;    // the multiples of detect_period using awaken_id_ano, if 0, no need
    unsigned short awaken_id_another; // another awaken_id
} uc_lpm_rx_cfg_t, *uc_lpm_rx_cfg_p;

typedef enum
{
    NORMAL_BROADCAST = 0, //normal broadcast data,the amount of data is small,and the transmission rate is slow.
    OTA_BROADCAST = 1,    //OTA broadcast data,large amount of data,faster transmission rate
    INVALID_BROACAST,
} uc_bc_mode_e;

typedef enum
{
    UC_RATE_NORMAL = 0,   //set dl mcs
    UC_RATE_MID = 1,      //muti sm mode
    UC_RATE_HIGH = 2,     //grant mode
    UC_RATE_CRC_TYPE = 3, //only one byte crc
} uc_data_rate_mode_e;

typedef enum
{
    UC_MCS_LEVEL_0 = 0,
    UC_MCS_LEVEL_1,
    UC_MCS_LEVEL_2,
    UC_MCS_LEVEL_3,
    UC_MCS_LEVEL_4,
    UC_MCS_LEVEL_5,
    UC_MCS_LEVEL_6,
    UC_MCS_LEVEL_7,
    UC_MCS_LEVEL_AUTO = 8,
    UC_MCS_LEVEL_INVALID = 9,
} uc_mcs_level_e;

typedef enum
{
    WIOTA_STATE_DEFAULT = 0,
    WIOTA_STATE_INIT = 1,
    WIOTA_STATE_RUN = 2,
    WIOTA_STATE_EXIT = 3
} uc_wiota_run_state_e;

typedef enum
{
    DATA_TYPE_ACCESS = 0,
    DATA_TYPE_ACTIVE = 1,
    DATA_TYPE_SUBF_DATA = 2,
} uc_recv_data_type_e;

typedef enum
{
    TIME_SERVICE_NULL = 0,
    TIME_SERVICE_START = 1,
    TIME_SERVICE_SUC = 2,
    TIME_SERVICE_FAIL = 3,
    TIME_SERVICE_INIT_END = 4,
    TIME_SERVICE_ALIGN_END = 5,
    TIME_SERVICE_STOP = 6,
} time_service_state_e;

typedef enum
{
    TIME_SERVICE_GNSS = 0,
    TIME_SERVICE_1588_PS = 1,
    TIME_SERVICE_SYNC_ASSISTANT = 2,
} time_service_type_e;

typedef enum
{
    STATE_ABNORMAL = 0,
    STATE_NORMAL = 1,
} ap8288_state_e;

typedef enum
{
    AWAKENED_CAUSE_HARD_RESET = 0, // also watchdog reset, spi cs reset
    AWAKENED_CAUSE_SLEEP = 1,
    AWAKENED_CAUSE_PAGING = 2,          // then get uc_lpm_paging_waken_cause_e
    AWAKENED_CAUSE_GATING = 3,          // no need care
    AWAKENED_CAUSE_FORCED_INTERNAL = 4, // not use
    AWAKENED_CAUSE_OTHERS,
} uc_awakened_cause_e;

typedef enum
{
    PAGING_WAKEN_CAUSE_NULL = 0,            // not from paging
    PAGING_WAKEN_CAUSE_PAGING_TIMEOUT = 1,  // from lpm timeout
    PAGING_WAKEN_CAUSE_PAGING_SIGNAL = 2,   // from lpm signal
    PAGING_WAKEN_CAUSE_SYNC_PG_TIMEOUT = 3, // from sync paging timeout
    PAGING_WAKEN_CAUSE_SYNC_PG_SIGNAL = 4,  // from sync paging signal
    PAGING_WAKEN_CAUSE_SYNC_PG_TIMING = 5,  // from sync paging timing set
    PAGING_WAKEN_CAUSE_MAX,
} uc_lpm_paging_waken_cause_e;

typedef struct
{
    unsigned char is_cs_awakened;
    unsigned char awaken_cause;          // uc_awakened_cause_e
    unsigned char lpm_last_wakeup_cause; // uc_lpm_paging_waken_cause_e
    unsigned char lpm_last_wakeup_idx;   // default first
    unsigned int lpm_detected_times;
} uc_awaken_cause_t;

typedef struct
{
    unsigned char block_size;
    unsigned char send_round;
    unsigned char na[2];
} uc_subf_cfg_t;

typedef struct
{
    unsigned int user_id;
    unsigned int recv_cnt;
} uc_subf_node_t;

typedef struct
{
    uc_subf_node_t *subf_node; // ul
    unsigned int subf_node_num;// ul
    unsigned int send_cnt; //dl
} uc_subf_test_t;

typedef struct
{
    unsigned int user_id;
    uc_recv_data_type_e data_type;
    unsigned char *data;
    unsigned short data_len;
    signed char rssi;
    unsigned char delay;
    unsigned char fn_cnt;
    unsigned char group_idx;
    unsigned char burst_idx;
    unsigned char slot_idx;
    unsigned int frame_num;
} uc_recv_detail_t;

typedef struct
{
    unsigned int ts_state;    // time_service_state_e
    unsigned int frame_head;
    int frame_head_offset;
    unsigned int cur_time_s;
    unsigned int cur_time_us;
    int pos_x;
    int pos_y;
    int pos_z;
    float longitude;
    float latitude;
    float altitude;
} uc_ts_info_t;

typedef void (*uc_send_callback)(uc_send_recv_t *result);
typedef void (*uc_scan_callback)(uc_scan_recv_t *result);
typedef void (*uc_temp_callback)(uc_temp_recv_t *result);
typedef void (*uc_paging_callback)(uc_paging_recv_t *result);
typedef void (*uc_paging_ctrl_callback)(unsigned int user_id, unsigned char burst_idx, unsigned int fn_index);
typedef void (*uc_time_service_callback)(time_service_state_e state);
typedef void (*uc_time_service_info_callback)(uc_ts_info_t *ts_info);
typedef void (*uc_drop_callback)(unsigned int user_id);
typedef void (*uc_recv_callback)(unsigned int user_id, uc_dev_pos_t dev_pos, unsigned char *data, unsigned short data_len, uc_recv_data_type_e data_type);
typedef void (*uc_recv_detail_callback)(uc_recv_detail_t *recv_detail);
typedef void (*uc_fn_refresh_callback)(unsigned int frame_num);

typedef struct
{
    // recv
    unsigned int user_id;
    unsigned int start_recv_fn;
    unsigned char recv_fns;
    // send
    unsigned char send_fns;     // start_send_fn = start_recv_fn + recv_fns(after recv end)
    unsigned short data_len;
    unsigned char *data;        // is null, not dl
    uc_send_callback callback;  // only support no-blocking calls, must not be empty
    void *para;
} recv_send_by_fn_t;

void uc_wiota_set_state(uc_wiota_run_state_e state);

unsigned char uc_wiota_get_state(void);

void uc_wiota_read_value_from_mem(unsigned int type, unsigned int read_addr, unsigned int read_len, unsigned char *out_buf);

//void uc_wiota_write_value_to_reg(unsigned int value, unsigned int addr);

unsigned int uc_wiota_read_dfe_counter(unsigned char reg_type);

int uc_wiota_set_default_dfe(unsigned int default_dfe);

unsigned int uc_wiota_get_frame_head_dfe(void);

// time service api
int uc_wiota_set_frame_boundary_align_func(unsigned char is_open);

void uc_wiota_register_time_service_state_callback(uc_time_service_callback callback);

void uc_wiota_register_time_service_info_callback(uc_time_service_info_callback callback);

int uc_wiota_set_time_service_func(time_service_type_e type, unsigned char is_open);

unsigned char uc_wiota_get_time_service_func(time_service_type_e type);

int uc_wiota_set_sync_assistant_pps(unsigned char is_pps);

unsigned char uc_wiota_get_sync_assistant_pps(void);

time_service_state_e uc_wiota_get_time_service_state(void);

int uc_wiota_set_broadcast_utc_func(unsigned char is_bc_utc);

int uc_wiota_set_time_service_cycle(unsigned char cycle_min); // unit: minute

unsigned char uc_wiota_get_time_service_cycle(void);

void uc_wiota_time_service_start(void);

void uc_wiota_time_service_stop(void);

void uc_wiota_gnss_query_coordinate_xyz(int *pos_x, int *pos_y, int *pos_z);

void uc_wiota_gnss_query_coordinate_lla(float *longitude, float *latitude, float *altitude);

int uc_wiota_set_gnss_relocation(unsigned char is_relocation);

int uc_wiota_set_1588_protocol_rtc(unsigned int timestamp, unsigned int usec);
// time service api end

ap8288_state_e uc_wiota_get_ap8288_state(void);

int uc_wiota_set_paging_tx_cfg(uc_lpm_tx_cfg_t *config);

void uc_wiota_get_paging_tx_cfg(uc_lpm_tx_cfg_t *config);

int uc_wiota_paging_tx_start(void);

// paging rx, require new hardware support, old hardware called directly return
int uc_wiota_set_paging_rx_cfg(uc_lpm_rx_cfg_t *config);

void uc_wiota_get_paging_rx_cfg(uc_lpm_rx_cfg_t *config);

int uc_wiota_paging_rx_enter(unsigned char is_need_32k_div, unsigned int timeout_max);

void uc_wiota_get_awakened_cause(uc_awaken_cause_t *awaken_cause);

unsigned char uc_wiota_get_is_new_hardware(void); // 1 new hardware, 0 old hardware
//}

void uc_wiota_register_sync_paging_callback(uc_paging_ctrl_callback callback);

uc_result_e uc_wiota_sync_paging(uc_paging_info_t *paging_info, uc_paging_callback callback);

unsigned char uc_wiota_get_sync_paging_num(unsigned char group_idx, unsigned char subf_idx);

int uc_wiota_set_single_tone(unsigned int is_open);

int uc_wiota_set_broadcast_fn_cycle(unsigned char bc_fn_cycle);

unsigned char uc_wiota_get_broadcast_fn_cycle(void);

unsigned int uc_wiota_get_frame_len(void);

unsigned int uc_wiota_get_frame_num(void);

int uc_wiota_iote_leaving_active_state(unsigned int *user_id, unsigned int id_num);

void uc_wiota_get_module_id(unsigned char *module_id);

//{subf mode
int uc_wiota_set_subframe_mode_cfg(uc_subf_cfg_t *subf_cfg);

void uc_wiota_get_subframe_mode_cfg(uc_subf_cfg_t *subf_cfg);

int uc_wiota_set_ul_subframe_mode(unsigned char subf_mode, unsigned int user_id, unsigned char rach_delay); // ul

int uc_wiota_add_dl_subframe_data(unsigned char *data, unsigned char data_len, unsigned char fn);  // dl

int uc_wiota_set_subframe_test(unsigned char mode); // 0: close test, 1: open test, 2: clear test info

uc_subf_test_t *uc_wiota_get_subframe_test(void);
//} subf mode

int uc_wiota_set_boost_level_0_5(unsigned char is_open);

unsigned char uc_wiota_get_sm_resend_times(void);

int uc_wiota_set_sm_resend_times(unsigned char resend_times);

void uc_wiota_register_fn_refresh_callback(uc_fn_refresh_callback callback);

uc_result_e uc_wiota_recv_send_sm_by_fn(recv_send_by_fn_t *rs_fn);

int uc_wiota_set_ramp_type(unsigned char ramp_type);

int uc_wiota_set_ramp_value(unsigned int ramp_value);

int uc_wiota_set_rf_ctrl_idx(unsigned int rf_ctrl_idx);

int uc_wiota_set_aagc_idx(unsigned char aagc_idx);

// This function definition has been moved to uc_wiota_static.h
//void uc_wiota_get_freq_list(unsigned char *list);

/*********************************************************************************
 This function is get version of sw

 param:
        in:NULL.
        out:
            wiota_version_8088:version of ap8088 sw.
            git_info_8088:current wiota git info of ap8088.
            make_time_8088:build time of ap8088.
            wiota_version_8088:version of ap8288 sw.
            git_info_8088:current wiota git info of ap8288.
            make_time_8088:build time of ap8288.
            cce_version:cce version

 return:NULL.
**********************************************************************************/
void uc_wiota_get_version(unsigned char *wiota_version_8088, unsigned char *git_info_8088, unsigned char *make_time_8088,
                          unsigned char *wiota_version_8288, unsigned char *git_info_8288, unsigned char *make_time_8288, unsigned int *cce_version);

/*********************************************************************************
 This function is to set system config

 param:
        in:
            config:struct of sub_system_config_t.
        out:NULL.

 return:
    int.
**********************************************************************************/
int uc_wiota_set_system_config(sub_system_config_t *config);

/*********************************************************************************
 This function is to get system config

 param:
        in:NULL
        out:
            config: struct of sub_system_config_t.
 return:
    NULL.
**********************************************************************************/
void uc_wiota_get_system_config(sub_system_config_t *config);

/*********************************************************************************
 This function is to set ap8288 rf power.

 param:
        in:
            rf_power:the value of the ap8288 rf power to be set.
        out:NULL.

 return:
    int.
**********************************************************************************/
int uc_wiota_set_ap_tx_power(signed char rf_power);

/*********************************************************************************
 This function is to set single frequency point.

 param:
        in:
            freq_idx:single frequency point.
        out:NULL.

 return:
    int.
**********************************************************************************/
int uc_wiota_set_freq_info(unsigned char freq_idx);

/*********************************************************************************
 This function is to get frequency point.

 param:NULL.

 return:
    freq_idx.
**********************************************************************************/
unsigned char uc_wiota_get_freq_info(void);

/*********************************************************************************
 This function is to set frequency point of hopping.

 param:
        in:
            hopping_freq_t:frequency point of hopping.
        out:NULL.

 return:
    int.
**********************************************************************************/
int uc_wiota_set_hopping_freq(unsigned char hopping_freq);

/*********************************************************************************
 This function is to set mode of hopping frequency.

 param:
        in:
            ori_freq_count:working ori freq frames.
            hopping_freq_count:working hopping freq frames.
        out:NULL.

 return:
    int.
**********************************************************************************/
int uc_wiota_set_hopping_mode(unsigned char ori_freq_frame, unsigned char hopping_freq_frame);

/**********************************************************************************
 This function is to set max iote num of active state.

 param:
        in:
            iote_num:max iote num of active state.
        out:NULL.
 return:
    int.
**********************************************************************************/
int uc_wiota_set_max_num_of_active_iote(unsigned short max_iote_num);

unsigned short uc_wiota_get_max_num_of_active_iote(void);

/**********************************************************************************
 This function is to set data rate by mode.

 param:
        in:
            rate_mode:mode of the rate(uc_data_rate_mode_e).
            rate_value:rate value.
        out:NULL.
 return:
    int.
**********************************************************************************/
int uc_wiota_set_data_rate(uc_data_rate_mode_e rate_mode, unsigned int rate_value);
unsigned int uc_wiota_get_data_rate_value(uc_data_rate_mode_e rate_mode);

/**********************************************************************************
 This function is to set mcs of broadcast.

 param:
        in:
            mcs:mcs of broadcast(uc_mcs_level_e).
        out:NULL.
 return:
    int.
**********************************************************************************/
int uc_wiota_set_broadcast_mcs(uc_mcs_level_e bc_mcs);

uc_mcs_level_e uc_wiota_get_broadcast_mcs(void);

int uc_wiota_set_broadcast_send_round(unsigned char round);

unsigned char uc_wiota_get_broadcast_send_round(void);

/**********************************************************************************
 This function is to set whether to open crc16 verification and length of verification.

 param:
        in:
            crc_limit: 0: close crc16 verification, > 0:open crc16 verification.
        out:NULL.
 return:
    int.
**********************************************************************************/
int uc_wiota_set_crc(unsigned short crc_limit);

/**********************************************************************************
 This function is to get crcLimit value.

 param:
        in:NULL.
        out:NULL.
 return:
    crc_limit.
**********************************************************************************/
unsigned short uc_wiota_get_crc(void);

/*********************************************************************************
 This function is to get the header of the blacklist linked list.

 param:
        in:NULL.
        out:
            blacklist_num:number of blacklist linked list nodes.

 return:
        uc_blacklist_t:pointer of blacklist linked list header.
**********************************************************************************/
uc_blacklist_t *uc_wiota_get_blacklist(unsigned short *blacklist_num);

/*********************************************************************************
 This function is to print all the information in the blacklist.

 param:
    NULL.

 return:
    NULL.
**********************************************************************************/
void uc_wiota_print_blacklist(void);

/*********************************************************************************
 This function is to add one or more iotes to the blacklist linked list.

 param:
        in:
            user_id:the first address of the user id array of iote.
            user_id_num:number of user id.
        out:NULL.

 return:
    int.
**********************************************************************************/
int uc_wiota_add_iote_to_blacklist(unsigned int *user_id, unsigned short user_id_num);

/*********************************************************************************
 This function is to removed one or more iotes from the blacklist linked list.

 param:
        in:
            user_id:the first address of the user id array of iote.
            user_id_num:number of user id.
        out:NULL.

 return:
    int.
**********************************************************************************/
int uc_wiota_remove_iote_from_blacklist(unsigned int *user_id, unsigned short user_id_num);

#ifdef WIOTA_IOTE_INFO
/*********************************************************************************
 This function is to printing all iote information.

 param:
    NULL.

 return:
    NULL.
**********************************************************************************/
void uc_wiota_print_iote_info(void);

/*********************************************************************************
 This function is to query the information of all iotes.

 param:
        in:NULL.
        out:
            online_num:number of connected iotes.
            offline_num:number of disconnected iotes.

 return:
        uc_iote_info_t:pointer of iote information linked list header.
**********************************************************************************/
uc_iote_info_t *uc_wiota_get_iote_info(unsigned short *online_num, unsigned short *offline_num);
#endif

#ifdef WIOTA_AP_STATE_INFO
/*********************************************************************************
 This function is to query single state of single iote.

 param:
        in:
            user_id:user_id of iote.
            state_type:uc_state_type_e.
        out:NULL

 return:
        single state value.
**********************************************************************************/
unsigned int uc_wiota_get_single_state_info_of_iote(unsigned int user_id, uc_state_type_e state_type);

/*********************************************************************************
 This function is to query all state of single iote.

 param:
        in:
            user_id:user_id of iote.
        out:NULL

 return:
        uc_state_info_t.
**********************************************************************************/
uc_state_info_t *uc_wiota_get_all_state_info_of_iote(unsigned int user_id);

/*********************************************************************************
 This function is to query all state of all iote.

 param:
        in:NULL
        out:NULL

 return:
        uc_state_info_t.
**********************************************************************************/
uc_state_info_t *uc_wiota_get_all_state_info(void);

/*********************************************************************************
 This function is to reset single state of single iote.

 param:
        in:
            user_id:user_id of iote.
            state_type:uc_state_type_e.
        out:NULL

 return:
        NULL.
**********************************************************************************/
void uc_wiota_reset_single_state_info_of_iote(unsigned int user_id, uc_state_type_e state_type);

/*********************************************************************************
 This function is to reset all state of single iote.

 param:
        in:
            user_id:user_id of iote.
        out:NULL

 return:
        NULL.
**********************************************************************************/
void uc_wiota_reset_all_state_info_of_iote(unsigned int user_id);

/*********************************************************************************
 This function is to reset all state of all iote.

 param:
        in:NULL
        out:NULL

 return:
        NULL.
**********************************************************************************/
void uc_wiota_reset_all_state_info(void);
#endif

/*********************************************************************************
 This function is to sending broadcast data.

 param:
        in:
            send_data:data to be sent.
            send_data_len:the length of data to be sent.(the max is 1024 bit)
            mode:
                0:normal broadcast data,the amount of data is small,and the
                    transmission rate is slow.
                1:OTA broadcast data,large amount of data,faster transmission rate
            timeout:send data timeout time,unit:ms
            callback:send data result callback
                     when callback==NULL,is blocking call.
                     Non-blocking call when callback != NULL.
        out:NULL.

 return:
    uc_result_e.

 note:
        1.if the callback is NULL,the data sent is larger than 1k,and the next
    packet can be sent only after the function return value is UC_OP_SUCC.
        2..if the callback is not NULL,the data sent is larger than 1k,you need to
    wait until the registered callback returns UC_OP_SUCC before sending the next
    packet.
**********************************************************************************/
uc_result_e uc_wiota_send_broadcast_data(unsigned char *send_data, unsigned short send_data_len, uc_bc_mode_e mode, signed int timeout, uc_send_callback callback, void *para);

/*********************************************************************************
 This function is to sending multicast data.

 param:
        in:
            send_data:data to be sent.
            send_data_len:the length of data to be sent.
            multicast_id:specify the multicast id send.
            timeout:send data timeout time,unit:ms
            callback:send data result callback.
                     when callback==NULL,is blocking call.
                     Non-blocking call when callback != NULL.
            para:data address, which will be returned after sending.
            if no need to fill in NULL
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_send_multicast_data(unsigned char *send_data, unsigned short send_data_len, unsigned int multicast_id, signed int timeout, uc_send_callback callback, void *para);

// set multicast_id,befor send multicast data
int uc_wiota_set_multicast_id(unsigned int *multicast_id, unsigned int id_num);

// del multicast_id
int uc_wiota_del_multicast_id(unsigned int *multicast_id, unsigned int id_num);

/*********************************************************************************
 This function is to paging iote and sending non-broadcast data.

 param:
        in:
            send_data:non-broadcast data to be sent.
            send_data_len:the length of non-broadcast data to be sent.
            user_id:specify the user id send.
            timeout:send data timeout time,unit:ms
            callback:send data result callback.
                     when callback==NULL,is blocking call.
                     Non-blocking call when callback != NULL.
            para:data address, which will be returned after sending.
            if no need to fill in NULL
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_send_data(unsigned char *send_data, unsigned short send_data_len, unsigned int user_id, signed int timeout, uc_send_callback callback, void *para);

uc_result_e uc_wiota_send_data_order(unsigned char *send_data,
                                     unsigned short send_data_len,
                                     unsigned int user_id,
                                     signed int timeout,
                                     unsigned int order_business,
                                     uc_send_callback callback,
                                     void *para);

/*********************************************************************************
 This function is to scaning frequency point collection.(Not supported at the moment)

 param:
        in:
            freq:frequency point collection.
            freq_num:number of frequency point.
            timeout:scan timeout time,unit:ms.
            callback:scan result callback.
                     when callback==NULL,is blocking call.
                     Non-blocking call when callback != NULL.
            scan_result: infomation of eace frequency point after sacnning.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_scan_freq(unsigned char *freq, unsigned char freq_num, unsigned char scan_type, signed int timeout, uc_scan_callback callback, uc_scan_recv_t *scan_result);

/*********************************************************************************
 This function is to register iote drop prompt callback.

 param:
        in:
            callback:function pointer,the parameter is the user id of the dropped
                iote.
        out:NULL.

 return:
    NULL.
**********************************************************************************/
void uc_wiota_register_iote_dropped_callback(uc_drop_callback callback);

/*********************************************************************************
 This function is to register uplink data prompt callback.

 param:
        in:
            callback:function pointer,the parameter is the user id of the iote that
                report the data.
            data:data content report.
            data_len:data length reported
        out:NULL.

 return:
    NULL.
**********************************************************************************/
void uc_wiota_register_recv_data_callback(uc_recv_callback callback);

void uc_wiota_register_recv_data_detail_callback(uc_recv_detail_callback detail_callback);

/*********************************************************************************
 This function is to wiota to init.

 param:
        in:NULL.
        out:NULL.

 return:NULL.
**********************************************************************************/
void uc_wiota_init(void);

/*********************************************************************************
 This function is to wiota to run.

 param:
        in:NULL.
        out:NULL.

 return:NULL.
**********************************************************************************/
void uc_wiota_run(void);

/*********************************************************************************
 This function is to wiota to exit.

 param:
        in:NULL.
        out:NULL.

 return:NULL.
**********************************************************************************/
void uc_wiota_exit(void);

/*********************************************************************************
 This function is to set the connection timeout of iote in idle state.

 param:
        in:
            active_time:connection timeout, unit:second
        out:NULL.

 return:
    int.
**********************************************************************************/
int uc_wiota_set_active_time(unsigned int active_s);

/*********************************************************************************
 This function is to get the connection timeout of iote in idle state.

 param:
        in:NULL.
        out:NULL.

 return:connection timeout.
**********************************************************************************/
unsigned int uc_wiota_get_active_time(void);

/*********************************************************************************
 This function is to get the ap8288 temperature.

 param:
        in:
            timeout:execution timeout.
            callback:when call==NULL,is blocking call;
                     Non-blocking call when callback != NULL
        out:
            read_temp:temperature result.

 return:uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_read_temperature(uc_temp_callback callback, uc_temp_recv_t *read_temp, signed int timeout);

/*********************************************************************************
 This function is to set log type.

 param:
        in:
            timeout:execution timeout.
            callback:when call==NULL,is blocking call;
                     Non-blocking call when callback != NULL
        out:
            read_temp:temperature result.

 return:int.
**********************************************************************************/
int uc_wiota_log_switch(uc_log_type_e log_type, unsigned char is_open);

/*********************************************************************************
 This function is to query dev pos on frame structure by user id.

 param:
        in:
            user_id:the first address of the id array to be queried.
            user_id_num:number of the id to be queried.
        out:
            NULL.

 return:uc_dev_pos_t.
**********************************************************************************/
uc_dev_pos_t *uc_wiota_query_dev_pos_by_userid(unsigned int *user_id, unsigned int user_id_num);

#ifdef __cplusplus
}
#endif
#endif /*__UC_WIOTA_API_H__ */