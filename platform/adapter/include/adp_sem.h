
#ifndef     _ADP_SEM_H_
#define     _ADP_SEM_H_

#define SYS_SEM_WAITING_FOREVER -1
/**
 * IPC flags and control command definitions
 */
#define ADP_SME_IPC_FLAG_FIFO                0x00            /**< FIFOed IPC. @ref IPC. */
#define ADP_SME_IPC_FLAG_PRIO                0x01            /**< PRIOed IPC. @ref IPC. */

void * uc_create_sem(char *name, unsigned int value, unsigned char flag);
int uc_wait_sem(void *sem, signed   int  timeout);
int uc_signed_sem(void *sem);

void *uc_create_lock(char *name);
int uc_lock(void *sem, signed   int  timeout);
int uc_unlock(void *sem);

#endif


