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
 * 2022-08-05     Lujun        flash uses Little-Endian stroage
 */

#ifndef _HTTP_DOWNLOADER_H_
#define _HTTP_DOWNLOADER_H_

// #define HTTP_DOWNLOAD_AP_DEBUG
// #define HTTP_DOWNLOAD_IOTE_DEBUG

/* the number of retries after download failed */
#define HTTP_DOWNLOADER_RETRY      3
/* the delay milliseconds after download failed */
#define HTTP_DOWNLOADER_DELAY      2000
/* the block size of downloaded file */
#define HTTP_DOWNLOADER_BLOCK_SIZE 4096

/* reading and writing falsh must be in 4 byte */
#define FLASH_ALIGN_SIZE           4
#define FLASH_ALIGN(size)          RT_ALIGN(size, FLASH_ALIGN_SIZE)

#ifdef __cplushplus
extern "C"
{
#endif

/**
 * @brief message for downloader.
 *
 */
typedef struct app_downloader_message
{
    int            cmd;                /**< command */
    char          *file_name;          /**< file name */
    char          *md5;                /**< MD5 of file content */
    void          *data;               /**< meesage */
} t_app_downloader_message;

/**
 * @brief download file information.
 *
 */
typedef struct download_file_info
{
    char          *file_name;          /**< file name */
    int            file_size;          /**< actual file size */
    int            recv_len;           /**< received data length */
    char          *md5;                /**< MD5 of file content */
} t_download_file_info;

/**
 * @brief  create a message queue for downloader
 *
 * @return 0: if successful
 *         !0: otherwise
 */
int manager_create_downloader_queue(void);

/**
 * @brief  send a message to the message queue for HTTP downloader
 *
 * @param  src_task the task source
 * @param  cmd the command
 * @param  file_name the file name
 * @param  md5 the MD5 of file content
 * @param  data the message
 * @return 0: if successful
 *         !0: otherwise
 */
int downloader_manager_send_page(int src_task, int cmd, char *file_name, char *md5, void *data);

/**
 * @brief  HTTP download task
 *
 * @param  params the task parameter
 */
void manager_downloader_task(void *params);

#ifdef __cplushplus
}
#endif // __cplushplus

#endif // _HTTP_DOWNLOADER_H_
