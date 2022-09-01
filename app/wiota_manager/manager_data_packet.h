/*
 * Copyright (c) 2022, Chongqing UCchip InfoTech Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @brief double linked list for temporary storage of received packets
 * @note  source_id and packet_id are used as unique identifiers
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-29     Lujun        the first version
 * 2022-07-04     Lujun        remove 'state' from data_segment
 * 2022-07-21     Lujun        adjust the order of 'node' in data_packet_node
 */

#ifndef _MANAGER_DATA_PACKET_H_
#define _MANAGER_DATA_PACKET_H_

#include <rtthread.h>

#ifdef __cplushplus
extern "C"
{
#endif

/**
 * data segment structure
 *
 */
typedef struct data_segment
{
    rt_uint16_t     length;             /**< segment length. */
    rt_uint8_t     *data;               /**< segment data. */
} data_segment_t;

/**
 * data packet structure
 *
 */
typedef struct data_packet_node
{
    rt_list_t       node;               /**< double linked list. */
    rt_uint32_t     source_id;          /**< source id. */
    rt_uint32_t     packet_id;          /**< packet id. */
    rt_uint8_t      seg_total;          /**< segment total. */
    rt_uint8_t      seg_count;          /**< segment count. */
    data_segment_t *segment;            /**< segment. */
} data_packet_t;

/**
 * @brief  initialize recv_data_packet
 *
 */
void recv_data_packet_init(void);

/**
 * @brief  whether recv_data_packet is empty
 *
 * @return 1: if empty
 *         0: otherwise
 */
int recv_data_packet_isempty(void);

/**
 * @brief  get recv_data_packet length
 *
 * @return the length of list
 */
unsigned int recv_data_packet_len(void);

/**
 * @brief  find in recv_data_packet
 *
 * @param  source_id the source id
 * @param  packet_id the packet id
 * @return the node: if successful
 *         RT_NULL: otherwise
 */
data_packet_t* recv_data_packet_find(unsigned int source_id, unsigned int packet_id);

/**
 * @brief  add an element at the end of recv_data_packet
 *
 * @param  source_id the source id
 * @param  packet_id the packet id
 * @param  seg_total the total number of segment
 * @return the node of new element
 */
data_packet_t* recv_data_packet_append(unsigned int source_id, unsigned int packet_id, unsigned char seg_total);

/**
 * @brief  add an element at the begin of recv_data_packet
 *
 * @param  source_id the source id
 * @param  packet_id the packet id
 * @param  seg_total the total number of segment
 * @return the node of new element
 */
data_packet_t* recv_data_packet_prepend(unsigned int source_id, unsigned int packet_id, unsigned char seg_total);

/**
 * @brief  add data segment to recv_data_packet
 *
 * @param  pos the node to write from recv_data_packet
 * @param  seg_id the segment id
 * @param  data the data written to recv_data_packet
 * @param  len the length of data written to recv_data_packet
 * @return 0: if successful
 *         1: otherwise
 */
int recv_data_packet_add_segment(data_packet_t *pos, unsigned char seg_id, unsigned char *data, unsigned short len);

/**
 * @brief  try to read all data from recv_data_packet
 *
 * @param  pos the node to read from recv_data_packet
 * @param  data the data read from recv_data_packet
 * @param  len the length of data read from recv_data_packet
 * @return 0: if successful
 *         1: otherwise
 * @note   call rt_free release the data by user
 */
int recv_data_packet_reader(data_packet_t *pos, unsigned char **data, unsigned int *len);

/**
 * @brief remove node from recv_data_packet
 *
 * @param pos the node to remove from recv_data_packet
 */
void recv_data_packet_remove(data_packet_t *pos);

/**
 * @brief clear recv_data_packet
 *
 */
void recv_data_packet_clear(void);

/**
 * @brief print recv_data_packet
 *
 */
void recv_data_packet_print(void);

#ifdef __cplushplus
}
#endif // !__cplushplus

#endif // !_MANAGER_DATA_PACKET_H_
