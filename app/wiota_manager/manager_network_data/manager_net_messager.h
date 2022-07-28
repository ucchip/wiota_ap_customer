#ifndef MANAGER_CUSTOM_ADDRESS_H_
#define MANAGER_CUSTOM_ADDRESS_H_

typedef struct from_network_message
{
    //unsigned int src_id;
    unsigned int dest_len;
    unsigned int *dest_address;
    int cmd;
    unsigned int request_number;
    unsigned char *json_data;
} t_from_network_message;

typedef struct
{
    unsigned int start_address;
    unsigned int end_address;
} t_address_period;

int manager_pasing_address(unsigned char *data, t_address_period *address_info);

int manager_net_data_logic(t_from_network_message *page);

// void send_big_data_to_wiota();

#endif
