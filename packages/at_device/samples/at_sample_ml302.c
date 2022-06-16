/*
 * File      : at_sample_ml302.c
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
 * 2019-12-07     liang.shao     first version
 */

#include <at_device_ml302.h>

#define LOG_TAG                        "at.sample"
#include <at_log.h>

#define ML302_SAMPLE_DEIVCE_NAME     "ml302"

static struct at_device_ml302 sim0 =
{
    ML302_SAMPLE_DEIVCE_NAME,
    ML302_SAMPLE_CLIENT_NAME,

    ML302_SAMPLE_POWER_EN_PIN,
    ML302_SAMPLE_POWER_PIN,
    ML302_SAMPLE_STATUS_PIN,
    ML302_SAMPLE_RECV_BUFF_LEN,
};

static int ml302_device_register(void)
{
    struct at_device_ml302 *ml302 = &sim0;

    return at_device_register(&(ml302->device),
                              ml302->device_name,
                              ml302->client_name,
                              AT_DEVICE_CLASS_ML302,
                              (void *) ml302);
}
INIT_APP_EXPORT(ml302_device_register);
//MSH_CMD_EXPORT(ml302_device_register,ml302_device_register);
