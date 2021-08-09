#ifndef __UCMMAP_H__
#define __UCMMAP_H__


extern volatile unsigned int * ccebar;
#define CCE_BAR  ((unsigned int) ccebar)


#define CCE_CSR_SIZE 32 //IRQ stuff at the top 

#define RV_TRACE_BUFFER_SIZE  (2 * 1024)
extern unsigned char rv_trace_buf[ RV_TRACE_BUFFER_SIZE ];
#define RV_TRACE_BUFFER (rv_trace_buf)


#define CCE_RING_BASE ( CCE_BAR+CCE_CSR_SIZE )

/* define a local ring_buffer */
#define GSMPKT_ENTRIES 16
#define GSMRING_SIZE_MASK 0xF
#define GSMTAP_INFO_SIZE 54   //CS-4 max 53 B

#endif //UCMMAP_H

