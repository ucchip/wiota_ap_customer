/* startup.c */

#include "ptpv2_main.h"

PtpClock *ptpClock;

void ptpdShutdown()
{
    netShutdown(&ptpClock->netPath);

    free(ptpClock->foreign);
    free(ptpClock);
}

PtpClock *ptpdStartup(Integer16 *ret, RunTimeOpts *rtOpts)
{
    // int c, fd = -1, nondaemon = 0, noclose = 0;

    ptpClock = (PtpClock *)calloc(1, sizeof(PtpClock));

    if (!ptpClock)
    {
        PERROR("failed to allocate memory for protocol engine data\r\n");
        *ret = 2;
        return 0;
    }
    else
    {
        DBG("allocated %d bytes for protocol engine data\r\n", (int)sizeof(PtpClock));

        ptpClock->foreign = (ForeignMasterRecord *)calloc(rtOpts->max_foreign_records, sizeof(ForeignMasterRecord));

        if (!ptpClock->foreign)
        {
            PERROR("failed to allocate memory for foreign master data\r\n");

            *ret = 2;

            free(ptpClock);

            return 0;
        }
        else
        {
            DBG("allocated %d bytes for foreign master data\n", (int)(rtOpts->max_foreign_records * sizeof(ForeignMasterRecord)));
        }
    }

    /*Init to 0 net buffer*/
    memset(ptpClock->msgIbuf, 0, PACKET_SIZE);
    memset(ptpClock->msgObuf, 0, PACKET_SIZE);

    *ret = 0;

    return ptpClock;
}
