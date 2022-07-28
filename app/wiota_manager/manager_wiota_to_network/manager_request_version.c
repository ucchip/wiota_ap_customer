#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "manager_wiota_attribute.h"
#include "manager_logic_cmd.h"
#include "net_passthrough.h"
#include "manager_addrss.h"
#include "uc_wiota_static.h"
#include "cJSON.h"

void manager_check_ap_version(void)
{
    cJSON *root = cJSON_CreateObject();
    void *data;
    unsigned char version[24] = {0};

    uc_wiota_get_hardware_ver(version);

    cJSON_AddStringToObject(root, "soft_version", AP_DEMO_VERSION);
    cJSON_AddStringToObject(root, "hand_version", (char *)version);
    cJSON_AddStringToObject(root, "type", "ap");

    data = cJSON_Print(root);
    rt_kprintf("manager_check_ap_version:%s\n", data);

    manager_send_data_logic_to_net(APP_CMD_UPDATE_VERSION_REQUEST, data, rt_strlen(data), AP_DEFAULT_ADDRESS);

    cJSON_Delete(root);
}
#endif
