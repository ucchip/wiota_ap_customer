#define	__CCE_C_

#include "AddrShareMemory.h"
#include "Cce.h"
#include "Uart.h"

#include "cce_firmware_210125.h"

void InitCce()
{
	volatile S16 i;
	
	REG_CCE_RUN_CTRL = 0x0;	// close cce
	REG_CCE_PWR_CTRL &= 0xFFFEFFFF;		// cce power off
	for(i = 0; i < 100; i++);
//	REG_CCE_PWR_CTRL |= ((1 << 16) | (1 << 15));	// cce power on and reset
	REG_CCE_PWR_CTRL |= ((1 << 16) | (1 << 15) | (1 << 8));	// cce power on and reset
	REG_INT_ENABLE |= (1 << 1);		// enable CCE interrupt for PULPino event handler
}

void CceLoad()
{
	S32 i;
	
	for(i = 0; i < sizeof(cce_init_ar_case_789732294569) / sizeof(U32); i++)
	{
		REG_CCE_AR(i) = cce_init_ar_case_789732294569[i];
	}
	
	for(i = 0; i < sizeof(cce_init_dr_case_789732294569) / sizeof(U32); i++)
	{
		REG_CCE_DR(i) = cce_init_dr_case_789732294569[i];
	}
	
	for(i = 0; i < sizeof(cce_init_membk_case_789732294569) / sizeof(U32); i++)
	{
		REG_CCE_MEMBK(i) = cce_init_membk_case_789732294569[i];
	}
	
	for(i = 6; i < sizeof(cce_init_csr_case_789732294569) / sizeof(U32); i++)
	{
		REG_CCE_CSR(i) = cce_init_csr_case_789732294569[i];
	}
	
	MEM_CCE_TRACE_OPT = ReverseByte(7);
}
