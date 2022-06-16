#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "manager_userid.h"
#include "uc_wiota_api.h"
#include "manager_list.h"
#include "manager_module.h"

static t_manager_list *manager_access_dev_hash = RT_NULL;
static t_manager_list *manager_reserve_addr_hash;
static rt_timer_t timer_handle;

static int manager_address_query_callback(t_manager_list *node, void *target)
{
    t_manager_address *address_node = node->data;
    unsigned int *address = target;
    if (address_node->address == *address)
        return 0;
    return 1;
}

static void manager_access_func(unsigned int user_id, unsigned char group_idx, unsigned char subframe_idx)
{
    // sub_system_config_t config;
    t_manager_address *manager_address_node = rt_malloc(sizeof(t_manager_address));
    MEMORY_ASSERT(manager_address_node);
    
    //uc_wiota_get_system_config(&config);

    rt_kprintf("manager_access_func user_id 0x%x, subframe_idx %d group_idx %d\n", user_id, subframe_idx, group_idx); 
    if(RT_NULL == query_head_list(&manager_access_dev_hash[subframe_idx + group_idx * 8],  \
        (void *)&user_id, \
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
}

static void manager_drop_func(unsigned int user_id)
{
    rt_kprintf("manager_drop_func user_id 0x%x\n", user_id);
}


static void manager_replace_addr_timer_func(void *parameter)
{
    manager_start_replace_address start_replace_address_cb = parameter;
    rt_kprintf("manager_replace_addr_timer_func\n");
    if (RT_NULL != start_replace_address_cb)
        start_replace_address_cb();
}

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

        count = (1<< config.group_number) * (1 << config.dlul_ratio)*8;
        rt_kprintf("manager_address_init all count %d,  config.group_number = %d, config.dlul_ratio = %d\n", count,  config.group_number, config.dlul_ratio);
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
        timer_handle = rt_timer_create("replace_address",  manager_replace_addr_timer_func, start_replace, MANAGER_ADDR_PERIODIC_TIME, RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
        MEMORY_ASSERT(timer_handle);
        rt_timer_start(timer_handle);

        // reserved address segment is requested from server. ???
        if(1)
        {// request reserved address
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

void manager_address_exit(void)
{
     rt_timer_stop(timer_handle);
}
static  int query_element_allocate(t_manager_list *node, void *target)
{
    t_manager_address *data = node->data;
    if (data->allocate_flag == ADDRESS_RESERVED)
    {
        rt_kprintf("query_element_allocate address = 0x%x\n", data->address);
        return 0;
    }
    return 1;
}

void manager_operation_replace_address(manager_send_replace_message function)
{
     int i = 0;
     int dev_count = 0;
     int all_index = 0;
     sub_system_config_t config;
     t_dev_statistical *dev_statistical;

    MEMORY_ASSERT(function);
    
    uc_wiota_get_system_config(&config);
    all_index = (1<< config.group_number) * (1 << config.dlul_ratio)*8;
    
    rt_kprintf("config.group_number = %d,  config.dlul_ratio = %d, all_index = %d\n", \
        config.group_number,  config.dlul_ratio, all_index);

    dev_statistical = rt_malloc(sizeof(t_dev_statistical) * all_index);
    MEMORY_ASSERT(dev_statistical);
    
    // replace address statistical .
    for(; i < all_index; i++)
    {
        dev_statistical[i].dev_count = count_manager_list(&manager_access_dev_hash[i]);
        
        dev_count += dev_statistical[i].dev_count;
        rt_kprintf("dev_statistical[%d].dev_count = %d, dev_count = %d\n", i, dev_statistical[i].dev_count, dev_count);
    }

    // address strategy
    for(i = 0; i < all_index; i++)
    {
        while (dev_statistical[i].dev_count > dev_count/all_index && dev_statistical[i].dev_count > MANAGER_SAME_POSITION_MIN_NUM)
        {
            int n;
            t_manager_address *old_access_dev;
            t_manager_address *new_reserve_dev;

            for(n = 0; n < all_index; n ++)
            {
                if (dev_statistical[n].dev_count < dev_count/all_index && count_manager_list(&manager_reserve_addr_hash[n]))
                    break;
            }

            if (!(n < all_index))
                return ;
            
            old_access_dev = query_head_list(&manager_access_dev_hash[i], RT_NULL, query_element_allocate)->data;
            new_reserve_dev = query_head_list(&manager_reserve_addr_hash[n], RT_NULL, query_element_allocate)->data;
            new_reserve_dev->allocate_flag = ADDRESS_PREALLOCATED;

            // send mssage to opteration task.
            rt_kprintf("old_access_dev->address %x new_reserve_dev->address %x\n", old_access_dev->address, new_reserve_dev->address);
            function(old_access_dev->address, new_reserve_dev->address);
            dev_statistical[i].dev_count --;
        }
    }
    
    rt_free(dev_statistical);
}

static int remove_address_callback(t_manager_list *node, void *parament)
{
    t_manager_address *data = node->data;
    t_manager_address_frame *dev_info = parament;
    if (dev_info->address == data->address)
    {
        if (0 == dev_info->type)
            insert_head_manager_list(&manager_reserve_addr_hash[dev_info->pos], data);
        else
            insert_head_manager_list(&manager_access_dev_hash[dev_info->pos], data);
        return 0;
    }
    return 1;
}

int manager_replace_address_result(unsigned int old_address, unsigned int new_address)
{
    t_manager_address_frame  dev_info;
    unsigned int id_array[] = {old_address, new_address};
    uc_query_recv_t query_result;
    dev_pos_t *dev_pos;
    //sub_system_config_t config;

    //uc_wiota_get_system_config(&config);
    
    // query id of position
    if (UC_OP_SUCC != uc_wiota_query_scrambleid_by_userid(id_array, 2, RT_NULL, &query_result) )
    {
        return 1;
    }

    dev_pos = uc_wiota_get_dev_pos_by_scrambleid(query_result.scramble_id, query_result.scramble_id_num);
    
    // remove old id  from access list hash to reserve list hash
    dev_info.address = old_address;
    dev_info.pos = dev_pos[0].subframe_idx + dev_pos[0].group_idx * 8;
    dev_info.type = 0;
    remove_manager_node(&manager_access_dev_hash[dev_info.pos], &dev_info, remove_address_callback);

    // remove new id from reserve list hash to access list hash
    dev_info.address = new_address;
    dev_info.pos = dev_pos[1].subframe_idx + dev_pos[1].group_idx * 8;
    dev_info.type = 1;
    remove_manager_node(&manager_reserve_addr_hash[dev_info.pos], &dev_info, remove_address_callback);
    
    rt_free(dev_pos);
    rt_free(query_result.scramble_id);

    return 0;
}

void manager_get_reserved_address(unsigned int reserved_start_address, unsigned int reserved_end_address)
{
    int i = 0;
    dev_pos_t *dev_pos_array;
    uc_query_recv_t query_result = {0};
    int count = (reserved_end_address - reserved_start_address + 1);
    unsigned int *id_array = rt_malloc(count* 4);
    MEMORY_ASSERT(id_array);



    for(; i < count; i++)
    {
        id_array[i] = reserved_start_address + i;
    }

#if 1
    // get position
    if (UC_OP_SUCC != uc_wiota_query_scrambleid_by_userid(id_array, count,RT_NULL, &query_result))
    {
        rt_kprintf("uc_wiota_query_scrambleid_by_userid fail, count %d,id_num %d\n", count, query_result.scramble_id_num);
        rt_free(id_array);
        return ;
    }
    
    dev_pos_array = uc_wiota_get_dev_pos_by_scrambleid(query_result.scramble_id, query_result.scramble_id_num);
#else
    query_result.scramble_id = rt_malloc(count* 4);//test
    query_result.scramble_id_num = count; //test
    
    dev_pos_array = rt_malloc(count* sizeof(dev_pos_t));
    for(i = 0; i < count; i++)
    {
        query_result.scramble_id[i] = reserved_start_address + i;//test
        dev_pos_array[i].group_idx = 0;
        dev_pos_array[i].subframe_idx = 0;
    }
    
#endif

    rt_kprintf("reserved_start_address %x, count %d\n", reserved_start_address, count);
    for(i = 0; i < count; i++)
    {
        //unsigned char group_idx, unsigned char subframe_idx;
        t_manager_address *manager_address_node = rt_malloc(sizeof(t_manager_address));
        MEMORY_ASSERT(manager_address_node);
        
        // insert list
        manager_address_node->address =  id_array[i];
        manager_address_node->allocate_flag= ADDRESS_RESERVED;
        
         insert_head_manager_list(\
            &manager_reserve_addr_hash[ 1 + dev_pos_array[i].subframe_idx + dev_pos_array[i].group_idx * 8], \
            manager_address_node);
         #if 0
         insert_head_manager_list(\
            &manager_access_dev_hash[ dev_pos_array[i].subframe_idx + dev_pos_array[i].group_idx * 8], \
            manager_address_node); //del. now test
        #endif
         

         rt_kprintf("id %d pos %d\n", id_array[i], dev_pos_array[i].subframe_idx + dev_pos_array[i].group_idx * 8);
    }
    
    rt_free(id_array);
    rt_free(dev_pos_array);
    rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    rt_free(query_result.scramble_id);
    //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
}


#endif
