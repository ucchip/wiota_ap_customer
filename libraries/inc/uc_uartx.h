#ifndef _UC_UARTX_H_
#define _UC_UARTX_H_

#include "pulpino.h"
#include <stdint.h>

/*------------LSR bit 1 receive data valid---------*/

#define RX_VALID_MASK        0x1             
#define RBR_MASK             0xff
#define TX_DONE_MASK         0x40
#define RX_FIFO_CLEAN_MASK   0x02
#define TX_FIFO_CLEAN_MASK   0x04

#define DLAB(x) x<<7 	    //DLAB bit in LCR reg
#define STOPBIT(x) x<<2 	//STOP bit in LCR reg
#define PARITYBIT(x) x<<3 	//STOP bit in LCR reg

typedef enum{
    SUCCESS = 1,
    ERROR_TIMEOUT = 0
}FlagStatus;

typedef enum {
    PARITYBIT_ENABLE = 1,
    PARITYBIT_DISABLE = 0
}UART_PARITYBIT_EN;
             
typedef enum {
    DATABIT_5 = 0,
    DATABIT_6 = 1,
    DATABIT_7 = 2,
    DATABIT_8 = 3
}UART_DATABIT_Type;

typedef enum {
    STOPBIT_ENABLE = 1,
    STOPBIT_DISABLE = 0
}UART_STOPBIT_EN;

typedef enum {
    RESET_LOW   = 0,
    RESET_HIGH  = 1
}UART_RESET_STATUS;

typedef enum {
    BYTE_1 = 0,
    BYTE_4 = 1,
    BYTE_8 = 2,
    BYTE_14 = 3
}TRIGGER_LEVEL;

typedef enum {
    UART_RX_IT = 0,
    UART_TX_IT = 2,
    UART_RXLINE_IT = 4,
    UART_MODEM_IT = 8
}UART_IT;


typedef struct{
    uint32_t Baud_rate;
    UART_PARITYBIT_EN     Parity;      /* parity bit enable */
    UART_DATABIT_Type     Databits;     
    UART_STOPBIT_EN       Stopbits;    /* stop  bit enable */
    UART_RESET_STATUS     Reset;
    TRIGGER_LEVEL         level;
}UART_CFG_Type;


#define PARAM_UART_DATABIT(Databits)    ((Databits==DATABIT_5)||(Databits==DATABIT_6) \
||(Databits==DATABIT_7)||(Databits==DATABIT_8))

#define PARAM_UART_IT(it_type)    ((it_type==UART_RX_IT)||(it_type==UART_TX_IT) \
||(it_type==UART_RXLINE_IT)||(it_type==UART_MODEM_IT))

#define PARAM_UART_LEVEL(level)    ((level==BYTE_1)||(level==BYTE_4) \
||(level==BYTE_8)||(level==BYTE_14))

#define PARAM_UART_PARITYBIT(Parity)    ((Parity==PARITYBIT_ENABLE)||(Parity==PARITYBIT_DISABLE))

#define PARAM_UART_STOPBIT(Stopbits)    ((Stopbits==STOPBIT_ENABLE)||(Stopbits==STOPBIT_DISABLE))

#define PARAM_UART_RESET_STATUS(Reset)    ((Reset==RESET_LOW)||(Reset==RESET_HIGH))

#define PARAM_UART(SCD)    (SCD==UC_UART)

#define PARAM_UART_CLK_RATE(Clk_rate)    ((Clk_rate<((uint32_t)(configCPU_CLOCK_HZ/2)-1) \
&&(Clk_rate>((uint32_t)(configCPU_CLOCK_HZ/5)-1)))


void uartx_Init(UART_TYPE *SCD, UART_CFG_Type *SDC_ConfigStruct);
void uartx_sendchar(UART_TYPE *SCD, uint8_t data);
FlagStatus uartx_getchar(UART_TYPE *SCD, uint8_t *data, uint32_t timeout);
void uartx_wait_tx_done(UART_TYPE *SCD);
void uartx_clean_rxfifo(UART_TYPE *SCD);
void uartx_clean_txfifo(UART_TYPE *SCD);
void uartx_set_Baud(UART_TYPE *SCD, uint32_t fi, uint32_t di);



#endif