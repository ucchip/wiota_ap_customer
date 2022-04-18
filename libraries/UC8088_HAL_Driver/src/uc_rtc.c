//#include "uctypes.h"
#include "uc_event.h"
//#include "rtc_reg.h"
#include "uc_rtc.h"
#include "uc_timer.h"
//#include "trace_interface.h"
#include "rtthread.h"

#ifndef NULL
#define NULL 0
#endif

//#define REG(x) (*((volatile unsigned int*)(x)))
//#ifndef BIT
#define BIT(x) (1 << (x))
//#endif
//#ifndef MASK
#define MASK(H, L) (u32_t)((~((0xffffffffU << (H)) << 1)) & (0xffffffffU << (L)))
//#endif

#define RC32K_REFERENCE_VALUE 131000000U
//#define RC32K_DEVIATION_VALUE	13100000U/*rc32K accuracy +/-10%*/
//#define RC32K_DEVIATION_VALUE	6550000U/*rc32K accuracy +/-5%*/
//#define RC32K_DEVIATION_VALUE	1310000U/*rc32K accuracy +/-1%*/
#define RC32K_DEVIATION_VALUE 655000U /*rc32K accuracy +/-0.5%*/
//#define RC32K_DEVIATION_VALUE	131000U/*rc32K accuracy +/-0.1%*/
#define RC32K_UPPER_LIMIT_VALUE (RC32K_REFERENCE_VALUE + RC32K_DEVIATION_VALUE)
#define RC32K_LOWER_LIMIT_VALUE (RC32K_REFERENCE_VALUE - RC32K_DEVIATION_VALUE)

enum
{
    RC32K_FREQ_FIRST,
    RC32K_BIAS_FIRST,
};

//#define RC32K_DEBUG

#ifdef RC32K_DEBUG
#define RC32K_PRINTF rt_kprintf
#else
#define RC32K_PRINTF(...)
#endif

rtc_alm_db_t db;

rtc_cb_func rtc_interrupt_callback = NULL;
extern u8_t test_rtc_flag;
/*RTC handle */
static u8_t g_calibrate_alarm_flag = 0;
void ISR_RTC(void)
{
    rt_interrupt_enter();
    if (NULL != rtc_interrupt_callback)
    {
        rtc_interrupt_callback();
    }

    //IER |= 1 << 0;
    IER &= (~(1 << 0)); //close RTC alm interrput
    ICP |= 1;           //clear RTC interrput pending
    g_calibrate_alarm_flag = 1;
    rt_interrupt_leave();
}

int interval2date(int interval, rtc_date_t *date);

void rc32k_set_clock_freq(u32_t freq_code)
{
    REG(0x1a104228) |= BIT(14);                                                               //manual mode
    REG(0x1a104228) = (REG(0x1a104228) & ~MASK(22, 15)) | ((freq_code << 15) & MASK(22, 15)); //set rc32k clock frequency code
}

void rc32k_set_bias_current(u32_t current_code)
{
    REG(0x1a104228) |= BIT(14);                                                                  //manual mode
    REG(0x1a104228) = (REG(0x1a104228) & ~MASK(12, 10)) | ((current_code << 10) & MASK(12, 10)); //set rc32k bias current code
}

void rc32k_calibrate(void)
{
    REG(0x1a104228) |= BIT(0) | BIT(1); //enable and reset calibrate
    while (REG(0x1a104228) & BIT(0))    //wait calibrate complete
    {
        asm volatile("nop");
    }
}

static u32_t rc32k_get_trim(u32_t *bias_code, u32_t *freq_code)
{
    *bias_code = (REG(0x1a104228) & MASK(12, 10)) >> 10;
    *freq_code = (REG(0x1a104228) & MASK(22, 15)) >> 15;
    return 0;
}

#define RTC_AS0 (((RTC_TypeDef *)UC_RTC)->AS0)
#define RTC_AS1 (((RTC_TypeDef *)UC_RTC)->AS1)
#define RTC_ACTRL (((RTC_TypeDef *)UC_RTC)->ACTRL)

static void rc32k_alarm(u32_t seconds)
{
    rtc_date_t date;

    rtc_get_date(&date.year, &date.mon, &date.day, &date.wday, &date.hour, &date.min, &date.sec);
    interval2date(1, &date);

    //set alram date and time
    if (date.year >= 2000)
    {
        date.year -= 2000;
    }
    RTC_AS0 = (((date.hour & 0x1f) << 16) | ((date.min & 0x3f) << 8) | ((date.sec & 0x3f)));
    RTC_AS1 = (((date.year & 0x3f) << 16) | ((date.mon & 0xf) << 12) | ((date.day & 0x1f) << 4) | (date.wday & 0x7));

    g_calibrate_alarm_flag = 0;
    RTC_ACTRL |= BIT(8); //enable alarm
    ICP |= BIT(0);       //clear alarm pending
    IER |= BIT(0);       //enable alarm interrupt
    //while ((IPR & BIT(0)) == 0)//wait alarm pending
    while (g_calibrate_alarm_flag == 0) //wait alarm pending
    {
        //asm volatile ("nop");
        rt_thread_mdelay(5);
    }
    //ICP |= BIT(0);
}

#define TPRB (((TIMER_TYPE *)UC_TIMER1)->CTR)
#define TIRB (((TIMER_TYPE *)UC_TIMER1)->TRR)

u32_t rc32k_measure(u32_t seconds)
{
    u32_t count;
    //start timer counting
    TPRB &= ~BIT(0);
    TIRB = 0;
    TPRB = BIT(0);
    rc32k_alarm(seconds); //to measure the time
    count = TIRB;
    TPRB &= ~BIT(0);
    //stop timer counting

    return count;
}

static u32_t rc32k_abs_diff(u32_t value)
{
    return value <= RC32K_REFERENCE_VALUE ? (RC32K_REFERENCE_VALUE - value) : (value - RC32K_REFERENCE_VALUE);
}

u32_t rc32k_autoset_bias(void)
{
    u32_t try_cnt, cur, delta, count, diff;
    u32_t best_bias_code, best_count, best_diff;

    best_diff = 0xffffffffU;
    best_count = 0;
    delta = (1 << 2);
    cur = delta;
    best_bias_code = cur;
    for (try_cnt = 0; try_cnt < 3; try_cnt++)
    {
        rc32k_set_bias_current(cur);
        rc32k_calibrate();
        count = rc32k_measure(1);
        diff = rc32k_abs_diff(count);
        if (diff < best_diff)
        {
            best_diff = diff;
            best_bias_code = cur;
            best_count = count;
        }
        RC32K_PRINTF("cur code=%d count=%d\n", cur, count);
        if (count >= RC32K_UPPER_LIMIT_VALUE) //RC32K is too slow
        {
            cur &= ~delta;
            if (try_cnt == 2)
            {
                rc32k_set_bias_current(cur);
                rc32k_calibrate();
                count = rc32k_measure(1);
                diff = rc32k_abs_diff(count);
                if (diff < best_diff)
                {
                    best_diff = diff;
                    best_bias_code = cur;
                    best_count = count;
                }
                RC32K_PRINTF("cur code=%d count=%d\n", cur, count);
            }
        }
        else if (count <= RC32K_LOWER_LIMIT_VALUE) //RC32K is too fast
        {
            //do nothing
        }
        else //RC32K is OK
        {
            break;
        }
        delta >>= 1;
        cur |= delta;
    }

    if (cur != best_bias_code)
    {
        cur = best_bias_code;
        rc32k_set_bias_current(cur);
        rc32k_calibrate();
        count = best_count;
        RC32K_PRINTF("cur code=%d count=%d\n", cur, count);
    }

    return count;
}

u32_t rc32k_autoset_freq(void)
{
    u32_t try_cnt, cur, delta, diff, count;
    u32_t best_freq_code, best_count, best_diff;

    best_diff = 0xffffffffU;
    best_count = 0;
    delta = (1 << 7);
    cur = delta;
    best_freq_code = cur;
    for (try_cnt = 0; try_cnt < 8; try_cnt++)
    {
        rc32k_set_clock_freq(cur);
        rc32k_calibrate();
        count = rc32k_measure(1);
        diff = rc32k_abs_diff(count);
        if (diff < best_diff)
        {
            best_diff = diff;
            best_freq_code = cur;
            best_count = count;
        }
        RC32K_PRINTF("freq code=%d count=%d\n", cur, count);
        if (count >= RC32K_UPPER_LIMIT_VALUE) //RC32K is too slow
        {
            cur &= ~delta;
            if (try_cnt == 7)
            {
                rc32k_set_clock_freq(cur);
                rc32k_calibrate();
                count = rc32k_measure(1);
                diff = rc32k_abs_diff(count);
                if (diff < best_diff)
                {
                    best_diff = diff;
                    best_freq_code = cur;
                    best_count = count;
                }
                RC32K_PRINTF("freq code=%d count=%d\n", cur, count);
            }
        }
        else if (count <= RC32K_LOWER_LIMIT_VALUE) //RC32K is too fast
        {
            //do nothing
        }
        else //RC32K is OK
        {
            break;
        }
        delta >>= 1;
        cur |= delta;
    }

    if (cur != best_freq_code)
    {
        cur = best_freq_code;
        rc32k_set_clock_freq(cur);
        rc32k_calibrate();
        count = best_count;
        RC32K_PRINTF("freq code=%d count=%d\n", cur, count);
    }

    return count;
}

void rc32k_autoset(u32_t mode, u32_t *freq_val, u32_t *bias_val)
{
    u16_t year = 2020, month = 1, day = 1, week = 3, hour = 0, min = 0, sec = 0;
    u32_t try_cnt, cur, delta, count, diff;
    u32_t best_bias_code, best_freq_code, best_count, best_diff;

    //try to auto set bias current, only do it once after power up
    rtc_set_date(year, month, day, week, hour, min, sec);

    rc32k_alarm(2); //to find starting line

    if (mode == RC32K_FREQ_FIRST)
    {
        best_diff = 0xffffffffU;
        best_count = 0;
        delta = (1 << 7);
        cur = delta;
        for (try_cnt = 0; try_cnt < 8; try_cnt++)
        {
            rc32k_set_clock_freq(cur);
            rc32k_calibrate();
            count = rc32k_autoset_bias();
            diff = rc32k_abs_diff(count);
            if (diff < best_diff)
            {
                best_diff = diff;
                best_count = count;
                rc32k_get_trim(&best_bias_code, &best_freq_code);
                RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
            }
            if (count >= RC32K_UPPER_LIMIT_VALUE) //RC32K is too slow
            {
                cur &= ~delta;
                if (try_cnt == 7)
                {
                    rc32k_set_clock_freq(cur);
                    rc32k_calibrate();
                    count = rc32k_autoset_bias();
                    diff = rc32k_abs_diff(count);
                    if (diff < best_diff)
                    {
                        best_diff = diff;
                        best_count = count;
                        rc32k_get_trim(&best_bias_code, &best_freq_code);
                        RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
                    }
                }
            }
            else if (count <= RC32K_LOWER_LIMIT_VALUE) //RC32K is too fast
            {
                //do nothing
            }
            else //RC32K is OK
            {
                break;
            }
            delta >>= 1;
            cur |= delta;
        }

        if (cur != best_freq_code)
        {
            rc32k_set_bias_current(best_bias_code);
            rc32k_set_clock_freq(best_freq_code);
            rc32k_calibrate();
            count = best_count;
            RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
        }
    }
    else if (mode == RC32K_BIAS_FIRST)
    {
        best_count = 0;
        best_diff = 0xffffffffU;
        rc32k_get_trim(&best_bias_code, &best_freq_code);
        delta = (1 << 2);
        cur = delta;
        for (try_cnt = 0; try_cnt < 3; try_cnt++)
        {
            rc32k_set_bias_current(cur);
            rc32k_calibrate();
            count = rc32k_autoset_freq();
            diff = rc32k_abs_diff(count);
            if (diff < best_diff)
            {
                best_diff = diff;
                best_count = count;
                rc32k_get_trim(&best_bias_code, &best_freq_code);
                RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
            }
            if (count >= RC32K_UPPER_LIMIT_VALUE) //RC32K is too slow
            {
                cur &= ~delta;
                if (try_cnt == 2)
                {
                    rc32k_set_bias_current(cur);
                    rc32k_calibrate();
                    count = rc32k_autoset_freq();
                    diff = rc32k_abs_diff(count);
                    if (diff < best_diff)
                    {
                        best_diff = diff;
                        best_count = count;
                        rc32k_get_trim(&best_bias_code, &best_freq_code);
                        RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
                    }
                }
            }
            else if (count <= RC32K_LOWER_LIMIT_VALUE) //RC32K is too fast
            {
                //do nothing
            }
            else //RC32K is OK
            {
                break;
            }
            delta >>= 1;
            cur |= delta;
        }

        if (cur != best_bias_code)
        {
            rc32k_set_bias_current(best_bias_code);
            rc32k_set_clock_freq(best_freq_code);
            rc32k_calibrate();
            count = best_count;
            RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
        }
    }
    if (freq_val != NULL)
    {
        *freq_val = best_freq_code;
    }
    if (bias_val != NULL)
    {
        *bias_val = best_bias_code;
    }
    //RC32K_PRINTF("best_bias_code=%d best_freq_code=%d count=%d\n", best_bias_code, best_freq_code, count);
}

void rc32k_init(u8_t auto_calib, u32_t *freq_val, u32_t *bias_val)
{
    if (auto_calib)
    {
        //rc32k_autoset(RC32K_FREQ_FIRST);
        rc32k_autoset(RC32K_BIAS_FIRST, freq_val, bias_val);
    }
    else
    {
        //rc32k_set_clock_freq(0x80);
        //rc32k_set_bias_current(1);//set bias current code manually
        rc32k_set_clock_freq(*freq_val);
        rc32k_set_bias_current(*bias_val); //set bias current code manually
        rc32k_calibrate();
    }
}
void rtc_init()
{
    db.alm_mode = 0;
    db.alm_length = 0;
    db.alm_timer_type = 0;
    db.rtc_reload = 0;
    db.rtc_wokenUp_callback = NULL;
    return;
}

void rtc_power(RTC_POWER_STATE power)
{
    volatile unsigned int *set_timer_control = pRtcTimerCtrl0;
    if (RTC_POWER_OFF == power)
        *set_timer_control |= (1 << 0);
    else
        *set_timer_control &= (~(1 << 0));
}

/**
 * @brief this function can only be called in the case that MCU exit out of sleeping when RTC time expiry.
 */
#if 0
void    rtc_one_running()
{
    TRACE_PRINTF("%s line %d\n", __FUNCTION__, __LINE__);
    volatile unsigned int * set_alm_control = pRtcAlmCtrl;
    if(!db.rtc_reload)
    {
        db.rtc_reload = 1;
        SET_RTC_WAKEN_UP_FLAG();
        rtc_recover_cb_pointer();
        rtc_recover_alm_timeLength();
        GET_RTC_MODE(db.alm_mode);
        GET_RTC_TYPE(db.alm_timer_type);
        //reset alm time here...
        if(db.alm_timer_type == RTC_TYPE_INTERVAL)
        {
            if(db.alm_mode == RTC_ALM_MODE_INF)
            {
                rtc_set_almLength(db.alm_length, NULL);//set the time when nexttime to wake up.
                //mask keep the same as the previous.
                //pRtcAlmCtrl->alm_en = 1;
                *set_alm_control |= (1 << 8);
            }
            else //no need to alm anymore.
            {
                //pRtcAlmCtrl->alm_en = 0;
                *set_alm_control &=(~(1 << 8));
            }
        }
        //now we can start the cb.(the cb can reset/reconf the alm setting depending on the app implementation.)
        if(db.rtc_wokenUp_callback)
            db.rtc_wokenUp_callback();

    }
}

void    rtc_storing_cb_pointer()
{
    s32_t func = (int)db.rtc_wokenUp_callback;
    s32_t func_h29 = (func >> 3);
    s16_t func_l3  = (func & 0x7);
    SET_REG(CB_FUNC_29,31,3,func_h29); //storing h29bit to 3~31bit of CB_FUNC_29
    SET_REG(CB_FUNC_3, 11,9,func_l3);  //storing L3bit  to 9~11bit of CB_FUNC_3

    SET_CB_FUNC_FLAG(UC_ENABLE);
    //SET_CB_FUNC_FLAG_TEST(UC_ENABLE);

    int a = 0;
    GET_CB_FUNC_FLAG(a);
    //GET_CB_FUNC_FLAG_TEST(a);
    TRACE_PRINTF("GET_CB_FUNC_FLAG(UC_ENABLE)=%d\n", a);
    return;
}
void    rtc_recover_cb_pointer()
{
    s32_t func;
    s32_t func_h29;
    s16_t func_l3;
    u32_t func_flag;

    GET_CB_FUNC_FLAG(func_flag);
    //GET_CB_FUNC_FLAG_TEST(func_flag);

    TRACE_PRINTF("%s line %d: func_flag=%d\n", __FUNCTION__, __LINE__, func_flag);
    //if(func_flag)
    {
        GET_REG(CB_FUNC_29,31,3,func_h29);
        GET_REG(CB_FUNC_3, 11,9,func_l3);
        func = (((func_h29 << 3) & (0xffffffff << 3)) | (func_l3 & 0x7));
        db.rtc_wokenUp_callback = (rtc_cb_func)func;
        TRACE_PRINTF("%s line %d, function=0x%x\n", __FUNCTION__, __LINE__, db.rtc_wokenUp_callback);
    }

    return;
}

void    rtc_storing_alm_timeLength()
{
    u32_t len = db.alm_length;
    u32_t len_h9 = (len >> 23);
    u32_t len_l23  = (len & 0x7fffff);
    SET_REG(RTC_LEN_9,  31, 23,len_h9);
    SET_REG(RTC_LEN_23, 31, 9,len_l23);
    return;
}
void    rtc_recover_alm_timeLength()
{
    u32_t len_h9 = 0;
    u32_t len_l23 = 0;
    GET_REG(RTC_LEN_9,  31, 23,len_h9);
    GET_REG(RTC_LEN_23, 31, 9,len_l23);
    db.alm_length = ((len_h9 << 23) | len_l23);
    return;

}
#endif
void rtc_set_date(u16_t year, u16_t month, u16_t day, u16_t week, u16_t hour, u16_t min, u16_t sec)
{
    volatile unsigned int *set_date_address = pRtcTimerSet0;
    volatile unsigned int *set_year_address = pRtcTimerSet1;
    volatile unsigned int *set_timer_control = pRtcTimerCtrl0;

    if (year >= 2000 && year < 2099)
        year -= 2000;
    *set_date_address = (sec & 0x3f) | ((min & 0x3f) << 8) | ((hour & 0x1f) << 16);
    //pRtcTimerSet0->set_sec  = sec;
    //pRtcTimerSet0->set_min  = min;
    //pRtcTimerSet0->set_hour = hour;

    //pRtcTimerSet1->set_day  = day;
    //pRtcTimerSet1->set_mon  = month;
    //pRtcTimerSet1->set_week = week;
    //pRtcTimerSet1->set_year = year;
    *set_year_address = (((year)&0x7f) << 16) | ((month & 0xf) << 12) | ((day & 0x1f) << 4) | (week & 0x7);

    //set ctrl0 to update the date of RTC...
    //pRtcTimerCtrl0->tsr     = 1;
    *set_timer_control |= (1 << 1);
}

void rtc_get_date(u16_t *year, u16_t *month, u16_t *day, u16_t *week, u16_t *hour, u16_t *min, u16_t *sec)
{
    volatile unsigned int *set_current_date = pRtcTimer0;
    volatile unsigned int *set_current_year = pRtcTimer1;
    volatile unsigned int *set_timer_control = pRtcTimerCtrl0;

    *set_timer_control |= (1 << 2);
    while ((*set_timer_control & 0x4) != 0) //wait until the timer is lock complete.
    {
        asm("nop");
    }
    *year = ((*set_current_year >> 16) & 0x7f) + 2000;
    //TRACE_PRINTF("====> *year=%d\n", *year);
    *month = (*set_current_year >> 12) & 0xf;
    *day = (*set_current_year >> 4) & 0x1f;
    *week = (*set_current_year) & 0x7;

    *hour = (*set_current_date >> 16) & 0x1f;
    *min = (*set_current_date >> 8) & 0x3f;
    *sec = (*set_current_date) & 0x3f;
}
#if 0
/**
 * @brief
 * @param alm_mod:  1: one time, 2: inf.
 * @param alm_mask: see rtc_alm_ctrl for details.
 */
void   rtc_set_alm_attr(u8_t alm_mod, u8_t alm_type, u8_t alm_mask,rtc_cb_func cb)
{
    db.alm_mask = alm_mask; // interrupt enable mask
    db.alm_mode = alm_mod;
    db.alm_timer_type = alm_type;


    TRACE_PRINTF("%s line %d, cb=0x%x\n", __FUNCTION__, __LINE__, cb);
    db.rtc_wokenUp_callback = cb;

    //save to reg for recoverying...
    SET_RTC_MODE(alm_mod);
    SET_RTC_TYPE(alm_type);
    rtc_storing_cb_pointer();

    return;
}
void   rtc_clr_alm_setting()
{
    volatile unsigned int * set_alm_control = pRtcAlmCtrl;
    //pRtcAlmCtrl->alm_en = 0; //no alm.
    *set_alm_control &= (~(1 << 8));
    SET_RTC_TYPE(RTC_TIMER_NULL);
    SET_RTC_MODE(RTC_ALM_MODE_NULL);
    return;
}
#endif
void rtc_get_almTime(u16_t *year, u16_t *month, u16_t *day, u16_t *week, u16_t *hour, u16_t *min, u16_t *sec)
{
    volatile unsigned int *set_date_address = pRtcTimerAlm0;
    volatile unsigned int *set_year_address = pRtcTimerAlm1;

    *year = ((*set_year_address >> 16) & 0x7f) + 2000;
    *month = ((*set_year_address >> 12) & 0xf);
    *day = ((*set_year_address >> 4) & 0x1f);
    *week = ((*set_year_address) & 0x7);

    *hour = ((*set_date_address >> 16) & 0x1f);
    *min = ((*set_date_address >> 8) & 0x3f);
    *sec = ((*set_date_address) & 0x3f);
}

void rtc_set_almTime(u16_t year, u16_t month, u16_t day, u16_t week, u16_t hour, u16_t min, u16_t sec, rtc_cb_func cb)
{
    volatile unsigned int *set_date_address = pRtcTimerAlm0;
    volatile unsigned int *set_year_address = pRtcTimerAlm1;
    volatile unsigned int *set_alm_ctrl = pRtcAlmCtrl;

    /*0 - 99*/
    if (year >= 2000)
        year -= 2000;

    //pRtcTimerAlm0->alm_sec  = sec;
    //pRtcTimerAlm0->alm_min  = min;
    //pRtcTimerAlm0->alm_hour = hour;

    *set_date_address = (((hour & 0x1f) << 16) | ((min & 0x3f) << 8) | ((sec & 0x3f)));

    //pRtcTimerAlm1->alm_day  = day;
    //pRtcTimerAlm1->alm_week = week;
    //pRtcTimerAlm1->alm_mon  = month;
    //pRtcTimerAlm1->alm_year = year;
    *set_year_address = (((year & 0x3f) << 16) | ((month & 0xf) << 12) | ((day & 0x1f) << 4) | (week & 0x7));

    //alm eable
    *set_alm_ctrl |= ((1 << 8) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3));

    rtc_start_alm(cb);
}

int is_leap_year(int year)
{
    if ((year & 0x03) == 0) //simplify for '2000~2099'
    {
        return 1; //true
    }

    return 0; //false
}

int get_month_day(int year, int mon)
{
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (is_leap_year(year) && (mon == 2))
    {
        return 29;
    }
    else
    {
        return days[mon - 1];
    }
}

int interval2date(int interval, rtc_date_t *date)
{
    int i;
    int delta;
    int total = 0;

    date->wday = (date->wday + ((interval + date->hour * SECOND_PER_HOUR + date->min * SECOND_PER_MINUTE + date->sec) / SECOND_PER_DAY)) % 7;

    for (i = 1; i < date->mon; i++)
    {
        total += get_month_day(date->year, i) * SECOND_PER_DAY;
    }

    total += (date->day - 1) * SECOND_PER_DAY + date->hour * SECOND_PER_HOUR + date->min * SECOND_PER_MINUTE + date->sec;
    interval += total;
    //calculate from xxxx-01-01 00:00:00, 'xxxx' is current year
    date->mon = 1;
    date->day = 1;
    date->hour = 0;
    date->min = 0;
    date->sec = 0;

    while (interval > 0)
    {
        delta = is_leap_year(date->year) ? SECOND_PER_LEAP_YEAR : SECOND_PER_NORMAL_YEAR;
        if (interval >= delta)
        {
            interval -= delta;
            date->year++;
        }
        else
        {
            break;
        }
    }

    for (i = 1; i < 12; i++)
    {
        delta = get_month_day(date->year, i) * SECOND_PER_DAY;
        if (interval >= delta)
        {
            interval -= delta;
            date->mon++;
        }
        else
        {
            break;
        }
    }

    date->day += interval / SECOND_PER_DAY;
    interval = interval % SECOND_PER_DAY;

    date->hour += interval / SECOND_PER_HOUR;
    interval = interval % SECOND_PER_HOUR;

    date->min += interval / SECOND_PER_MINUTE;
    date->sec += interval % SECOND_PER_MINUTE;

    if (date->wday == 0)
    {
        date->wday = 7; //due to rtc hardware
    }

    return 0;
}

void rtc_set_alarm_interval(u32_t seconds)
{
    rtc_date_t date;

    if (seconds == 0)
    {
        return;
    }

    rtc_get_date(&date.year, &date.mon, &date.day, &date.wday, &date.hour, &date.min, &date.sec);
    interval2date(seconds, &date);
    rtc_set_almTime(date.year, date.mon, date.day, date.wday, date.hour, date.min, date.sec, NULL);

    return;
}

void rtc_set_almLength(u32_t secLength, rtc_cb_func cb)
{
    //volatile unsigned int *set_alarm_date = pRtcTimerAlm0;
    //volatile unsigned int *set_alarm_year = pRtcTimerAlm1;

    u16_t DaysOfMon[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    u8_t leapYearFlag = 0;
    u16_t currentYear = 0;
    u16_t currentMont = 0, currentDate = 0, currentWeek = 0;
    u16_t currentH = 0, currentM = 0, currentS = 0;
    int addY = 0, addMo = 0, addD = 0, /*addW = 0,*/ addH = 0, addM = 0, addS = 0;
    int almY = 0, almMo = 0, almD = 0, almW = 0, almH = 0, almM = 0, almS = 0;
    u8_t idx = 0;
    u32_t nextSecLength = 0, tmp = 0;

    if (!secLength)
        return;

    db.alm_length = secLength;

    rtc_get_date(&currentYear, &currentMont, &currentDate, &currentWeek, &currentH, &currentM, &currentS);
    //TRACE_PRINTF("currentYear=%d, currentMont=%d, currentDate=%d, currentWeek=%d, currentH=%d, currentM=%d, currentS=%d\n", currentYear, currentMont, currentDate, currentWeek, currentH, currentM, currentS);
    leapYearFlag = (currentYear % 400 == 0 || (currentYear % 4 == 0 && currentYear % 100 != 0));
    if (leapYearFlag)
        DaysOfMon[1] = 29;

    //TRACE_PRINTF("currentYear = %d, leapYearFlag=%d, secLength=%d\n", currentYear, leapYearFlag, secLength);

    addY = secLength / ((365 + leapYearFlag) * 24 * 60 * 60);

    nextSecLength = (secLength % ((365 + leapYearFlag) * 24 * 60 * 60));
    //TRACE_PRINTF("nextSecLength=%d, currentDate=%d\n", nextSecLength, currentDate);

    tmp = currentDate;
    for (idx = currentMont - 1; idx < 12; idx++)
    {
        if (nextSecLength > (DaysOfMon[idx] - tmp) * 24 * 60 * 60)
        {
            addMo += 1;
            nextSecLength -= (DaysOfMon[idx] - tmp) * 24 * 60 * 60;
            tmp = 0;
        }
        else
        {
            break;
        }
    }
    //TRACE_PRINTF("line = %d nextSecLength=%d\n", __LINE__, nextSecLength);

    if (nextSecLength > 24 * 60 * 60)
    {
        addD += (nextSecLength / (24 * 60 * 60));
        nextSecLength = nextSecLength % (24 * 60 * 60);
    }
    //TRACE_PRINTF("line = %d nextSecLength=%d\n", __LINE__, nextSecLength);

    if (nextSecLength > 60 * 60)
    {
        addH += (nextSecLength / (60 * 60));
        nextSecLength = (nextSecLength % (60 * 60));
    }
    //TRACE_PRINTF("line = %d nextSecLength=%d\n", __LINE__, nextSecLength);

    if (nextSecLength > 60)
    {
        addM += (nextSecLength / 60);
    }
    addS = (nextSecLength % 60);
    //TRACE_PRINTF("addY = %d, addMo = %d, addD = %d, addW = %d, addH = %d, addM = %d, addS = %d, nextSecLength=%d\n", addY, addMo, addD, addW , addH, addM, addS, nextSecLength);

    if (currentS + addS >= 60)
        almM += 1;
    almS = (currentS + addS) % 60;

    if (almM + addM + currentM >= 60)
        almH += 1;
    almM = (almM + addM + currentM) % 60;

    if (almH + addH + currentH >= 24)
        almD += 1;
    almH = (almH + addH + currentH) % 24;

    if (DaysOfMon[currentMont - 1] < currentDate + almD + addD)
        almMo += 1;
    almD = (currentDate + almD + addD) % (DaysOfMon[currentMont - 1]);

    if (addMo + currentMont + almMo >= 12)
        almY += 1;
    almMo = (addMo + currentMont + almMo) % 12;

    almY += (addY + currentYear);

    almW = (almD + 2 * (almMo + 1) + 3 * (almMo + 1) / 5 + almY + almY / 4 + almY / 100 + almY / 400) % 7 + 1;

    rtc_set_almTime(almY, almMo, almD, almW, almH, almM, almS, cb);
    return;
}

void rtc_start_alm(rtc_cb_func cb)
{
    volatile unsigned int *set_alm_control = pRtcAlmCtrl;
#if 0
    if(db.alm_timer_type == RTC_TYPE_INTERVAL) //for interval type, we should enable all.
    {
        db.alm_mask = 0x7f;
    }

    * set_alm_control = db.alm_mask; //save mask.
#endif
    rtc_interrupt_callback = cb;
    if (NULL == rtc_interrupt_callback)
        RC32K_PRINTF("%s line %d: rtc_interrupt_callback is null\n", __FUNCTION__, __LINE__);
    //pRtcAlmCtrl->alm_en = 1;
    *set_alm_control |= (1 << 8);
    IER |= 1 << 0;
    return;
}
