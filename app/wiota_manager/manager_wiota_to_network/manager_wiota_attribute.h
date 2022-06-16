#ifndef MANAGER_WIOTA_ATTRIBUTE_H_
#define MANAGER_WIOTA_ATTRIBUTE_H_

typedef struct app_logic_to_net
{
    unsigned int src_addr;
    unsigned int data_len;
    void *data;
} t_app_logic_to_net;

void manager_input_attribute(void);

int manager_send_data_logic_to_net(int cmd, unsigned char *data, unsigned int data_len, unsigned int id);
#endif
