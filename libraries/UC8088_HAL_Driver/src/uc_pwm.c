#include "uc_pwm.h"

void pwm_enable(PWM_TypeDef* PWM)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    PWM->CTRL |= (1<<0);
}

void pwm_disable(PWM_TypeDef* PWM)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    PWM->CTRL &= ~(1<<0);
}

void pwm_set_period(PWM_TypeDef* PWM, int period_cnt)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    PWM->CNTMAX = period_cnt;
}

void pwm_set_duty(PWM_TypeDef* PWM, int duty_cnt)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    int max_cnt = PWM->CNTMAX;
    if(duty_cnt > max_cnt)
        PWM->DUTY = max_cnt;
    else
        PWM->DUTY = duty_cnt;
}

int pwm_get_period(PWM_TypeDef* PWM)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    return PWM->CNTMAX;
}

int pwm_get_duty(PWM_TypeDef* PWM)
{
    CHECK_PARAM(PARAM_PWM(PWM));

    return PWM->DUTY;
}