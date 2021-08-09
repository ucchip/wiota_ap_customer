
#include "board.h"
#include<rtthread.h>
#include<rtdevice.h>

#ifdef RT_USING_RTC

#include "uc_rtc.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.rtc"
#include <drv_log.h>

static struct rt_device rtc;

static time_t get_rtc_timestamp(void)
{
    struct tm tm_new;
    uint16_t year = 0;
    uint16_t month = 0;
    uint16_t day = 0;
    uint16_t week = 0;
    uint16_t hour = 0;
    uint16_t min = 0;
    uint16_t sec = 0;
    
    rtc_get_date(&year, &month, &day, &week, &hour, &min, &sec);

    tm_new.tm_sec  = sec;
    tm_new.tm_min  = min;
    tm_new.tm_hour = hour;
    tm_new.tm_mday = day;
    tm_new.tm_mon  = month - 1;
    tm_new.tm_year = year - 1900;
    tm_new.tm_wday  = week - 1;

    LOG_D("get rtc time.");
    return mktime(&tm_new);
}

static rt_err_t set_rtc_time_stamp(time_t time_stamp)
{
    struct tm *p_tm;

    p_tm = localtime(&time_stamp);
    if (p_tm->tm_year < 100)
    {
        LOG_D("set rtc time. tm_year Err!");
        return -RT_ERROR;
    }
    
    LOG_D("set rtc time.");
    rtc_set_date(p_tm->tm_year - 100, p_tm->tm_mon + 1, p_tm->tm_mday, p_tm->tm_wday + 1,
        p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
    
    return RT_EOK;
}

static void rt_rtc_init(void)
{
    //rc32k_init(1, NULL, NULL);
}

void rtc_calibrate(uint8_t auto_calib, uint32_t *freq_val, uint32_t *bias_val)
{
    rc32k_init(auto_calib, freq_val, bias_val);
}

static rt_err_t rt_rtc_config(struct rt_device *dev)
{
    uint16_t year = 0;
    uint16_t month = 0;
    uint16_t day = 0;
    uint16_t week = 0;
    uint16_t hour = 0;
    uint16_t min = 0;
    uint16_t sec = 0;
    
    rtc_get_date(&year, &month, &day, &week, &hour, &min, &sec);
    if ((year > 2099)
        || (month > 12)
        || (day > 31)
        || (week > 7)
        || (hour > 23)
        || (min > 59)
        || (sec > 59))
    {
        //rt_kprintf("year = %d, month = %d, day = %d, week = %d, hour = %d, min = %d, sec = %d\r\n", 
        //    year, month, day, week, hour, min, sec);
        rtc_set_date(0x14, 0x06, 0x01, 0x01, 0x0c, 0x0, 0x0);
    }
    
    return RT_EOK;
}

static rt_err_t rt_rtc_control(rt_device_t dev, int cmd, void *args)
{
    rt_err_t result = RT_EOK;
    RT_ASSERT(dev != RT_NULL);
    switch (cmd)
    {
    case RT_DEVICE_CTRL_RTC_GET_TIME:
        *(rt_uint32_t *)args = get_rtc_timestamp();
        LOG_D("RTC: get rtc_time %d\n", *(rt_uint32_t *)args);
        break;

    case RT_DEVICE_CTRL_RTC_SET_TIME:
        if (set_rtc_time_stamp(*(rt_uint32_t *)args))
        {
            result = -RT_ERROR;
        }
        LOG_D("RTC: set rtc_time %d\n", *(rt_uint32_t *)args);
        break;
    }

    return result;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops rtc_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    rt_rtc_control
};
#endif

static rt_err_t rt_hw_rtc_register(rt_device_t device, const char *name, rt_uint32_t flag)
{
    RT_ASSERT(device != RT_NULL);

    rt_rtc_init();
    if (rt_rtc_config(device) != RT_EOK)
    {
        return -RT_ERROR;
    }
#ifdef RT_USING_DEVICE_OPS
    device->ops         = &rtc_ops;
#else
    device->init        = RT_NULL;
    device->open        = RT_NULL;
    device->close       = RT_NULL;
    device->read        = RT_NULL;
    device->write       = RT_NULL;
    device->control     = rt_rtc_control;
#endif
    device->type        = RT_Device_Class_RTC;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;
    device->user_data   = RT_NULL;

    /* register a character device */
    return rt_device_register(device, name, flag);
}

int rt_hw_rtc_init(void)
{
    rt_err_t result;
    result = rt_hw_rtc_register(&rtc, "rtc", RT_DEVICE_FLAG_RDWR);
    if (result != RT_EOK)
    {
        LOG_E("rtc register err code: %d", result);
        return result;
    }
    LOG_D("rtc init success");
    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_rtc_init);

#endif /* BSP_USING_ONCHIP_RTC */
