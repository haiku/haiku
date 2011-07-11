/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the GNU General Public License.
 *
 *	Based on GPL code developed by: Beceem Communications Pvt. Ltd
 *
 *	Handle the non-volatile memory space on the Beceem device.
 *	These functions act as a middleware between the raw usb registers
 *	and the onboard non-volatile storage (which can be flash or eeprom)
 */


#include "Settings.h"

#include <KernelExport.h>

#include "Driver.h"

#include "BeceemDevice.h"
#include "BeceemNVM.h"


BeceemNVM::BeceemNVM()
{
	TRACE("Debug: Load non-volatile memory handler\n");
}


status_t
BeceemNVM::NVMInit(WIMAX_DEVICE* swmxdevice)
{
	TRACE("Debug: Init non-volatile memory handler\n");
	pwmxdevice = swmxdevice;

	pwmxdevice->nvmFlashCSDone = false;

	NVMFlush();

	// Detect non-volatile memory type
	if (NVMDetect() != B_OK)
		return B_ERROR;

	unsigned short	usNVMVersion  = 0;

	NVMRead(NVM_VERSION_OFFSET, 2, (unsigned int*)&usNVMVersion);
	pwmxdevice->nvmVerMinor = usNVMVersion&0xFF;
	pwmxdevice->nvmVerMajor = ((usNVMVersion>>8)&0xFF);

	TRACE_ALWAYS("Info: non-volatile memory version: Major:%i Minor:%i\n",
			pwmxdevice->nvmVerMajor, pwmxdevice->nvmVerMinor);

	return B_OK;
}


status_t
BeceemNVM::NVMFlush()
{
	/* Chipset Bug : Clear the Avail bits on the Read queue. The default
	 * value on this register is supposed to be 0x00001102.
	 * But we get 0x00001122. */
	TRACE("Debug: Fixing reset value on 0x0f003004\n");

	unsigned int value = NVM_READ_DATA_AVAIL;
	BizarroWriteRegister(NVM_SPI_Q_STATUS1_REG, sizeof(value), &value);

	/* Flush the all of the NVM queues. */
	TRACE("Debug: Flushing the NVM Queues...\n");
	value = NVM_ALL_QUEUE_FLUSH;
	BizarroWriteRegister(SPI_FLUSH_REG, sizeof(value), &value);
	value = 0;
	BizarroWriteRegister(SPI_FLUSH_REG, sizeof(value), &value);

	return B_OK;
}


status_t
BeceemNVM::NVMDetect()
{
	unsigned int uiData = 0;

	EEPROMBulkRead(0x0, 4, &uiData);
	if (uiData == BECM) {
		TRACE_ALWAYS("Info: EEPROM nvm detected.\n");
		pwmxdevice->nvmType = NVM_EEPROM;
		pwmxdevice->nvmDSDSize = EEPROMGetSize();
		return B_OK;
	}

	// As this isn't a EEPROM device, lets look for a flash CS...
	if (FlashReadCS() != B_OK)
		return B_ERROR;

	FlashBulkRead(0x0 + pwmxdevice->nvmFlashCalStart, 4, &uiData);
	if (uiData == BECM) {
			TRACE_ALWAYS("Info: Flash nvm detected.\n");
			pwmxdevice->nvmType = NVM_FLASH;
			pwmxdevice->nvmDSDSize = FlashGetSize();
			return B_OK;
	}

	pwmxdevice->nvmType = NVM_UNKNOWN;

	TRACE_ALWAYS("Error: Couldn't detect nvm storage method, found 0x%X\n",
		uiData);

	return B_ERROR;
}


status_t
BeceemNVM::NVMChipSelect(unsigned int offset)
{
	int chipIndex = offset / FLASH_PART_SIZE;

	// Chip to be selected is already selected
	if (bSelectedChip == chipIndex)
		return B_OK;

	TRACE("Debug: Selecting chip %d for transaction at 0x%x\n",
		chipIndex, offset);

	// Migrate selected chip to new selection
	bSelectedChip = chipIndex;

	unsigned int flashConfig = 0;
	BizarroReadRegister(FLASH_CONFIG_REG, 4, &flashConfig);

	unsigned int gpioConfig = 0;
	BizarroReadRegister(FLASH_GPIO_CONFIG_REG, 4, &gpioConfig);

	// TRACE("Reading GPIO config 0x%x\n", &GPIOConfig);
	// TRACE("Reading Flash config 0x%x\n", &FlashConfig);

	unsigned int partitionNumber = 0;
	switch (chipIndex) {
	case 0:
		partitionNumber = 0;
		break;
	case 1:
		partitionNumber = 3;
		gpioConfig |= 0x4 << 12;
		break;
	case 2:
		partitionNumber = 1;
		gpioConfig |= 0x1 << 12;
		break;
	case 3:
		partitionNumber = 2;
		gpioConfig |= 0x2 << 12;
		break;
	}

	if (partitionNumber == ((flashConfig >> 12) & 0x3))
		return B_OK;

	flashConfig &= 0xFFFFCFFF;
	flashConfig = (flashConfig | (partitionNumber<<12));

	// TRACE("Writing GPIO config 0x%x\n", &gpioConfig);
	// TRACE("Writing Flash config 0x%x\n", &flashConfig);

	TRACE("Debug: Selecting chip %x.\n", bSelectedChip);

	BizarroWriteRegister(FLASH_GPIO_CONFIG_REG, 4, &gpioConfig);
	snooze(100);

	BizarroWriteRegister(FLASH_CONFIG_REG, 4, &flashConfig);
	snooze(100);

	return B_OK;
}


int
BeceemNVM::NVMRead(unsigned int offset, unsigned int size, unsigned int* buffer)
{
	unsigned int temp = 0;
	unsigned int value = 0;
	int status = 0;
	unsigned int myOffset = 0;

	if (pwmxdevice->nvmType == NVM_FLASH) {

		// TODO : if bFlashRawRead is requested, this will need to be skipped
		// Increase offset by FlashCalibratedStart
		myOffset = offset + pwmxdevice->nvmFlashCalStart;

		TRACE("Debug: Performing read of flash nvm at 0x%x (%d)\n",
			myOffset, size);

		// If firmware was already pushed, Beceem notes a hardware bug here.
		if (pwmxdevice->driverFwPushed == true) {
			// Save data at register
			BizarroReadRegister(0x0f000C80, sizeof(temp), &temp);
			// Flash register with 0
			BizarroWriteRegister(0x0f000C80, sizeof(value), &value);
		}

		status = FlashBulkRead(myOffset, size, buffer);

		if (pwmxdevice->driverFwPushed == true) {
			// Write stored value back
			BizarroWriteRegister(0x0f000C80, sizeof(temp), &temp);
		}

	} else if (pwmxdevice->nvmType == NVM_EEPROM) {
		TRACE_ALWAYS("TODO: NVM_EEPROM Read\n");
		status = -1;
	} else {
		status = -1;
	}

	return status;
}


int
BeceemNVM::NVMWrite(unsigned int offset, unsigned int size,
	unsigned int* buffer)
{
	unsigned int temp = 0;
	unsigned int value = 0;
	int status = 0;

	if (pwmxdevice->nvmType == NVM_FLASH) {
		// Save data at register
		BizarroReadRegister(0x0f000C80, sizeof(temp), &temp);

		// Flash register with 0
		BizarroWriteRegister(0x0f000C80, sizeof(value), &value);

		status = FlashBulkWrite(offset, size, buffer);

		// Write stored value back
		BizarroWriteRegister(0x0f000C80, sizeof(temp), &temp);

	} else if (pwmxdevice->nvmType == NVM_EEPROM) {
		TRACE_ALWAYS("Todo: NVM_EEPROM Write\n");
		status = -1;
	} else {
		status = -1;
	}

	return status;
}


int
BeceemNVM::FlashGetBaseAddr()
{
	if (pwmxdevice->driverDDRinit == true) {
		if (pwmxdevice->nvmFlashCSDone == true
			&& pwmxdevice->nvmFlashRaw == false
			&& !((pwmxdevice->nvmFlashMajor == 1)
			&& (pwmxdevice->nvmFlashMinor == 1))) {
			return pwmxdevice->nvmFlashBaseAddr;
		} else {
			return FLASH_CONTIGIOUS_START_ADDR_AFTER_INIT;
		}
	} else {
		if (pwmxdevice->nvmFlashCSDone == true
			&& pwmxdevice->nvmFlashRaw == false
			&& !((pwmxdevice->nvmFlashMajor == 1)
			&& (pwmxdevice->nvmFlashMinor == 1))) {
			return pwmxdevice->nvmFlashBaseAddr
				| FLASH_CONTIGIOUS_START_ADDR_BEFORE_INIT;
		} else {
			return FLASH_CONTIGIOUS_START_ADDR_BEFORE_INIT;
		}
	}

	TRACE_ALWAYS("Error: no base address?\n");
	return B_ERROR;
}


unsigned int
BeceemNVM::FlashGetSize()
{
	// TODO : Check for Flash2X
	return 32 * 1024;
}


unsigned long
BeceemNVM::FlashReadID()
{
	// Read the ID from FLASH_CMD_READ_ID
	unsigned int value = FLASH_CMD_READ_ID << 24;
	BizarroWriteRegister(FLASH_SPI_CMDQ_REG, sizeof(value), &value);

	snooze(10);

	// Read SPI READQ Register, The output will be 4 bytes long
	// The ID is the first 3 bytes.
	unsigned long readQID = 0;
	BizarroReadRegister(FLASH_SPI_READQ_REG, sizeof(readQID),
		(unsigned int*)&readQID);

	return readQID >> 8;
}


status_t
BeceemNVM::FlashReadCS()
{
	// Set initial start addresses
	pwmxdevice->nvmFlashCSStart = FLASH_CS_INFO_START_ADDR;
	pwmxdevice->nvmFlashBaseAddr = 0;
	pwmxdevice->nvmFlashCalStart = 0;
	pwmxdevice->nvmFlashMajor = 0;
	pwmxdevice->nvmFlashMinor = 0;

	memset(pwmxdevice->nvmFlashCSInfo, 0 , sizeof(FLASH_CS_INFO));

	if (pwmxdevice->driverDDRinit == false)
	{
		unsigned int value = FLASH_CONTIGIOUS_START_ADDR_BEFORE_INIT;
		BizarroWriteRegister(0xAF00A080, sizeof(value), &value);
	}

	// CS Signature(4), Minor(2), Major(2)
	FlashBulkRead(pwmxdevice->nvmFlashCSStart, 8,
		(unsigned int*)pwmxdevice->nvmFlashCSInfo);

	pwmxdevice->nvmFlashCSInfo->FlashLayoutVersion
		= ntohl(pwmxdevice->nvmFlashCSInfo->FlashLayoutVersion);

	TRACE_ALWAYS("Info: Flash CS Version/Signature: 0x%X/0x%X\n",
		(pwmxdevice->nvmFlashCSInfo->FlashLayoutVersion),
		ntohl(pwmxdevice->nvmFlashCSInfo->MagicNumber));

	if (ntohl(pwmxdevice->nvmFlashCSInfo->MagicNumber) == FLASH_CS_SIGNATURE) {
		pwmxdevice->nvmFlashMajor
			= MAJOR_VERSION(pwmxdevice->nvmFlashCSInfo->FlashLayoutVersion);
		pwmxdevice->nvmFlashMinor
			= MINOR_VERSION(pwmxdevice->nvmFlashCSInfo->FlashLayoutVersion);
	} else {
		TRACE_ALWAYS("Error: Unknown flash magic signature!\n");
		pwmxdevice->nvmFlashMajor = 0;
		pwmxdevice->nvmFlashMinor = 0;
		return B_ERROR;
	}

	if (pwmxdevice->nvmFlashMajor == 0x1)
	{
		// device is older flash map
		FlashBulkRead(pwmxdevice->nvmFlashCSStart, sizeof(FLASH_CS_INFO),
			(unsigned int*)pwmxdevice->nvmFlashCSInfo);
		snooze(100);
		FlashCSFlip(pwmxdevice->nvmFlashCSInfo);
		FlashCSDump(pwmxdevice->nvmFlashCSInfo);

		pwmxdevice->nvmFlashCalStart
			= (pwmxdevice->nvmFlashCSInfo->OffsetFromZeroForCalibrationStart);

		if (!((pwmxdevice->nvmFlashMajor == 1)
			&& (pwmxdevice->nvmFlashMinor == 1))) {
			pwmxdevice->nvmFlashCSStart
				= pwmxdevice->nvmFlashCSInfo->OffsetFromZeroForControlSectionStart;
		}

		// TODO : Flash Write sizes.. no write support atm

		pwmxdevice->nvmFlashBaseAddr = pwmxdevice->nvmFlashCSInfo->FlashBaseAddr
			& 0xFCFFFFFF;

	} else {
		// device is newer flash map layout
		TRACE_ALWAYS("New flash map detected, not yet supported!\n");
		return B_ERROR;
	}

	pwmxdevice->nvmFlashID = FlashReadID();
	pwmxdevice->nvmFlashCSDone = true;

	return B_OK;
}


status_t
BeceemNVM::FlashCSFlip(PFLASH_CS_INFO FlashCSInfo)
{
	FlashCSInfo->MagicNumber = ntohl(FlashCSInfo->MagicNumber);
	FlashCSInfo->FlashLayoutVersion = ntohl(FlashCSInfo->FlashLayoutVersion);
	FlashCSInfo->ISOImageVersion = ntohl(FlashCSInfo->ISOImageVersion);

	// won't convert according to old assumption
	FlashCSInfo->SCSIFirmwareVersion = FlashCSInfo->SCSIFirmwareVersion;

	FlashCSInfo->OffsetFromZeroForPart1ISOImage
		= ntohl(FlashCSInfo->OffsetFromZeroForPart1ISOImage);
	FlashCSInfo->OffsetFromZeroForScsiFirmware
		= ntohl(FlashCSInfo->OffsetFromZeroForScsiFirmware);
	FlashCSInfo->SizeOfScsiFirmware
		= ntohl(FlashCSInfo->SizeOfScsiFirmware);
	FlashCSInfo->OffsetFromZeroForPart2ISOImage
		= ntohl(FlashCSInfo->OffsetFromZeroForPart2ISOImage);
	FlashCSInfo->OffsetFromZeroForCalibrationStart
		= ntohl(FlashCSInfo->OffsetFromZeroForCalibrationStart);
	FlashCSInfo->OffsetFromZeroForCalibrationEnd
		= ntohl(FlashCSInfo->OffsetFromZeroForCalibrationEnd);
	FlashCSInfo->OffsetFromZeroForVSAStart
		= ntohl(FlashCSInfo->OffsetFromZeroForVSAStart);
	FlashCSInfo->OffsetFromZeroForVSAEnd
		= ntohl(FlashCSInfo->OffsetFromZeroForVSAEnd);
	FlashCSInfo->OffsetFromZeroForControlSectionStart
		= ntohl(FlashCSInfo->OffsetFromZeroForControlSectionStart);
	FlashCSInfo->OffsetFromZeroForControlSectionData
		= ntohl(FlashCSInfo->OffsetFromZeroForControlSectionData);
	FlashCSInfo->CDLessInactivityTimeout
		= ntohl(FlashCSInfo->CDLessInactivityTimeout);
	FlashCSInfo->NewImageSignature
		= ntohl(FlashCSInfo->NewImageSignature);
	FlashCSInfo->FlashSectorSizeSig
		= ntohl(FlashCSInfo->FlashSectorSizeSig);
	FlashCSInfo->FlashSectorSize
		= ntohl(FlashCSInfo->FlashSectorSize);
	FlashCSInfo->FlashWriteSupportSize
		= ntohl(FlashCSInfo->FlashWriteSupportSize);
	FlashCSInfo->TotalFlashSize
		= ntohl(FlashCSInfo->TotalFlashSize);
	FlashCSInfo->FlashBaseAddr
		= ntohl(FlashCSInfo->FlashBaseAddr);
	FlashCSInfo->FlashPartMaxSize
		= ntohl(FlashCSInfo->FlashPartMaxSize);
	FlashCSInfo->IsCDLessDeviceBootSig
		= ntohl(FlashCSInfo->IsCDLessDeviceBootSig);
	FlashCSInfo->MassStorageTimeout
		= ntohl(FlashCSInfo->MassStorageTimeout);

	return B_OK;
}


status_t
BeceemNVM::FlashCSDump(PFLASH_CS_INFO FlashCSInfo)
{
	TRACE_ALWAYS("************Debug dump of Flash CS Info map************\n");
	TRACE_ALWAYS("                         MagicNumber is 0x%x\n",
		FlashCSInfo->MagicNumber);
	TRACE_ALWAYS("                  FlashLayoutVersion is 0x%x\n",
		FlashCSInfo->FlashLayoutVersion);
	TRACE_ALWAYS("                     ISOImageVersion is 0x%x\n",
		FlashCSInfo->ISOImageVersion);
	TRACE_ALWAYS("                 SCSIFirmwareVersion is 0x%x\n",
		FlashCSInfo->SCSIFirmwareVersion);
	TRACE_ALWAYS("      OffsetFromZeroForPart1ISOImage is 0x%x\n",
		FlashCSInfo->OffsetFromZeroForPart1ISOImage);
	TRACE_ALWAYS("       OffsetFromZeroForScsiFirmware is 0x%x\n",
		FlashCSInfo->OffsetFromZeroForScsiFirmware);
	TRACE_ALWAYS("                  SizeOfScsiFirmware is 0x%x\n",
		FlashCSInfo->SizeOfScsiFirmware);
	TRACE_ALWAYS("      OffsetFromZeroForPart2ISOImage is 0x%x\n",
		FlashCSInfo->OffsetFromZeroForPart2ISOImage);
	TRACE_ALWAYS("   OffsetFromZeroForCalibrationStart is 0x%x\n",
		FlashCSInfo->OffsetFromZeroForCalibrationStart);
	TRACE_ALWAYS("     OffsetFromZeroForCalibrationEnd is 0x%x\n",
		FlashCSInfo->OffsetFromZeroForCalibrationEnd);
	TRACE_ALWAYS("           OffsetFromZeroForVSAStart is 0x%x\n",
		FlashCSInfo->OffsetFromZeroForVSAStart);
	TRACE_ALWAYS("             OffsetFromZeroForVSAEnd is 0x%x\n",
		FlashCSInfo->OffsetFromZeroForVSAEnd);
	TRACE_ALWAYS("OffsetFromZeroForControlSectionStart is 0x%x\n",
		FlashCSInfo->OffsetFromZeroForControlSectionStart);
	TRACE_ALWAYS(" OffsetFromZeroForControlSectionData is 0x%x\n",
		FlashCSInfo->OffsetFromZeroForControlSectionData);
	TRACE_ALWAYS("             CDLessInactivityTimeout is 0x%x\n",
		FlashCSInfo->CDLessInactivityTimeout);
	TRACE_ALWAYS("                   NewImageSignature is 0x%x\n",
		FlashCSInfo->NewImageSignature);
	TRACE_ALWAYS("                  FlashSectorSizeSig is 0x%x\n",
		FlashCSInfo->FlashSectorSizeSig);
	TRACE_ALWAYS("                     FlashSectorSize is 0x%x\n",
		FlashCSInfo->FlashSectorSize);
	TRACE_ALWAYS("               FlashWriteSupportSize is 0x%x\n",
		FlashCSInfo->FlashWriteSupportSize);
	TRACE_ALWAYS("                      TotalFlashSize is 0x%x\n",
		FlashCSInfo->TotalFlashSize);
	TRACE_ALWAYS("                       FlashBaseAddr is 0x%x\n",
		FlashCSInfo->FlashBaseAddr);
	TRACE_ALWAYS("                    FlashPartMaxSize is 0x%x\n",
		FlashCSInfo->FlashPartMaxSize);
	TRACE_ALWAYS("               IsCDLessDeviceBootSig is 0x%x\n",
		FlashCSInfo->IsCDLessDeviceBootSig);
	TRACE_ALWAYS("                  MassStorageTimeout is 0x%x\n",
		FlashCSInfo->MassStorageTimeout);
	TRACE_ALWAYS("*******************************************************\n");

	return B_OK;
}


status_t
BeceemNVM::FlashBulkRead(unsigned int offset, unsigned int size,
	unsigned int* buffer)
{
	if (pwmxdevice->driverHalt == true)
		return -ENODEV;

	if (size > sizeof(&buffer))
		TRACE("Warning: Reading more then the buffer can handle\n");

	bSelectedChip = RESET_CHIP_SELECT;

	TRACE("Debug: About to read %d bytes from 0x%x \n", size, offset);

	unsigned int workOffset = offset; // our scratch work offset
	unsigned int bytesLeft = size; // counter holding bytes left
	unsigned int outputOffset = 0; // where we are in the output

	while (bytesLeft > 0)
	{
		NVMChipSelect(workOffset);

		unsigned int partOffset = (workOffset & (FLASH_PART_SIZE - 1))
			+ FlashGetBaseAddr();
		unsigned int workBytes = MIN(MAX_RW_SIZE, bytesLeft);
			// We read the max RW size or whats left

		TRACE("Debug: reading %d bytes from 0x%x to 0x%x (output offset %d)\n",
			workBytes, partOffset, partOffset + workBytes, outputOffset);

		if (ReadRegister(partOffset, workBytes,
			(unsigned int*)((unsigned char*)buffer + outputOffset)) != B_OK) {
				// I've only done this once before.
				TRACE_ALWAYS("Error: Read error at 0x%x."
					" Read of %d bytes failed.\n",
					partOffset, workBytes);
				bSelectedChip = RESET_CHIP_SELECT;
				return B_ERROR;
		}

		bytesLeft 		-= workBytes;
			// subtract the bytes we read
		workOffset		+= workBytes;
			// add the bytes we read to the offset
		outputOffset	+= workBytes;
			// increase output location by number of bytes worked on
	}

	bSelectedChip = RESET_CHIP_SELECT;
	return B_OK;
}


status_t
BeceemNVM::RestoreBlockProtect(unsigned long writestatus)
{
	unsigned int value = (FLASH_CMD_WRITE_ENABLE<< 24);
	BizarroWriteRegister(FLASH_SPI_CMDQ_REG, sizeof(value), &value);

	snooze(20);
	value = (FLASH_CMD_STATUS_REG_WRITE<<24)|(writestatus << 16);
	BizarroWriteRegister(FLASH_SPI_CMDQ_REG, sizeof(value), &value);
	snooze(20);

	return B_OK;
}


unsigned long
BeceemNVM::DisableBlockProtect(unsigned int offset, size_t size)
{
	return 0;
}


status_t
BeceemNVM::FlashBulkWrite(unsigned int offset, unsigned int size,
	unsigned int* buffer)
{
	// TODO : Implement flash writing, not really needed for normal use
	TRACE_ALWAYS("%s: Not yet implemented\n", __func__);
	return B_ERROR;
}


status_t
BeceemNVM::FlashSectorErase(unsigned int addr, unsigned int numOfSectors)
{
	TRACE("Debug: Erasing %d sectors at 0x%x\n", numOfSectors, addr);

	unsigned int uiStatus = 0;
	unsigned int iIndex;
	for (iIndex = 0 ; iIndex < numOfSectors ; iIndex++) {
		unsigned int value = 0x06000000;
		BizarroWriteRegister(FLASH_SPI_CMDQ_REG, sizeof(value), &value);

		value = (0xd8000000 | (addr & 0xFFFFFF));
		BizarroWriteRegister(FLASH_SPI_CMDQ_REG, sizeof(value), &value);
		unsigned int iRetries = 0;

		do {
			value = (FLASH_CMD_STATUS_REG_READ << 24);
			if ( BizarroWriteRegister(FLASH_SPI_CMDQ_REG,
				sizeof(value), &value) != B_OK ) {
				TRACE_ALWAYS("Error: Failure writing FLASH_SPI_CMDQ_REG\n");
				return B_ERROR;
			}

			if ( BizarroReadRegister(FLASH_SPI_READQ_REG,
				sizeof(uiStatus), &uiStatus) != B_OK) {
				TRACE_ALWAYS("Error: Failure reading FLASH_SPI_READQ_REG\n");
				return B_ERROR;
			}
			iRetries++;
			// After every try lets make the CPU free for 10 ms. generally time
			// taken by the the sector erase cycle is 500 ms to 40000 msec.
			// Sleeping 10 ms won't hamper performance in any case.
			snooze(10);
		} while ((uiStatus & 0x1) && (iRetries < 400));

		if (uiStatus & 0x1) {
			TRACE_ALWAYS("Error: sector erase retry loop exhausted");
			return B_ERROR;
		}

		addr += NVM_SECTOR_SIZE;
	}

	return B_OK;
}


status_t
BeceemNVM::ReadMACFromNVM(ether_address *address)
{
	unsigned char MacAddr[6] = {0};
	status_t status = NVMRead(MAC_ADDR_OFFSET, 6, (unsigned int*)&MacAddr[0]);
	memcpy(address, MacAddr, 6);

	return (status);
}


unsigned int
BeceemNVM::EEPROMGetSize()
{
	// To find the EEPROM size read the possible boundaries of the
	// EEPROM like 4K,8K etc..accessing the EEPROM beyond its size will
	// result in wrap around. So when we get the End of the EEPROM we will
	// get 'BECM' string which is indeed at offset 0.

	unsigned int uiData = 0;
	EEPROMBulkRead(0x0, 4, &uiData);

	if (ntohl(uiData) == BECM) {
		// If EEPROM is present, it will have 'BECM' string at 0th offset.
		unsigned int uiIndex;
		for (uiIndex = 1 ; uiIndex <= 256; uiIndex *= 2) {
			EEPROMBulkRead(uiIndex * 1024, 4, &uiData);

			if (ntohl(uiData) == BECM)
				return uiIndex * 1024;
		}
	}

	return B_ERROR;
}


status_t
BeceemNVM::EEPROMRead(unsigned int offset, unsigned int *pdwData)
{
	// Read 4 bytes from EEPROM

	// read 0x0f003020 until bit 2 of 0x0f003008 is set.
	unsigned int regValue = 0;
	BizarroReadRegister(EEPROM_SPI_Q_STATUS_REG,
		sizeof(unsigned int), &regValue);

	unsigned int retries = 16;
	while (((regValue >> 2) & 1) == 0) {
		retries--;

		snooze(200);

		if (retries == 0) {
			TRACE_ALWAYS("Error: CMD FIFO is not empty\n");
			return B_ERROR;
		}

		BizarroReadRegister(EEPROM_SPI_Q_STATUS_REG,
			sizeof(unsigned int), &regValue);
	}

	// wrm (0x0f003018, 0xNbXXXXXX)
	// N is the number of bytes u want to read
	// (0 means 1, f means 16, b is the opcode for page read)

	// Follow it up by N executions of rdm(0x0f003020) to read
	// the rxed bytes from rx queue.

	offset |= 0x3b000000;

	BizarroWriteRegister(EEPROM_CMDQ_SPI_REG, sizeof(unsigned int), &offset);

	retries = 50;

	BizarroReadRegister(EEPROM_SPI_Q_STATUS_REG, sizeof(unsigned int),
		&regValue);

	while (((regValue >> 1) & 1) == 1) {
		retries--;

		snooze(200);

		if (retries == 0) {
			TRACE_ALWAYS("Error: READ FIFO is empty\n");
			return B_ERROR;
		}

		BizarroReadRegister(EEPROM_SPI_Q_STATUS_REG, sizeof(unsigned int),
			&regValue);
	}

	BizarroReadRegister(EEPROM_READ_DATAQ_REG, sizeof(unsigned int), &regValue);

	unsigned int dwReadValue = regValue;

	BizarroReadRegister(EEPROM_READ_DATAQ_REG, sizeof(unsigned int), &regValue);

	dwReadValue |= regValue << 8;

	BizarroReadRegister(EEPROM_READ_DATAQ_REG, sizeof(unsigned int), &regValue);

	dwReadValue |= regValue << 16;

	BizarroReadRegister(EEPROM_READ_DATAQ_REG, sizeof(unsigned int), &regValue);

	dwReadValue |= regValue << 24;

	*pdwData = dwReadValue;

	return B_OK;
}


status_t
BeceemNVM::EEPROMBulkRead(unsigned int offset, size_t numBytes,
	unsigned int* buffer)
{
	unsigned int	uiData[4] = {0};
	unsigned int	uiBytesRemaining = numBytes;
	unsigned int	uiIndex = 0;

	unsigned int	uiTempOffset = 0;
	unsigned int	uiExtraBytes = 0;
	unsigned char*	pcBuff = (unsigned char*)buffer;

	TRACE("Debug: Reading %x bytes at offset %x.\n", numBytes, offset);

	if (offset%16 && uiBytesRemaining) {
		uiTempOffset = offset - (offset%16);
		uiExtraBytes = offset - uiTempOffset;

		EEPROMBulkRead(uiTempOffset, 16, (unsigned int*)&uiData[0]);

		if (uiBytesRemaining >= (16 - uiExtraBytes)) {
			memcpy(buffer,
				(((unsigned char*)&uiData[0])+uiExtraBytes),
				16 - uiExtraBytes);
			uiBytesRemaining -= (16 - uiExtraBytes);
			uiIndex += (16 - uiExtraBytes);
			offset += (16 - uiExtraBytes);
		} else {
			memcpy(buffer,
				(((unsigned char*)&uiData[0])+uiExtraBytes),
				uiBytesRemaining);
			uiIndex += uiBytesRemaining;
			offset += uiBytesRemaining;
			uiBytesRemaining = 0;
		}
	}

	while (uiBytesRemaining) {
		if (uiBytesRemaining >= 16) {
			// for the requests more than or equal to 16 bytes,
			// use bulk read function to make the access faster.

			// TODO : Implement function to read more data faster
			// ReadBeceemEEPROMBulk(offset, &uiData[0]);
			EEPROMRead(offset, &uiData[0]);
			memcpy(pcBuff + uiIndex, &uiData[0], 16); // yes requested.
			offset += 16;
			uiBytesRemaining -= 16;
			uiIndex += 16;
		} else if (uiBytesRemaining >= 4) {
			EEPROMRead(offset, &uiData[0]);
			memcpy(pcBuff + uiIndex, &uiData[0], 4); // yes requested.
			offset += 4;
			uiBytesRemaining -= 4;
			uiIndex+= 4;
		} else {
			// Handle the reads less than 4 bytes...
			unsigned char*	pCharBuff = (unsigned char*)buffer;
			pCharBuff += uiIndex;
			EEPROMRead(offset, &uiData[0]); // read 4 byte
			memcpy(pCharBuff, &uiData[0], uiBytesRemaining);
				// copy only the bytes requested.
			uiBytesRemaining = 0;
		}
	}

	return B_OK;
}


status_t
BeceemNVM::ValidateDSD(unsigned long hwParam)
{
	unsigned long dwReadValue = hwParam + DSD_START_OFFSET;
		// Get DSD start offset

	//   DSD
	//  +---+
	// B| 2 |  Hardware Param Length
	// Y+---+
	// T| X |  Hardware Param
	// E+---+
	// S| 2 |  Stored Checksum
	//  +---+

	/* Read the Length of structure */
	unsigned short hwParamLen = 0;
	NVMRead(dwReadValue, 2, (unsigned int*)&hwParamLen);
	hwParamLen = ntohs(hwParamLen);

	/* Validate length */
	if (0==hwParamLen || hwParamLen > MAX_NVM_SIZE) {
		TRACE_ALWAYS("Error: Param length is invalid! (%d)\n", hwParamLen);
		return B_ERROR;
	} else
		TRACE("Debug: Found valid param length. (%d)\n", hwParamLen);

	unsigned char* puBuffer = (unsigned char*)malloc(hwParamLen);
		// Allocate memory to calculate checksum on

	if (!puBuffer) {
		TRACE_ALWAYS("Error: HW_PARAM buffer is too small!\n");
		free(puBuffer);
		return B_ERROR;
	}

	NVMRead(dwReadValue, hwParamLen, (unsigned int*)puBuffer);
		// Populate allocated memory with string for checksum

	unsigned short usChksmCalc = 0;
	usChksmCalc = CalculateHWChecksum(puBuffer, hwParamLen);
		// Perform checksum on values

	unsigned short usChksmOrg = 0;
	NVMRead(dwReadValue + hwParamLen, 2, (unsigned int*)&usChksmOrg);
		// Read what the device thinks the checksum should be

	usChksmOrg = ntohs(usChksmOrg);
		// Compare the checksum calculated with the checksum in nvm

	if (usChksmCalc ^ usChksmOrg) {
		TRACE_ALWAYS("Error: Param checksum mismatch!\n");
		TRACE_ALWAYS("       HW Param Length = %d\n", hwParamLen);
		TRACE_ALWAYS("       Calculated checksum = %x\n", usChksmCalc);
		TRACE_ALWAYS("       Stored checksum = %x\n", usChksmOrg);
		free(puBuffer);
		return B_ERROR;
	} else {
		TRACE_ALWAYS("Info: Hardware param checksum valid."
			" Calculated: 0x%x, Stored: 0x%x\n",
			usChksmCalc, usChksmOrg);
	}

	free(puBuffer);

	return B_OK;
}

