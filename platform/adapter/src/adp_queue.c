#if defined(  _FREERTOS_)
#include  "FreeRTOS.h"
#include  "queue.h"
#include  "task.h"
#elif defined( _RT_THREAD_)
#include <rtthread.h>
#else

#endif
#include "adp_queue.h"

void * uc_create_queue(const char *name,  unsigned int msg_size, unsigned int max_msgs, unsigned char flag)
{
#if defined(  _FREERTOS_)
    return xQueueCreate_MISO(max_msgs, msg_size);
#elif defined( _RT_THREAD_)
    return  rt_mq_create(name, msg_size, max_msgs, flag);
#else

#endif
}

int uc_recv_queue(void *queue, void *buf, unsigned int size, signed int timeout)
{
#if defined(  _FREERTOS_)
    return xQueueReceive_MISO( queue, buf, timeout );
#elif defined( _RT_THREAD_)
   return rt_mq_recv( queue, buf, size, timeout);
#else

#endif
}

int uc_send_queue(void *queue, void *buf, unsigned int size, signed int timeout)
{
#if defined(  _FREERTOS_)
        return  xQueueSend_MISO( queue, buf, timeout );
#elif defined( _RT_THREAD_)
//       return rt_mq_send_wait( queue, buf, size, timeout);
        return rt_mq_send( queue, buf, size);
#else

#endif
}

int uc_dele_queue(void *queue)
{
#if defined(  _FREERTOS_)
            return  xQueueDelete_MISO( queue);
#elif defined( _RT_THREAD_)
           return rt_mq_delete( queue);
#else

#endif
}
