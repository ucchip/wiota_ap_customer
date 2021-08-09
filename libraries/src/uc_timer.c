// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "uc_timer.h"


void TIMER_Init(TIMER_TYPE *TIMERx, TIMER_CFG_Type *TIMERx_ConfigStruct)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));
    CHECK_PARAM(PARAM_TIMER_COMPAREVALUE(TIMERx_ConfigStruct->Compare_Value));
    CHECK_PARAM(PARAM__TIMER_PRESCALER(TIMERx_ConfigStruct->Prescaler));
    
    TIMERx->CTR |= (TIMERx_ConfigStruct->Prescaler << 3);
    
    TIMERx->CMP = TIMERx_ConfigStruct->Compare_Value;
    
    TIMERx->TRR = TIMERx_ConfigStruct->Count;
    /* enable timer */
    
    TIMERx->CTR |= (TIMER_ENABLE_MASK);
    
}

void TIMER_Cmd(TIMER_TYPE *TIMERx, FunctionalState NewState)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));
    CHECK_PARAM(PARAM_TIMER_STATE(NewState));
    
    if(NewState)
    {
        TIMERx->CTR |= (TIMER_ENABLE_MASK);
    }
    else
    {
        TIMERx->CTR &= ~(TIMER_ENABLE_MASK);
    }
}

uint32_t TIMER_GetCount(TIMER_TYPE *TIMERx)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));
    
    return TIMERx->TRR;
}

void TIMER_Set_Count(TIMER_TYPE *TIMERx, uint32_t count)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));
    
    TIMERx->TRR = count;
}

void TIMER_Set_Compare_Value(TIMER_TYPE *TIMERx, uint32_t value)
{
    CHECK_PARAM(PARAM_TIMER(TIMERx));
    
    TIMERx->CMP = value;
}