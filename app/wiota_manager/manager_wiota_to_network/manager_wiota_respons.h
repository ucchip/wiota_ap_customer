#ifndef MANAGER_WIOTA_RESPONSE_H_
#define MANAGER_WIOTA_RESPONSE_H_

typedef struct net_messager_response_result
{
    unsigned int response_number;
    int result;
} t_net_messager_response;

int manager_response_message_result(int result, unsigned int number);

void manager_network_to_wiota_result(void *buf);

#endif
