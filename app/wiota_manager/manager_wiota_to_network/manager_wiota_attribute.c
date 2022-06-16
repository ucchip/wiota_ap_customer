#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "manager_wiota_attribute.h"
#include "manager_logic_cmd.h"
#include "net_passthrough.h"
#include "cJSON.h"

#define AP_ADDRESS 0x123456
void manager_input_attribute(void)
{
    cJSON *root = cJSON_CreateObject();
    void *data;

    cJSON_AddNumberToObject(root, "src_address", AP_ADDRESS);

    data = cJSON_Print(root);
    rt_kprintf("data:%s\n", data);
    rt_free(data);

    if(0 != to_network_data(APP_CMD_AP_PROPERTY_REPORT, root))
    {
        cJSON_Delete(root);
    }
}

int  manager_send_data_logic_to_net(int cmd, unsigned char *data, unsigned int data_len, unsigned int id)
{

    t_app_logic_to_net *l_to_n = (t_app_logic_to_net *)rt_malloc(sizeof(t_app_logic_to_net));
    if (l_to_n == RT_NULL)
    {
        rt_kprintf("%s, %d malloc error\n", __FUNCTION__, __LINE__);
        return 1;
    }
    rt_memset(l_to_n, 0, sizeof(t_app_logic_to_net));
    l_to_n->src_addr = id;
    l_to_n->data_len = data_len;
    l_to_n->data = data;    // manually release after use

    if(0 != to_network_data(cmd, l_to_n))
    {
        rt_free(l_to_n);
    }
    return 0;
}
#endif
