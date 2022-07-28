#ifndef _MANAGER_USERID_H_
#define _MANAGER_USERID_H_

#define MANAGER_SAME_POSITION_MIN_NUM 0

/*20 min */
#define MANAGER_ADDR_PERIODIC_TIME 20 * 60 * 1000

typedef enum
{
    ADDRESS_RESERVED = 0,
    ADDRESS_PREALLOCATED,
    ADDRESS_USING,
} e_allocate_flag;

typedef struct manager_address
{
    unsigned int address; // wiota id
    unsigned int dev_mac;
    unsigned char allocate_flag;
    unsigned char current_com_count;
    unsigned char old_com_count;
    unsigned char reserved;
} t_manager_address;

typedef struct
{
    unsigned int dev_count;
} t_dev_statistical;

typedef struct manager_address_frame
{
    unsigned int address;
    int pos;
    int type; /*0: old address, 1: new address*/
} t_manager_address_frame;

typedef void (*manager_start_replace_address)(void);
typedef void (*manager_send_replace_message)(unsigned int src_address, unsigned int dest_address, unsigned int new_address);

void manager_address_init(manager_start_replace_address request_reserve_address);
void manager_wiotaid_start(void);

void manager_address_exit(void);
void manager_reserved_address(unsigned int reserved_start_address, unsigned int reserved_end_address);
void manager_get_reserved_address(unsigned int reserved_start_address, unsigned int reserved_end_address);
int manager_replace_address_result(unsigned int old_address, unsigned int new_address);
void manager_reallocated_wiotaid(unsigned int dev_address, unsigned int wiota_address, manager_send_replace_message function);
unsigned int manager_query_wiotaid(unsigned int mac);

#endif
