#ifndef APP_MANAGER_LOGIC_H_
#define APP_MANAGER_LOGIC_H_

typedef struct app_logic_message
{
    int cmd;
    void *data;
} t_app_logic_message;

int manager_create_logic_queue(void);
void manager_wiota_task(void *pPara);

void to_queue_logic_data(int src_task, int cmd, void *data);

int manager_logic(void);

#endif
