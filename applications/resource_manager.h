/*
 * Copyright (c) 2022, Chongqing UCchip InfoTech Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @brief MQTT communication
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-01-30     Lujun        the first version
 */

#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

enum resource_manager_mode
{
    RESOURCE_DETAIL_MODE = 0,
    RESOURCE_SIMPLE_MODE,
};

#ifdef __cplushplus
extern "C"
{
#endif

/**
 * @brief resource manager
 *
 * @param mode the print mode
 */
void resource_manager(int mode);

/**
 * @brief print memory resource
 *
 */
void print_memory_resource(void);

/**
 * @brief print thread resource
 *
 */
void print_thread_resource(void);

#ifdef __cplushplus
}
#endif // __cplushplus

#endif // _RESOURCE_MANAGER_H_
