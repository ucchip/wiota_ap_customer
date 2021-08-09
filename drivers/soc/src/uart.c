// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
#include <stdint.h>
#include <stdio.h>
#include "uart.h"
#include "utils.h"
#include "uart.h"
#include "pulpino.h"
#include "sectdefs.h"
#include "stack_trace.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "timer.h"

uint32_t td_done = 0;

uint32_t rx_buffer_index = 0;
uart_rx_buffer_t uart_rx_data[MAX_BUFFER_SIZE];
/**
 * Setup UART. The UART defaults to 8 bit character mode with 1 stop bit.
 *
 * parity       Enable/disable parity mode
 * clk_counter  Clock counter value that is used to derive the UART clock.
 *              It has to be in the range of 1..2^16.
 *              There is a prescaler in place that already divides the SoC
 *              clock by 16.  Since this is a counter, a value of 1 means that
 *              the SoC clock divided by 16*2 = 32 is used. A value of 31 would mean
 *              that we use the SoC clock divided by 16*32 = 512.
 */
__reset2 void uart_set_cfg(int parity, uint16_t clk_counter)
{
    #ifdef _ASIC_   //according to initial summit "addi   x11, x0, 70"on branch llz_test for the uart setting.
    //clk_counter = 70;
    clk_counter = 15; //131M/460800/16-1 = 17
    #endif
    CGREG |= (1 << CGUART); // don't clock gate UART
    *(volatile unsigned int*)(UART_REG_LCR) = 0x83; //sets 8N1 and set DLAB to 1
    *(volatile unsigned int*)(UART_REG_DLM) = (clk_counter >> 8) & 0xFF;
    *(volatile unsigned int*)(UART_REG_DLL) =  clk_counter       & 0xFF;
    *(volatile unsigned int*)(UART_REG_FCR) = 0xE7; //enables 16byte FIFO and clear FIFOs,Trigger level 14byte
    *(volatile unsigned int*)(UART_REG_LCR) = 0x03; //sets 8N1 and set DLAB to 0

//  *(volatile unsigned int*)(UART_REG_IER) = 0x01; // set IER (interrupt enable register) on UART,recived data avaliable interrupt enable
    *(volatile unsigned int*)(UART_REG_IER) = ((*(volatile unsigned int*)(UART_REG_IER)) & 0xF0) | 0x1; // set IER (interrupt enable register) on UART

    //IER |= (1<<23);	//enable uart interrupt for PULPino event handler
}

CRT0_SECTION(printf, 1_1_1_2)  void uart_send(const char* str, unsigned int len) 
{
	unsigned int i;

	while (len > 0) {
		// process this in batches of 16 bytes to actually use the FIFO in the UART
		// wait until there is space in the fifo
		while( (*(volatile unsigned int*)(UART_REG_LSR) & 0x20) == 0);

		for (i = 0; (i < UART_FIFO_DEPTH) && (len > 0); i++) {
			// load FIFO
			*(volatile unsigned int*)(UART_REG_THR) = *str++;
			len--;	
			while( (*(volatile unsigned int*)(UART_REG_LSR) & 0x20) == 0);
		}
	}
}


void uart_send_string(char* str, unsigned int len) 
{
	unsigned int i;
	
	// process this in batches of 16 bytes to actually use the FIFO in the UART
	// wait until there is space in the fifo
	while( (*(volatile unsigned int*)(UART_REG_LSR) & 0x20) == 0);
    reset_timer();

	for (i = 0; i < len; i++) {
		// load FIFO
		*(volatile unsigned int*)(UART_REG_THR) = str[i];	
		while( (*(volatile unsigned int*)(UART_REG_LSR) & 0x20) == 0);

	}
}

uint32_t uart_send_test(unsigned int len) 
{
	unsigned int i;
	
	// process this in batches of 16 bytes to actually use the FIFO in the UART
	// wait until there is space in the fifo
	reset_timer();
	while( (*(volatile unsigned int*)(UART_REG_LSR) & 0x20) == 0);
	
	for (i = 0; i < len; i++) {
		// load FIFO
		*(volatile unsigned int*)(UART_REG_THR) = '0';
		while( (*(volatile unsigned int*)(UART_REG_LSR) & 0x20) == 0);
	}
	
	return (get_time());
}

char uart_getchar() {
    while((*((volatile int*)UART_REG_LSR) & 0x1) != 0x1);
    return *(volatile int*)UART_REG_RBR;
}

void uart_read_char(char* buf,unsigned char* len)
{
   if((*((volatile int*)UART_REG_LSR) & 0x1) == 0x1)
   {
		*buf = *(volatile int*)UART_REG_RBR;
		*len = 1;
   }
   return;
}

void uart_write_char(char* buf,unsigned char* len, unsigned char len_limit)
{
	*len = 0;
	while( (*(volatile unsigned int*)(UART_REG_LSR) & 0x20) != 0)
	{
		*(volatile unsigned int*)(UART_REG_THR) = *(buf + *len);
		(*len) ++;
		if(*len >= len_limit)
			break;
	}
	return;
}

uint8_t uart_getchar_unblock(char *data, uint32_t time_out) {
    uint32_t max_delay = time_out;
	
    while (--max_delay) {
        if ((*((volatile int*)UART_REG_LSR) & 0x1)) {
            *data = *(volatile int*)UART_REG_RBR;
			return 1;
		}
	}
	return 0;
}

CRT0_SECTION(printf, 1_1_1_1)  void uart_sendchar(const char c) 
{
	// wait until there is space in the fifo
	while( (*(volatile unsigned int*)(UART_REG_LSR) & 0x20) == 0);

	// load FIFO
	*(volatile unsigned int*)(UART_REG_THR) = c;
}

void uart_wait_tx_done(void) 
{
	// wait until there is space in the fifo
	while( (*(volatile unsigned int*)(UART_REG_LSR) & 0x40) == 0);
}

/*
void ISR_UART1(void)
{
	int iir = 0;
	char temp;
	uart_rx_buffer_t *rx_buffer;
	
	iir = ((*(volatile unsigned int*)UART_REG_IIR) & 0x0F);
	
	rx_buffer = &uart_rx_data[rx_buffer_index];
	//stack_trace();
    //printf(" iir %d",iir);
	switch (iir) {
	case UART_ITR_MS:    
	case UART_ITR_RL:
	    break;
	case UART_ITR_TD:
        break;
    case UART_ITR_RD:    
	case UART_ITR_RT:
	    //get data from fifo
		while (uart_getchar_unblock(&temp, 0xFF)) {
			rx_buffer->data[rx_buffer->len] = temp;
			if (++rx_buffer->len >= SERIAL_RX_BUFFER_SIZE) {
				rx_buffer->len = 0;
			}
		} 
		
		if (rx_buffer->len >= 2) {
			//data frame end with "\r\n"
			if (rx_buffer->data[rx_buffer->len - 2] == 0x0D && rx_buffer->data[rx_buffer->len - 1] == 0x0A) {
				rx_buffer->rx_done = 0;
				rx_buffer->len    -= 2; 
				rx_buffer->data[rx_buffer->len] = 0;
				TRACE_PRINTF("rx %s",rx_buffer->data);
				if (++rx_buffer_index >= MAX_BUFFER_SIZE) {
					rx_buffer_index = 0;
				}
				
			}
		}
        break;
	default:
		//TRACE_PRINTF(" iir %d",iir);
        break;
	}		
	ICP |= (1<<23);
}
*/