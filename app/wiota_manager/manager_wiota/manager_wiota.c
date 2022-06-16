#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "manager_wiota.h"

enum wiota_manager_process
{
    WIOTA_MANAGER_PROCESS_DEFAULT = 0,
    WIOTA_MANAGER_PROCESS_SCAN = 1,
    WIOTA_MANAGER_PROCESS_INIT = 2,
    WIOTA_MANAGER_PROCESS_RUN = 3,
    WIOTA_MANAGER_PROCESS_STRATEGY = 4,
    WIOTA_MANAGER_PROCESS_EXIT = 5,
    WIOTA_MANAGER_PROCESS_END
};

enum wiota_manager_report_state
{
    WIOTA_MANAGER_REPORT_FREQ_SUC = 0,
    WIOTA_MANAGER_REPORT_FREQ_FAIL = 1,
    WIOTA_MANAGER_REPORT_INIT = 2,
    WIOTA_MANAGER_REPORT_USER_FREQ = 3,
    WIOTA_MANAGER_REPORT_RUN = 4,
    WIOTA_MANAGER_REPORT_EXIT = 5
};

typedef struct freq_list_manager_node
{
    unsigned char freq;
    signed char snr;
    signed char rssi;
    unsigned char is_synced;
    char send_cucess_rate;
} t_freq_node_manager;

typedef struct freq_list_manager
{
    t_freq_node_manager node;
    struct freq_list_manager *next;
    struct freq_list_manager *pre;
} t_freq_list_manager;

typedef struct at_wiota_manager_parament
{
    //rt_thread_t task_handle;
    t_freq_list_manager freq_list;
    t_freq_list_manager *current_freq_node;
    int continue_scan_fail;
    char manager_state;
    char wiota_state;
} t_wiota_manager_freq;

#define WIOTA_SCAN_TIMEOUT 40000

#define SET_WIOTA_MANAGER_PROCESS(state) g_wiota_manager_freq.manager_state = state
#define GET_WIOTA_MANAGER_PROCESS g_wiota_manager_freq.manager_state

t_wiota_manager_freq g_wiota_manager_freq = {0};

static int manager_wiota_get_static_freq(char *list)
{
    int num;
    uc_wiota_get_freq_list((unsigned char *)list);

    for (num = 0; num < 16; num++)
    {
        // rt_kprintf("static freq *(list+%d) %d\n", num, *(list+num));
        if (*(list + num) == 0xFF)
            break;
    }
    return num;
}

static void manager_wiota_freq_list_init(t_freq_list_manager *freq_list)
{
    if (freq_list != RT_NULL && RT_NULL == freq_list->next && RT_NULL == freq_list->pre)
    {
        freq_list->next = freq_list;
        freq_list->pre = freq_list;
    }
}
static void manager_wiota_print_freq_list(t_freq_list_manager *list)
{
    t_freq_list_manager *tmp = list->next;

    while (tmp != list)
    {
        rt_kprintf("address 0x%x freq %d snr %d rssi %d is_synced %d\n", tmp, tmp->node.freq, tmp->node.snr, tmp->node.rssi, tmp->node.is_synced);
        tmp = tmp->next;
    }
}
static void manager_wiota_add_freq_list(t_freq_list_manager *freq_list, t_freq_list_manager *node)
{
    //t_freq_list_manager *tmp = freq_list->next;

    rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

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

static void manager_wiota_clean_freq_list(t_freq_list_manager *freq_list)
{
    t_freq_list_manager *op_node = freq_list->next;
    while (op_node != freq_list)
    {
        t_freq_list_manager *tmp = op_node;
        op_node = op_node->next;
        rt_free(tmp);
        tmp = RT_NULL;
    }
    freq_list->next = freq_list;
    freq_list->pre = freq_list;
}

static int manager_wiota_des_sort_freq(t_freq_list_manager *list)
{
    t_freq_list_manager *head = list;
    t_freq_list_manager *compare = head->next;

    if (compare == head)
    {
        rt_kprintf("%s line %d list is null\n", __FUNCTION__, __LINE__);
        return 1;
    }

    while (compare != head)
    {
        t_freq_list_manager *tmp = compare->next;
        t_freq_list_manager *get_min = compare;

        //rt_kprintf("%s line %d head 0x%x compare 0x%x tmp 0x%x\n", __FUNCTION__, __LINE__, head, compare, tmp);
        while (tmp != head)
        {
            if (get_min->node.rssi > tmp->node.rssi)
                get_min = tmp;
            tmp = tmp->next;
        }
        // rt_kprintf("%s line %d get_min 0x%x\n", __FUNCTION__, __LINE__, get_min);

        //rt_kprintf("%s line %d compare 0x%x\n", __FUNCTION__, __LINE__, compare);

        if (compare != get_min && compare->next != get_min)
        {
            t_freq_list_manager *get_min_pre_tmp = get_min->pre;
            t_freq_list_manager *get_min_next_tmp = get_min->next;

            //rt_kprintf("%s line %d back 0x%x next 0x%x\n", __FUNCTION__, __LINE__, get_min , get_min->next);
            compare->pre->next = get_min;
            compare->next->pre = get_min;
            get_min->next->pre = compare;
            get_min->pre->next = compare;

            get_min->pre = compare->pre;
            get_min->next = compare->next;
            compare->pre = get_min_pre_tmp;
            compare->next = get_min_next_tmp;

            //rt_kprintf("%s line %d get_max 0x%x next 0x%x pre 0x%x\n", __FUNCTION__, __LINE__, get_min , get_min->next, get_min->pre);
            //rt_kprintf("%s line %d compare 0x%x next 0x%x pre 0x%x\n", __FUNCTION__, __LINE__, compare , compare->next, compare->pre);
            compare = get_min->next;
        }
        else if (compare != get_min)
        {

            compare->pre->next = get_min;
            get_min->next->pre = compare;

            compare->next = get_min->next;
            get_min->pre = compare->pre;
            compare->pre = get_min;
            get_min->next = compare;

            //rt_kprintf("%s line %d get_min 0x%x next 0x%x pre 0x%x\n", __FUNCTION__, __LINE__, get_min , get_min->next, get_min->pre);
            //rt_kprintf("%s line %d compare 0x%x next 0x%x pre 0x%x\n", __FUNCTION__, __LINE__, compare , compare->next, compare->pre);
        }
        else
            compare = compare->next;
    }

    return 0;
}

static int manager_wiota_choose_freq(t_wiota_manager_freq *manager)
{
    t_freq_list_manager *head = &(manager->freq_list);
    t_freq_list_manager *tmp = head->next;

    rt_kprintf("%s line %d tmp 0x%x\n", __FUNCTION__, __LINE__, tmp);

    if (tmp == head)
    {
        rt_kprintf("%s line %d list is null\n", __FUNCTION__, __LINE__);
        return 1;
    }
    if (manager->current_freq_node == RT_NULL)
    {
        manager->current_freq_node = tmp;
    }
    else
    {
        manager->current_freq_node = manager->current_freq_node->next;
        if (manager->current_freq_node == head)
        {
            manager->current_freq_node = RT_NULL;
            return 2;
        }
    }

    return 0;
}

static int manager_wiota_only_freq(char freq, t_wiota_manager_freq *manager)
{
    t_freq_list_manager *data = rt_malloc(sizeof(t_freq_list_manager));
    if (data == RT_NULL)
    {
        rt_kprintf("%s line %d malloc error\n", __FUNCTION__, __LINE__);
        return 1;
    }
    rt_memset(data, 0, sizeof(t_freq_list_manager));
    data->node.freq = freq;
    manager_wiota_add_freq_list(&(manager->freq_list), data);
    return 0;
}

static int manager_wiota_freq_manager(uc_scan_recv_t result, t_wiota_manager_freq *manager)
{
    int re = 1;

    if (UC_OP_SUCC == result.result)
    {
        uc_scan_freq_t *freq_list = (uc_scan_freq_t *)result.data;
        int freq_num = result.data_len / sizeof(uc_scan_freq_t);
        int i = 0;

        for (i = 0; i < freq_num; i++)
        {
            rt_kprintf("%s line %d freq_num %d i %d index %d is_synced %d snr %d rssi %d\n", __FUNCTION__, __LINE__,
                       freq_num, i, freq_list->freq_idx, freq_list->is_synced, freq_list->snr, freq_list->rssi);
            if (freq_list->is_synced == 0)
            {
                t_freq_list_manager *data = rt_malloc(sizeof(t_freq_list_manager));
                if (data == RT_NULL)
                {
                    rt_kprintf("%s line %d malloc error\n", __FUNCTION__, __LINE__);
                    return 1;
                }
                data->node.freq = freq_list->freq_idx;
                data->node.is_synced = freq_list->is_synced;
                data->node.rssi = freq_list->rssi;
                data->node.snr = freq_list->snr;
                data->node.send_cucess_rate = 0;
                manager_wiota_add_freq_list(&(manager->freq_list), data);
                re = 0;
            }
            freq_list++;
        }

        if (!re)
        {
            manager_wiota_des_sort_freq(&(manager->freq_list));
            manager_wiota_print_freq_list(&(manager->freq_list));
        }

        rt_free(result.data);
    }

    return re;
}

static int manager_wiota_scan_freq(t_wiota_manager_freq *manager)
{
    uc_scan_recv_t result;
    u8_t list[16] = {0};
    int list_len = 0;
    int res;

    uc_wiota_init();
    uc_wiota_run();

    list_len = manager_wiota_get_static_freq((char *)list);
    rt_kprintf("%s line %d list_len %d\n", __FUNCTION__, __LINE__, list_len);

    switch (list_len)
    {
    case 0:
    {
        uc_wiota_scan_freq(RT_NULL, 0, -1, RT_NULL, &result);
        rt_kprintf("%s line %d uc_wiota_scan_freq result %d\n", __FUNCTION__, __LINE__, result.result);
        res = manager_wiota_freq_manager(result, manager);
        break;
    }
    case 1:
    {
        res = manager_wiota_only_freq(list[0], manager);
        rt_kprintf("%s line %d res = %d\n", __FUNCTION__, __LINE__, res);
        break;
    }
    default:
    {
        uc_wiota_scan_freq(list, list_len, WIOTA_SCAN_TIMEOUT, RT_NULL, &result);
        rt_kprintf("%s line %d uc_wiota_scan_freq result %d\n", __FUNCTION__, __LINE__, result.result);
        res = manager_wiota_freq_manager(result, manager);
        break;
    }
    }
    uc_wiota_exit();

    return res;
}

static void manager_wiota_startegy(void)
{
    uc_wiota_reset_all_state_info();
    while (1)
    {
        uc_state_info_t *p_node = uc_wiota_get_all_state_info();

        while (p_node != RT_NULL)
        {
            rt_kprintf("user_id 0x%x, ul_recv_len %d, ul_recv_suc %d, dl_send_len %d, dl_send_suc %d, dl_send_fail %d\n",
                       p_node->user_id, p_node->ul_recv_len, p_node->ul_recv_suc,
                       p_node->dl_send_len, p_node->dl_send_suc, p_node->dl_send_fail);
            p_node = p_node->next;
        }
        uc_wiota_reset_all_state_info();
        rt_thread_mdelay(5000);
    }
}

static void manager_wiota_report_state(int type, t_wiota_manager_freq *manager)
{
    rt_kprintf("%s type = %d\n", __FUNCTION__, type);
    switch (type)
    {
    case WIOTA_MANAGER_REPORT_FREQ_SUC:
    {
        t_freq_list_manager *head = &(manager->freq_list);
        t_freq_list_manager *tmp = head->next;

        while (tmp != head)
        {
            rt_kprintf("+SCANFFREQ %d,%d,%d,%d\n", tmp->node.freq, tmp->node.snr, tmp->node.rssi, tmp->node.is_synced);
            tmp = tmp->next;
        }
        break;
    }
    case WIOTA_MANAGER_REPORT_FREQ_FAIL:
    {
        rt_kprintf("+SCANFFREQ:FAIL\n");
        break;
    }
    case WIOTA_MANAGER_PROCESS_INIT:
    {
        rt_kprintf("+WIOTA:INIT\n");
        break;
    }
    case WIOTA_MANAGER_REPORT_USER_FREQ:
    {
        if (manager->current_freq_node != RT_NULL)
            rt_kprintf("+WIOTA:FREQ %d,%d,%d\n", manager->current_freq_node->node.freq, manager->current_freq_node->node.rssi, manager->current_freq_node->node.snr);
        break;
    }
    case WIOTA_MANAGER_REPORT_RUN:
    {
        rt_kprintf("+WIOTA:RUN\n");
        break;
    }
    case WIOTA_MANAGER_REPORT_EXIT:
    {
        rt_kprintf("+WIOTA:EXIT\n");
        break;
    }
    }
}

int manager_wiota(void)
{
    manager_wiota_freq_list_init(&g_wiota_manager_freq.freq_list);

    SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_SCAN);

    while (WIOTA_MANAGER_PROCESS_STRATEGY != GET_WIOTA_MANAGER_PROCESS)
    {
        rt_kprintf("%s line %d state %d\n", __FUNCTION__, __LINE__, GET_WIOTA_MANAGER_PROCESS);
        switch (GET_WIOTA_MANAGER_PROCESS)
        {
        case WIOTA_MANAGER_PROCESS_SCAN:
        {
            if (manager_wiota_scan_freq(&g_wiota_manager_freq))
            {
                manager_wiota_report_state(WIOTA_MANAGER_REPORT_FREQ_FAIL, &g_wiota_manager_freq);
                g_wiota_manager_freq.continue_scan_fail++;
                rt_thread_mdelay(1000 * g_wiota_manager_freq.continue_scan_fail);
                SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_SCAN);
            }
            else
            {
                rt_kprintf("%s line head 0x%x tmp 0x%x next node 0x%x\n", __FUNCTION__, __LINE__, &g_wiota_manager_freq.freq_list, g_wiota_manager_freq.freq_list.next);
                manager_wiota_report_state(WIOTA_MANAGER_REPORT_FREQ_SUC, &g_wiota_manager_freq);
                SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_INIT);
            }

            break;
        }
        case WIOTA_MANAGER_PROCESS_INIT:
        {
            if (manager_wiota_choose_freq(&g_wiota_manager_freq))
            {
                manager_wiota_clean_freq_list(&g_wiota_manager_freq.freq_list);
                SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_SCAN);
            }
            else
            {
                uc_wiota_init();
                manager_wiota_report_state(WIOTA_MANAGER_REPORT_INIT, RT_NULL);
                rt_kprintf("%s line %d freq %d\n", __FUNCTION__, __LINE__, g_wiota_manager_freq.current_freq_node->node.freq);
                uc_wiota_set_freq_info(g_wiota_manager_freq.current_freq_node->node.freq);
                manager_wiota_report_state(WIOTA_MANAGER_REPORT_USER_FREQ, &g_wiota_manager_freq);
                SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_RUN);
            }
            break;
        }
        case WIOTA_MANAGER_PROCESS_RUN:
        {
            uc_wiota_run();
            manager_wiota_report_state(WIOTA_MANAGER_REPORT_RUN, RT_NULL);
            g_wiota_manager_freq.continue_scan_fail = 0;
            SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_STRATEGY);
            break;
        }
        case WIOTA_MANAGER_PROCESS_STRATEGY:
        {
            manager_wiota_startegy();
            SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_EXIT);
            break;
        }

        case WIOTA_MANAGER_PROCESS_EXIT:
        {
            uc_wiota_exit();
            manager_wiota_report_state(WIOTA_MANAGER_REPORT_EXIT, RT_NULL);
            SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_INIT);
            break;
        }
        }
    }

    return 0;
}


void manager_wiota_exit(void)
{
    uc_wiota_exit();
}
#endif
