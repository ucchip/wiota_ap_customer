/*
 * uc_wiota_api.h
 *
 *  Created on: 2021.07.20
 *  Author: jpwang
 */

#ifndef _UC_WIOTA_INTERFACE_H_
#define _UC_WIOTA_INTERFACE_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned long long u64_t;
typedef signed long long s64_t;
typedef unsigned long ul32_t;
typedef signed long sl32_t;
typedef signed int s32_t;
typedef unsigned int u32_t;
typedef signed short s16_t;
typedef unsigned short u16_t;
typedef char n8_t;
typedef signed char s8_t;
typedef unsigned char u8_t;
typedef unsigned char boolean;

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
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
} uc_result_e;

typedef struct
{
    void *semaphore;
    u32_t result;
    u32_t user_id;
    u32_t data_id;
} uc_send_result_t;

typedef struct
{
    u32_t user_id;
    u32_t data_id;
    u32_t result;
} uc_send_recv_t;

typedef struct
{
    void *semaphore;
    u16_t data_len;
    u16_t result;
    u8_t *data;
} uc_scan_result_t;

typedef struct
{
    u16_t data_len;
    u16_t result;
    u8_t *data;
} uc_scan_recv_t;

typedef struct
{
    u8_t freq_idx;
    s8_t snr;
    s8_t rssi;
    u8_t is_synced;
} uc_scan_freq_t;

typedef struct
{
    void *semaphore;
    s8_t temp;
    u8_t result;
} uc_temp_result_t;

typedef struct
{
    s8_t temp;
    u8_t result;
} uc_temp_recv_t;

typedef struct
{
    u8_t group_idx;
    u8_t burst_idx;
    u8_t slot_idx;
    u8_t reserved;
} uc_dev_pos_t;

typedef struct
{
    void *semaphore;
    u32_t result;
    u32_t scramble_id_num;
    u32_t *scramble_id;
} uc_query_result_t;

typedef struct
{
    u32_t result;
    u32_t scramble_id_num;
    u32_t *scramble_id;
} uc_query_recv_t;

typedef struct
{
    u32_t user_id;
    u16_t data_len;
    u8_t type;
    u8_t *data;
} uc_recv_ul_data_t;

typedef struct
{
    s8_t ap_max_power;  //21, 30
    u8_t id_len;        //id len
    u8_t pp;            //0: 1, 1: 2, 2: 4, 3: not use
    u8_t symbol_length; //128,256,512,1024
    u8_t dlul_ratio;    //0 1:1,  1 1:2
    u8_t bt_value;      //bt from rf 1: 0.3, 0: 1.2
    u8_t group_number;  //frame ul group number: 1,2,4,8
    u8_t spectrum_idx;  //default value:3(470M-510M)
    u8_t old_subsys_v;  //default 0, if set 1, match old version(v2.3_ap8088) subsystem id
    u8_t bitscb;        //bit scrambling flag bit, default 1, if set 0, match old version(v2.3_ap8288)
    u8_t reserved[2];   //for 4bytes alain
    u32_t subsystem_id;
} sub_system_config_t;

typedef struct blacklist
{
    slist_t node;
    u32_t user_id;
} uc_blacklist_t;

#ifdef WIOTA_IOTE_INFO
typedef struct iote_info
{
    slist_t node;
    u32_t user_id;
    u8_t iote_status;
    u8_t group_idx;
    u8_t subframe_idx;
    u8_t reserved;
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
    u32_t user_id;
    u32_t ul_recv_len;
    u32_t dl_send_len;
    u32_t ul_recv_suc;
    u32_t dl_send_suc;
    u32_t dl_send_fail;
    u16_t ul_recv_max;
    u16_t dl_send_max;
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
    unsigned short reserved;
    unsigned int send_time; // ms, at least rx detect period
} uc_lpm_tx_cfg_t, *uc_lpm_tx_cfg_p;

typedef enum
{
    NORMAL_BROADCAST = 0, //normal broadcast data,the amount of data is small,and the transmission rate is slow.
    OTA_BROADCAST = 1,    //OTA broadcast data,large amount of data,faster transmission rate
    INVALID_BROACAST,
} uc_bc_mode_e;

typedef enum
{
    UC_RATE_NORMAL = 0, //not currently supported
    UC_RATE_MID = 1,    //muti sm mode
    UC_RATE_HIGH = 2    //grant mode
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
} time_service_type_e;

typedef enum
{
    STATE_ABNORMAL = 0,
    STATE_NORMAL = 1,
} ap8288_state_e;

extern boolean uc_get_grant_mode(void);
#ifdef UC8088_FACTORY_TEST
extern s32_t factory_test_hanlde_rs_msg(u32_t subType, u32_t data);
extern s32_t factory_test_handle_loop_msg(u8_t mcs, u32_t packetNum, u8_t dlMode);
extern void factory_test_set_save_loop_id_flag(boolean isSave);
extern u8_t factory_test_get_loop_is_rach(void);
extern u8_t *factory_test_gen_rand_data(u16_t data_len);
#endif

typedef void (*uc_send_callback)(uc_send_recv_t *result);
typedef void (*uc_scan_callback)(uc_scan_recv_t *result);
typedef void (*uc_temp_callback)(uc_temp_recv_t *result);
typedef void (*uc_query_callback)(uc_query_recv_t *result);
typedef void (*uc_time_service_callback)(time_service_state_e state);
typedef void (*uc_iote_drop)(u32_t user_id);
typedef void (*uc_recv)(u32_t user_id, uc_dev_pos_t dev_pos, u8_t *data, u16_t data_len, uc_recv_data_type_e data_type);

void uc_wiota_set_state(uc_wiota_run_state_e state);

u8_t uc_wiota_get_state(void);

void uc_wiota_read_value_from_mem(u32_t type, u32_t read_addr, u32_t read_len, u8_t *out_buf);

//void uc_wiota_write_value_to_reg(u32_t value, u32_t addr);

u32_t uc_wiota_read_dfe_counter(u8_t reg_type);

void uc_wiota_set_default_dfe(u32_t default_dfe);

u32_t uc_wiota_get_frame_head_dfe(void);

// time service api
void uc_wiota_set_frame_boundary_align_func(u8_t is_open);

void uc_wiota_register_time_service_state_callback(uc_time_service_callback callback);

void uc_wiota_set_time_service_func(time_service_type_e type, u8_t is_open);

u8_t uc_wiota_get_time_service_func(time_service_type_e type);

time_service_state_e uc_wiota_get_time_service_state(void);

void uc_wiota_time_service_start(void);

void uc_wiota_time_service_stop(void);

void uc_wiota_gnss_query_fix_pos(float *pos_x, float *pos_y, float *pos_z);

void uc_wiota_set_gnss_relocation(u8_t is_relocation);

void uc_wiota_set_1588_protocol_rtc(u32_t timestamp, u32_t msec);
// time service api end

ap8288_state_e uc_wiota_get_ap8288_state(void);

void uc_wiota_set_paging_tx_cfg(uc_lpm_tx_cfg_t *config);

uc_lpm_tx_cfg_t* uc_wiota_get_paging_tx_cfg(void);

void uc_wiota_start_paging_tx(void);

#if 1//def RAMP_RF_SET_SUPPORT
void uc_wiota_set_ramp_value(u32_t ramp_value);

void uc_wiota_set_rf_ctrl_idx(u32_t rf_ctrl_idx);
#endif

// This function definition has been moved to uc_wiota_static.h
//void uc_wiota_get_freq_list(u8_t *list);

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
void uc_wiota_get_version(u8_t *wiota_version_8088, u8_t *git_info_8088, u8_t *make_time_8088,
                          u8_t *wiota_version_8288, u8_t *git_info_8288, u8_t *make_time_8288, u32_t *cce_version);

/*********************************************************************************
 This function is to set system config

 param:
        in:
            config:struct of sub_system_config_t.
        out:NULL.

 return:
    NULL.
**********************************************************************************/
void uc_wiota_set_system_config(sub_system_config_t *config);

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
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_ap_max_power(s8_t rf_power);

/*********************************************************************************
 This function is to set single frequency point.

 param:
        in:
            freq_idx:single frequency point.
        out:NULL.

 return:
    NULL.
**********************************************************************************/
void uc_wiota_set_freq_info(u8_t freq_idx);

/*********************************************************************************
 This function is to get frequency point.

 param:NULL.

 return:
    freq_idx.
**********************************************************************************/
u8_t uc_wiota_get_freq_info(void);

/*********************************************************************************
 This function is to set frequency point of hopping.

 param:
        in:
            hopping_freq_t:frequency point of hopping.
        out:NULL.

 return:
    NULL.
**********************************************************************************/
void uc_wiota_set_hopping_freq(u8_t hopping_freq);

/*********************************************************************************
 This function is to set mode of hopping frequency.

 param:
        in:
            ori_freq_count:working ori freq frames.
            hopping_freq_count:working hopping freq frames.
        out:NULL.

 return:
    NULL.
**********************************************************************************/
void uc_wiota_set_hopping_mode(u8_t ori_freq_frame, u8_t hopping_freq_frame);

/**********************************************************************************
 This function is to set max iote num of active state in the same subframe.

 param:
        in:
            iote_num:max iote num of active state.
        out:NULL.
 return:
    NULL.
**********************************************************************************/
void uc_wiota_set_max_active_iote_num_in_the_same_subframe(u8_t max_iote_num);

/**********************************************************************************
 This function is to set data rate by mode.

 param:
        in:
            rate_mode:mode of the rate(uc_data_rate_mode_e).
            rate_value:rate value.
        out:NULL.
 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_data_rate(uc_data_rate_mode_e rate_mode, u32_t rate_value);

/**********************************************************************************
 This function is to set mcs of broadcast.

 param:
        in:
            mcs:mcs of broadcast(uc_mcs_level_e).
        out:NULL.
 return:
    NULL.
**********************************************************************************/
void uc_wiota_set_broadcast_mcs(uc_mcs_level_e bc_mcs);

/**********************************************************************************
 This function is to set whether to open crc16 verification and length of verification.

 param:
        in:
            crc_limit: 0: close crc16 verification, > 0:open crc16 verification.
        out:NULL.
 return:
    NULL.
**********************************************************************************/
void uc_wiota_set_crc(u16_t crc_limit);

/**********************************************************************************
 This function is to get crcLimit value.

 param:
        in:NULL.
        out:NULL.
 return:
    crc_limit.
**********************************************************************************/
u16_t uc_wiota_get_crc(void);

/*********************************************************************************
 This function is to get the header of the blacklist linked list.

 param:
        in:NULL.
        out:
            blacklist_num:number of blacklist linked list nodes.

 return:
        uc_blacklist_t:pointer of blacklist linked list header.
**********************************************************************************/
uc_blacklist_t *uc_wiota_get_blacklist(u16_t *blacklist_num);

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
    NULL.
**********************************************************************************/
void uc_wiota_add_iote_to_blacklist(u32_t *user_id, u16_t user_id_num);

/*********************************************************************************
 This function is to removed one or more iotes from the blacklist linked list.

 param:
        in:
            user_id:the first address of the user id array of iote.
            user_id_num:number of user id.
        out:NULL.

 return:
    NULL.
**********************************************************************************/
void uc_wiota_remove_iote_from_blacklist(u32_t *user_id, u16_t user_id_num);

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
uc_iote_info_t *uc_wiota_get_iote_info(u16_t *online_num, u16_t *offline_num);
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
u32_t uc_wiota_get_single_state_info_of_iote(u32_t user_id, uc_state_type_e state_type);

/*********************************************************************************
 This function is to query all state of single iote.

 param:
        in:
            user_id:user_id of iote.
        out:NULL

 return:
        uc_state_info_t.
**********************************************************************************/
uc_state_info_t *uc_wiota_get_all_state_info_of_iote(u32_t user_id);

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
void uc_wiota_reset_single_state_info_of_iote(u32_t user_id, uc_state_type_e state_type);

/*********************************************************************************
 This function is to reset all state of single iote.

 param:
        in:
            user_id:user_id of iote.
        out:NULL

 return:
        NULL.
**********************************************************************************/
void uc_wiota_reset_all_state_info_of_iote(u32_t user_id);

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
uc_result_e uc_wiota_send_broadcast_data(u8_t *send_data, u16_t send_data_len, uc_bc_mode_e mode, s32_t timeout, uc_send_callback callback, void *para);

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
uc_result_e uc_wiota_send_multicast_data(u8_t *send_data, u16_t send_data_len, u32_t multicast_id, s32_t timeout, uc_send_callback callback, void *para);

// set multicast_id,befor send multicast data
void uc_wiota_set_multicast_id(u32_t *multicast_id, u32_t id_num);

// del multicast_id
void uc_wiota_del_multicast_id(u32_t *multicast_id, u32_t id_num);

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
uc_result_e uc_wiota_send_data(u8_t *send_data, u16_t send_data_len, u32_t user_id, s32_t timeout, uc_send_callback callback, void *para);

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
uc_result_e uc_wiota_scan_freq(u8_t *freq, u8_t freq_num, u8_t scan_type, s32_t timeout, uc_scan_callback callback, uc_scan_recv_t *scan_result);

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
void uc_wiota_register_iote_dropped_callback(uc_iote_drop callback);

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
void uc_wiota_register_recv_data_callback(uc_recv callback);

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
    NULL.
**********************************************************************************/
void uc_wiota_set_active_time(u32_t active_s);

/*********************************************************************************
 This function is to get the connection timeout of iote in idle state.

 param:
        in:NULL.
        out:NULL.

 return:connection timeout.
**********************************************************************************/
u32_t uc_wiota_get_active_time(void);

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
uc_result_e uc_wiota_read_temperature(uc_temp_callback callback, uc_temp_recv_t *read_temp, s32_t timeout);

/*********************************************************************************
 This function is to set log type.

 param:
        in:
            timeout:execution timeout.
            callback:when call==NULL,is blocking call;
                     Non-blocking call when callback != NULL
        out:
            read_temp:temperature result.

 return:uc_result_e.
**********************************************************************************/
void uc_wiota_log_switch(uc_log_type_e log_type, u8_t is_open);

/*********************************************************************************
 This function is to query scramble id by user id.

 param:
        in:
            user_id:the first address of the id array to be queried.
            user_id_num:number of the id to be queried.
            callback:
                     when callback==NULL,is blocking call.
                     Non-blocking call when callback != NULL.
        out:
            query_result, when callback==NULL effieient.

 return:NULL.
**********************************************************************************/
uc_result_e uc_wiota_query_scrambleid_by_userid(u32_t *user_id, u32_t user_id_num, uc_query_callback callback, uc_query_recv_t *query_result);

uc_dev_pos_t *uc_wiota_get_dev_pos_by_scrambleid(u32_t *scramble_id, u32_t scramble_id_num);
#ifdef __cplusplus
}
#endif
#endif