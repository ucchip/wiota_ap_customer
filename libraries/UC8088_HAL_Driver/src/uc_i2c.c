// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include <uc_i2c.h>

void I2C_Setup(I2C_TYPE *I2C, I2C_CFG_Type *I2CconfigStruct)
{
    CHECK_PARAM(PARAM_I2C(I2C));
    CHECK_PARAM(PARAM_I2C_TRANSFER_RATE(I2CconfigStruct->prescaler));
    
    I2C->CPR = I2CconfigStruct->prescaler & I2C_PRESCALER_MASK;
    
    I2C->CTR |= (I2C_ENABLE_MASK);
}


void I2C_Cmd(I2C_TYPE *I2C, FunctionalState NewState)
{
    CHECK_PARAM(PARAM_I2C(I2C));
    CHECK_PARAM(PARAM_I2C_ENBIT(NewState));
    
    if(NewState)
    {
        I2C->CTR |= (I2C_ENABLE_MASK);
    }
    else
    {
        I2C->CTR &= ~(I2C_ENABLE_MASK);
    }
}

void I2C_Send_Command(I2C_TYPE *I2C, I2C_CMD cmd)
{
    CHECK_PARAM(PARAM_I2C(I2C));
    CHECK_PARAM(PARAM_I2C_CMD(cmd));
    
    I2C->CDR = cmd;
}

void I2C_Send_Data(I2C_TYPE *I2C, uint8_t data)
{
    CHECK_PARAM(PARAM_I2C(I2C));
    
    I2C->TXR = data;
}

uint32_t I2C_Get_Status(I2C_TYPE *I2C)
{
    uint32_t temreg;
    
    CHECK_PARAM(PARAM_I2C(I2C));
    
    temreg = I2C->STR;
    
    return temreg;
}

I2CTXStatus I2C_Get_TXStatus(I2C_TYPE *I2C)
{
    I2CTXStatus temstatus;
    
    CHECK_PARAM(PARAM_I2C(I2C));
    
    temstatus = (I2CTXStatus)(I2C->STR & I2C_TIP_MASK);
    
    return temstatus;
}

I2CACK I2C_Get_ACK(I2C_TYPE *I2C)
{
	while((I2C->STR & I2C_STATUS_TIP) == 0);
	while((I2C->STR & I2C_STATUS_TIP) != 0);
	return !(I2C->STR & I2C_STATUS_RXACK);
}


uint32_t I2C_Get_Data(I2C_TYPE *I2C)
{	
	volatile int tempdata;
    CHECK_PARAM(PARAM_I2C(I2C));
    
    tempdata = *(volatile int*)(I2C->RXR);
	return tempdata;
}

I2CStatus I2C_Busy(I2C_TYPE *I2C)
{
    uint32_t temreg;
    
    CHECK_PARAM(PARAM_I2C(I2C));
    
    temreg = (I2C->STR&I2C_BUSY_MASK);
 
    return (temreg==I2C_BUSY_MASK);
}

