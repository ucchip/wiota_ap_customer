#include "uc_uartx.h"
#include "pulpino.h"

#define FI_DEFAULT      327
#define DI_DEFAULT      1

void uartx_init(UART_TYPE *UARTx,UART_CFG_Type *SDC_ConfigStruct) {
    uint32_t tem;
    uint32_t integerdivider;
    
    CHECK_PARAM(PARAM_UART(SCD));
    CHECK_PARAM(PARAM_UART_PARITYBIT(SDC_ConfigStruct->Parity));
    CHECK_PARAM(PARAM_UART_DATABIT(SDC_ConfigStruct->Databits));
    CHECK_PARAM(PARAM_UART_STOPBIT(SDC_ConfigStruct->Stopbits));
    CHECK_PARAM(PARAM_UART_CLK_RATE(SDC_ConfigStruct->Baud_rate));
    CHECK_PARAM(PARAM_UART_RESET_STATUS(SDC_ConfigStruct->Reset));

    
/*------------------------- UART LCR reg Configuration-----------------------*/

    tem = SDC_ConfigStruct->Databits;
    tem += STOPBIT(SDC_ConfigStruct->Stopbits);
    tem += PARITYBIT(SDC_ConfigStruct->Parity);
    UARTx->LCR = tem;
    
/*--------------- UART baud rate Configuration-----------------------*/
    integerdivider = (SYSTEM_CLK/15)/SDC_ConfigStruct->Baud_rate-1;
    UARTx->DLM = (integerdivider >> 8) & 0xFF;
    UARTx->DLL = integerdivider & 0xFF;
    
/*--------------- set trigger level ,enable fifo,reset rx and tx fifo ----------------------*/

    UARTx->FCR = ((SDC_ConfigStruct->level>>6) + 0x7);
}

void uartx_ITConfig(UART_TYPE *UARTx,UART_IT it_type)
{
    UARTx->LCR &= (~0x80);
    UARTx->IER = it_type;
}

void uartx_sendchar(UART_TYPE *UARTx, uint8_t data)
{
    CHECK_PARAM(UARTx);
    
/*--------------- UART send data-----------------------*/

    UARTx->THR = data;
    
}

FlagStatus uartx_getchar(UART_TYPE *UARTx, uint8_t *data, uint32_t timeout)
{
    uint32_t count = 0,temp = 0;
    
    CHECK_PARAM(UARTx);

    do{
        count++;
        temp = UARTx->LSR & RX_VALID_MASK;     // rx data valid
    
    }while((!temp)&&(count<timeout));    //while timeout or rx data valid

    if(count >= timeout)
    {
        return ERROR_TIMEOUT;          
    }
    data[0] = (uint8_t)(UARTx->RBR&RBR_MASK);
    return SUCCESS;
}

void uartx_wait_tx_done(UART_TYPE *UARTx)
{
    CHECK_PARAM(UARTx);
    
    while(!(UARTx->LSR & TX_DONE_MASK));
}


void uartx_clean_rxfifo(UART_TYPE *UARTx)
{
    CHECK_PARAM(UARTx);

    UARTx->FCR |= TX_FIFO_CLEAN_MASK; 
}

void uartx_clean_txfifo(UART_TYPE *UARTx)
{
    CHECK_PARAM(UARTx);
    
    UARTx->FCR |= TX_FIFO_CLEAN_MASK; 
}
