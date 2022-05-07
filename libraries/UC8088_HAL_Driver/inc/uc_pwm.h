#ifndef _UC_PWM_H_
#define _UC_PWM_H_

#include "pulpino.h"

#define PARAM_PWM(pwm)    (pwm==UC_PWM)

void pwm_enable(PWM_TypeDef* PWM);
void pwm_disable(PWM_TypeDef* PWM);
void pwm_set_period(PWM_TypeDef* PWM, int period_cnt);
void pwm_set_duty(PWM_TypeDef* PWM, int duty_cnt);
int pwm_get_period(PWM_TypeDef* PWM);
int pwm_get_duty(PWM_TypeDef* PWM);
#endif