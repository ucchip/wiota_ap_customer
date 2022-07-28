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

#include <rtthread.h>
#include "manager_data_packet.h"

/**
 * receive data packet double linked list
 *
 */
static data_packet_t recv_data_packet = {0};

/**
 * @brief initialize recv_data_packet
 *
 */
void recv_data_packet_init(void)
{
    rt_list_init(&recv_data_packet.node);
}

/**
 * @brief whether recv_data_packet is empty
 *
 * @return 1: if empty
 *         0: otherwise
 */
int recv_data_packet_isempty(void)
{
    return rt_list_isempty(&recv_data_packet.node);
}

/**
 * @brief  get recv_data_packet length
 *
 * @return length of list
 */
unsigned int recv_data_packet_len(void)
{
    return rt_list_len(&recv_data_packet.node);
}

/**
 * @brief  find in recv_data_packet
 *
 * @param  source_id the source id
 * @param  packet_id the packet id
 * @return data packet node if successful, otherwise RT_NULL
 */
data_packet_t *recv_data_packet_find(rt_uint32_t source_id, rt_uint32_t packet_id)
{
    data_packet_t *pos = RT_NULL;
    rt_list_for_each_entry(pos, &recv_data_packet.node, node)
    {
        if (pos->source_id == source_id && pos->packet_id == packet_id)
            return pos;
    }
    return RT_NULL;
}

/**
 * @brief  add an element at the end of recv_data_packet
 *
 * @param  source_id the source id
 * @param  packet_id the packet id
 * @param  seg_total segment total
 * @return data packet node
 */
data_packet_t *recv_data_packet_append(rt_uint32_t source_id, rt_uint32_t packet_id, rt_uint8_t seg_total)
{
    data_packet_t *ele = (data_packet_t *)rt_malloc(sizeof(data_packet_t));
    RT_ASSERT(ele != RT_NULL);
    ele->source_id = source_id;
    ele->packet_id = packet_id;
    ele->seg_total = seg_total;
    ele->seg_count = 0;
    ele->segment = RT_NULL;
    /* assign data segments */
    if (ele->seg_total > 0)
    {
        ele->segment = (data_segment_t *)rt_malloc(ele->seg_total * sizeof(data_segment_t));
        RT_ASSERT(ele->segment != RT_NULL);
        rt_memset(ele->segment, 0, ele->seg_total * sizeof(data_segment_t));
    }
    /* insert an element after the list */
    rt_list_insert_after(recv_data_packet.node.prev, &ele->node);
    return ele;
}

/**
 * @brief  add an element at the begin of recv_data_packet
 *
 * @param  source_id the source id
 * @param  packet_id the packet id
 * @param  seg_total segment total
 * @return data packet node
 */
data_packet_t *recv_data_packet_prepend(rt_uint32_t source_id, rt_uint32_t packet_id, rt_uint8_t seg_total)
{
    data_packet_t *ele = (data_packet_t *)rt_malloc(sizeof(data_packet_t));
    RT_ASSERT(ele != RT_NULL);
    ele->source_id = source_id;
    ele->packet_id = packet_id;
    ele->seg_total = seg_total;
    ele->seg_count = 0;
    ele->segment = RT_NULL;
    /* assign data segments */
    if (ele->seg_total > 0)
    {
        ele->segment = (data_segment_t *)rt_malloc(ele->seg_total * sizeof(data_segment_t));
        RT_ASSERT(ele->segment != RT_NULL);
        rt_memset(ele->segment, 0, ele->seg_total * sizeof(data_segment_t));
    }
    /* insert an element before the list */
    rt_list_insert_before(recv_data_packet.node.next, &ele->node);
    return ele;
}

/**
 * @brief  add data segment to recv_data_packet
 *
 * @param  pos the node to write from recv_data_packet
 * @param  seg_id the segment id
 * @param  data data written to recv_data_packet
 * @param  len length of data written to recv_data_packet
 * @return 0: if successful
 *         1: otherwise
 */
int recv_data_packet_add_segment(data_packet_t *pos, unsigned char seg_id, rt_uint8_t *data, rt_uint16_t len)
{
    RT_ASSERT(pos != RT_NULL && pos->segment != RT_NULL);
    if (seg_id >= pos->seg_total || data == RT_NULL)
        return 1;
    /* write a new segment */
    if (pos->segment[seg_id].data == RT_NULL)
    {
        pos->segment[seg_id].data = data;
        pos->segment[seg_id].length = len;
        pos->seg_count += 1;
    }
    /* update an existing segment */
    else
    {
        rt_free(pos->segment[seg_id].data);
        pos->segment[seg_id].data = data;
        pos->segment[seg_id].length = len;
    }
    return 0;
}

/**
 * @brief  try to read all data from recv_data_packet
 * @param  pos the node to read from recv_data_packet
 * @param  data data read from recv_data_packet
 * @param  len length of data read from recv_data_packet
 * @return 0: if successful
 *         1: otherwise
 * @note   call rt_free release the data by user
 */
int recv_data_packet_reader(data_packet_t *pos, rt_uint8_t **data, rt_uint32_t *len)
{
    rt_uint32_t i, offset;
    RT_ASSERT(pos != RT_NULL && pos->segment != RT_NULL);
    /* check whether the data is complete */
    if (pos->seg_count != pos->seg_total)
        return 1;
    /* calculate data length */
    *len = 0;
    for (i = 0; i < pos->seg_total; ++i)
        *len += (rt_uint32_t)pos->segment[i].length;
    if (*len > 0)
    {
        /* allocate memory */
        *data = (rt_uint8_t *)rt_malloc(*len);
        RT_ASSERT(*data != RT_NULL);
        /* read data from segments */
        offset = 0;
        for (i = 0; i < pos->seg_total; ++i)
        {
            rt_memcpy(*data + offset, pos->segment[i].data, pos->segment[i].length);
            offset += (rt_uint32_t)pos->segment[i].length;
        }
    }
    return 0;
}

/**
 * @brief remove node from recv_data_packet
 *
 * @param pos the node to remove from recv_data_packet
 */
void recv_data_packet_remove(data_packet_t *pos)
{
    RT_ASSERT(pos != RT_NULL);
    /* remove an element from list */
    rt_list_remove(&pos->node);
    /* free data segment */
    if (pos->segment != RT_NULL)
    {
        for (int i = 0; i < pos->seg_total; ++i)
        {
            if (pos->segment[i].data != RT_NULL)
            {
                rt_free(pos->segment[i].data);
                pos->segment[i].data = RT_NULL;
            }
        }
        rt_free(pos->segment);
        pos->segment = RT_NULL;
    }
    /* free data packet */
    rt_free(pos);
    pos = RT_NULL;
}

/**
 * @brief clear recv_data_packet
 *
 */
void recv_data_packet_clear(void)
{
    data_packet_t *pos = RT_NULL;
    rt_list_t *next = RT_NULL;
    rt_list_t *curr = recv_data_packet.node.next;
    while (curr != &recv_data_packet.node)
    {
        next = curr->next;
        pos = rt_list_entry(curr, data_packet_t, node);
        /* free data segment */
        if (pos->segment != RT_NULL)
        {
            for (int i = 0; i < pos->seg_total; ++i)
            {
                if (pos->segment[i].data != RT_NULL)
                {
                    rt_free(pos->segment[i].data);
                    pos->segment[i].data = RT_NULL;
                }
            }
            rt_free(pos->segment);
            pos->segment = RT_NULL;
        }
        /* free data packet */
        rt_free(pos);
        pos = RT_NULL;
        curr = next;
    }
    /* initialize the list */
    rt_list_init(&recv_data_packet.node);
}

/**
 * @brief print recv_data_packet
 *
 */
void recv_data_packet_print(void)
{
    data_packet_t *pos = RT_NULL;
    rt_list_for_each_entry(pos, &recv_data_packet.node, node)
        rt_kprintf("element: %d, %d: %d/%d\n", pos->source_id, pos->packet_id, pos->seg_count, pos->seg_total);
}
