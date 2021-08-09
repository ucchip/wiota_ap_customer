#ifndef	__ADDR_SHARE_MEMORY_H_
#define	__ADDR_SHARE_MEMORY_H_

#include "Type.h"

#define int_disable()   asm ( "csrci mstatus, 0x08")
#define int_enable()    asm ( "csrsi mstatus, 0x08") 

#define REG_INT_ENABLE 			*((volatile U32 *)0x1A104000)
#define REG_INT_PEND			*((volatile U32 *)0x1A104004)
#define REG_INT_CLEAR			*((volatile U32 *)0x1A10400C)

#define ADDR_BASE_MEM			(0x0039F800)			// base address of shared memory

#define	REG_CCE_PWR_CTRL		*((volatile U32 *)0x1A104200)	// Power control register of CCE
#define	REG_DCDC_PWR_CTRL		*((volatile U32 *)0x1A104204)
#define	REG_CLK_PLL_RST			*((volatile U32 *)0x1A104208)
#define	REG_CLK_PLL_INT			*((volatile U32 *)0x1A104210)
#define	REG_CLK_PLL_FRAC		*((volatile U32 *)0x1A104214)
#define	REG_RF_DCXO				*((volatile U32 *)0x1A104220)
#define	REG_RC_CTRL				*((volatile U32 *)0x1A104228)

#define	REG_XIP_CTRL			*((volatile U32 *)0x1A10C02C)
#define	REG_RF_GPS_RX0			*((volatile U32 *)0x1A106000)
#define	REG_RF_GPS_RX1			*((volatile U32 *)0x1A106004)
#define	REG_RF_GPS_RX2			*((volatile U32 *)0x1A106008)
#define	REG_RF_GPS_RX3			*((volatile U32 *)0x1A10600C)
#define	REG_RF_GPS_RX4			*((volatile U32 *)0x1A106010)
#define	REG_RF_LOPLL0			*((volatile U32 *)0x1A106014)
#define	REG_RF_LOPLL1			*((volatile U32 *)0x1A106018)
#define	REG_RF_LOPLL2			*((volatile U32 *)0x1A10601C)
#define	REG_RF_LOPLL3			*((volatile U32 *)0x1A106020)
#define	REG_RF_LOPLL4			*((volatile U32 *)0x1A106024)
#define	REG_RF_STATUS			*((volatile U32 *)0x1A106028)

#define	REG_CCE_RUN_CTRL		*((volatile U32 *)0x1A10701C)	// Start control register of CCE

// CCE initialization load
#define	REG_CCE_MEMBK(idx)		*((volatile U32 *)(0x00380000 + ((idx) << 2)))
#define	REG_CCE_CSR(idx)		*((volatile U32 *)(0x003A0000 + ((idx) << 2)))
#define	REG_CCE_AR(idx)			*((volatile U32 *)(0x003A1000 + ((idx) << 2)))
#define	REG_CCE_DR(idx)			*((volatile U32 *)(0x003A1040 + ((idx) << 2)))

#define	REG_CCE_STATUS			REG_CCE_CSR(8)					// status of cce
#define	REG_CCE_LOOP_ORDER		REG_CCE_CSR(14)					// bit19 ~ bit12:rx_sc, bit11:eph_corr_en, bit10 ~ bit3:loss_thr, bit2:ph_rev_en, bit1:dll_order, bit0:carrier_order

/*
 * GPS/BDS related address mapping
 */
// module of tracking loop config
#define	MEM_CCE_TRACE_OPT		*((volatile U32 *)0x00391FAC)	// merge with bd

#define	MEM_GPS_ALL_CFG(ms, para)			*((volatile U32 *)(0x003905C0 + (((ms) * 6 + (para)) << 2)))	// ms:20ms/2*20ms/5*20ms/10*20ms/15*20ms, para:0 ~ 5

#define	MEM_GPS_DLL2_CFG1(snr, ms, para)	*((volatile U32 *)(0x00390248 + (((ms) * 18 + (snr) * 6 + (para)) << 2)))	// snr:high/middle/low, ms:1ms/20ms(general bn)/20ms(narrow bn), para:0 ~ 5
#define	MEM_GPS_DLL2_CFG2(ms, para)			*((volatile U32 *)(0x00390320 + (((ms) * 6 + (para)) << 2)))	// ms:narrow corr/100ms corr, para:0 ~ 5
#define	MEM_GPS_DLL2_CFG3(snr, ms, para)	*((volatile U32 *)(0x00390638 + (((ms) * 9 + (snr) * 3 + (para)) << 2)))	// snr:high/middle/low, ms:20ms(width bn)/20ms(narrow bn), para:0 ~ 2
#define	MEM_GPS_DLL2_CFG4(ms, para)			*((volatile U32 *)(0x00390680 + (((ms) * 3 + (para)) << 2)))	// ms:narrow corr/2*20ms/5*20ms/10*20ms/15*20ms/30*20ms, para:0 ~ 2

#define	MEM_GPS_PLL2_CFG1(snr, ms, para)	*((volatile U32 *)(0x003903E0 + (((ms) * 18 + (snr) * 6 + (para)) << 2)))	// snr:high/middle/low, ms:1ms/2ms/4ms/10ms/20ms, para:0 ~ 5
#define	MEM_GPS_PLL2_CFG2(ms, para)			*((volatile U32 *)(0x00390548 + (((ms) * 6 + (para)) << 2)))	// ms:2*20ms/5*20ms/10*20ms/15*20ms/30*20ms, para:0 ~ 5

#define	MEM_GPS_FLL2_CFG(snr, ms, para)		*((volatile U32 *)(0x00390350 + (((ms) * 9 + (snr) * 3 + (para)) << 2)))	// snr:high/middle/low, ms:1ms/2ms/4ms/10ms, para:0 ~ 2

#define	MEM_GPS_PLL3_CFG(snr, ms, para)		*((volatile U32 *)(0x003907E8 + (((ms) * 27 + (snr) * 9 + (para)) << 2)))	// snr:high/middle/low, ms:1ms/2ms/4ms/10ms/20ms, para:0 ~ 8
#define	MEM_GPS_FLL3_CFG(snr, ms, para)		*((volatile U32 *)(0x003906C8 + (((ms) * 18 + (snr) * 6 + (para)) << 2)))	// snr:high/middle/low, ms:1ms/2ms/4ms/10ms, para:0 ~ 5

#define	MEM_GPS_FLI_SMOOTH(idx)				*((volatile U32 *)(0x003901F8 + ((idx) << 2)))
#define	MEM_GPS_PLI_SMOOTH(idx)				*((volatile U32 *)(0x00390200 + ((idx) << 2)))

#define	MEM_GPS_FLI_PLI_TH(snr, idx)		*((volatile U32 *)(0x00390208 + (((snr) * 2 + (idx)) << 2)))

#define	MEM_GPS_CN0_TH(idx)					*((volatile U32 *)(0x00390220 + ((idx) << 2)))

#endif
