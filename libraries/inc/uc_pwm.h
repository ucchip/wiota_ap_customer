#ifndef _UC_PWM_H_
#define _UC_PWM_H_

#include "pulpino.h"

#define PARAM_PWM(pwm)    (pwm==UC_PWM)

void pwm_enable(PWM_TypeDef* PWM);
void pwm_disable(PWM_TypeDef* PWM);
void set_pwm_cnt_max(PWM_TypeDef* PWM, int max_cnt);
void set_pwm_duty(PWM_TypeDef* PWM, int duty_cnt);

#endif