#include "ptpv2_main.h"
#include "easy_ptp.h"

static rt_uint32_t g_vaild_cnt = 0;

void initClock(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    DBG("initClock\n");
    /* clear vars */
    ptpClock->master_to_slave_delay.seconds = ptpClock->master_to_slave_delay.nanoseconds = 0;
    ptpClock->slave_to_master_delay.seconds = ptpClock->slave_to_master_delay.nanoseconds = 0;
    ptpClock->observed_drift = 0; /* clears clock servo accumulator (the I term) */
    ptpClock->owd_filt.s_exp = 0; /* clears one-way delay filter */

    /* level clock */
    if (!rtOpts->noAdjust)
    {
        adjFreq(0);
    }
}

void updateDelay(one_way_delay_filter *owd_filt, RunTimeOpts *rtOpts, PtpClock *ptpClock, TimeInternal *correctionField)
{

    if (rtOpts->offset_first_updated)
    {
        Integer16 s;

        /* calc 'slave_to_master_delay' (Master to Slave delay is already computed in updateOffset )*/
        subTime(&ptpClock->delaySM, &ptpClock->delay_req_receive_time, &ptpClock->delay_req_send_time);

        /* update 'one_way_delay' */
        addTime(&ptpClock->meanPathDelay, &ptpClock->delaySM, &ptpClock->delayMS);

        /*Substract correctionField*/
        subTime(&ptpClock->meanPathDelay, &ptpClock->meanPathDelay, correctionField);

        /*Compute one-way delay*/
        ptpClock->meanPathDelay.seconds /= 2;
        ptpClock->meanPathDelay.nanoseconds /= 2;

        if (ptpClock->meanPathDelay.seconds)
        {
            /* cannot filter with secs, clear filter */
            owd_filt->s_exp = owd_filt->nsec_prev = 0;
            return;
        }

        /* avoid overflowing filter */
        s = rtOpts->s;
        while (abs(owd_filt->y) >> (31 - s))
            --s;

        /* crank down filter cutoff by increasing 's_exp' */
        if (owd_filt->s_exp < 1)
            owd_filt->s_exp = 1;
        else if (owd_filt->s_exp < 1 << s)
            ++owd_filt->s_exp;
        else if (owd_filt->s_exp > 1 << s)
            owd_filt->s_exp = 1 << s;

        /* filter 'meanPathDelay' */
        owd_filt->y = (owd_filt->s_exp - 1) * owd_filt->y / owd_filt->s_exp +
                      (ptpClock->meanPathDelay.nanoseconds / 2 + owd_filt->nsec_prev / 2) / owd_filt->s_exp;

        owd_filt->nsec_prev = ptpClock->meanPathDelay.nanoseconds;
        ptpClock->meanPathDelay.nanoseconds = owd_filt->y;

        DBGV("delay filter %d, %d\n", owd_filt->y, owd_filt->s_exp);
    }
}

void updatePeerDelay(one_way_delay_filter *owd_filt, RunTimeOpts *rtOpts, PtpClock *ptpClock, TimeInternal *correctionField, Boolean twoStep)
{
    Integer16 s;

    DBGV("updateDelay\n");

    if (twoStep)
    {
        /* calc 'slave_to_master_delay' */
        subTime(&ptpClock->pdelayMS, &ptpClock->pdelay_resp_receive_time, &ptpClock->pdelay_resp_send_time);
        subTime(&ptpClock->pdelaySM, &ptpClock->pdelay_req_receive_time, &ptpClock->pdelay_req_send_time);

        /* update 'one_way_delay' */
        addTime(&ptpClock->peerMeanPathDelay, &ptpClock->pdelayMS, &ptpClock->pdelaySM);

        /*Substract correctionField*/
        subTime(&ptpClock->peerMeanPathDelay, &ptpClock->peerMeanPathDelay, correctionField);

        /*Compute one-way delay*/
        ptpClock->peerMeanPathDelay.seconds /= 2;
        ptpClock->peerMeanPathDelay.nanoseconds /= 2;
    }
    else /*One step clock*/
    {

        subTime(&ptpClock->peerMeanPathDelay, &ptpClock->pdelay_resp_receive_time, &ptpClock->pdelay_req_send_time);

        /*Substract correctionField*/
        subTime(&ptpClock->peerMeanPathDelay, &ptpClock->peerMeanPathDelay, correctionField);

        /*Compute one-way delay*/
        ptpClock->peerMeanPathDelay.seconds /= 2;
        ptpClock->peerMeanPathDelay.nanoseconds /= 2;
    }

    if (ptpClock->peerMeanPathDelay.seconds)
    {
        /* cannot filter with secs, clear filter */
        owd_filt->s_exp = owd_filt->nsec_prev = 0;
        return;
    }

    /* avoid overflowing filter */
    s = rtOpts->s;
    while (abs(owd_filt->y) >> (31 - s))
        --s;

    /* crank down filter cutoff by increasing 's_exp' */
    if (owd_filt->s_exp < 1)
        owd_filt->s_exp = 1;
    else if (owd_filt->s_exp < 1 << s)
        ++owd_filt->s_exp;
    else if (owd_filt->s_exp > 1 << s)
        owd_filt->s_exp = 1 << s;

    /* filter 'meanPathDelay' */
    owd_filt->y = (owd_filt->s_exp - 1) * owd_filt->y / owd_filt->s_exp +
                  (ptpClock->peerMeanPathDelay.nanoseconds / 2 + owd_filt->nsec_prev / 2) / owd_filt->s_exp;

    owd_filt->nsec_prev = ptpClock->peerMeanPathDelay.nanoseconds;
    ptpClock->peerMeanPathDelay.nanoseconds = owd_filt->y;

    DBGV("delay filter %d, %d\n", owd_filt->y, owd_filt->s_exp);
}

void updateOffset(TimeInternal *send_time, TimeInternal *recv_time,
                  offset_from_master_filter *ofm_filt, RunTimeOpts *rtOpts, PtpClock *ptpClock, TimeInternal *correctionField)
{
    // static rt_uint32_t printcnt = 0;

    TimeInternal temptime = {0, 0};
    /* calc 'master_to_slave_delay' */ //t2-t1
    subTime(&ptpClock->master_to_slave_delay, recv_time, send_time);

    subTime(&ptpClock->delayMS, recv_time, send_time); //Used just for End to End mode..

    /*Take care about correctionField*/ //t2-t1-tms(网络传输延迟)
    subTime(&ptpClock->master_to_slave_delay, &ptpClock->master_to_slave_delay, correctionField);

    /* update 'offsetFromMaster' */
    if (!rtOpts->E2E_mode)
    {
        subTime(&ptpClock->offsetFromMaster, &ptpClock->master_to_slave_delay, &temptime); //&ptpClock->peerMeanPathDelay);
    }
    else //(End to End mode)
    {
        subTime(&ptpClock->offsetFromMaster, &ptpClock->master_to_slave_delay, &ptpClock->meanPathDelay);

        // rt_kprintf("offset FromMaster 1=%d.%09d\n", ptpClock->offsetFromMaster.seconds, ptpClock->offsetFromMaster.nanoseconds);
    }

    if (((ptpClock->offsetFromMaster.nanoseconds / 10000000) > 10) || ptpClock->offsetFromMaster.seconds)
    {
        /* cannot filter with secs, clear filter */
        rt_kprintf("offset FromMaster more than 10ms =%d\n", ptpClock->offsetFromMaster.seconds);

        ofm_filt->nsec_prev = 0;

        return;
    }

    /* filter 'offsetFromMaster' 当没有秒误差时,误差=当前的误差的一半加上一次误差的一半*/
    ofm_filt->y = ptpClock->offsetFromMaster.nanoseconds / 2 + ofm_filt->nsec_prev / 2;
    //  ofm_filt->nsec_prev = ptpClock->offsetFromMaster.nanoseconds;
    ptpClock->offsetFromMaster.nanoseconds = ofm_filt->y;

#if 0
    printcnt++;

    // if(printcnt%30==0)
    {
        rt_kprintf("\r\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
        rt_kprintf("PTP MS Delay=%d.%09d,SM Delay=%d.%09d,MeanPath Deley=%d.%09d\r\n", ptpClock->master_to_slave_delay.seconds, ptpClock->master_to_slave_delay.nanoseconds,
                   ptpClock->slave_to_master_delay.seconds, ptpClock->slave_to_master_delay.nanoseconds,
                   ptpClock->meanPathDelay.seconds, ptpClock->meanPathDelay.nanoseconds);

        if (ptpClock->offsetFromMaster.nanoseconds < 0)
        {
            rt_kprintf("current offset from master: %ds.%03dms.%03dus.%03dns\r\n",
                       ptpClock->offsetFromMaster.seconds, (0 - ptpClock->offsetFromMaster.nanoseconds) / 1000000, (0 - ptpClock->offsetFromMaster.nanoseconds) / 1000 - (((0 - ptpClock->offsetFromMaster.nanoseconds) / 1000000) * 1000), (0 - ptpClock->offsetFromMaster.nanoseconds) % 1000);
        }
        else
        {
            rt_kprintf("current offset from master: %ds.%03dms.%03dus.%03dns\r\n",
                       ptpClock->offsetFromMaster.seconds, ptpClock->offsetFromMaster.nanoseconds / 1000000, ptpClock->offsetFromMaster.nanoseconds / 1000 - ((ptpClock->offsetFromMaster.nanoseconds / 1000000) * 1000), ptpClock->offsetFromMaster.nanoseconds % 1000);
        }
    }
#endif
    //Offset must have been computed at least one time before computing end to end delay
    if (!rtOpts->offset_first_updated)
    {
        rtOpts->offset_first_updated = TRUE;
    }
}

void updateClock(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    Integer32 adj = 0;
    s_systime timeTmp = {0, 0};
    TimeInternal time3;

    //  rt_kprintf("updateClock\n");

    if (((ptpClock->offsetFromMaster.nanoseconds / 10000000) > 10) || ptpClock->offsetFromMaster.seconds)
    {
        /* if secs, reset clock or set freq adjustment to max */
        if (!rtOpts->noAdjust)
        {
            if (!rtOpts->noResetClock)
            {
                sys_time_get(&timeTmp);

                time3.seconds = timeTmp.sec;
                time3.nanoseconds = timeTmp.usec * 1000;

                subTime(&time3, &time3, &ptpClock->offsetFromMaster);

                timeTmp.sec = time3.seconds;
                timeTmp.usec = time3.nanoseconds / 1000;

                sys_time_set(&timeTmp);

                // rt_kprintf("updateClock offset master sec =%u,  nanosec=%09u\r\n", ptpClock->offsetFromMaster.seconds, ptpClock->offsetFromMaster.nanoseconds);

                initClock(rtOpts, ptpClock);
            }
            else
            {
                adj = ptpClock->offsetFromMaster.nanoseconds > 0 ? ADJ_FREQ_MAX : -ADJ_FREQ_MAX;
                adjFreq(-adj);
            }
        }
        if (abs(ptpClock->offsetFromMaster.nanoseconds) < 1000000 && easy_ptp_start_get() == 1)
        {
            if (g_vaild_cnt >= 10)
            {
                easy_ptp_set_time();
            }
            else
            {
                g_vaild_cnt++;
            }
        }
    }
    else
    {
        /* the PI controller */

        /* no negative or zero attenuation 无负或零衰减*/
        if (rtOpts->ap < 1)
            rtOpts->ap = 1;
        if (rtOpts->ai < 1)
            rtOpts->ai = 1;

        /* the accumulator for the I component I分量的累加器,精度us,ai=1000*/
        ptpClock->observed_drift += ptpClock->offsetFromMaster.nanoseconds / rtOpts->ai;

        // rt_kprintf("observed_drift=%d\n", ptpClock->observed_drift);

        /* clamp the accumulator to ADJ_FREQ_MAX for sanity 为防止溢出，将累加器钳位到ADJ_FREQ_MAX(限制在-max~+max之间)ADJ_FREQ_MAX=512000*/
        if (ptpClock->observed_drift > ADJ_FREQ_MAX)
            ptpClock->observed_drift = ADJ_FREQ_MAX;
        else if (ptpClock->observed_drift < -ADJ_FREQ_MAX)
            ptpClock->observed_drift = -ADJ_FREQ_MAX;

        //AP=10
        adj = ptpClock->offsetFromMaster.nanoseconds / rtOpts->ap + ptpClock->observed_drift;

        /* apply controller output as a clock tick rate adjustment */
        if (!rtOpts->noAdjust)
        {
            adjFreq(-adj);
        }
        if (abs(ptpClock->offsetFromMaster.nanoseconds) < 1000000 && easy_ptp_start_get() == 1)
        {
            if (g_vaild_cnt >= 10)
            {
                easy_ptp_set_time();
            }
            else
            {
                g_vaild_cnt++;
            }
        }
    }

    if (rtOpts->displayStats)
        displayStats(rtOpts, ptpClock);
}
