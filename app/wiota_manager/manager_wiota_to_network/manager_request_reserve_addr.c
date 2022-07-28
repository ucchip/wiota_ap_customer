#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "cJSON.h"
#include "manager_addrss.h"
#include "net_passthrough.h"
#include "manager_logic_cmd.h"
#include "manager_request_reserve_addr.h"

void manager_request_reserve_addr(void)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "src_address", AP_DEFAULT_ADDRESS);
    cJSON_AddStringToObject(root, "type", "ap");

    // send data to net_passthrough task
    if (0 != to_network_data(WIOTA_AP_REQUEST_RESERVE_ADDR, root))
    {
        cJSON_Delete(root);
    }
}
#endif
