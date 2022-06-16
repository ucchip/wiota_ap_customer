#ifndef _MANAGER_USERID_H_
#define _MANAGER_USERID_H_

#define MANAGER_SAME_POSITION_MIN_NUM 2

/*20 min */
#define MANAGER_ADDR_PERIODIC_TIME 1200000

typedef enum
{
    ADDRESS_RESERVED = 0,
    ADDRESS_PREALLOCATED,
    ADDRESS_USING,
}e_allocate_flag;

typedef struct manager_address
{
    e_allocate_flag allocate_flag;
    unsigned int address;
} t_manager_address;

typedef struct
{
    unsigned int dev_count;
}t_dev_statistical;


typedef struct manager_address_frame
{
    unsigned int address;
    int pos;
    int type;/*0: old address, 1: new address*/
} t_manager_address_frame;



typedef void (*manager_start_replace_address)(void);
typedef void (*manager_send_replace_message)(unsigned int old_address, unsigned int new_address);

void manager_address_init(manager_start_replace_address start_replace, manager_start_replace_address request_reserve_address);
void manager_address_exit(void);
void manager_reserved_address(unsigned int reserved_start_address, unsigned int reserved_end_address);
void manager_get_reserved_address(unsigned int reserved_start_address, unsigned int reserved_end_address);
int manager_replace_address_result(unsigned int old_address, unsigned int new_address);
void manager_operation_replace_address(manager_send_replace_message function);


#endif
