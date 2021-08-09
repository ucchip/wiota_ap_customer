#ifndef	__UART_H_
#define	__UART_H_

#include <rtthread.h>

#include "Type.h"
#include "AddrShareMemory.h"
#include "libNavProc.h"

#ifndef	__UART_C_
#define	EXT_UART		extern
#else
#define	EXT_UART
#endif

#define	RST			0x525354
#define	QRY			0x515259
#define	MOW			0x4D4F57
#define	BOU			0x424F55
#define	SOS			0x534F53
#define	TST			0x545354
#define SRP			0x535250			// set ref pos		


#define	UART_FIFO_LEN	(64)
#define	UART_RX_LVL		(32)
#define	UART_BUF_LEN	(8192)

typedef enum
{
	UART_BSP_9600 = 9600, 
	UART_BSP_19200 = 19200, 
	UART_BSP_38400 = 38400, 
	UART_BSP_57600 = 57600, 
	UART_BSP_115200 = 115200
}ENUM_UART_BSP;

typedef struct
{
	U08 ucBuf[UART_BUF_LEN];
	S16 sWrite;
	S16 sRead;
}STU_UART_QUEUE, *PSTU_UART_QUEUE;

// register
#define REG_UART_RBR	*(volatile U32*)(UART_BASE_ADDR + 0x00) // Receiver Buffer Register (Read Only)
#define REG_UART_DLL	*(volatile U32*)(UART_BASE_ADDR + 0x00) // Divisor Latch (LS)
#define REG_UART_THR	*(volatile U32*)(UART_BASE_ADDR + 0x00) // Transmitter Holding Register (Write Only)
#define REG_UART_DLM	*(volatile U32*)(UART_BASE_ADDR + 0x04) // Divisor Latch (MS)
#define REG_UART_IER	*(volatile U32*)(UART_BASE_ADDR + 0x04) // Interrupt Enable Register
#define REG_UART_IIR	*(volatile U32*)(UART_BASE_ADDR + 0x08) // Interrupt Identity Register (Read Only)
#define REG_UART_FCR	*(volatile U32*)(UART_BASE_ADDR + 0x08) // FIFO Control Register (Write Only)
#define REG_UART_LCR	*(volatile U32*)(UART_BASE_ADDR + 0x0C) // Line Control Register
#define REG_UART_MCR	*(volatile U32*)(UART_BASE_ADDR + 0x10) // MODEM Control Register
#define REG_UART_LSR	*(volatile U32*)(UART_BASE_ADDR + 0x14) // Line Status Register
#define REG_UART_MSR	*(volatile U32*)(UART_BASE_ADDR + 0x18) // MODEM Status Register
#define REG_UART_SCR	*(volatile U32*)(UART_BASE_ADDR + 0x1C) // Scratch Register

#define MASK_BIT_UART0	(23)
#define MASK_BIT_UART1	(24)

#define	UART_INT_MDM	(0x0)		// modem status
#define	UART_INT_TSP	(0x2)		// transmitter holding register empty
#define	UART_INT_DAT	(0x4)		// received data available
#define	UART_INT_ERR	(0x6)		// receiver line status
#define	UART_INT_TMT	(0xC)		// character timeout

// function
EXT_UART void InitUart(IN ENUM_UART_BSP eBsp);
EXT_UART void UartSend(IN const U08 *pData, IN U16 usLen);
EXT_UART void ISR_UART0();
EXT_UART void TaskUartTx(void *pvParameters);
EXT_UART void TaskUartRx(void *pvParameters);

EXT_UART U32 g_nLoopCfg;
EXT_UART ENUM_PVT_MODE g_eModePvt;
EXT_UART ENUM_RST_MODE g_eModeRst;
EXT_UART ENUM_SCENE_MODE g_eScene;
EXT_UART ENUM_WORK_MODE g_eWork;

EXT_UART struct rt_semaphore *g_hSemReset;

#ifdef	__UART_C_
void SetBsp(ENUM_UART_BSP eBsp);
U08 hextoint(IN U08 ucData);
void Resp(IN U08 ucId, IN U08 ucState);
U08 UartSum(IN register const U08 *pData, IN S16 sLen);
U08 LoopPara(IN const U08 *pBuf);

ENUM_UART_BSP m_eBsp;
struct rt_semaphore *g_hSemUartTx, *g_hSemUartRx;
STU_UART_QUEUE g_tQueUartTx, g_tQueUartRx;

#endif

#endif
