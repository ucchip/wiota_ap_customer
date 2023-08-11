/*
 * Copyright (c) 2006-2020, UCCHIP Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-26     kxiang       创建
 */
#ifndef _PTP_MUTEX_H_
#define _PTP_MUTEX_H_

#define MUTEX_WAITING_FOREVER (~0)

void *ptp_mutex_create(const char *name);
void ptp_mutex_del(void *mutex);
int32_t ptp_mutex_pend(void *pmutex, uint32_t timeout);
void ptp_mutex_post(void *pmutex);

#endif /* _MUTEX_H_ */
