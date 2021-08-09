#ifndef	__SPI_FLASH_H_
#define	__SPI_FLASH_H_

#include "Type.h"

#ifndef	__SPI_FLASH_C_
#define	EXT_FLASH		extern
#else
#define	EXT_FLASH
#endif

// addr of spi-flash
#define	REG_SPI_STATUS			*((volatile U32 *)0x1A10C000)
#define	REG_SPI_CMD				*((volatile U32 *)0x1A10C008)
#define	REG_SPI_ADDR			*((volatile U32 *)0x1A10C00C)
#define	REG_SPI_LEN				*((volatile U32 *)0x1A10C010)
#define	REG_SPI_DUMMY			*((volatile U32 *)0x1A10C014)
#define	REG_SPI_FIFO_TX			*((volatile U32 *)0x1A10C018)
#define	REG_SPI_FIFO_RX			*((volatile U32 *)0x1A10C020)

#define	FLASH_PAGE_BIT_SHIFT	(8)
#define	FLASH_SECTOR_BIT_SHIFT	(12)
#define	FLASH_BLOCK_BIT_SHIFT	(16)

#define	FLASH_PAGE_MASK			(0xFF)

#define	FLASH_PAGE_SIZE			(256)		// Unit:Byte
#define	FLASH_SECTOR_SIZE		(4096)		// Unit:Byte

#define	FLASH_PAGE_OF_SECTOR	(16)		// Unit:page
#define	FLASH_PAGE_OF_BLOCK		(256)		// Unit:page

#define	FLASH_SECTOR_OF_BLOCK	(16)		// Unit:Sector
#define	FLASH_SECTOR_OF_CHIP	(512)		// Unit:Sector

#define	FLASH_BLOCK_OF_CHIP		(32)		// Unit:Block

typedef enum
{
	FLASH_CMD_ID = 0x9F, 
	FLASH_CMD_ERASE_SECTOR = 0x20, 
	FLASH_CMD_ERASE_BLOCK = 0xD8, 
	FLASH_CMD_STATUS = 0x05, 
	FLASH_CMD_ENABLE_WR = 0x06, 
	FLASH_CMD_QREAD = 0x11101011, 
	FLASH_CMD_READ = 0x3, 
	FLASH_CMD_PAGE_PROGRAM = 0x02
}ENUM_FLASH_CMD;

// function
EXT_FLASH U32 ReadFlashID();
EXT_FLASH void FlashEraseSector(IN U32 nBaseAddr);
EXT_FLASH void FlashWrite(IN U32 nAddr, IN const U08 *pData, IN U16 usLen);
EXT_FLASH void FlashRead(IN U32 nAddr, OUT U08 *pData, IN U16 usLen);
EXT_FLASH void FlashQRead(IN U32 nAddr, OUT U08 *pData, IN U16 usLen);
EXT_FLASH U08 FlashCrc(IN const U08 *pData, IN U16 usLen);

#ifdef __SPI_FLASH_C_
void FlashEnableWr(void);
void FlashWaitForWr(void);
U08 FlashStatus(void);
void FlashPageProgram(IN U32 nAddr, IN const U08 *pData, IN U16 usLen);
void FlashPageRead(IN U32 nAddr, OUT U08 *pData, IN U16 usLen);

#endif

#endif
