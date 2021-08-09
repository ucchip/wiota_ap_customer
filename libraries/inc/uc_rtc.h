#ifndef     _UC_RTC_H_
#define     _UC_RTC_H_
/*
 * REM: the following rule MUST be followed:
 * 1:   RTC Timer&ALM ONLY used in Sleep-MODE.
 * 2:   only one RTC ALM VALUE is ACTIVE.
 * 3:   no RTC IRQ can be triggered during normal mode.
 * 4:   for timer requirement, use OS or timerB instead of RTC.
 * */

//#include "rtc_reg.h"
//#include "uctypes.h"


typedef enum 
{
    RTC_POWER_OFF = 0,
    RTC_POWER_ON,
}RTC_POWER_STATE;

void rtc_power(RTC_POWER_STATE power);

typedef void   (*rtc_cb_func)();

#if 0
/*Macro for usage*/
#define     SET_RTC_WAKEN_UP_FLAG()     SET_REG(RTC_POWER,3,3,1) //this field is used to check whether the boot is an rtc woken up 
                                                                  //or first time powerOn
#define     GET_RTC_WAKEN_UP_FLAG(v)    GET_REG(RTC_POWER,3,3,v)

#define     SET_CB_FUNC_FLAG(v)         SET_REG(CB_FUNC_FLAG,3,3,v)
#define     GET_CB_FUNC_FLAG(v)         GET_REG(CB_FUNC_FLAG,3,3,v)

//#define     SET_CB_FUNC_FLAG_TEST(v)         SET_REG(REG17,31,31,v)
//#define     GET_CB_FUNC_FLAG_TEST(v)         GET_REG(REG17,31,31,v)


#define     SET_RTC_MODE(v)             SET_REG(RTC_MODE,13,12,v)
#define     GET_RTC_MODE(v)             GET_REG(RTC_MODE,13,12,v)

#define     SET_RTC_TYPE(v)             SET_REG(RTC_TYPE,31,30,v)
#define     GET_RTC_TYPE(v)             GET_REG(RTC_TYPE,31,30,v)

#define     SET_RTC_LPM_STATUS(v)       SET_REG(RTC_EVENT,7,4,v)    
#define     GET_RTC_LPM_STATUS(v)       GET_REG(RTC_EVENT,7,4,v)
#define     CLR_RTC_LPM_STATUS()        SET_REG(RTC_EVENT,7,4,0)  /*default value is Zero*/  
#else
typedef uint8_t   u8_t;
typedef int8_t    s8_t;
typedef uint16_t  u16_t;
typedef int16_t   s16_t;
typedef uint32_t  u32_t;
typedef int32_t   s32_t;
#endif
////////////////////////////////////////////////////////////////////////////////////////////////
//RTC TIMER PART
////////////////////////////////////////////////////////////////////////////////////////////////
enum
{
    nSECS_L_YEAR      =   31622400UL, /*Leap year*/
    nSECS_N_YEAR      =   31536000UL, /*normal year*/
    nSECS_31_DAY      =   2678400UL,
    nSECS_30_DAY      =   2592000UL,
    nSECS_29_DAY      =   2505600UL,
    nSECS_28_DAY      =   2419200UL,
    nSECS_PER_DAY     =   86400UL,
    nSECS_PER_HOUR    =   3600UL,
    nSECS_PER_MIN     =   60UL,
};


#define     pRtcTimerCtrl0      (volatile unsigned int *)(EVENT_UNIT_BASE_ADDR + REG_RTC_CTRL)

#define     pRtcTimer0          (volatile unsigned int *)(EVENT_UNIT_BASE_ADDR + REG_RTC_TIME0)
#define     pRtcTimer1          (volatile unsigned int *)(EVENT_UNIT_BASE_ADDR + REG_RTC_TIME1)

#define     pRtcTimerSet0       (volatile unsigned int *)(EVENT_UNIT_BASE_ADDR + REG_RTC_TSET0)
#define     pRtcTimerSet1       (volatile unsigned int *)(EVENT_UNIT_BASE_ADDR + REG_RTC_TSET1)

#define     pRtcTimerAlm0       (volatile unsigned int *)(EVENT_UNIT_BASE_ADDR + REG_RTC_ASET0)
#define     pRtcTimerAlm1       (volatile unsigned int *)(EVENT_UNIT_BASE_ADDR + REG_RTC_ASET1)
#define     pRtcAlmCtrl         (volatile unsigned int *)(EVENT_UNIT_BASE_ADDR + REG_RTC_ACTRL)

#define SECOND_PER_MINUTE	(60UL)
#define SECOND_PER_HOUR		(60UL*SECOND_PER_MINUTE)
#define SECOND_PER_DAY		(24UL*SECOND_PER_HOUR)
#define SECOND_PER_NORMAL_YEAR		(365UL*SECOND_PER_DAY)
#define SECOND_PER_LEAP_YEAR		(366UL*SECOND_PER_DAY)

//global variables.

typedef struct
{
    unsigned char    alm_mode;       // null, one, inf.
    unsigned char   alm_timer_type; // null,interval,fixed time.
                            // together with alm_mode: we have these alm types:
                            // 1. one alm with a specified interval.
                            // 2. periodic alm and the interval is fixed.
                            // 3. one alm with a speficed time&mask.
                            // 4. periodic almo with a specified time&mask.
    unsigned char     alm_mask;
    unsigned char     rtc_reload;
    
    unsigned int    alm_length;     //only valid if alm_timer_type = interval;
    rtc_cb_func rtc_wokenUp_callback;
}rtc_alm_db_t;

typedef struct
{
	u16_t year;
	u16_t mon;
	u16_t day;
	u16_t wday;
	u16_t hour;
	u16_t min;
	u16_t sec;
}rtc_date_t;

typedef struct
{
	unsigned waken_up_flag:1;
	unsigned cb_func_flag:1;
	unsigned mode:2;
	unsigned type:2;
	unsigned event:4;
	u32_t cb_func;
	u32_t alm_len;
}rrb_content_t;

extern void rc32k_set_bias_current(u32_t current_code);
extern void rc32k_calibrate(void);
extern void rc32k_init(u8_t auto_calib, u32_t *freq_val, u32_t *bias_val);

void    rtc_init();
#if 0
//void    rtc_one_running();
void   rtc_storing_cb_pointer();
void    rtc_recover_cb_pointer();
void    rtc_storing_alm_timeLength();
void    rtc_recover_alm_timeLength();
#endif
void    rtc_set_date(u16_t year, u16_t month, u16_t day, u16_t week,u16_t hour, u16_t min, u16_t sec);
void    rtc_get_date(u16_t *year, u16_t *month, u16_t *day, u16_t *week, u16_t *hour, u16_t *min, u16_t *sec);
#if 0
void   rtc_set_alm_attr(u8_t alm_mod, u8_t alm_type, u8_t alm_mask,rtc_cb_func cb);
void   rtc_clr_alm_setting();
#endif
void   rtc_set_almTime(u16_t year, u16_t month, u16_t day, u16_t week,u16_t hour, u16_t min, u16_t sec, rtc_cb_func cb);
void   rtc_get_almTime(u16_t *year, u16_t *month, u16_t *day, u16_t *week,u16_t *hour, u16_t *min, u16_t *sec);
void   rtc_set_almLength(u32_t secLength, rtc_cb_func cb);
void rtc_start_alm(rtc_cb_func cb);
//boolean rtc_cmp_cur_alm();

extern void rtc_set_alarm_interval(u32_t seconds);

#endif
