/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the GNU General Public License.
 *
 *	Based on GPL code developed by: Beceem Communications Pvt. Ltd
 */
#ifndef _USB_BECEEM_NVM_H_
#define _USB_BECEEM_NVM_H_

#include <ByteOrder.h>
#include <ether_driver.h>

#include "util.h"
#include "DeviceStruct.h"


#define BECM					ntohl(0x4245434d)

#define MAC_ADDR_OFFSET			0x200

#define SPI_FLUSH_REG			0x0F00304C

// Our Flash addresses
#define FLASH_PART_SIZE							(16 * 1024 * 1024)
#define FLASH_CONFIG_REG						0xAF003050
#define FLASH_GPIO_CONFIG_REG					0xAF000030

// Locations for Flash calibration info
#define FLASH_CS_INFO_START_ADDR				0xFF0000
#define FLASH_AUTO_INIT_BASE_ADDR				0xF00000

// NVM Flash addresses before and after DDR Init
#define FLASH_CONTIGIOUS_START_ADDR_BEFORE_INIT	0x1F000000
#define FLASH_CONTIGIOUS_START_ADDR_AFTER_INIT	0x1C000000
#define FLASH_SPI_CMDQ_REG						0xAF003040
#define FLASH_SPI_WRITEQ_REG					0xAF003044
#define FLASH_SPI_READQ_REG						0xAF003048
#define FLASH_SIZE_ADDR							0xFFFFEC
#define FLASH_CMD_STATUS_REG_WRITE				0x01
#define FLASH_CMD_STATUS_REG_READ				0x05
#define FLASH_CMD_WRITE_ENABLE					0x06
#define FLASH_CMD_READ_ID						0x9F
#define FLASH_CS_SIGNATURE						0xBECEF1A5

// Our EEPROM addresses
#define EEPROM_SPI_Q_STATUS_REG					0x0F003008
#define EEPROM_CMDQ_SPI_REG						0x0F003018
#define EEPROM_READ_DATAQ_REG					0x0F003020

#define RESET_CHIP_SELECT		-1
#define MAX_RW_SIZE				0x10			// bytes
#define MAX_NVM_SIZE			(8*1024)

// Different models have different storage devices (sigh)
#define NVM_UNKNOWN	0
#define NVM_EEPROM	1
#define NVM_FLASH	2

#define NVM_VERSION_OFFSET		0x020E
#define NVM_SECTOR_SIZE			0x10000

#define NVM_READ_DATA_AVAIL		0x00000020		// was EEPROM_READ_DATA_AVAIL
#define NVM_SPI_Q_STATUS1_REG	0x0F003004		// was EEPROM_SPI_Q_STATUS1_REG
#define NVM_ALL_QUEUE_FLUSH		0x0000000f		// was EEPROM_ALL_QUEUE_FLUSH

#define SCSI_FIRMWARE_MINOR_VERSION		0x5

#ifndef MIN
#define MIN(_a, _b) ((_a) < (_b)? (_a): (_b))
#endif

#define MAJOR_VERSION(x) (x & 0xFFFF)
#define MINOR_VERSION(x) ((x >>16) & 0xFFFF)


class BeceemNVM
{
public:
								BeceemNVM();
			status_t			NVMInit(WIMAX_DEVICE* swmxdevice);
			status_t			ReadMACFromNVM(ether_address *address);
			status_t			ValidateDSD(unsigned long hwParam);


			int					NVMRead(unsigned int offset, unsigned int size,
									unsigned int* buffer);
			int					NVMWrite(unsigned int offset, unsigned int size,
									unsigned int* buffer);

	// yuck.  These are in a child class class
	virtual status_t			ReadRegister(unsigned int reg,
									size_t size, uint32_t* buffer)
									{ return NULL; };
	virtual status_t			WriteRegister(unsigned int reg,
									size_t size, uint32_t* buffer)
									{ return NULL; };
	virtual status_t			BizarroReadRegister(unsigned int reg,
									size_t size, uint32_t* buffer)
									{ return NULL; };
	virtual status_t			BizarroWriteRegister(unsigned int reg,
									size_t size, uint32_t* buffer)
									{ return NULL; };

			int 				bSelectedChip;			// selected chip

private:
			WIMAX_DEVICE*	pwmxdevice;

			status_t			NVMDetect();
			status_t			NVMFlush();
			status_t			NVMChipSelect(unsigned int offset);
			status_t			RestoreBlockProtect(unsigned long writestatus);
			unsigned long		DisableBlockProtect(unsigned int offset,
									size_t size);

			int					FlashGetBaseAddr();
			unsigned long		FlashReadID();
			status_t			FlashReadCS();
			status_t			FlashSectorErase(unsigned int addr,
									unsigned int numOfSectors);
			status_t			FlashBulkRead(unsigned int offset,
									unsigned int size, unsigned int* buffer);
			status_t			FlashBulkWrite(unsigned int offset,
									unsigned int size, unsigned int* buffer);
			status_t			FlashCSFlip(PFLASH_CS_INFO FlashCSInfo);
			status_t			FlashCSDump(PFLASH_CS_INFO FlashCSInfo);

			unsigned int		EEPROMGetSize();
			unsigned int		FlashGetSize();

			status_t			EEPROMRead(unsigned int offset,
									unsigned int *pdwData);
			status_t			EEPROMBulkRead( unsigned int offset,
									size_t numBytes, unsigned int* buffer);
};


#endif	// _USB_BECEEM_NVM_H

