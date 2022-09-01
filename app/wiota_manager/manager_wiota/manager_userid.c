#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "manager_userid.h"
#include "uc_wiota_api.h"
#include "manager_list.h"
#include "manager_module.h"
#include "manager_addrss.h"
//static t_manager_list *manager_access_dev_hash = RT_NULL;
static t_manager_list *manager_reserve_addr_hash;
static unsigned char id_reallocated_index = 0;
static rt_timer_t timer_handle;

static int manager_address_query_callback(t_manager_list *node, void *target)
{
    t_manager_address *address_node = node->data;
    unsigned int *address = target;
    if (address_node->address == *address)
        return 0;
    return 1;
}

static void manager_access_func(unsigned int user_id, unsigned char group_idx, unsigned char burst_idx, unsigned char slot_idx)
{
#if 0
    // sub_system_config_t config;
    t_manager_address *manager_address_node = rt_malloc(sizeof(t_manager_address));
    MEMORY_ASSERT(manager_address_node);

    //uc_wiota_get_system_config(&config);

    rt_kprintf("manager_access_func user_id 0x%x, subframe_idx %d group_idx %d\n", user_id, subframe_idx, group_idx);
    if (RT_NULL == query_head_list(&manager_access_dev_hash[subframe_idx + group_idx * 8],
                                   (void *)&user_id,
                                   manager_address_query_callback))
    {
        rt_kprintf("list no id 0x%x. now add list\n", user_id);
        manager_address_node->address = user_id;
        insert_head_manager_list(&manager_access_dev_hash[subframe_idx + group_idx * 8], manager_address_node);
    }
    else
    {
        rt_kprintf("list include id(0x%x) info.\n", user_id);
        rt_free(manager_address_node);
    }
#else
    rt_kprintf("manager_access_func wiota_id 0x%x, group_idx %d, subframe_idx %d, slot_idx %d\n", user_id, group_idx, burst_idx, slot_idx);
#endif
}

static void manager_drop_func(unsigned int user_id)
{
    rt_kprintf("manager_drop_func user_id 0x%x\n", user_id);
}

static void manager_replace_addr_timer_func(void *parameter)
{
}

#if 0
void manager_address_init(manager_start_replace_address start_replace, manager_start_replace_address request_reserve_address)
{
    sub_system_config_t config;
    int num = 0;
    int count = 0;

    MEMORY_ASSERT(start_replace);
    MEMORY_ASSERT(request_reserve_address);

    if (RT_NULL == manager_access_dev_hash)
    {
        // get config info.init access dev hash list
        uc_wiota_get_system_config(&config);

        count = (1 << config.group_number) * (1 << config.dlul_ratio) * 8;
        rt_kprintf("manager_address_init all count %d,  config.group_number = %d, config.dlul_ratio = %d\n", count, config.group_number, config.dlul_ratio);
        manager_access_dev_hash = rt_malloc(sizeof(t_manager_list) * count);
        MEMORY_ASSERT(manager_access_dev_hash);
        manager_reserve_addr_hash = rt_malloc(sizeof(t_manager_list) * count);
        MEMORY_ASSERT(manager_reserve_addr_hash);

        for (; num < count; num++)
        {
            init_manager_list(&manager_access_dev_hash[num]);
            init_manager_list(&manager_reserve_addr_hash[num]);
        }

        // create timer.id change periodically.
        timer_handle = rt_timer_create("replace_address", manager_replace_addr_timer_func, start_replace, MANAGER_ADDR_PERIODIC_TIME, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
        MEMORY_ASSERT(timer_handle);
        rt_timer_start(timer_handle);

        // reserved address segment is requested from server.
        if (1)
        { // request reserved address
            rt_kprintf("request_reserve_address start\n");
            request_reserve_address();
        }
        else
        { // sort reserved static address ???
        }
    }
    // wiota register access
    uc_wiota_register_iote_access_callback(manager_access_func);
    uc_wiota_register_iote_dropped_callback(manager_drop_func);
}

#else
void manager_wiotaid_start(void)
{
    if (RT_NULL == timer_handle)
    {
        // create timer.id change periodically.
        timer_handle = rt_timer_create("replace_address", manager_replace_addr_timer_func, RT_NULL, MANAGER_ADDR_PERIODIC_TIME, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
        MEMORY_ASSERT(timer_handle);
        rt_timer_start(timer_handle);
    }
    uc_wiota_register_iote_access_callback(manager_access_func);
    uc_wiota_register_iote_dropped_callback(manager_drop_func);
}
void manager_address_init(manager_start_replace_address request_reserve_address)
{
    sub_system_config_t config;
    int num = 0;
    int count = 0;

    MEMORY_ASSERT(request_reserve_address);

    if (RT_NULL == manager_reserve_addr_hash)
    {
        // get config info.init access dev hash list
        uc_wiota_get_system_config(&config);

        count = (1 << config.group_number) * (1 << config.dlul_ratio) * 8;
        rt_kprintf("manager_address_init all count %d,  config.group_number = %d, config.dlul_ratio = %d\n", count, config.group_number, config.dlul_ratio);

        manager_reserve_addr_hash = rt_malloc(sizeof(t_manager_list) * count);
        MEMORY_ASSERT(manager_reserve_addr_hash);

        for (; num < count; num++)
        {
            init_manager_list(&manager_reserve_addr_hash[num]);
        }

        // reserved address segment is requested from server.
        // request reserved address
        rt_kprintf("request_reserve_address start\n");
        request_reserve_address();
    }
}

#endif
void manager_address_exit(void)
{
    rt_timer_stop(timer_handle);
    rt_timer_delete(timer_handle);
    timer_handle = RT_NULL;
}

void manager_get_reserved_address(unsigned int reserved_start_address, unsigned int reserved_end_address)
{
    int i = 0;
    uc_dev_pos_t *dev_pos_array;
    uc_query_recv_t query_result = {0};
    int count = (reserved_end_address - reserved_start_address + 1);

    unsigned int *id_array = rt_malloc(count * 4);
    MEMORY_ASSERT(id_array);

    rt_kprintf("%s line %d %d~%d\n", __FUNCTION__, __LINE__, reserved_start_address, reserved_end_address);

    for (; i < count; i++)
    {
        id_array[i] = reserved_start_address + i;
    }
    rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
#if 1
    // get position
    if (UC_OP_SUCC != uc_wiota_query_scrambleid_by_userid(id_array, count, RT_NULL, &query_result))
    {
        rt_kprintf("uc_wiota_query_scrambleid_by_userid fail, count %d,id_num %d\n", count, query_result.scramble_id_num);
        rt_free(id_array);
        return;
    }

    dev_pos_array = uc_wiota_get_dev_pos_by_scrambleid(query_result.scramble_id, query_result.scramble_id_num);
#else
    query_result.scramble_id = rt_malloc(count * 4); //test
    query_result.scramble_id_num = count;            //test

    dev_pos_array = rt_malloc(count * sizeof(uc_dev_pos_t));
    for (i = 0; i < count; i++)
    {
        query_result.scramble_id[i] = reserved_start_address + i; //test
        dev_pos_array[i].group_idx = 0;
        dev_pos_array[i].burst_idx = 0;
    }

#endif

    rt_kprintf("reserved_start_address %x, count %d\n", reserved_start_address, count);
    for (i = 0; i < count; i++)
    {
        //unsigned char group_idx, unsigned char burst_idx;
        t_manager_address *manager_address_node = rt_malloc(sizeof(t_manager_address));
        MEMORY_ASSERT(manager_address_node);
        rt_memset(manager_address_node, 0, sizeof(t_manager_address));

        rt_kprintf("manager_address_node 0x%x id 0x%x pos %d\n", manager_address_node, id_array[i], dev_pos_array[i].burst_idx + dev_pos_array[i].group_idx * 8);

        // insert list
        manager_address_node->address = id_array[i];
        manager_address_node->allocate_flag = ADDRESS_RESERVED;

        insert_head_manager_list(
            &manager_reserve_addr_hash[dev_pos_array[i].burst_idx + dev_pos_array[i].group_idx * 8],
            manager_address_node);
#if 0
         insert_head_manager_list(\
            &manager_access_dev_hash[ dev_pos_array[i].burst_idx + dev_pos_array[i].group_idx * 8], \
            manager_address_node); //del. now test
#endif
    }

    rt_free(id_array);
    rt_free(dev_pos_array);
    rt_free(query_result.scramble_id);
    //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
}

static int query_element_allocate(t_manager_list *node, void *target)
{
    unsigned int *mac = target;
    t_manager_address *data = node->data;
    if (data->allocate_flag == ADDRESS_PREALLOCATED &&
        *mac == data->dev_mac)
    {
        rt_kprintf("query_element_allocate address = 0x%x, mac = 0x%x\n", data->address, data->dev_mac);
        return 0;
    }
    return 1;
}
static int counter_element_allocate(t_manager_list *node, void *target)
{
    t_manager_address *data = node->data;
    if (data->allocate_flag == ADDRESS_PREALLOCATED)
    {
        rt_kprintf("counter_element_allocate address = 0x%x, mac = 0x%x\n", data->address, data->dev_mac);
        return 0;
    }
    return 1;
}

static int counter_element_reserved(t_manager_list *node, void *target)
{
    t_manager_address *data = node->data;
    if (data->allocate_flag == ADDRESS_RESERVED)
    {
        rt_kprintf("counter_element_allocate address = 0x%x, mac = 0x%x\n", data->address, data->dev_mac);
        return 0;
    }
    return 1;
}

void manager_reallocated_wiotaid(unsigned int dev_address, unsigned int wiota_address, manager_send_replace_message function)
{
    int i;
    int all_index = 0;
    sub_system_config_t config;

    MEMORY_ASSERT(function);

    uc_wiota_get_system_config(&config);
    all_index = (1 << config.group_number) * (1 << config.dlul_ratio) * 8;

    // query whether an allocation already exists.
    for (i = 0; i < all_index; i++)
    {
        t_manager_list *node = query_head_list(&manager_reserve_addr_hash[i], &dev_address, query_element_allocate);
        if (RT_NULL != node)
        {
            t_manager_address *reserve_data = node->data;
            reserve_data->current_com_count++;
            rt_kprintf("the dev address 0x%x existing\n", dev_address);
            function(AP_DEFAULT_ADDRESS, wiota_address, reserve_data->address);
            return;
        }
    }

#if 1
    for (i = 0; i < all_index; i++)
    {
        if (count_the_manager_list(&manager_reserve_addr_hash[id_reallocated_index], RT_NULL, counter_element_reserved))
        {
            t_manager_address *reserve_addr;
            // query the node
            t_manager_list *op_node = query_head_list(&manager_reserve_addr_hash[id_reallocated_index], RT_NULL, counter_element_reserved);
            if (RT_NULL == op_node)
            {
                rt_kprintf("manager_distribution_wiotaid query_head_list error\n");
                continue;
            }

            reserve_addr = op_node->data;
            reserve_addr->allocate_flag = ADDRESS_PREALLOCATED;
            reserve_addr->current_com_count++;
            reserve_addr->dev_mac = dev_address;
            rt_kprintf("the dev address 0x%x  -> 0x%x\n", dev_address, reserve_addr->address);
            // response reassigns id.
            function(AP_DEFAULT_ADDRESS, wiota_address, reserve_addr->address);

            id_reallocated_index++;
            if (id_reallocated_index > all_index - 1)
                id_reallocated_index = 0;
            break;
        }
    }

#else
    int n = 0;
    int dev_count = 0;
    t_dev_statistical *dev_statistical;
    dev_statistical = rt_malloc(sizeof(t_dev_statistical) * all_index);
    MEMORY_ASSERT(dev_statistical);

    // query the resources that can be allocated base on the balancing policy.
    for (; i < all_index; i++)
    {
        dev_statistical[i].dev_count = count_the_manager_list(&manager_reserve_addr_hash[i], RT_NULL, counter_element_allocate);

        dev_count += dev_statistical[i].dev_count;
        rt_kprintf("dev_statistical[%d].dev_count = %d, dev_count = %d\n", i, dev_statistical[i].dev_count, dev_count);
    }

    // query replacement location.
    for (n = 0; n < all_index; n++)
    {
        rt_kprintf("manager_reserve_addr_hash[%d] = %d\n", n, count_manager_list(&manager_reserve_addr_hash[n]));
        rt_kprintf("dev_statistical[%d].dev_count = %d\n", n, dev_statistical[n].dev_count);
        // ??????????
        if (dev_statistical[n].dev_count < dev_count / all_index && count_the_manager_list(&manager_reserve_addr_hash[n], RT_NULL, counter_element_reserved))
        {
            t_manager_address *reserve_addr;
            // query the node
            t_manager_list *op_node = query_head_list(&manager_reserve_addr_hash[n], RT_NULL, counter_element_reserved);
            if (RT_NULL == op_node)
            {
                rt_kprintf("manager_distribution_wiotaid query_head_list error\n");
                continue;
            }

            reserve_addr = op_node->data;
            reserve_addr->allocate_flag = ADDRESS_PREALLOCATED;
            reserve_addr->current_com_count++;
            reserve_addr->dev_mac = mac;

            // response reassigns id.
            function(mac, reserve_addr->address);
            break;
        }
    }
distribution_wiotaid_out:
    rt_free(dev_statistical);
#endif
}

unsigned int manager_query_wiotaid(unsigned int mac)
{
    int all_index = 0;
    sub_system_config_t config;
    int i = 0;

    uc_wiota_get_system_config(&config);
    all_index = (1 << config.group_number) * (1 << config.dlul_ratio) * 8;

    for (; i < all_index; i++)
    {
        t_manager_list *node = query_head_list(&manager_reserve_addr_hash[i], &mac, query_element_allocate);
        if (RT_NULL != node)
        {
            t_manager_address *reserve_data = node->data;
            rt_kprintf("manager query id 0x%x->0x%x(dev id->wiota id)\n", mac,  reserve_data->address);
            return reserve_data->address;
        }
    }
    rt_kprintf("manager query wiotaid 0x%x is error\n", mac);
    return mac;
}
#endif
