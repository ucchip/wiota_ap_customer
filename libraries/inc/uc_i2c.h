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
 * @brief I2C Peripheral Library.
 *
 * I2C functions for sending and receiving data
 * over an I2C bus.
 *
 */
#ifndef _UC_I2C_H_
#define _UC_I2C_H_

#include <pulpino.h>

#define I2C_PRESCALER_MASK    0xffff
#define I2C_RXDATA_MASK       0xff
#define I2C_ENABLE_MASK       0x80
#define I2C_BUSY_MASK         0x40 
#define I2C_AL_MASK           0x20
#define I2C_TIP_MASK          0x02
#define I2C_IF_MASK           0x01
#define I2C_RXACK_MASK        0x80
#define I2C_CTR_EN       	  0x80 // enable only
#define I2C_STATUS_TIP		  0x02
#define I2C_STATUS_RXACK	  0x80

typedef enum {
    Busy = 1,
    Idel = 0
}I2CStatus;

typedef enum{
    SUCCESS = 1,
    ERROR_TIMEOUT = 0
}I2CACK;

typedef enum {
    TRANSFER_DONE = 0,
    TRANSFER_ING = 1     //Transfer in progress
}I2CTXStatus;

typedef enum {
    I2C_START = 0x80,
    I2C_STOP  = 0x40,
    I2C_READ  = 0x20,
    I2C_WRITE = 0x10,
    I2C_CLR_INT = 0x01,
    I2C_START_READ = 0xA0,
    I2C_STOP_READ = 0x60,
    I2C_START_WRITE = 0x90,
    I2C_STOP_WRITE = 0x50
}I2C_CMD;

#define PARAM_I2C(i2c)                            ((i2c==UC_I2C))

#define PARAM_I2C_ENBIT(Enablebits)               ((Enablebits==ENABLE)||(Enablebits==DISABLE))
#define PARAM_I2C_TIMEOUT(I2C_TimeOut)            ((I2C_TimeOut>=0)&&(I2C_TimeOut<=0xfffff))
#define PARAM_I2C_TRANSFER_RATE(transfer_rate)    ((transfer_rate>=0)&&(transfer_rate<0xffff))

#define PARAM_I2C_CMD(cmd)  ((cmd==I2C_STOP_READ) || (cmd==I2C_START_WRITE ) \
        || (cmd==I2C_START_READ) || (cmd==I2C_WRITE) || (cmd==I2C_CLR_INT) \
        || (cmd==I2C_STOP) || (cmd==I2C_START) || (cmd==I2C_READ) \
        || (cmd==I2C_STOP_WRITE)) 

typedef struct{
    uint32_t            prescaler;
    uint32_t            I2C_TimeOut;
    FunctionalState     Enable;      /* parity bit enable */
}I2C_CFG_Type;


void I2C_Init(I2C_TYPE *I2C);
void I2C_Setup(I2C_TYPE *I2C, I2C_CFG_Type *I2CconfigStruct);
void I2C_Cmd(I2C_TYPE *I2C, FunctionalState NewState);
void I2C_Send_Command(I2C_TYPE *I2C, I2C_CMD cmd);
void I2C_Send_Data(I2C_TYPE *I2C, uint8_t data);
uint32_t I2C_Get_Status(I2C_TYPE *I2C);
I2CTXStatus I2C_Get_TXStatus(I2C_TYPE *I2C);
I2CACK I2C_Get_ACK(I2C_TYPE *I2C);
uint32_t I2C_Get_Data(I2C_TYPE *I2C);
I2CStatus I2C_Busy(I2C_TYPE *I2C);



#endif // _I2C_H_
