// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

/**
 * @file
 * @brief Timer library.
 *
 * Provides Timer function like writing the appropriate
 * timer registers and uttility functions to cycle count
 * certain events. Used in bench.h.
 *
 * @author Florian Zaruba
 *
 * @version 1.0
 *
 * @date 2/10/2015
 *
 */
#ifndef __UC_TIMER_H__
#define __UC_TIMER_H__

#include "pulpino.h"


#define TIMER_ENABLE_MASK 0x1

typedef struct{
    uint8_t             Prescaler;
    uint32_t            Compare_Value;
    uint32_t            Count;
}TIMER_CFG_Type;


#define PARAM_TIMER(TIMERx)                         ((TIMERx==UC_TIMER0)||(TIMERx==UC_TIMER1))

#define PARAM_TIMER_STATE(NewState)                 ((NewState==ENABLE)||(NewState==DISABLE))

#define PARAM_TIMER_COMPAREVALUE(Compare_Value)     ((Compare_Value>0)&&(Compare_Value<0xffffffff))

#define PARAM__TIMER_PRESCALER(Prescaler)           ((Prescaler>0)&&(Prescaler<0xf))

#define PARAM_TIMER_COUNT(Time_Count)     ((Time_Count>0)&&(Time_Count<0xffffffff))

void TIMER_Init(TIMER_TYPE *TIMERx, TIMER_CFG_Type *TIMERx_ConfigStruct);

void TIMER_Cmd(TIMER_TYPE *TIMERx, FunctionalState NewState);

uint32_t TIMER_GetCount(TIMER_TYPE *TIMERx);

void TIMER_Set_Count(TIMER_TYPE *TIMERx, uint32_t count);

void TIMER_Set_Compare_Value(TIMER_TYPE *TIMERx, uint32_t value);

#endif
