#ifndef _MANAGER_TASK_H_
#define _MANAGER_TASK_H_

int manager_thread_create_task(void **thread,
                               char *name, void (*entry)(void *parameter),
                               void *parameter, unsigned int stack_size,
                               unsigned char priority,
                               unsigned int tick);

int manager_thread_del(void *thread);

#endif
