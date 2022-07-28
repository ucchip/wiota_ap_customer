

#ifndef _MANAGER_UPDATE_H_
#define _MANAGER_UPDATE_H_

typedef struct manager_netwwork_info
{
    int access;
    char *username;
    char *password;
    char *path;
    char *address;
    int port;
} t_manager_netwwork_info;

typedef struct manager_update_info
{
    int active_state;
    unsigned int iote_num;
    unsigned int *iote_id;
    char *new_version;
    char *old_version;
    char *dev_type;
    int update_type;
    int range;
    char *file_name;
    int file_len;
    char *md5;
    t_manager_netwwork_info network;
} t_manager_update_info;

typedef enum
{
    MANAGER_UPDATE_DEFAULT = 0,
    MANAGER_UPDATE_REQUEST_MSG,
    MANAGER_UPDATE_RESPONSE_MSG,
    MANAGER_UPDATE_DOWN_PAGE,
    MANAGER_START_TRANSFER_DATA,
    MANAGER_UPDATE_SYSTEM,
    MANAGER_UPDATE_STOP,
} MANAGER_UPDATE_STATE;

typedef enum
{
    MANAGER_OTA_STOP = 0,
    MANAGER_OTA_GONGING = 1,
}MANAGER_OTA_STATE;


typedef struct manager_update
{
    MANAGER_UPDATE_STATE state;
    unsigned int ota_op_address; // ota flash address
    t_manager_update_info info;
} t_manager_update;

typedef enum
{
    HTTP_DOWNLOAD_OK = 0,
    HTTP_DOWNLOAD_FAIL,
}HTTP_MESSAGE_CMD;

typedef struct http_message
{
    int state;
} t_http_message;

//typedef int (*manager_parsing_update_msg)(void *page, t_manager_update *manager_up_info);
typedef void (*manager_send_iote_update_msg)(void *page);
typedef void (*manager_start_down_file)(t_manager_netwwork_info network_info);
typedef void (*manager_free_msg)(void *page);

#define MANAGER_OTA_SEND_BIN_SIZE 512
#define MANAGER_OTA_ADDRESS 618 * 1024 // ??

void manager_set_ota_state(MANAGER_UPDATE_STATE state);

MANAGER_UPDATE_STATE manager_get_ota_state(void);

int manager_update_response(void *page, manager_free_msg free_msg_func,\
                                                          manager_send_iote_update_msg iote_response_msg_func);

void manager_read_update_file(int offset, void *data, int len);

int manager_write_update_file(void *data, int len);

int manager_start_update(void *data);

void manager_ap_update(void);

int manager_ota_send_state(int state);

#endif
