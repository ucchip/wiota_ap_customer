#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>
#include "manager_list.h"
#include "manager_module.h"

void init_manager_list(t_manager_list *freq_list)
{
    freq_list->next = freq_list;
    freq_list->pre = freq_list;
}

int count_manager_list(t_manager_list *freq_list)
{
    int count = 0;
    t_manager_list *op_node = freq_list->next;
    
    while(op_node != freq_list)
    {
        count ++;
        op_node = op_node->next;
    }

    return count;
}

void insert_head_manager_list(t_manager_list *freq_list, void *data)
{
    t_manager_list *node = rt_malloc(sizeof(t_manager_list));
    if (RT_NULL == node)
        return;

    //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

    node->data = data;

    if (freq_list->next == freq_list)
    {
        freq_list->next = node;
        freq_list->pre = node;
        node->next = freq_list;
        node->pre = freq_list;
        //rt_kprintf("%s line %d node 0x%x, freq_list->next 0x%x\n", __FUNCTION__, __LINE__, node, freq_list->next);
    }
    else
    {
        node->next = freq_list->next;
        node->pre = freq_list;
        freq_list->next->pre = node;
        freq_list->next = node;
        //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    }
}

void insert_tail_manager_list(t_manager_list *freq_list, void *data)
{
    t_manager_list *node = rt_malloc(sizeof(t_manager_list));
    MEMORY_ASSERT(node);

    //rt_kprintf("%s line %d node 0x%x\n", __FUNCTION__, __LINE__, node);

    node->data = data;

    if (freq_list->next == freq_list)
    {
        freq_list->next = node;
        freq_list->pre = node;
        node->next = freq_list;
        node->pre = freq_list;
        //rt_kprintf("%s line %d list current is null\n", __FUNCTION__, __LINE__);
    }
    else
    {
        t_manager_list *old_tail = freq_list->pre;

        old_tail->next = node;
        node->pre = old_tail;
        //old_tail = node;
        node->next = freq_list;
        freq_list->pre = node;
        
         //rt_kprintf("%s line %d list current no null old_tail 0x%x\n", __FUNCTION__, __LINE__, old_tail);
    }
}

t_manager_list *get_head_list(t_manager_list *freq_list)
{
    t_manager_list *op_node = freq_list->next;

    if (op_node != freq_list)
        return op_node;
    
    return RT_NULL;
}

t_manager_list *query_head_list(t_manager_list *freq_list, void *target, query_element_callback cb)
{
    t_manager_list *op_node = freq_list->next;

    while (op_node != freq_list)
    {
        if (RT_NULL != cb && 0 == cb(op_node, target))
            return op_node;
        op_node = op_node->next;
    }

    return RT_NULL;
}

int remove_head_manager_node(t_manager_list *freq_list, void *parament)
{
    t_manager_list *op_node = freq_list->next;

    while (op_node != freq_list)
    {
        if (parament == op_node)
        {
            // remove from original position
            op_node->next->pre = op_node->pre;
            op_node->pre->next = op_node->next;
            // add to head
            insert_head_manager_list(freq_list, op_node->data);
            rt_free(op_node);
            return 0;
        }
        op_node = op_node->next;
    }
    return 1;
}

int del_manager_node(t_manager_list *freq_list, void *parament, del_element_callback cb)
{
    t_manager_list *op_node = freq_list->next;

    while (op_node != freq_list)
    {
        if (0 == cb(op_node, parament))
        {
            op_node->pre->next = op_node->next;
            op_node->next->pre = op_node->pre;
            rt_free(op_node->data);
            rt_free(op_node);
            op_node = RT_NULL;
            return 0;
        }
        op_node = op_node->next;
    }
    return 1;
}

int remove_manager_node(t_manager_list *freq_list, void *parament, del_element_callback cb)
{
    t_manager_list *op_node = freq_list->next;

    while (op_node != freq_list)
    {
        if (0 == cb(op_node, parament))
        {
            op_node->pre->next = op_node->next;
            op_node->next->pre = op_node->pre;
            rt_free(op_node);
            op_node = RT_NULL;
            return 0;
        }
        op_node = op_node->next;
    }
    return 1;
}


int modify_manager_node(t_manager_list *freq_list, void *parament, modify_element_callback cb)
{
    t_manager_list *op_node = freq_list->next;

    while (op_node != freq_list)
    {
        if (RT_NULL != cb && 0 == cb(parament, op_node))
            return 0;
        op_node = op_node->next;
    }

    return 1;
}

void clean_manager_list(t_manager_list *freq_list)
{
    t_manager_list *op_node = freq_list->next;
    while (op_node != freq_list)
    {
        t_manager_list *tmp = op_node;
        op_node = op_node->next;
        rt_free(tmp->data);
        rt_free(tmp);
        tmp = RT_NULL;
    }
    freq_list->next = freq_list;
    freq_list->pre = freq_list;
}

void test_head_list(t_manager_list *freq_list, void *target, test_element_callback cb)
{
    t_manager_list *op_node = freq_list->next;
    while (op_node != freq_list)
    {
        cb(op_node, target);
        op_node = op_node->next;
    }
}

#endif
