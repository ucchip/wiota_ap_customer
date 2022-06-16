/*
 * File      : at_sample_ml305.c
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

#include <at_device_ml305.h>

#define LOG_TAG                        "at.sample"
#include <at_log.h>

#define ML305_SAMPLE_DEIVCE_NAME     "ml305"

static struct at_device_ml305 sim0 =
{
    ML305_SAMPLE_DEIVCE_NAME,
    ML305_SAMPLE_CLIENT_NAME,

    -1,
    ML305_SAMPLE_POWER_PIN,
    -1,
    ML305_SAMPLE_STATUS_PIN,
    ML305_SAMPLE_RECV_BUFF_LEN,
};

static int ml305_device_register(void)
{
    struct at_device_ml305 *ml305 = &sim0;

    return at_device_register(&(ml305->device),
                              ml305->device_name,
                              ml305->client_name,
                              AT_DEVICE_CLASS_ML305,
                              (void *) ml305);
}
INIT_APP_EXPORT(ml305_device_register);
//MSH_CMD_EXPORT(ml305_device_register,ml305_device_register);
