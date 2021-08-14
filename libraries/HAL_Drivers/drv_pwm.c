
#include <board.h>
#include<rtthread.h>
#include<rtdevice.h>

#ifdef RT_USING_PWM
//#include "drv_config.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.pwm"
#include <drv_log.h>

#include "uc_pwm.h"


struct rt_device_pwm pwm_device;
static uint32_t pwm_count = 0;
static uint32_t pwm_duty = 0;

static rt_err_t drv_pwm_control(struct rt_device_pwm *device, int cmd, void *arg)
{
    rt_err_t result = RT_EOK;
    struct rt_pwm_configuration *configuration = (struct rt_pwm_configuration *)arg;

    switch (cmd)
    {
    case PWM_CMD_ENABLE:        
        pwm_enable(UC_PWM);
        break;
    
    case PWM_CMD_DISABLE:    
        pwm_disable(UC_PWM);
        break;
    
    case PWM_CMD_SET:
        pwm_count = configuration->period;
        pwm_duty = configuration->pulse;
        set_pwm_cnt_max(UC_PWM, pwm_count);
        set_pwm_duty(UC_PWM, pwm_duty);
        break;
        
    case PWM_CMD_GET:
        configuration->period = pwm_count;
        configuration->pulse = pwm_duty;
        break;
        
    default:
        result = RT_EINVAL;
        break;
    }

    return result;
}

static struct rt_pwm_ops drv_ops =
{
    drv_pwm_control
};

int rt_hw_pwm_init(void)
{    
    rt_err_t result;
    result = rt_device_pwm_register(&pwm_device, "pwm", &drv_ops, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("pwm register err code: %d", result);
        return result;
    }
    LOG_D("pwm init success");
    return RT_EOK;
}
//INIT_DEVICE_EXPORT(rt_hw_pwm_init);
#endif /* RT_USING_PWM */

