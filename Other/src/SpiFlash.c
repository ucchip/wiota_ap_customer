#define	__SPI_FLASH_C_

#include "AddrShareMemory.h"
#include "SpiFlash.h"
#include "uc_spi.h"

#include <sectdefs.h>
#define __critical __attribute__((section(".crt0")))
#define __critical_64 __attribute__((section(".crt0"), aligned(64))) 
#define __critical_128 __attribute__((section(".crt0"), aligned(128))) 
#define __critical_512 __attribute__((section(".crt0"), aligned(512)))

#define WAIT_XIP_FREE     while(REG_XIP_CTRL & 0x1)
#define WAIT_FOR_WR_DONE  while(Flash_Read_SR()&0x03)
#define WAIT_SPI_IDLE     while(REG_SPI_STATUS != 1)

#define SPI_START(cmd)  (REG_SPI_STATUS = (1<<(SPI_CSN0+8))|(1<<(cmd))); //start

extern U16 auto_dummy;

__critical_512 U32 Flash_Read_SR()
{
	U32 data;
	REG_SPI_CMD = FLASH_CMD_STATUS << 24; //read sr 
	REG_SPI_LEN = 0x200008;
	WAIT_XIP_FREE;
	SPI_START(SPI_CMD_RD);
	while((REG_SPI_STATUS & 0xFF0000)==0);
	data = REG_SPI_FIFO_RX;
	return data;
}

__critical void FlashEraseSector(IN U32 nBaseAddr)
{
//	Flash_Read_SR();
	WAIT_XIP_FREE;
	FlashEnableWr();
	REG_SPI_CMD = FLASH_CMD_ERASE_SECTOR << 24;
	REG_SPI_ADDR = (nBaseAddr << 8);
    REG_SPI_LEN  = 0x1808;
    REG_SPI_DUMMY = 0;
	WAIT_XIP_FREE;
	SPI_START(SPI_CMD_RD);
    WAIT_SPI_IDLE;
	WAIT_FOR_WR_DONE;
}

__reset void FlashWrite(IN U32 nAddr, IN const U08 *pData, IN U16 usLen)
{
	U16 usPage, i, usLenTmp;
	U32 nTmp;
	
	usPage = (usLen & FLASH_PAGE_MASK) > 0 ? (usLen >> FLASH_PAGE_BIT_SHIFT) + 1 : usLen >> FLASH_PAGE_BIT_SHIFT;
	WAIT_FOR_WR_DONE;
	
	for(i = 0; i < usPage; i++)
	{
		nTmp = i << FLASH_PAGE_BIT_SHIFT;
		usLenTmp = (usLen >= (nTmp + FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : usLen - nTmp);
		FlashEnableWr();
		FlashPageProgram(nAddr + nTmp, pData + nTmp, usLenTmp);
	}
}

__critical void FlashRead(IN U32 nAddr, OUT U08 *pData, IN U16 usLen)
{
	spi_read_fifo(NULL,0);
	WAIT_XIP_FREE;
	REG_SPI_CMD = FLASH_CMD_READ << 24;	// set cmd
	REG_SPI_ADDR = (nAddr << 8);
	REG_SPI_LEN = 0x1808|(usLen<<19);	// set cmd,addr and data len
	WAIT_XIP_FREE;
	SPI_START(SPI_CMD_RD);
	spi_read_fifo((int *) pData,(usLen<<3));
}

__critical void FlashQRead(IN U32 nAddr, OUT U08 *pData, IN U16 usLen)
{
	spi_read_fifo(NULL,0);
	REG_SPI_DUMMY = auto_dummy;
	WAIT_XIP_FREE;
	REG_SPI_CMD = FLASH_CMD_QREAD;	// set cmd
	REG_SPI_ADDR = (nAddr << 8);
	REG_SPI_LEN = 0x1820|(usLen<<19);	// set cmd,addr and data len
	WAIT_XIP_FREE;
	SPI_START(SPI_CMD_QRD);
	spi_read_fifo((int *) pData,(usLen<<3));
}

U08 FlashCrc(IN const U08 *pData, IN U16 usLen)
{
	S16 i;
	U08 ucRes = 0x00;
	
	for(i = 0; i < usLen; i++)
		ucRes ^= pData[i];
	
	return ucRes;
}

__critical void FlashPageProgram(IN U32 nAddr, IN const U08 *pData, IN U16 usLen)
{
	spi_write_fifo(NULL, 0);
	WAIT_XIP_FREE;
	REG_SPI_CMD = FLASH_CMD_PAGE_PROGRAM << 24;  //set cmd
	REG_SPI_ADDR = (nAddr << 8);
	/* spi len reg format *
	 * bit16: bit15:8  bit7:0 
	 * DLEN   ADDRLEN  CMDLEN
	 */
	REG_SPI_LEN = 0x1808 | (usLen << 19);  //set cmd,addr and data len
	WAIT_XIP_FREE;
	SPI_START(SPI_CMD_WR);
	WAIT_XIP_FREE;
	spi_write_fifo((S32 *)pData, usLen << 3);
    WAIT_SPI_IDLE;
	WAIT_FOR_WR_DONE;
}

__critical void FlashEnableWr(void)
{
	REG_SPI_CMD = FLASH_CMD_ENABLE_WR << 24;	// set cmd
	REG_SPI_LEN = 0x0008;      // set cmd and data len
	WAIT_XIP_FREE;
	SPI_START(SPI_CMD_WR);
	while(REG_SPI_STATUS != 1);
}
