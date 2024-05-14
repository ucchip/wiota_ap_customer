/* ptpd.c */
#include <rtthread.h>
#include "ptpv2_main.h"
#include "global_station_config.h"

RunTimeOpts rtOpts; /* statically allocated run-time configuration data */

//0:default master,
//1:real master
//2:real slave
static uint8_t ptp_real_mode = 0;

void set_ptp_real_mode(uint8_t val)
{
    ptp_real_mode = val;
}

//0:default master,
//1:real master
//2:real slave

uint8_t get_ptp_real_mode(void)
{
    return ptp_real_mode;
}

static void ptpd_thread(void *parameter)
{
    PtpClock *ptpClock;
    Integer16 ret;

    /* initialize run-time options to default values */
    rtOpts.announceInterval = DEFAULT_ANNOUNCE_INTERVAL;
    rtOpts.syncInterval = DEFAULT_SYNC_INTERVAL;
    rtOpts.clockQuality.clockAccuracy = DEFAULT_CLOCK_ACCURACY;
    rtOpts.clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
    rtOpts.clockQuality.offsetScaledLogVariance = DEFAULT_CLOCK_VARIANCE;
    rtOpts.priority1 = DEFAULT_PRIORITY1;
    rtOpts.priority2 = DEFAULT_PRIORITY2;
    rtOpts.domainNumber = DEFAULT_DOMAIN_NUMBER;
    rtOpts.slaveOnly = SLAVE_ONLY;
    rtOpts.currentUtcOffset = DEFAULT_UTC_OFFSET;
    rtOpts.noResetClock = DEFAULT_NO_RESET_CLOCK;
    rtOpts.noAdjust = NO_ADJUST;
    rtOpts.inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
    rtOpts.outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
    rtOpts.s = DEFAULT_DELAY_S;
    rtOpts.ap = DEFAULT_AP;
    rtOpts.ai = DEFAULT_AI;
    rtOpts.max_foreign_records = DEFAULT_MAX_FOREIGN_RECORDS;
    rtOpts.E2E_mode = TRUE;

    //if mode is 0,slave,else master,default slave
    if (get_dev_info()->ptp_mode == 0)
    {
        rtOpts.slaveOnly = TRUE;
        rt_kprintf("\r\n*****IEEE 1588 v2 is in Slave mode*****\r\n");
    }
    else
    {
        rtOpts.slaveOnly = FALSE;
        rt_kprintf("\r\n*****IEEE 1588 v2 is in Master mode*****\r\n");
    }

    //#ifndef PTPD_DBG
    //    rtOpts.displayStats = TRUE;
    //#endif

    /*Initialize run time options with command line arguments*/
    if (!(ptpClock = ptpdStartup(&ret, &rtOpts)))
    {
        return;
    }

    /* do the protocol engine */
    protocol(&rtOpts, ptpClock); //forever loop..

    ptpdShutdown();

    NOTIFY("self shutdown, probably due to an error\n");

    return;
}

#define THREAD_PRIORITY RT_MAIN_THREAD_PRIORITY
#define THREAD_STACK_SIZE 4096
#define THREAD_TIMESLICE 5

int ptpd_task(void)
{
    rt_thread_t tid = RT_NULL;

    tid = rt_thread_create("ptpv2", ptpd_thread, RT_NULL, THREAD_STACK_SIZE, THREAD_PRIORITY / 2, 5);

    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    return 0;
}
INIT_APP_EXPORT(ptpd_task);