#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "manager_wiota_freq.h"

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
    t_freq_node_manager data;
    rt_list_t node;
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

static void manager_wiota_freq_list_init(void)
{
    rt_memset(&g_wiota_manager_freq, 0, sizeof(t_wiota_manager_freq));
    rt_list_init(&g_wiota_manager_freq.freq_list.node);
}

static void manager_wiota_print_freq_list(void)
{
    t_freq_list_manager *temp_node;

    rt_list_for_each_entry(temp_node, &g_wiota_manager_freq.freq_list.node, node)
    {
        rt_kprintf("address 0x%x freq %d snr %d rssi %d is_synced %d\n",
                   temp_node, temp_node->data.freq, temp_node->data.snr, temp_node->data.rssi, temp_node->data.is_synced);
    }
}

static t_freq_list_manager *manager_wiota_find_node(unsigned char freq)
{
    t_freq_list_manager *temp_node;

    rt_list_for_each_entry(temp_node, &g_wiota_manager_freq.freq_list.node, node)
    {
        if (temp_node->data.freq == freq)
        {
            return temp_node;
        }
    }
    return RT_NULL;
}

static void manager_wiota_add_freq_list(uc_scan_freq_t *node)
{
    t_freq_list_manager *temp = manager_wiota_find_node(node->freq_idx);
    if (temp)
    {
        temp->data.snr = node->snr;
        temp->data.rssi = node->rssi;
        temp->data.is_synced = node->is_synced;
        temp->data.send_cucess_rate = 0;
    }
    else
    {
        t_freq_list_manager *new_node = rt_malloc(sizeof(t_freq_list_manager));
        if (new_node == RT_NULL)
        {
            rt_kprintf("%s line %d malloc error\n", __FUNCTION__, __LINE__);
            return;
        }
        new_node->data.freq = node->freq_idx;
        new_node->data.is_synced = node->is_synced;
        new_node->data.rssi = node->rssi;
        new_node->data.snr = node->snr;
        new_node->data.send_cucess_rate = 0;

        t_freq_list_manager *temp_node1;

        rt_list_for_each_entry(temp_node1, &g_wiota_manager_freq.freq_list.node, node)
        {
            if (temp_node1->data.rssi > new_node->data.rssi)
            {
                rt_list_insert_before(&temp_node1->node, &new_node->node);
                return;
            }
        }

        rt_list_insert_after(g_wiota_manager_freq.freq_list.node.prev, &new_node->node);
    }
}

static void manager_wiota_remove_freq_list(uc_scan_freq_t *node)
{
    t_freq_list_manager *temp = manager_wiota_find_node(node->freq_idx);
    if (temp)
    {
        rt_list_remove(&temp->node);
        rt_free(temp);
        temp = NULL;
    }
}

static void manager_wiota_clean_freq_list(void)
{
    rt_list_t *next_node = g_wiota_manager_freq.freq_list.node.next;
    while (next_node != &g_wiota_manager_freq.freq_list.node)
    {
        t_freq_list_manager *temp_node = rt_list_entry(next_node, t_freq_list_manager, node);
        rt_free(temp_node);
        temp_node = RT_NULL;
        next_node = next_node->next;
    }
    manager_wiota_freq_list_init();
}

static int manager_wiota_choose_freq(void)
{
    rt_list_t *head = &(g_wiota_manager_freq.freq_list.node);
    rt_list_t *tmp = head->next;

    rt_kprintf("%s line %d tmp 0x%x\n", __FUNCTION__, __LINE__, tmp);

    if (tmp == head)
    {
        rt_kprintf("%s line %d list is null\n", __FUNCTION__, __LINE__);
        return 1;
    }
    if (g_wiota_manager_freq.current_freq_node == RT_NULL)
    {
        g_wiota_manager_freq.current_freq_node = rt_list_entry(tmp, t_freq_list_manager, node);
    }
    else
    {
        tmp = g_wiota_manager_freq.current_freq_node->node.next;
        if (tmp == head)
        {
            g_wiota_manager_freq.current_freq_node = RT_NULL;
            return 2;
        }
        g_wiota_manager_freq.current_freq_node = rt_list_entry(tmp, t_freq_list_manager, node);
    }

    return 0;
}

static int manager_wiota_only_freq(char freq)
{
    uc_scan_freq_t freq_list = {0};

    freq_list.freq_idx = freq;
    manager_wiota_add_freq_list(&freq_list);
    return 0;
}

static int manager_wiota_freq_manager(uc_scan_recv_t result)
{
    int re = 1;

    if (UC_OP_SUCC == result.result)
    {
        uc_scan_freq_t *freq_list = (uc_scan_freq_t *)result.data;
        int freq_num = result.data_len / sizeof(uc_scan_freq_t);
        int i = 0;

        for (i = 0; i < freq_num; i++)
        {
            rt_kprintf("%s line %d freq_num %d i %d freq_idx %d is_synced %d snr %d rssi %d\n", __FUNCTION__, __LINE__,
                       freq_num, i, freq_list->freq_idx, freq_list->is_synced, freq_list->snr, freq_list->rssi);
            if (freq_list->is_synced == 0)
            {
                manager_wiota_add_freq_list(freq_list);
            }
            else
            {
                manager_wiota_remove_freq_list(freq_list);
            }
            re = 0;
            freq_list++;
        }

        if (!re)
        {
            manager_wiota_print_freq_list();
        }

        rt_free(result.data);
    }

    return re;
}

static int manager_wiota_scan_freq()
{
    uc_scan_recv_t result;
    u8_t list[16] = {0};
    int list_len = 0;
    int res;

    uc_wiota_init();
    uc_wiota_run();

    manager_wiota_clean_freq_list();

    list_len = manager_wiota_get_static_freq((char *)list);
    rt_kprintf("%s line %d list_len %d\n", __FUNCTION__, __LINE__, list_len);

    switch (list_len)
    {
    case 0:
    {
        uc_wiota_scan_freq(RT_NULL, 0, 0, -1, RT_NULL, &result);
        rt_kprintf("%s line %d uc_wiota_scan_freq result %d\n", __FUNCTION__, __LINE__, result.result);
        res = manager_wiota_freq_manager(result);
        break;
    }
    case 1:
    {
        res = manager_wiota_only_freq(list[0]);
        rt_kprintf("%s line %d res = %d\n", __FUNCTION__, __LINE__, res);
        break;
    }
    default:
    {
        uc_wiota_scan_freq(list, list_len, 0, WIOTA_SCAN_TIMEOUT, RT_NULL, &result);
        rt_kprintf("%s line %d uc_wiota_scan_freq result %d\n", __FUNCTION__, __LINE__, result.result);
        res = manager_wiota_freq_manager(result);
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
        uc_state_info_t *curr_node;

        rt_slist_for_each_entry(curr_node, &p_node->node, node)
        {
            rt_kprintf("user_id 0x%x, ul_recv_len %d, ul_recv_suc %d, dl_send_len %d, dl_send_suc %d, dl_send_fail %d\n",
                       curr_node->user_id, curr_node->ul_recv_len, curr_node->ul_recv_suc,
                       curr_node->dl_send_len, curr_node->dl_send_suc, curr_node->dl_send_fail);
        }
        uc_wiota_reset_all_state_info();
        rt_thread_mdelay(5000);
    }
}

static void manager_wiota_report_state(int type)
{
    rt_kprintf("%s type = %d\n", __FUNCTION__, type);
    switch (type)
    {
    case WIOTA_MANAGER_REPORT_FREQ_SUC:
    {
        t_freq_list_manager *tmp;

        rt_list_for_each_entry(tmp, &g_wiota_manager_freq.freq_list.node, node)
        {
            rt_kprintf("+SCANFFREQ %d,%d,%d,%d\n", tmp->data.freq, tmp->data.snr, tmp->data.rssi, tmp->data.is_synced);
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
        t_freq_list_manager *curr_node = g_wiota_manager_freq.current_freq_node;
        if (curr_node != RT_NULL)
            rt_kprintf("+WIOTA:FREQ %d,%d,%d\n", curr_node->data.freq, curr_node->data.rssi, curr_node->data.snr);
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
    manager_wiota_freq_list_init();

    SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_SCAN);

    while (WIOTA_MANAGER_PROCESS_STRATEGY != GET_WIOTA_MANAGER_PROCESS)
    {
        rt_kprintf("%s line %d state %d\n", __FUNCTION__, __LINE__, GET_WIOTA_MANAGER_PROCESS);
        switch (GET_WIOTA_MANAGER_PROCESS)
        {
        case WIOTA_MANAGER_PROCESS_SCAN:
        {
            if (manager_wiota_scan_freq())
            {
                manager_wiota_report_state(WIOTA_MANAGER_REPORT_FREQ_FAIL);
                g_wiota_manager_freq.continue_scan_fail++;
                rt_thread_mdelay(1000 * g_wiota_manager_freq.continue_scan_fail);
                SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_SCAN);
            }
            else
            {
                manager_wiota_report_state(WIOTA_MANAGER_REPORT_FREQ_SUC);
                SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_INIT);
            }

            break;
        }
        case WIOTA_MANAGER_PROCESS_INIT:
        {
            if (manager_wiota_choose_freq())
            {
                SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_SCAN);
            }
            else
            {
                uc_wiota_init();
                manager_wiota_report_state(WIOTA_MANAGER_REPORT_INIT);
                rt_kprintf("%s line %d freq %d\n", __FUNCTION__, __LINE__, g_wiota_manager_freq.current_freq_node->data.freq);
                uc_wiota_set_freq_info(g_wiota_manager_freq.current_freq_node->data.freq);
                manager_wiota_report_state(WIOTA_MANAGER_REPORT_USER_FREQ);

                if (!manager_wiota_choose_freq())
                {
                    /* set hopping frequency point */
                    rt_kprintf("%s line %d hopping freq %d\n", __FUNCTION__, __LINE__, g_wiota_manager_freq.current_freq_node->data.freq);
                    uc_wiota_set_hopping_freq(g_wiota_manager_freq.current_freq_node->data.freq);
                    /* set hopping frequency mode */
                    rt_kprintf("%s line %d hopping mode %d-%d\n", __FUNCTION__, __LINE__, 2, 2);
                    uc_wiota_set_hopping_mode(2, 2);
                }
                SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_RUN);
            }
            break;
        }
        case WIOTA_MANAGER_PROCESS_RUN:
        {
            uc_wiota_run();
            manager_wiota_report_state(WIOTA_MANAGER_REPORT_RUN);
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
            manager_wiota_report_state(WIOTA_MANAGER_REPORT_EXIT);
            SET_WIOTA_MANAGER_PROCESS(WIOTA_MANAGER_PROCESS_INIT);
            break;
        }
        }
    }

    return 0;
}

void manager_wiota_exit(void)
{
    manager_wiota_clean_freq_list();
    uc_wiota_exit();
}
#endif
