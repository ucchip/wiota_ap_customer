#ifndef _MANAGER_OPERATION_H_
#define _MANAGER_OPERATION_H_

#define MANAGER_CHECKOUT_AP_STATE_TIMEOUT 4*60*1000
typedef enum manager_send_state
{
    MANAGER_SEND_DEFAULT = 0,
    MANAGER_SEND_SUCCESS,
    MANAGER_SEND_FAIL,
}e_manager_operation_result;

typedef void (*manager_message_exe_result)(void *buf);

typedef struct app_operation_head
{
    unsigned int src_address; /*src address is 0,no message response is required*/
    unsigned int id;
    e_manager_operation_result state;
    manager_message_exe_result result_function;
    unsigned int cmd;
    unsigned int request_number;
} t_app_operation_head;



typedef struct app_operation_data
{
    t_app_operation_head head;
    int len;
    void *pload;
} t_app_operation_data;

typedef struct app_recv_wiota_info
{
    unsigned int id;
    unsigned int len;
    void *data;
} t_app_recv_wiota_info;

typedef struct userid_info
{
    unsigned int id;
    unsigned int scramble_id;
} t_userid_info;

#define MANAGER_MAX_SEND_NUM 3

int manager_create_operation_queue(void);

void manager_logic_send_to_operater(unsigned int request_number, unsigned int src_address, unsigned int cmd, unsigned int id, int len, void *data, manager_message_exe_result func);

void to_operation_wiota_data(int cmd, void *data);

void manager_operation_task(void *pPara);

void manager_recv_data(unsigned int user_id, unsigned char *recv_data, unsigned int data_len, unsigned char type);

#endif
