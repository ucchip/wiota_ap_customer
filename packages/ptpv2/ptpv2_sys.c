/* sys.c */

#include "ptpv2_main.h"
#include "easy_ptp.h"

void displayStats(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    static int start = 1;
    static char sbuf[SCREEN_BUFSZ];
    char *s;
    int len = 0;

    if (start && rtOpts->csvStats)
    {
        start = 0;
        printf("state, one way delay, offset from master, drift");
        fflush(stdout);
    }

    memset(sbuf, ' ', SCREEN_BUFSZ);

    switch (ptpClock->portState)
    {
    case PTP_INITIALIZING:
        s = "init";
        break;
    case PTP_FAULTY:
        s = "flt";
        break;
    case PTP_LISTENING:
        s = "lstn";
        break;
    case PTP_PASSIVE:
        s = "pass";
        break;
    case PTP_UNCALIBRATED:
        s = "uncl";
        break;
    case PTP_SLAVE:
        s = "slv";
        break;
    case PTP_PRE_MASTER:
        s = "pmst";
        break;
    case PTP_MASTER:
        s = "mst";
        break;
    case PTP_DISABLED:
        s = "dsbl";
        break;
    default:
        s = "?";
        break;
    }

    len += sprintf(sbuf + len, "%s%s", rtOpts->csvStats ? "\n" : "\rstate: ", s);

    if (ptpClock->portState == PTP_SLAVE)
    {
        len += sprintf(sbuf + len,
                       ", %s%d.%09d"
                       ", %s%d.%09d",
                       rtOpts->csvStats ? "" : "owd: ",
                       ptpClock->meanPathDelay.seconds,
                       ptpClock->meanPathDelay.nanoseconds,
                       //abs(ptpClock->meanPathDelay.nanoseconds),
                       rtOpts->csvStats ? "" : "ofm: ",
                       ptpClock->offsetFromMaster.seconds,
                       ptpClock->offsetFromMaster.nanoseconds);
        //abs(ptpClock->offsetFromMaster.nanoseconds));

        len += sprintf(sbuf + len, ", %s%d\n", rtOpts->csvStats ? "" : "drift: ", ptpClock->observed_drift);
    }

    write(1, sbuf, rtOpts->csvStats ? len : SCREEN_MAXSZ + 1);
}

Boolean nanoSleep(TimeInternal *t)
{
#ifdef __SPU__
    struct timespec ts, tr;

    ts.tv_sec = t->seconds;
    ts.tv_nsec = t->nanoseconds;

    if (nanosleep(&ts, &tr) < 0)
    {
        t->seconds = tr.tv_sec;
        t->nanoseconds = tr.tv_nsec;
        return FALSE;
    }
#endif

    return TRUE;
}

double getRand()
{
    return ((rand() * 1.0) / RAND_MAX);
}

Boolean adjFreq(Integer32 adj)
{
    s_systime timeTmp = {0, 0};
    Integer32 tempadj = 0;

    sys_time_get(&timeTmp);

    if (adj > 0) //slave less than master shoudle be plus adj
    {
        tempadj = (adj + 500) / 1000; //四舍五入

        timeTmp.usec = timeTmp.usec + tempadj;
    }
    else //slave more than master shoudle be dec adj
    {
        tempadj = (adj - 500) / 1000; //四舍五入

        timeTmp.usec = timeTmp.usec + tempadj;
    }

    sys_time_set(&timeTmp);

    sys_time_get(&timeTmp);

    return 0;
}
