
//#include <interrupt.h>
#include <rthw.h>

#include <board.h>
//#include <platform.h>
//#include <encoding.h>
//#include <interrupt.h>

#include <uc_utils.h>
#include <uc_event.h>
#include "uc_timer.h"
#include "uc_spi_flash.h"

#if 0
extern void use_default_clocks(void);
extern void use_pll(int refsel, int bypass, int r, int f, int q);

#define TICK_COUNT  (2 * RTC_FREQ / RT_TICK_PER_SECOND)

#define MTIME       (*((volatile uint64_t *)(CLINT_CTRL_ADDR + CLINT_MTIME)))
#define MTIMECMP    (*((volatile uint64_t *)(CLINT_CTRL_ADDR + CLINT_MTIMECMP)))

/* system tick interrupt */
void handle_m_time_interrupt()
{
    MTIMECMP = MTIME + TICK_COUNT;
    rt_tick_increase();
}

/* fixed misaligned bug for qemu */
void *__wrap_memset(void *s, int c, size_t n)
{
    return rt_memset(s, c, n);
}

static void rt_hw_clock_init(void)
{
    use_default_clocks();
    use_pll(0, 0, 1, 31, 1);
}

static void rt_hw_timer_init(void)
{
    MTIMECMP = MTIME + TICK_COUNT;

    /*  enable timer interrupt*/
    set_csr(mie, MIP_MTIP);
}
#else
/* fixed misaligned bug for qemu */
void *__wrap_memset(void *s, int c, size_t n)
{
    return rt_memset(s, c, n);
}
#endif

void sys_tick_handler(void)
{
//	uint32_t mcause;
//    csrr(mcause, mcause);
//    ICP = (1 << mcause);
//	ECP = 0x20000000;
	ICP = 0x20000000;
    ECP = 0x20000000;
	
	/* enter interrupt */
    //rt_interrupt_enter();

    rt_tick_increase();
    TIMER_Set_Count(UC_TIMER0, 0);

    /* leave interrupt */
    //rt_interrupt_leave();
}

//#define configCPU_CLOCK_HZ			( ( unsigned long ) 4860000)   
//#define configTICK_RATE_DIV			( 500 )   //Div=Tick*PreScalar 2500?
#define configCPU_CLOCK_HZ			( ( unsigned long ) BSP_CLOCK_SYSTEM_FREQ_HZ)  
#define configTICK_RATE_DIV			( RT_TICK_PER_SECOND )

static void rt_hw_systick_init(void)
{
    /* Setup Timer A */    
    TIMER_CFG_Type TIMERX_InitStructure;
    
    /* set time count*/
    TIMERX_InitStructure.Count = 0x0;
    
    /* set compare value*/
    TIMERX_InitStructure.Compare_Value = configCPU_CLOCK_HZ / (5 * configTICK_RATE_DIV);
    
    /* set prescaler value*/
    TIMERX_InitStructure.Prescaler = 4;
    
    /* Timer0 start timer */
    TIMER_Init(UC_TIMER0,&TIMERX_InitStructure);
    
    /* Enable TA IRQ */
    IER |= 0x20000000; 
}

void rt_hw_board_init(void)
{
    /* initialize the system clock */
    //rt_hw_clock_init();

    /* initialize hardware interrupt */
    //rt_hw_interrupt_init();

#ifdef RT_USING_HEAP
    rt_system_heap_init((void *)HEAP_BEGIN, (void *)HEAP_END);
#endif

    /* initialize timer0 */
    rt_hw_systick_init();

    /* Pin driver initialization is open by default */
#ifdef RT_USING_PIN
    extern int rt_hw_pin_init(void);
    rt_hw_pin_init();
#endif
    
#ifdef RT_USING_SERIAL
    extern int rt_hw_usart_init(void);
	rt_hw_usart_init();
    extern int virtual_usart_init(void);
    virtual_usart_init();
#endif
	
#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

    return;
}

void uc8088_chip_reset(void)
{
    volatile uint32_t *pmu_ctrl = (uint32_t *)0x1a104200;

    *pmu_ctrl |= 1 << 14;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
static void reboot(uint8_t argc, char **argv)
{
    uc8088_chip_reset();
}
FINSH_FUNCTION_EXPORT_ALIAS(reboot, __cmd_reboot, Reboot System);
#endif /* RT_USING_FINSH */


