/*
 * Copyright (c) 2022, Chongqing UCchip InfoTech Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @brief HTTP OTA downloader
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-21     Lujun        the first version
 */

#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <tiny_md5.h>
#include <webclient.h>
#include "partition_info.h"
#include "uc_wiota_static.h"
#include "manager_module.h"
#include "manager_queue.h"
#include "manager_logic.h"
#include "manager_logic_cmd.h"
#include "manager_update.h"
#include "http_downloader.h"

/**
 * @brief download file information.
 *
 */
static t_download_file_info download_file = {0};

/**
 * @brief message queue for HTTP OTA downloader
 *
 */
static void *http_downloader_handle = RT_NULL;

/**
 * @brief  http breakpoint resume and shard download
 *
 * @param  buffer the received data buffer
 * @param  length the length of received data
 * @return 0
 */
static int shard_download_handle(char *buffer, int length)
{
    /* erase 4K flash */
    uc_wiota_flash_erase_4K(UC_OTA_PARTITION_START_ADDRESS + download_file.recv_len);
    /* write copy source_addr to dest_addr, without erase */
    uc_wiota_flash_write((unsigned char *)buffer, UC_OTA_PARTITION_START_ADDRESS + download_file.recv_len, FLASH_ALIGN(length));
    download_file.recv_len += length;

    /* print download progress */
    rt_kprintf("http ota download: %d%%\n", download_file.recv_len * 100 / download_file.file_size);

    /* release buffer */
    web_free(buffer);
    return RT_EOK;
}

/**
 * @brief  http breakpoint resume and shard download
 *
 * @param  uri uniform resource identifier
 * @param  handle_function handle function
 * @return 0: if successful
 *         !0: otherwise
 */
static int webclient_downloader(const char *uri, int (*handle_function)(char *buffer, int length))
{
    int length = 0;
    rt_err_t rc = RT_EOK;
    struct webclient_session* session = RT_NULL;
    do
    {
        /* create webclient session */
        session = webclient_session_create(1024);
        if (RT_NULL == session)
        {
            rc = -RT_ENOMEM;
            break;
        }
        /* get the real data length */
        rc = webclient_shard_head_function(session, uri, &length);
        if (rc < 0)
        {
            rc = -RT_ERROR;
            break;
        }

        /* download file information */
        download_file.file_size = length;
        download_file.recv_len = 0;

        /* register a handle function for http breakpoint resume and shard download */
        webclient_register_shard_position_function(session, shard_download_handle);
        /* http breakpoint resume and shard download */
        rc = webclient_shard_position_function(session, uri, 0, length, HTTP_DOWNLOADER_BLOCK_SIZE);
        if (WEBCLIENT_OK != rc)
        {
            rt_kprintf("webclient_downloader() error, code=%d.\n", rc);
            break;
        }
        /* clear the handle function */
        webclient_register_shard_position_function(session, RT_NULL);
        rc = RT_EOK;
    } while (0);

    /* close session */
    if (RT_NULL != session)
    {
        webclient_close(session);
        session = RT_NULL;
    }
    return rc;
}

/**
 * @brief MD5 file verification
 *
 * @return 0: if MD5 verification passed
 *         1: if download incomplete
 *         2: if MD5 verification failed
 */
static int md5_verification(void)
{
    int i, rc = 0;
    char md5[33] = {0};
    unsigned char *buffer = RT_NULL;
    unsigned char md5_value[16] = {0};
    tiny_md5_context ctx = {0};

    /* if download incomplete */
    if (download_file.recv_len < download_file.file_size)
        return 1;
    /* allocate memory */
    buffer = rt_malloc(HTTP_DOWNLOADER_BLOCK_SIZE);

    /* MD5 context setup */
    tiny_md5_starts(&ctx);
    for (i = 0; i < download_file.file_size - HTTP_DOWNLOADER_BLOCK_SIZE; i += HTTP_DOWNLOADER_BLOCK_SIZE)
    {
        /* read file from flash */
        uc_wiota_flash_read(buffer, UC_OTA_PARTITION_START_ADDRESS + i, HTTP_DOWNLOADER_BLOCK_SIZE);
        /* update MD5 value */
        tiny_md5_update(&ctx, buffer, HTTP_DOWNLOADER_BLOCK_SIZE);
    }
    if (i < download_file.file_size)
    {
        /* read file from flash */
        uc_wiota_flash_read(buffer, UC_OTA_PARTITION_START_ADDRESS + i, FLASH_ALIGN(download_file.file_size - i));
        /* update MD5 value */
        tiny_md5_update(&ctx, buffer, download_file.file_size - i);
    }
    /* MD5 final digest */
    tiny_md5_finish(&ctx, md5_value);

    /* MD5 verification */
    rt_snprintf(md5, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        md5_value[0], md5_value[1], md5_value[2], md5_value[3],
        md5_value[4], md5_value[5], md5_value[6], md5_value[7],
        md5_value[8], md5_value[9], md5_value[10], md5_value[11],
        md5_value[12], md5_value[13], md5_value[14], md5_value[15]);
    if (0 != rt_memcmp(download_file.md5, md5, 32))
    {
        rt_kprintf("md5:%s\n", md5);
        rc = 2;
    }

    /* release memory */
    if (RT_NULL != buffer)
    {
        rt_free(buffer);
        buffer = RT_NULL;
    }
    return rc;
}

/**
 * @brief  create a message queue for downloader
 *
 * @param message the downloader message
 * @return 0: if successful
 *         !0: otherwise
 */
static int http_ota_download_task(t_app_downloader_message *message)
{
    int rc = 1;
    t_manager_netwwork_info *network_info = (t_manager_netwwork_info *)message->data;

    /* download file information */
    download_file.file_name = message->file_name;
    download_file.md5 = message->md5;

    /* retry if download failed or MD5 verification failed */
    for (int i = 0; i < HTTP_DOWNLOADER_RETRY; ++i)
    {
        /* http breakpoint resume and shard download */
        if (RT_EOK == webclient_downloader(network_info->address, shard_download_handle))
        {
            if (0 == md5_verification())
            {
                rc = 0;
                rt_kprintf("http ota download successful.\n");
                break;
            }
        }
        /* try again in a moment */
        if (i < HTTP_DOWNLOADER_RETRY)
            rt_thread_mdelay(2000);
    }

    /* print result */
    if (rc != 0)
        rt_kprintf("http ota download failed.\n");

    return rc;
}

/**
 * @brief  create a message queue for downloader
 *
 * @return 0: if successful
 *         !0: otherwise
 */
int manager_create_downloader_queue(void)
{
    http_downloader_handle = manager_create_queue("http_downloader", 4, 16, UC_SYSTEM_IPC_FLAG_PRIO);
    if (RT_NULL == http_downloader_handle)
    {
        rt_kprintf("manager_create_downloader_queue() error.\n");
        return 1;
    }
    return 0;
}

/**
 * @brief  send a message to the message queue for HTTP downloader
 *
 * @param  src_task the task source
 * @param  cmd the command
 * @param  file_name the file name
 * @param  md5 the md5 of file content
 * @param  data the message
 * @return 0: if successful
 *         !0: otherwise
 */
int downloader_manager_send_page(int src_task, int cmd, char *file_name, char *md5, void *data)
{
    t_app_downloader_message *message = rt_malloc(sizeof(t_app_downloader_message));
    if (RT_NULL == message)
    {
        rt_kprintf("downloader_manager_send_page malloc error\n");
        return 1;
    }
    message->cmd = cmd;
    message->file_name = file_name;
    message->md5 = md5;
    message->data = data;
    return manager_send_page(http_downloader_handle, src_task, message);
}

/**
 * @brief  HTTP download task
 *
 * @param  params the task parameter
 */
void manager_downloader_task(void *params)
{
    t_app_manager_message *page = RT_NULL;
    t_app_downloader_message *message = RT_NULL;
    t_http_message *http_message = RT_NULL;

    while (1)
    {
        if (QUEUE_EOK != manager_recv_queue(http_downloader_handle, (void *)&page, UC_QUEUE_WAITING_FOREVER))
            continue;

        message = (t_app_downloader_message *)page->message;
        rt_kprintf("manager_downloader message->cmd 0x%x(%d)\n", message->cmd, message->cmd);

        switch (message->cmd)
        {
        case 1:
            http_message = rt_malloc(sizeof(t_http_message));
            http_message->state = http_ota_download_task(message);
            /* send http ota download result */
            to_queue_logic_data(MANAGER_OPERATION_INDENTIFICATION, MANAGER_LOGIC_FROM_HTTP_MESSAGE, (void *)http_message);
            break;
        }
        /* release memory */
        if (RT_NULL != page)
        {
            rt_free(page);
            page = RT_NULL;
        }
    }
    /* release message queue */
    if (RT_NULL == http_downloader_handle)
    {
        manager_dele_queue(http_downloader_handle);
        http_downloader_handle = RT_NULL;
    }
}

#endif // !WIOTA_APP_DEMO
