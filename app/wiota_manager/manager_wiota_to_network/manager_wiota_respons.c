#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "manger_net_messager.h"
#include "manager_logic_cmd.h"
#include "manager_wiota_respons.h"
#include "manager_operation_data.h"
#include "manager_wiota_attribute.h"

int manager_response_message_result(int result, unsigned int number)
{
       t_net_messager_response *response_number = rt_malloc(sizeof(t_net_messager_response));
       response_number->response_number = number;
       response_number->result = result;
       
       if(0 != manager_send_data_logic_to_net(MANAGER_LOGIC_MQTT_EXE_RESPONSE, (unsigned char *)response_number, 4, 0))
       {
            rt_free(response_number);
            return 1;
       }
       return 0;
}

 void manager_network_to_wiota_result(void *buf)
{
    t_app_operation_data *operation_data = buf;
    rt_kprintf("manager_message_def_exe_result cmd = %d\n", operation_data->head.cmd);

    if (SERVER_DEFAULT_ADDRESS == operation_data->head.src_address &&\
        MANAGER_SEND_FAIL == operation_data->head.state)
        manager_response_message_result(MANAGER_SEND_FAIL, operation_data->head.request_number);
    
   if (RT_NULL != operation_data->pload)
       rt_free(operation_data->pload);
}
 
#endif
