#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "manager_queue.h"
#include "manager_module.h"

void *manager_create_queue(const char *name, unsigned int msg_size, unsigned int max_msgs, unsigned char flag)
{
    rt_mq_t mq = rt_malloc(sizeof(struct rt_messagequeue));
    void *msgpool = rt_malloc(4 * max_msgs);

    if (RT_NULL == mq || RT_NULL == msgpool)
        return RT_NULL;

    if (RT_EOK != rt_mq_init(mq, name, msgpool, 4, 4 * max_msgs, flag))
        return RT_NULL;

    return (void *)mq;
}

int manager_recv_queue(void *queue, void **buf, signed int timeout)
{
    unsigned int address = 0;
    int result = 0;
    result = rt_mq_recv(queue, &address, 4, timeout);
    *buf = (void *)address;

    return result;
}

int manager_send_queue(void *queue, void *buf, signed int timeout)
{
    unsigned int address = (unsigned int)buf;

    return rt_mq_send_wait(queue, &address, 4, timeout);
}

int manager_dele_queue(void *queue)
{
    rt_err_t ret = rt_mq_detach(queue);
    rt_free(((rt_mq_t)queue)->msg_pool);
    rt_free(queue);
    return ret;
}

int manager_send_page(void *queue, int src_task, void *data)
{
    int result = 0;
    t_app_manager_message *new = rt_malloc(sizeof(t_app_manager_message));
    MEMORY_ASSERT(new);
    //rt_kprintf("manager_send_page new malloc address 0x%x\n", new);
    new->src_task = src_task;
    new->message = data;

    result = manager_send_queue(queue, new, 4);
    if (RT_EOK != result)
    {
        rt_free(new);
        rt_kprintf("manager_send_queue error, src_task %d,result %d\n", src_task, result);
        return 2;
    }

    return 0;
}

#endif
