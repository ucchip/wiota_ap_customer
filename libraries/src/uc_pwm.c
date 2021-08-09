#include "uc_pwm.h"

void pwm_enable(PWM_TypeDef* PWM)
{
    CHECK_PARAM(PARAM_PWM(PWM));
    
    PWM->CTRL |= 1;
}

void pwm_disable(PWM_TypeDef* PWM)
{
    CHECK_PARAM(PARAM_PWM(PWM));
    
    PWM->CTRL |= 1;
}

void set_pwm_cnt_max(PWM_TypeDef* PWM, int max_cnt)
{
    CHECK_PARAM(PARAM_PWM(PWM));
    
    PWM->CNTMAX = max_cnt;
}

void set_pwm_duty(PWM_TypeDef* PWM, int duty_cnt)
{
    CHECK_PARAM(PARAM_PWM(PWM));
    
    int max_cnt = PWM->CNTMAX;
    if(duty_cnt > max_cnt)
        PWM->DUTY = max_cnt;
    else
        PWM->DUTY = duty_cnt;
}