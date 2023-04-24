/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-26     chenyong     first version
 */

#ifndef __WIZ_H__
#define __WIZ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>
#include "wizchip_conf.h"

#define WIZ_SW_VERSION       "2.1.0"
#define WIZ_SW_VERSION_NUM   0x020100

#ifndef WIZ_SOCKETS_NUM
#define WIZ_SOCKETS_NUM      8
#endif

#ifndef WIZ_RX_MBOX_NUM
#define WIZ_RX_MBOX_NUM      10
#endif

/* WIZnet config */
int wiz_config(char *mac, unsigned short static_ip, char *ip, char *mask, char *gateway);

/* WIZnet initialize device and network */
int wiz_init(void);

uint32_t wiz_is_link_up(void);

uint32_t wiz_ip_is_assign(void);

void wiz_get_net_info(wiz_NetInfo *net_info);


#ifdef __cplusplus
}
#endif

#endif /* __WIZ_H__ */
