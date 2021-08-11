/* To do add our copyright statement */
/* This module init routing is not used. 2018/01/02*/
#ifndef __MODULES_H__
#define __MODULES_H__

typedef s32_t (*initcall_t)(void);
extern initcall_t __initcall_start, __initcall_end;

#define __initcall(fn) \
              initcall_t __initcall_##fn __init_call = fn
#define __init_call     __attribute__ ((unused,__section__ (".initcallptrs")))
#define module_init(x)  __initcall(x);

#define __init __attribute__ ((__section__ (".initseg")))

#endif 
