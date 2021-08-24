/*
 * uc_wiota_interface.h
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

typedef unsigned long long  u64_t;
typedef signed long long  s64_t;
typedef unsigned long  ul32_t;
typedef signed long  sl32_t;
typedef signed int  s32_t;
typedef unsigned int  u32_t;
typedef signed short  s16_t;
typedef unsigned short  u16_t;
typedef char n8_t;
typedef signed char  s8_t;
typedef unsigned char  u8_t;
typedef unsigned char boolean;

#ifndef NULL
#define NULL ((void*) 0)
#endif

#define UC_WIOTA_MAX_SEND_BROADCAST_DATA_LEN  400

#define UC_WIOTA_MAX_FREQUENCE_POINT 200

/* 470M */
#define UC_WIOTA_BASE_FREQUENCE 47000

/* 200 k */
#define UC_WIOTA_FREQUENCE_STEP 20

/* 470M ~ 510M */
#define UC_WIOTA_FREQUENCE(frequency_point)  (UC_WIOTA_BASE_FREQUENCE + frequency_point * UC_WIOTA_FREQUENCE_STEP)

/* 0 - 200 */
#define UC_WIOTA_FREQUENCE_POINT(frequency)  ((frequency - UC_WIOTA_BASE_FREQUENCE) / UC_WIOTA_FREQUENCE_STEP)

typedef enum
{
    UC_SUCCESS = 0,
    UC_TIMEOUT = 1,
    UC_FAILED = 2,
}uc_result_e;

typedef struct
{
    void * semaphore;
    uc_result_e result;
}result_sem_t;

typedef struct
{
    u32_t system_id;
    u32_t subsystem_id;
    u16_t reserved;
    u8_t  id_len;
    u8_t  pn_num;          // 0: 1, 1: 2, 2: 4, 3: not use
    u8_t  symbol_length;   //128,256,512,1024
    u8_t  dlul_ratio;      //0 1:1,  1 1:2
    u8_t  bt_value;         //bt from rf 1: 0.3, 0: 1.2
    u8_t  group_number;    //frame ul group number: 1,2,4,8
    u8_t  na[48];
}dynamic_para_t;

typedef struct blacklist
{
    u32_t user_id;
    struct blacklist *next;
}blacklist_t;

typedef struct iote_info
{
    u32_t user_id;
    // u8_t signal_quality;
    struct iote_info *next;
}iote_info_t;

typedef struct
{
    u32_t frequency_point;
    u32_t frequency;
    u32_t rssi;
    //TODO:
}scan_result_t;

typedef struct
{
    u64_t user_id;
    u32_t scramb_id;
    u16_t boost_level;
    u16_t cycle;
}iote_paging_info_t;

typedef struct
{
    //TODO:
}exception_info_t;

typedef enum
{
    SYMBOL_LENGTH_128 = 0,  //symbol length is 128.
    SYMBOL_LENGTH_256 = 1,  //symbol length is 256.
    SYMBOL_LENGTH_512 = 2,  //symbol length is 512.
    SYMBOL_LENGTH_1024 = 3, //symbol length is 1024.
    SYMBOL_LENGTH_INVALID,
}symblo_length_e;

typedef enum
{
    DL_UL_ONE_TO_ONE = 0, //1:1
    DL_UL_ONE_TO_TWO = 1, //1:2
    DL_UL_INVALID,
}dl_ul_ratio_e;

typedef enum
{
    BT_VALUE_0_POINT_3 = 0,//0.3:only GMSK is supported,but all symbol lengths are supported.
    BT_VALUE_1_POINT_2 = 1,//1.2:symbol length only supports 128 and 256,but supports GMSK,4PSK and 8PSK.
    BT_VALUE_INVALID,
}bt_value_e;

typedef enum
{
    GROUP_NUMBER_1 = 0, //group is 1.
    GROUP_NUMBER_2 = 1, //group is 2.
    GROUP_NUMBER_4 = 2, //group is 4.
    GROUP_NUMBER_8 = 3, //group is 8.
    GROUP_NUMBER_INVALID,
}group_number_e;

typedef void (*uc_result)(uc_result_e result);
typedef void (*uc_iote_access)(u32_t user_id);
typedef void (*uc_iote_drop)(u32_t user_id);
typedef void (*uc_report_data)(u32_t user_id, u8_t *report_data, u32_t report_data_len);

/*********************************************************************************
 This function is to set all dynamic parameter

 param:
        in:
            dyna_para:struct of dynamic parameter.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_all_dynamic_parameter(dynamic_para_t *dyna_para);

/*********************************************************************************
 This function is to get all dynamic parameter(need be manually released after use)

 param:NULL.

 return:
    dynamic_para_t.
**********************************************************************************/
dynamic_para_t *uc_wiota_get_all_dynamic_parameter(void);

/*********************************************************************************
 This function is to set system id.

 param:
        in:
            system_id:the value of the system id to be set.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_system_id(u32_t system_id);

/*********************************************************************************
 This function is to set user id length.

 param:
        in:
            user_id_len:the value of the user id lenth to be set.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_user_id_len(u8_t user_id_len);

/*********************************************************************************
 This function is to set subsystem id.

 param:
        in:
            subsystem_id:the value of the subsystem id to be set.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_subsystem_id(u32_t subsystem_id);

/*********************************************************************************
 This function is to set pn number.

 param:
        in:
            pn_num:the value of the pn number to be set.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_pn_number(u8_t pn_num);

/*********************************************************************************
 This function is to set symbol length.

 param:
        in:
            symbol_length:the value of the symbol length to be set.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_symbol_length(symblo_length_e symbol_length);

/*********************************************************************************
 This function is to set dl-ul ratio.

 param:
        in:
            dlul_ratio:the value of the dl-ul ratio to be set.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_dlul_ratio(dl_ul_ratio_e dlul_ratio);

/*********************************************************************************
 This function is to set bt value.

 param:
        in:
            bt_value:the value of the bt value to be set.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_bt_value(bt_value_e bt_value);

/*********************************************************************************
 This function is to set group number.

 param:
        in:
            group_number:the value of the group number to be set.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_group_number(group_number_e group_number);

/*********************************************************************************
 This function is to set dcxo.

 param:
        in:
            dcxo:the value of the frequency offset to be set.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_dcxo(u32_t dcxo);

/*********************************************************************************
 This function is to set single frequency point.

 param:
        in:
            frequency_point:single frequency point.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_frequency_point(u8_t frequency_point);

/*********************************************************************************
 This function is to get the header of the blacklist linked list.(Need to release
    the head pointer after use)

 param:
        in:NULL.
        out:
            blacklist_num:number of blacklist linked list nodes.

 return:
        blacklist_t:pointer of blacklist linked list header.
**********************************************************************************/
blacklist_t *uc_wiota_get_blacklist(u16_t *blacklist_num);

/*********************************************************************************
 This function is to print all the information in the blacklist.(Need to release
    the head pointer after use)

 param:
        in:
            head_node:blacklist linked list header.
            blacklist_num:number of blacklist nodes.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_print_blacklist(blacklist_t *head_node, u16_t blacklist_num);

/*********************************************************************************
 This function is to add one or more iotes to the blacklist linked lcist.

 param:
        in:
            user_id:the first address of the user id array of iote.
            user_id_num:number of user id.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_add_iote_to_blacklist(u32_t *user_id, u16_t user_id_num);

/*********************************************************************************
 This function is to removed one or more iotes from the blacklist linked list.

 param:
        in:
            user_id:the first address of the user id array of iote.
            user_id_num:number of user id.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_remove_iote_from_blacklist(u32_t *user_id, u16_t user_id_num);

/*********************************************************************************
 This function is to printing all iote information.

 param:
        in:
            head_node:iote information linked list header.
            iote_num:the number of iote currently connected.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_print_iote_info(iote_info_t *head_node, u16_t iote_num);

/*********************************************************************************
 This function is to query the information of all connected iotes.(Need to release
    the head pointer after use)

 param:
        in:NULL.
        out:
            iote_num:number of iote information linked list nodes.

 return:
        iote_info_t:pointer of iote information linked list header.
**********************************************************************************/
iote_info_t* uc_wiota_query_info_of_currently_connected_iote(u16_t *iote_num);

/*********************************************************************************
 This function is to sending broadcast data.(Not supported at the moment)

 param:
        in:
            send_data:data to be sent.
            send_data_len:the length of data to be sent.
            timeout:send data timeout time,unit:ms
            callback:send data result callback
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_send_broadcast_data(u8_t *send_data, u16_t send_data_len, u16_t timeout, uc_result callback);

/*********************************************************************************
 This function is to sending non-broadcast data.

 param:
        in:
            send_data:non-broadcast data to be sent.
            send_data_len:the length of non-broadcast data to be sent.
            userId:specify the user id send.
            userIdNum:number of user id.
            timeout:send data timeout time,unit:ms
            callback:send data result callback
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_send_normal_data(u8_t *send_data, u16_t send_data_len, u32_t *user_id, u32_t user_id_num, u16_t timeout, uc_result callback);

/*********************************************************************************
 This function is to scaning frequency point collection.(Not supported at the moment)

 param:
        in:
            frequency_point:frequency point collection.
            frequency_point_num:number of frequency point.
            timeout:scan timeout time,unit:ms
            callback:scan result callback
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_scan_frequency_point_collection(u8_t *frequency_point, u32_t frequency_point_num, u16_t timeout, uc_result callback);

/*********************************************************************************
 This function is to paging one or a group of iote.(Not supported at the moment)

 param:
        in:
            iote_paging:parameters of the iote to be paged.
            request_fn:requested frame number.
            iote_num:number of iotes to be paged
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_paging_iote(iote_paging_info_t *iote_paging, u32_t request_fn, u32_t iote_num);

/*********************************************************************************
 This function is to register iote access prompt callback.

 param:
        in:
            callback:function pointer,the parameter is the user id of the accessed
                iote.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_register_iote_access_callback(uc_iote_access callback);

/*********************************************************************************
 This function is to register iote drop prompt callback.

 param:
        in:
            callback:function pointer,the parameter is the user id of the dropped
                iote.
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_register_iote_dropped_callback(uc_iote_drop callback);

/*********************************************************************************
 This function is to register uplink data prompt callback.

 param:
        in:
            callback:function pointer,the parameter is the user id of the iote that
                report the data.
            report_data:data content report.
            report_data_len:data length reported
        out:NULL.

 return:
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_register_proactively_report_data_callback(uc_report_data callback);

/*********************************************************************************
 This function is to query exception information.(Empty function,not implemented)

 param:
        in:NULL.
        out:NULL.

 return:
    exception_info_t:
**********************************************************************************/
exception_info_t* uc_wiota_query_exception_infomation(void);

/*********************************************************************************
 This function is to wiota to reinit.

 param:
        in:NULL.
        out:NULL.

 return:NULL.
**********************************************************************************/
void uc_wiota_reinit(void);

/*********************************************************************************
 This function is to wiota to first init.

 param:
        in:NULL.
        out:NULL.

 return:NULL.
**********************************************************************************/
void uc_wiota_first_init(void);

/*********************************************************************************
 This function is to wiota to start.

 param:
        in:NULL.
        out:NULL.

 return:NULL.
**********************************************************************************/
void uc_wiota_start(void);

/*********************************************************************************
 This function is to wiota to exit.(Empty function,not implemented)

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
    uc_result_e.
**********************************************************************************/
uc_result_e uc_wiota_set_active_time(u32_t active_time);

/*********************************************************************************
 This function is to get the connection timeout of iote in idle state.

 param:
        in:NULL.
        out:NULL.

 return:connection timeout.
**********************************************************************************/
u32_t uc_wiota_get_active_time(void);
#ifdef __cplusplus
}
#endif
#endif