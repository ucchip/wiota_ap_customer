#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "uc_coding.h"
#include "uc_uboot.h"
#include "uc_wiota_static.h"
#include "manager_update.h"
#include "manager_addrss.h"
#include "manager_wiota_frame.h"
#include "manager_logic_cmd.h"
#include "manager_net_messager.h"
#include "http_downloader.h"
#include "partition_info.h"
#include "cJSON.h"

t_manager_update manager_update_parment = {0};

static int manager_start_send_page(void);
static void manager_ota_msg_result(void *buf);
void manager_ap_update(void);

void manager_set_ota_state(MANAGER_UPDATE_STATE state)
{
    manager_update_parment.state = state;
}

MANAGER_UPDATE_STATE manager_get_ota_state(void)
{
    return manager_update_parment.state;
}
/*
void manager_read_update_file(int offset, void *data, int len)
{
    uc_wiota_flash_read(data, UC_OTA_PARTITION_START_ADDRESS + offset, len);
}

// save ota file
int manager_write_update_file(void *data, int len)
{
    static char save_data[4*1024] = {0};
    static unsigned int all_len = 0;
    static unsigned int active_len = 0;

    all_len += len;

    if (len + active_len < 4096)
    {
        rt_memcpy(save_data + active_len, data, len);
        active_len += len;
    }
    else
    {
        rt_memcpy(save_data + active_len, data, 4096 - active_len);
        // erase 4K flash
        uc_wiota_flash_erase_4K(manager_update_parment.ota_op_address);
        // write copy source_addr to dest_addr, without erase
        uc_wiota_flash_write((unsigned char *)save_data, manager_update_parment.ota_op_address, 4096);
        manager_update_parment.ota_op_address += 4096;

        rt_memcpy(save_data, data + 4096 - active_len, len - (4096 - active_len));
        active_len = len - (4096 - active_len);
    }

    if (all_len >= manager_update_parment.info.file_len && active_len > 0)
    {
        // erase 4K flash
        uc_wiota_flash_erase_4K(manager_update_parment.ota_op_address);
        // write copy source_addr to dest_addr, without erase
        uc_wiota_flash_write((unsigned char *)save_data, manager_update_parment.ota_op_address, active_len);
        manager_update_parment.ota_op_address = UC_OTA_PARTITION_START_ADDRESS;
        all_len = 0;
        active_len = 0;
    }

    return 0;
}
*/
// // save ota file
// int manager_write_update_file(void *data, int len)
// {
//     static char save_data[4*1024] = {0};
//     static unsigned int all_len = 0;
//     static unsigned int active_len = 0;

//     all_len += len;

//     if (len + active_len < 4096)
//     {
//         rt_memcpy(save_data + active_len, data, len);
//         active_len += len;
//     }
//     else
//     {
//         rt_memcpy(save_data + active_len, data, 4096 - active_len - len);

//         // erase 4K flash
//         uc_wiota_flash_erase_4K(manager_update_parment.ota_op_address);
//         // write copy source_addr to dest_addr, without erase
//         uc_wiota_flash_write((unsigned char *)save_data, manager_update_parment.ota_op_address, 4096);
//         manager_update_parment.ota_op_address += 4096;

//         rt_memcpy(save_data, data + 4096 - active_len - len, len - (4096 - active_len - len));
//         active_len = len - (4096 - active_len - len);
//     }

//     if (all_len >= manager_update_parment.info.file_len && active_len > 0)
//     {
//         // erase 4K flash
//         uc_wiota_flash_erase_4K(manager_update_parment.ota_op_address);
//         // write copy source_addr to dest_addr, without erase
//         uc_wiota_flash_write((unsigned char *)save_data, manager_update_parment.ota_op_address, active_len);
//         manager_update_parment.ota_op_address += active_len;
//         all_len = 0;
//         active_len = 0;
//     }

//     return 0;
// }

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
        rt_memcpy(manager_up_info->info.dev_type, item, rt_strlen(item->valuestring) + 1);
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
        // manager_up_info->info.md5 = rt_malloc(rt_strlen(item->valuestring) + 1);
        // rt_memcpy(manager_up_info->info.md5, item->valuestring, rt_strlen(item->valuestring) + 1);
        // rt_kprintf("md5 %s\n", manager_up_info->info.md5);

        char md5[] = "7d0c22b08db697122b57b808beed737c";
        manager_up_info->info.md5 = rt_malloc(rt_strlen(md5) + 1);
        rt_memcpy(manager_up_info->info.md5, md5, rt_strlen(md5) + 1);
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
            // manager_up_info->info.network.address = rt_malloc(rt_strlen(item->valuestring) + 1);
            // rt_memcpy(manager_up_info->info.network.address, item->valuestring, rt_strlen(item->valuestring) + 1);

            // debug:
            // char uri[] = "http://www.rt-thread.com/service/rt-thread.txt";
            // char uri[] = "http://47.108.173.173:8085/static/test.txt";
            // char uri[] = "http://47.108.173.173:8085/static/test1.txt";
            char uri[] = "http://119.84.87.182:2021/test.txt";
            manager_up_info->info.network.address = rt_malloc(rt_strlen(uri) + 1);
            rt_memcpy(manager_up_info->info.network.address, uri, rt_strlen(uri) + 1);

            rt_kprintf("address %s\n", manager_up_info->info.network.address);

            manager_up_info->info.file_len = 451398;
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

static char * manager_ota_state_msg(int state)
{
    char *data = RT_NULL;
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "state", manager_update_parment.info.range);
    cJSON_AddNumberToObject(root, "range", manager_update_parment.info.update_type);
    cJSON_AddStringToObject(root, "new_version", manager_update_parment.info.new_version);
    cJSON_AddStringToObject(root, "old_version", manager_update_parment.info.old_version);
    cJSON_AddStringToObject(root, "dev_type", manager_update_parment.info.dev_type);
    cJSON_AddNumberToObject(root, "ota_state", state & 0x1);

    data = cJSON_Print(root);
    cJSON_Delete(root);

    return data;
}

static char *manager_ota_msg_of_data(void)
{
    unsigned char bin[MANAGER_OTA_SEND_BIN_SIZE] = {0};
    int len = MANAGER_OTA_SEND_BIN_SIZE;
    cJSON *root = cJSON_CreateObject();
    cJSON *tmp = RT_NULL;
    char *data = RT_NULL;
    char *result_data = RT_NULL;

    if (manager_update_parment.ota_op_address - UC_OTA_PARTITION_START_ADDRESS + MANAGER_OTA_SEND_BIN_SIZE > manager_update_parment.info.file_len &&
        manager_update_parment.ota_op_address - UC_OTA_PARTITION_START_ADDRESS < manager_update_parment.info.file_len)
        len = manager_update_parment.info.file_len - (manager_update_parment.ota_op_address - UC_OTA_PARTITION_START_ADDRESS);
    else if (manager_update_parment.ota_op_address - UC_OTA_PARTITION_START_ADDRESS >= manager_update_parment.info.file_len)
        return RT_NULL;

    // read flash
    uc_wiota_flash_read(bin, manager_update_parment.ota_op_address, len);
    manager_update_parment.ota_op_address += len;

    // coding
    cJSON_AddNumberToObject(root, "state", manager_update_parment.info.range);
    cJSON_AddNumberToObject(root, "range", manager_update_parment.info.update_type);

    if (manager_update_parment.info.update_type &&
        manager_update_parment.info.iote_num > 0)
    {
        cJSON *dev_list = cJSON_AddArrayToObject(root, "dev_list");
        int num = 0;
        for (num = 0; num < manager_update_parment.info.iote_num; num++)
            cJSON_AddItemToArray(dev_list, cJSON_CreateNumber(manager_update_parment.info.iote_id[num]));
    }

     cJSON_AddStringToObject(root, "new_version", manager_update_parment.info.new_version);
     cJSON_AddStringToObject(root, "old_version", manager_update_parment.info.old_version);
     cJSON_AddStringToObject(root, "dev_type", manager_update_parment.info.dev_type);
     tmp = cJSON_AddObjectToObject(root, "info");
     cJSON_AddNumberToObject(tmp, "offset", 0);
     cJSON_AddNumberToObject(tmp, "len", len);
     //cJSON_AddRawToObject(tmp, "data", (const char *)bin);
     data = cJSON_Print(root);

     result_data = rt_malloc(1024);
     rt_memcpy(result_data, data, rt_strlen(data));
     rt_memcpy(result_data + rt_strlen(data), bin, len);

     cJSON_free(data);
     cJSON_Delete(root);


     return result_data;
}

void manager_start_down(t_manager_update_info *updata_info)
{
    if (manager_update_parment.info.active_state)
    {
        // downloader_manager_send_page(0, 1, updata_info->file_name, updata_info->md5, &updata_info->network);
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
    manager_update_parment.info.iote_id = network_page->dest_address;

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
    manager_update_parment.ota_op_address = UC_OTA_PARTITION_START_ADDRESS;
    manager_start_down(&manager_update_parment.info);
    manager_set_ota_state(MANAGER_UPDATE_DOWN_PAGE);

    if (RT_NULL != free_msg_func)
        free_msg_func(network_page->json_data);

    return 0;
}


static int manager_ota_msg_coding(int cmd, void *data, unsigned char **output_data, unsigned int *output_len)
{
    app_ps_header_t ps_header = {0};

    app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
    ps_header.addr.src_addr = SERVER_DEFAULT_ADDRESS;
    ps_header.cmd_type = cmd;

    return app_data_coding(&ps_header, data, (unsigned int)rt_strlen(data), output_data, output_len);
}

static void manager_ota_msg_result(void *buf)
{
    t_app_operation_data *page = buf;

    manager_start_send_page();

    if (RT_NULL != page->pload)
        rt_free(page->pload);

    rt_free(page);
}
static void manager_ota_state_result(void *buf)
{
    t_app_operation_data *page = buf;

    if (RT_NULL != page->pload)
        rt_free(page->pload);

    rt_free(page);
}

int manager_ota_send_state(int state)
{
    char *data;
    unsigned char *output_data;
    unsigned int output_len;

    data = manager_ota_state_msg(state);

    if (RT_NULL != data && \
        0 == manager_ota_msg_coding(APP_CMD_GET_UPDATE_STATE_REQUEST, data, &output_data, &output_len))
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
    char *data;
    unsigned char *output_data;
    unsigned int output_len;

    data = manager_ota_msg_of_data();
    if (RT_NULL != data && \
        0 == manager_ota_msg_coding(APP_CMD_GET_SPECIFIC_DATA_SPONSE, data, &output_data, &output_len))
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
    }
    else
    {
        if (RT_NULL != data)
            rt_free(data);

        manager_update_parment.state = MANAGER_UPDATE_STOP;
        manager_update_parment.ota_op_address = UC_OTA_PARTITION_START_ADDRESS;

        // send ota state
        manager_ota_send_state(MANAGER_OTA_GONGING);
        return 1;
    }

    return 0;
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
void manager_ap_update(void)
{
    manager_update_parment.state = MANAGER_UPDATE_SYSTEM;

    // set uboot modem
    boot_set_mode(BOOT_RUN_OTA_ONLY_UPDATE);
    // resicv reboot
    boot_riscv_reboot();

    return;
}

int manager_start_update(void *data)
{
    t_http_message *http_data = data;

    if (http_data->state == HTTP_DOWNLOAD_OK)
    {
        if (AP_DEFAULT_ADDRESS == manager_update_parment.info.iote_id[0])
            manager_ap_update();
        else
        {
            manager_start_iote_update();
        }
    }
    else
    {
        manager_set_ota_state(MANAGER_UPDATE_STOP);
    }
    return 0;
}
#endif
