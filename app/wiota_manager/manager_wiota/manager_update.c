#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "uc_coding.h"
#include "uc_uboot.h"
#include "uc_wiota_static.h"
#include "manager_module.h"
#include "manager_update.h"
#include "manager_addrss.h"
#include "manager_wiota_frame.h"
#include "manager_logic_cmd.h"
#include "manager_net_messager.h"
#include "manager_userid.h"
#include "http_downloader.h"
#include "partition_info.h"
#include "cJSON.h"

t_manager_update manager_update_parment = {0};

static int manager_start_send_page(void);
static void manager_ota_msg_result(void *buf);
void manager_ap_update(int file_size);
static void manager_resend_data(void);

int manager_init_resend_sub_flag(char **flag, int *flag_num, int len, int sub_len)
{
    int sub_flag_bit_count = len / sub_len + (len % sub_len > 0? len % sub_len : 0);

    *flag_num = sub_flag_bit_count/8 + (sub_flag_bit_count % 8 > 0 ? 1:0);

    *flag = rt_realloc(*flag, sub_flag_bit_count/8 + 1); 
    if (RT_NULL == *flag)
    {
        rt_kprintf("manager_set_resend_sub_flag malloc error\n");
        return 1;
    }

    rt_memset(*flag, 0, sub_flag_bit_count/8 + 1);
    return 0;
}

void manager_test_resend_sub_flag(char *flag, int sub_len)
{
    int num = 0;
    rt_kprintf("manager_test_resend_sub_flag data:\n");
    for(num = 0; num <  sub_len; num++)
    {
        rt_kprintf("0x%x ",flag[num]);
    }
    rt_kprintf("\n");
}


int manager_set_resend_sub_flag(char *flag,  int offset, int len, int sub_len)
{
    int start_bit = offset/sub_len;
    int block_num = len/sub_len;

    while(block_num)
    {
        flag[start_bit/8] |= (1 << (start_bit%8));
        block_num--;
        start_bit ++;
    }
    return 0;
}

// query the position with the highest bit as 1 in descending order of bit
int manager_get_resend_sub_flag(char *flag, int flag_num)
{
    int i ;
    
    for(i = 0; i < flag_num; i ++)
    {
        if (flag[i] != 0)
        {
            int j = 0;
            int f = 0;
            
            for(j = 0; j < 8; j++)
            {
                if(flag[i] & (1 << j))
                {
                    f = 1;
                    break;
                }
            }
            
            if (0 == f)
                return -1;
            
            rt_kprintf("flag_num = %d, f = %d , flag[%d] = 0x%x, j = %d\n", flag_num, f, i , flag[i], j);
            return i * 8 + (f > 0? j : 0);
        }
    }

    return -2;
}

void manager_clean_resend_sub_flag(char *flag, int bit_num)
{
    flag[bit_num/8] &= (~(1 << ( bit_num%8)));
    rt_kprintf("flag[%d]=0x%x. bit_num%8 = %d\n", bit_num/8, flag[bit_num/8], bit_num%8);
}


void manager_set_ota_state(MANAGER_UPDATE_STATE state)
{
    manager_update_parment.state = state;
}

MANAGER_UPDATE_STATE manager_get_ota_state(void)
{
    return manager_update_parment.state;
}


int manager_parsing_network_ota_msg(void *page, t_manager_update *manager_up_info)
{
    cJSON *info;
    cJSON *item;
    unsigned char *json_data = page;
    cJSON *root = cJSON_Parse((char *)json_data);

    rt_kprintf("manager_parsing_network_ota_msg\n");

    item = cJSON_GetObjectItem(root, "state");
    if (RT_NULL != item && cJSON_Number == item->type)
    {
        manager_up_info->info.active_state = item->valueint;
    }

    item = cJSON_GetObjectItem(root, "range");
    if (RT_NULL != item && cJSON_Number == item->type)
    {
        manager_up_info->info.range = item->valueint;
    }
    else
    {
        manager_up_info->info.range = 0;
    }
    rt_kprintf("state %d range %d\n", manager_up_info->info.active_state, manager_up_info->info.range);

    item = cJSON_GetObjectItem(root, "new_version");
    if (RT_NULL != item && cJSON_String == item->type)
    {
        manager_up_info->info.new_version = rt_malloc(rt_strlen(item->valuestring) + 1);
        //memset(manager_up_info->info.new_version, 0, sizeof(manager_up_info->info.new_version)+1);
        rt_memcpy(manager_up_info->info.new_version, item->valuestring, rt_strlen(item->valuestring) + 1);
        rt_kprintf("new_servion %s\n", manager_up_info->info.new_version);
    }

    item = cJSON_GetObjectItem(root, "old_version");
    if (RT_NULL != item && cJSON_String == item->type)
    {
        manager_up_info->info.old_version = rt_malloc(rt_strlen(item->valuestring) + 1);
        rt_memcpy(manager_up_info->info.old_version, item->valuestring, rt_strlen(item->valuestring) + 1);
        rt_kprintf("old_version %s\n", manager_up_info->info.old_version);
    }

    item = cJSON_GetObjectItem(root, "dev_type");
    if (RT_NULL != item && cJSON_String == item->type)
    {
        manager_up_info->info.dev_type = rt_malloc(rt_strlen(item->valuestring) + 1);
        rt_memcpy(manager_up_info->info.dev_type, item->valuestring, rt_strlen(item->valuestring) + 1);
        rt_kprintf("dev_type %s\n", manager_up_info->info.dev_type);
    }

    manager_up_info->info.update_type = cJSON_GetObjectItem(root, "update_type")->valueint;
    rt_kprintf("update_type %d\n", manager_up_info->info.update_type);

    item = cJSON_GetObjectItem(root, "file");
    if (RT_NULL != item && cJSON_String == item->type)
    {
        manager_up_info->info.file_name = rt_malloc(rt_strlen(item->valuestring) + 1);
        rt_memcpy(manager_up_info->info.file_name, item->valuestring, rt_strlen(item->valuestring) + 1);
        rt_kprintf("file %s\n", manager_up_info->info.file_name);
    }

    manager_up_info->info.file_len = cJSON_GetObjectItem(root, "size")->valueint;
    rt_kprintf("file_len %d\n", manager_up_info->info.file_len);

    item = cJSON_GetObjectItem(root, "md5");
    if (RT_NULL != item && cJSON_String == item->type)
    {
        manager_up_info->info.md5 = rt_malloc(rt_strlen(item->valuestring) + 1);
        rt_memcpy(manager_up_info->info.md5, item->valuestring, rt_strlen(item->valuestring) + 1);
        rt_kprintf("md5 %s\n", manager_up_info->info.md5);
    }

    info = cJSON_GetObjectItem(root, "access_info");
    if (RT_NULL != info && cJSON_Object == info->type)
    {
        manager_up_info->info.network.access = cJSON_GetObjectItem(info, "access")->valueint;
        rt_kprintf("access %d\n", manager_up_info->info.network.access);

        item = cJSON_GetObjectItem(info, "username");
        if (RT_NULL != item && cJSON_String == item->type)
        {
            manager_up_info->info.network.username = rt_malloc(rt_strlen(item->valuestring) + 1);
            rt_memcpy(manager_up_info->info.network.username, item->valuestring, rt_strlen(item->valuestring) + 1);
            rt_kprintf("username %s\n", manager_up_info->info.network.username);
        }

        item = cJSON_GetObjectItem(info, "password");
        if (RT_NULL != item && cJSON_String == item->type)
        {
            manager_up_info->info.network.password = rt_malloc(rt_strlen(item->valuestring) + 1);
            rt_memcpy(manager_up_info->info.network.password, item->valuestring, rt_strlen(item->valuestring) + 1);
            rt_kprintf("password %s\n", manager_up_info->info.network.password);
        }

        item = cJSON_GetObjectItem(info, "path");
        if (RT_NULL != item && cJSON_String == item->type)
        {
            manager_up_info->info.network.path = rt_malloc(rt_strlen(item->valuestring) + 1);
            rt_memcpy(manager_up_info->info.network.path, item->valuestring, rt_strlen(item->valuestring) + 1);
            rt_kprintf("path %s\n", manager_up_info->info.network.path);
        }

        item = cJSON_GetObjectItem(info, "url");
        if (RT_NULL != item && cJSON_String == item->type)
        {
            manager_up_info->info.network.address = rt_malloc(rt_strlen(item->valuestring) + 1);
            rt_memcpy(manager_up_info->info.network.address, item->valuestring, rt_strlen(item->valuestring) + 1);
            rt_kprintf("address %s\n", manager_up_info->info.network.address);
        }

        item = cJSON_GetObjectItem(info, "port");
        if (RT_NULL != item && cJSON_Number == item->type)
        {
            manager_up_info->info.network.port = item->valueint;
            rt_kprintf("port %d\n", manager_up_info->info.network.port);
        }
    }
    cJSON_Delete(root);
    return 0;
}

void free_update_info(t_manager_update_info *info)
{
    info->active_state = 0;
    info->iote_num = 0;
    if (RT_NULL != info->iote_id)
    {
        rt_free(info->iote_id);
        info->iote_id = RT_NULL;
    }
    if (RT_NULL != info->new_version)
    {
        rt_free(info->new_version);
        info->new_version = RT_NULL;
    }
    if (RT_NULL != info->old_version)
    {
        rt_free(info->old_version);
        info->old_version = RT_NULL;
    }
    if (RT_NULL != info->dev_type)
    {
        rt_free(info->dev_type);
        info->dev_type = RT_NULL;
    }
    info->update_type = 0;
    info->range = 0;
    info->file_len = 0;
    if (RT_NULL != info->file_name)
    {
        rt_free(info->file_name);
        info->file_name = RT_NULL;
    }
    if (RT_NULL != info->md5)
    {
        rt_free(info->md5);
        info->md5 = RT_NULL;
    }
    info->network.access = 0;
    if (RT_NULL != info->network.username)
    {
        rt_free(info->network.username);
        info->network.username = RT_NULL;
    }
    if (RT_NULL != info->network.password)
    {
        rt_free(info->network.password);
        info->network.password = RT_NULL;
    }
    if (RT_NULL != info->network.path)
    {
        rt_free(info->network.path);
        info->network.path = RT_NULL;
    }
    if (RT_NULL != info->network.address)
    {
        rt_free(info->network.address);
        info->network.address = RT_NULL;
    }
    info->network.port = 0;
}

int manager_parsing_iote_resend_ota_msg(void *json_data, t_manager_iote_resend_info *resend_msg)
{
    cJSON *item;
    cJSON *info;
    //int array_size;
    cJSON *root = cJSON_Parse((char *)json_data);

    rt_kprintf("manager_parsing_iote_resend_ota_msg: %s\n", json_data);
    
    if (RT_NULL == root)
    {
        rt_kprintf("manager_parsing_iote_resend_ota_msg cJSON_Parse error\n");
        return 1;
    }
    
    item = cJSON_GetObjectItem(root, "dev_type");
    if (RT_NULL != item && cJSON_String == item->type)
    {
        resend_msg->dev_type = rt_malloc(rt_strlen(item->valuestring) + 1);
        rt_memcpy(resend_msg->dev_type, item->valuestring, rt_strlen(item->valuestring) + 1);
    }
    else
    {
         rt_kprintf("manager_parsing_iote_resend_ota_msg line %d cJSON_GetObjectItem error\n", __LINE__);
        return 2;
    }

    item = cJSON_GetObjectItem(root, "old_version");
    if (RT_NULL != item && cJSON_String == item->type)
    {
        resend_msg->old_version = rt_malloc(rt_strlen(item->valuestring) + 1);
        rt_memcpy(resend_msg->old_version, item->valuestring, rt_strlen(item->valuestring) + 1);
    }
    else
    {
         rt_kprintf("manager_parsing_iote_resend_ota_msg line %d cJSON_GetObjectItem error\n", __LINE__);
        return 2;
    }

    item = cJSON_GetObjectItem(root, "new_version");
    if (RT_NULL != item && cJSON_String == item->type)
    {
        resend_msg->new_version = rt_malloc(rt_strlen(item->valuestring) + 1);
        rt_memcpy(resend_msg->new_version, item->valuestring, rt_strlen(item->valuestring) + 1);
    }
    else
    {
         rt_kprintf("manager_parsing_iote_resend_ota_msg line %d cJSON_GetObjectItem error\n", __LINE__);
        return 2;
    }
    
    item = cJSON_GetObjectItem(root, "update_type");
    if (RT_NULL != item && cJSON_Number == item->type)
    {
        resend_msg->update_type = item->valueint;
    }
    else
    {
         rt_kprintf("manager_parsing_iote_resend_ota_msg line %d cJSON_GetObjectItem error\n", __LINE__);
        return 2;
    }

    info = cJSON_GetObjectItem(root, "data_info");
    if (RT_NULL == info)
    {
        rt_kprintf("manager_parsing_iote_resend_ota_msg cJSON_GetObjectItem error\n");
        return 3;
    }

    resend_msg->data_info_num = cJSON_GetArraySize(info);
    if (resend_msg->data_info_num > 0)
    {
        int n = resend_msg->data_info_num;
        int m = 0;
        resend_msg->data_info = rt_malloc(sizeof(t_data_offset)* resend_msg->data_info_num);
        if (RT_NULL == resend_msg->data_info)
         {
            rt_kprintf("resend_msg->data_info rt_malloc error\n");
            return 4;
         }

        for(m = 0; m < n; m++)
        {
            cJSON *tmp_array = cJSON_GetArrayItem(info, m);

            item = cJSON_GetObjectItem(tmp_array, "offset");
             if (RT_NULL != item && cJSON_Number == item->type)
             {
                resend_msg->data_info[m].offset = item->valueint;
                rt_kprintf("manager_parsing_iote_resend_ota_msg offset=%d\n", resend_msg->data_info[m].offset);
             }
            else
             {
                 rt_kprintf("manager_parsing_iote_resend_ota_msg line %d cJSON_GetObjectItem error\n", __LINE__);
                return 5;
            }

            item = cJSON_GetObjectItem(tmp_array, "len");
             if (RT_NULL != item && cJSON_Number == item->type)
             {
                resend_msg->data_info[m].len = item->valueint;
                rt_kprintf("manager_parsing_iote_resend_ota_msg len=%d\n", resend_msg->data_info[m].len);
             }
            else
             {
                 rt_kprintf("manager_parsing_iote_resend_ota_msg line %d cJSON_GetObjectItem error\n", __LINE__);
                return 6;
            }
        }
    }

    return 0;
}


static char * manager_ota_state_msg(int state)
{
    char *data = RT_NULL;
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "state", manager_update_parment.info.update_type );
    cJSON_AddNumberToObject(root, "range", manager_update_parment.info.range);
    cJSON_AddStringToObject(root, "new_version", manager_update_parment.info.new_version);
    cJSON_AddStringToObject(root, "old_version", manager_update_parment.info.old_version);
    cJSON_AddStringToObject(root, "dev_type", manager_update_parment.info.dev_type);
    cJSON_AddNumberToObject(root, "ota_state", state & 0x1);

    data = cJSON_Print(root);
    cJSON_Delete(root);

    return data;
}

static int manager_ota_msg_of_data(void **result_data)
{
    unsigned char bin[MANAGER_OTA_SEND_BIN_SIZE] = {0};
    int len = MANAGER_OTA_SEND_BIN_SIZE;
    cJSON *root = cJSON_CreateObject();
    cJSON *tmp = RT_NULL;
    char *data = RT_NULL;

    if (manager_update_parment.ota_op_address - UC_OTA_PARTITION_START_ADDRESS + MANAGER_OTA_SEND_BIN_SIZE > manager_update_parment.info.file_len &&
        manager_update_parment.ota_op_address - UC_OTA_PARTITION_START_ADDRESS < manager_update_parment.info.file_len)
    {
        len =  RT_ALIGN((manager_update_parment.info.file_len - (manager_update_parment.ota_op_address - UC_OTA_PARTITION_START_ADDRESS)), 4);
        rt_kprintf("manager_ota_msg_of_data offset 0x%x, len %d\n",manager_update_parment.ota_op_address - UC_OTA_PARTITION_START_ADDRESS,  len);
    }
    else if (manager_update_parment.ota_op_address - UC_OTA_PARTITION_START_ADDRESS >= manager_update_parment.info.file_len)
    {
        rt_kprintf("manager_ota_msg_of_data over\n");
        return -1;
    }


    // read flash
    uc_wiota_flash_read(bin, manager_update_parment.ota_op_address, len);
    manager_update_parment.ota_op_address += len;

    // coding
    cJSON_AddNumberToObject(root, "state", manager_update_parment.info.update_type);
    cJSON_AddNumberToObject(root, "range",  manager_update_parment.info.range );
    cJSON_AddNumberToObject(root, "size",  manager_update_parment.info.file_len );
    cJSON_AddStringToObject(root, "md5",  manager_update_parment.info.md5 );

    if (manager_update_parment.info.update_type &&
        manager_update_parment.info.iote_num > 0)
    {
        cJSON *dev_list = cJSON_AddArrayToObject(root, "dev_list");
        int num = 0;
        for (num = 0; num < manager_update_parment.info.iote_num; num++)
            cJSON_AddItemToArray(dev_list, cJSON_CreateNumber(manager_query_wiotaid(manager_update_parment.info.iote_id[num])));
    }

     cJSON_AddStringToObject(root, "new_version", manager_update_parment.info.new_version);
     cJSON_AddStringToObject(root, "old_version", manager_update_parment.info.old_version);
     cJSON_AddStringToObject(root, "dev_type", manager_update_parment.info.dev_type);
     tmp = cJSON_AddObjectToObject(root, "info");
     
     cJSON_AddNumberToObject(tmp, "offset", manager_update_parment.ota_op_address - len - UC_OTA_PARTITION_START_ADDRESS);  
     cJSON_AddNumberToObject(tmp, "len", len);
     //cJSON_AddRawToObject(tmp, "data", (const char *)bin);
     data = cJSON_Print(root);

     *result_data = rt_malloc(1024);
     rt_memcpy(*result_data, data, rt_strlen(data));
     rt_memcpy(*result_data + rt_strlen(data), bin, len);

     cJSON_free(data);
     cJSON_Delete(root);


    return  (rt_strlen(data) + len);
}

int manager_check_ap_update_info(t_manager_update_info ver)
{
    //check dev type
    if (0 != strcmp(ver.dev_type, "ap"))
        return 1;

    //check current version
    if (0 != strcmp(ver.old_version, AP_DEMO_VERSION))
        return 2;
    
    return 0;
}

void manager_start_down(t_manager_update_info *updata_info)
{
    if (manager_update_parment.info.active_state)
    {
        downloader_manager_send_page(MANAGER_OPERATION_INDENTIFICATION, MANAGER_OTA_DOWNLOAD_REQUEST,
            updata_info->file_name, updata_info->md5, &updata_info->network);
    }
}

/*
* page:   recv msg from mqtt task.
* parsing_up_msg_func: pasing the page msg callback function
* free_msg_func: free msg
*/
int manager_update_response(void *page, manager_free_msg free_msg_func,
                            manager_send_iote_update_msg iote_response_msg_func)

{
    t_from_network_message *network_page = page;

    manager_update_parment.info.iote_num = (network_page->dest_len >> 2);
    // manager_update_parment.info.iote_id = network_page->dest_address;
    manager_update_parment.info.iote_id = rt_malloc(sizeof(unsigned int) * network_page->dest_len);
    if (RT_NULL != manager_update_parment.info.iote_id)
        rt_memcpy(manager_update_parment.info.iote_id, network_page->dest_address, sizeof(unsigned int) * network_page->dest_len);

    rt_kprintf("manager_init_update\n");
    if (0 != manager_parsing_network_ota_msg(network_page->json_data, &manager_update_parment))
    {
        if (RT_NULL != iote_response_msg_func)
            iote_response_msg_func(network_page->json_data);

        rt_memset(&manager_update_parment, 0, sizeof(t_manager_update));

        if (RT_NULL != free_msg_func)
            free_msg_func(network_page->json_data);
        return 1;
    }

    if(2 == manager_check_ap_update_info(manager_update_parment.info))
    {
        rt_kprintf("manager_check_ap_update_info error\n");
        return 2;
    }
    
    manager_update_parment.ota_op_address = UC_OTA_PARTITION_START_ADDRESS;
    manager_start_down(&manager_update_parment.info);
    manager_set_ota_state(MANAGER_UPDATE_DOWN_PAGE);

    if (RT_NULL != free_msg_func)
        free_msg_func(network_page->json_data);

    return 0;
}


static int manager_ota_msg_coding(int cmd, void *data, int data_len, unsigned char **output_data, unsigned int *output_len)
{
    app_ps_header_t ps_header = {0};

    app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
    ps_header.addr.src_addr = SERVER_DEFAULT_ADDRESS;
    ps_header.cmd_type = cmd;

    return app_data_coding(&ps_header, data, data_len, output_data, output_len);
}

//extern void *test_result_fun ;
//extern void *test_result_judge ;

static void manager_ota_msg_result(void *buf)
{
    t_app_operation_data *page = buf;
    
    //test_result_fun = RT_NULL;
    
    rt_kprintf("manager_ota_msg_result\n");
    manager_start_send_page();
    //rt_kprintf("manager_ota_msg_result free data\n");
    if (RT_NULL != page->pload)
        rt_free(page->pload);

    //rt_free(page);
}

static void manager_ota_state_result(void *buf)
{
    t_app_operation_data *page = buf;

    if (RT_NULL != page->pload)
        rt_free(page->pload);

    //rt_free(page);
}

int manager_ota_send_state(int state)
{
    char *data;
    unsigned char *output_data;
    unsigned int output_len;

    rt_kprintf("manager_ota_send_state\n");

    data = manager_ota_state_msg(state);

    if (RT_NULL != data && \
        0 == manager_ota_msg_coding(APP_CMD_GET_UPDATE_STATE_REQUEST, data, rt_strlen(data), &output_data, &output_len))
    {
        // send ota data
        manager_logic_send_to_operater(\
                        MANAGER_OPERATION_OTA_RADIO, \
                        0, \
                        SERVER_DEFAULT_ADDRESS,\
                        APP_CMD_GET_SPECIFIC_DATA_SPONSE,  \
                        0,\
                        output_len,\
                        output_data,\
                        RT_NULL,\
                        manager_ota_state_result\
                        );
    }

    return 0;
}

static int manager_start_send_page(void)
{
    char *data = RT_NULL;
    unsigned char *output_data;
    unsigned int output_len;
    int data_len = 0;
    int result = 0;

    data_len = manager_ota_msg_of_data((void **)&data);
    if (RT_NULL != data && data_len > 0)
    {
        int num = 0;
        while(1)
        {
            if (0 == manager_ota_msg_coding(APP_CMD_GET_SPECIFIC_DATA_SPONSE, data, data_len, &output_data, &output_len))
            {
                // send ota data
                manager_logic_send_to_operater(\
                                MANAGER_OPERATION_OTA_RADIO, \
                                0, \
                                SERVER_DEFAULT_ADDRESS,
                                APP_CMD_GET_SPECIFIC_DATA_SPONSE,  0,
                                output_len,
                                output_data,
                                RT_NULL,
                                manager_ota_msg_result\
                                );
                //test_result_judge = manager_ota_msg_result;
                break;
            }
            else if ((num ++) > 3)
            {
                rt_kprintf("manager_ota_msg_coding error\n");
                manager_ota_send_state(MANAGER_OTA_STOP);
                result = 1;
                break;
            }
        }
    }
    else
    {
        manager_update_parment.state = MANAGER_UPDATE_STOP;
        manager_update_parment.ota_op_address = UC_OTA_PARTITION_START_ADDRESS;

        // send ota state
        manager_ota_send_state(MANAGER_OTA_STOP);
        result = 2;
    }

    if (RT_NULL != data)
        rt_free(data);    
    
    return result;
}


// manager iote enter update
static int manager_start_iote_update(void)
{
    manager_update_parment.ota_op_address = UC_OTA_PARTITION_START_ADDRESS;
    manager_update_parment.state = MANAGER_START_TRANSFER_DATA;
    if (manager_update_parment.info.active_state)
        return manager_start_send_page();
    else
        return 1;
}

// manager ap enter update modem
void manager_ap_update(int file_size)
{
    manager_update_parment.state = MANAGER_UPDATE_SYSTEM;
    free_update_info(&manager_update_parment.info);

    // set file size
    boot_set_file_size((unsigned int)file_size);
    // set uboot mode (BOOT_FLASH_ALL)
    boot_set_mode(BOOT_RUN_OTA_ONLY_UPDATE);
    // disable system interrupt
    rt_hw_interrupt_disable();
    // resicv reboot
    boot_riscv_reboot();

    return;
}

int manager_start_update(void *data)
{
    t_http_message *http_data = data;

    if (http_data->state == HTTP_DOWNLOAD_OK)
    {
        //manager_update_parment.info.file_len = http_data->down_file_size;
        rt_kprintf("http_data->down_file_size %d dev_type = %s\n", http_data->down_file_size, manager_update_parment.info.dev_type);

    #ifdef HTTP_DOWNLOAD_AP_DEBUG
        rt_kprintf("now AP update...\n");
        manager_ap_update(http_data->down_file_size);
    #elif defined HTTP_DOWNLOAD_IOTE_DEBUG
        rt_kprintf("now iote update...\n");
        manager_update_parment.info.file_len = http_data->down_file_size;
        manager_init_resend_sub_flag(&manager_update_parment.rem_sub_flag,\
                        &manager_update_parment.sub_flag_num, \
                        manager_update_parment.info.file_len,\
                        MANAGER_OTA_SEND_BIN_SIZE);
        manager_start_iote_update();
    #else
        if (AP_DEFAULT_ADDRESS == manager_update_parment.info.iote_id[0] &&\
            0 == rt_strcmp("ap", manager_update_parment.info.dev_type))
        {
            rt_kprintf("now AP update...\n");
            manager_ap_update(http_data->down_file_size);
        }
        else
        {
            rt_kprintf("now iote update...\n");
            manager_update_parment.info.file_len = http_data->down_file_size;
            manager_init_resend_sub_flag(&manager_update_parment.rem_sub_flag,\
                            &manager_update_parment.sub_flag_num, \
                            manager_update_parment.info.file_len,\
                            MANAGER_OTA_SEND_BIN_SIZE);
            manager_start_iote_update();
        }
    #endif
    }
    else
    {
        manager_set_ota_state(MANAGER_UPDATE_STOP);
    }
    return 0;
}


static void manager_ota_resend_msg_result(void *buf)
{
    t_app_operation_data *page = buf;
    
    manager_resend_data();
    
    if (RT_NULL != page->pload)
        rt_free(page->pload);

    //rt_free(page);
}


static void manager_resend_data(void)
{
    char *data;
    int data_len;
    char *output_data;
    int output_len;
    int loc = 0;

    // get flag
    loc = manager_get_resend_sub_flag(manager_update_parment.rem_sub_flag, manager_update_parment.sub_flag_num);
    if (loc < 0)
    {
        return ;
    }

    rt_kprintf("manager_resend_data loc %d\n", loc);
    
    // set address
    manager_update_parment.ota_op_address = UC_OTA_PARTITION_START_ADDRESS + loc * MANAGER_OTA_SEND_BIN_SIZE;

    // clear flag
    manager_clean_resend_sub_flag(manager_update_parment.rem_sub_flag, loc);
    manager_test_resend_sub_flag(manager_update_parment.rem_sub_flag, manager_update_parment.sub_flag_num);

    // get data from flash.
    data_len = manager_ota_msg_of_data(&data);
    if (RT_NULL != data && data_len > 0 &&\
        0 == manager_ota_msg_coding(APP_CMD_GET_SPECIFIC_DATA_SPONSE, data, data_len, &output_data, &output_len))
    {
        // send ota data
        manager_logic_send_to_operater(\
                        MANAGER_OPERATION_OTA_RADIO, \
                        0, \
                        SERVER_DEFAULT_ADDRESS,
                        APP_CMD_GET_SPECIFIC_DATA_SPONSE,  0,
                        output_len,
                        output_data,
                        RT_NULL,
                        manager_ota_resend_msg_result\
                        );
    }
    else
    {
        if (RT_NULL != data)
            rt_free(data);

        rt_kprintf("manager_resend_data error or over\n");

        return ;
    }
}

static int manager_check_iote_resend_quest(t_manager_iote_resend_info iote_info, t_manager_update_info manager_info)
{
    if (strcmp(iote_info.new_version, manager_info.new_version))
    {
        rt_kprintf("iote msg new version:%s\n", iote_info.new_version);
        rt_kprintf("manager msg new version:%s\n", manager_info.new_version);
        return 1;
    }

    if (strcmp(iote_info.old_version, manager_info.old_version))
    {
        rt_kprintf("iote msg old version:%s\n",  iote_info.old_version);
        rt_kprintf("manager msg old version:%s\n", manager_info.old_version);
        return 2;
    }

    if (iote_info.update_type != manager_info.update_type)
    {
        rt_kprintf("iote msg update_type %d\n", iote_info.update_type );
        rt_kprintf("manager update_type:%d\n", manager_info.update_type);
        return 3;
    }

    return 0;
}

int manager_ota_resend_data(void *msg)
{
    int i = 0;
    t_manager_iote_resend_info resend_requst_msg;
    int flag = 0;

    // parsing resend data msg 
    if(0 != manager_parsing_iote_resend_ota_msg(msg, &resend_requst_msg))
        return 1;

    if(0 != manager_check_iote_resend_quest(resend_requst_msg, manager_update_parment.info))
    {
        if (resend_requst_msg.data_info_num > 0)
        rt_free(resend_requst_msg.data_info);
        return 2;
    }
    
    flag = manager_get_resend_sub_flag(manager_update_parment.rem_sub_flag, manager_update_parment.sub_flag_num);
    rt_kprintf("sub_flag_num=%d, flag=%d\n", manager_update_parment.sub_flag_num, flag);
    
    // set flag
    for(i = 0; i < resend_requst_msg.data_info_num; i ++)
    {
        manager_set_resend_sub_flag(manager_update_parment.rem_sub_flag,  \
            resend_requst_msg.data_info[i].offset, \
            resend_requst_msg.data_info[i].len, \
            MANAGER_OTA_SEND_BIN_SIZE);
    }

    manager_test_resend_sub_flag(manager_update_parment.rem_sub_flag, manager_update_parment.sub_flag_num);
    
    if (flag)
    {
        // resend data
        manager_resend_data();
    }

    if (resend_requst_msg.data_info_num > 0)
        rt_free(resend_requst_msg.data_info);

    return 0;
}

#endif
