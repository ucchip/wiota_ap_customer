/*
 * File      : at_device_ml305.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-16     chenyong     first version
 */

#ifndef __AT_DEVICE_ML305_H__
#define __AT_DEVICE_ML305_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include <at_device.h>

/* The maximum number of sockets supported by the ml305 device */
#define AT_DEVICE_ML305_SOCKETS_NUM      6

struct at_device_ml305
{
    char *device_name;
    char *client_name;

    int power_en_pin;
    int power_pin;
    int power_rst_pin;
    int power_status_pin;
    size_t recv_line_num;
    struct at_device device;

    void *user_data;
};

struct at_device_ml305_module_info
{
    char model[32];       //型号
    char firmware[32];    //软件版本
    char imei[32];        //imei
    char imsi[32];        //imsi
    char iccid[32];       //sim卡ID
};

#ifdef AT_USING_SOCKET

/* ml305 device socket initialize */
int ml305_socket_init(struct at_device *device);

/* ml305 device class socket register */
int ml305_socket_class_register(struct at_device_class *class);

#endif /* AT_USING_SOCKET */

#ifdef __cplusplus
}
#endif

#endif /* __AT_DEVICE_ML305_H__ */
