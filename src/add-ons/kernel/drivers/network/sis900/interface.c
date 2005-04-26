/* MII, MDIO and EEPROM interface of SiS 900
 *
 * Copyright 2001-2005 pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */


#include <OS.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <PCI.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "ether_driver.h"
#include "driver.h"
#include "device.h"
#include "interface.h"
#include "sis900.h"


// EEPROM access definitions
#define EEPROM_DELAY()			read32(eepromAccess)
#define EEPROM_READ32()			read32(eepromAccess)
#define EEPROM_WRITE8(value)	write8(eepromAccess,value)
#define EEPROM_WRITE32(value)	write32(eepromAccess,value)


uint16
eeprom_read(struct sis_info *info, int address)
{
	long eepromAccess = (long)info->registers + SiS900_MAC_EEPROM_ACCESS;
	uint32 readCmd = SiS900_EEPROM_CMD_READ | address;
	uint16 returnValue = 0;
	int i;

	EEPROM_WRITE32(0);
	EEPROM_DELAY();
	EEPROM_WRITE32(SiS900_EEPROM_CLOCK);
	EEPROM_DELAY();

	// Shift the read command (9) bits out.
	for (i = 8; i >= 0; i--) {
		uint32 value = (readCmd & (1 << i))	? SiS900_EEPROM_DATA_IN | SiS900_EEPROM_SELECT
											: SiS900_EEPROM_SELECT;

		EEPROM_WRITE32(value);
		EEPROM_DELAY();
		EEPROM_WRITE32(value | SiS900_EEPROM_CLOCK);
		EEPROM_DELAY();
	}

	EEPROM_WRITE8(SiS900_EEPROM_SELECT);
	EEPROM_DELAY();

	// read in the 16 bit data
	for (i = 16; i > 0; i--) {
		EEPROM_WRITE32(SiS900_EEPROM_SELECT);
		EEPROM_DELAY();
		EEPROM_WRITE32(SiS900_EEPROM_SELECT | SiS900_EEPROM_CLOCK);
		EEPROM_DELAY();
		returnValue = (returnValue << 1) | ((EEPROM_READ32() & SiS900_EEPROM_DATA_OUT) ? 1 : 0);
		EEPROM_DELAY();
	}

	EEPROM_WRITE32(0);
	EEPROM_DELAY();
	EEPROM_WRITE32(SiS900_EEPROM_CLOCK);

	return returnValue;
}


/**************************** MII/MDIO ****************************/
// #pragma mark -


static inline uint32
mdio_delay(addr_t address)
{
	return read32(address);
}


static void
mdio_idle(uint32 address)
{
	write32(address, SiS900_MII_MDIO | SiS900_MII_MDDIR);
	mdio_delay(address);
	write32(address, SiS900_MII_MDIO | SiS900_MII_MDDIR | SiS900_MII_MDC);
}


static void
mdio_reset(uint32 address)
{
	int32 i;

	for (i = 32; i-- > 0;) {
		write32(address, SiS900_MII_MDIO | SiS900_MII_MDDIR);
		mdio_delay(address);
		write32(address, SiS900_MII_MDIO | SiS900_MII_MDDIR | SiS900_MII_MDC);
		mdio_delay(address);
	}
}


void
mdio_writeToPHY(struct sis_info *info, uint16 phy, uint16 reg, uint16 value)
{
	uint32 address = info->registers + SiS900_MAC_EEPROM_ACCESS;
	int32 cmd = MII_CMD_WRITE | (phy << MII_PHY_SHIFT) | (reg << MII_REG_SHIFT);
	int i;

	mdio_reset(address);
	mdio_idle(address);

	// issue the command
	for (i = 16; i-- > 0;) {
		int32 data = SiS900_MII_MDDIR | (cmd & (1 << i) ? SiS900_MII_MDIO : 0);

		write8(address, data);
		mdio_delay(address);
		write8(address, data | SiS900_MII_MDC);
		mdio_delay(address);
	}
	mdio_delay(address);

	// write the value
	for (i = 16; i-- > 0;) {
		int32 data = SiS900_MII_MDDIR | (value & (1 << i) ? SiS900_MII_MDIO : 0);

		write32(address, data);
		mdio_delay(address);
		write32(address, data | SiS900_MII_MDC);
		mdio_delay(address);
	}
	mdio_delay(address);

	// clear extra bits
	for (i = 2; i-- > 0;) {
		write8(address, 0);
		mdio_delay(address);
		write8(address, SiS900_MII_MDC);
		mdio_delay(address);
	}

	write32(address, 0);
}


uint16
mdio_readFromPHY(struct sis_info *info, uint16 phy, uint16 reg)
{
	uint32 address = info->registers + SiS900_MAC_EEPROM_ACCESS;
	int32 cmd = MII_CMD_READ | (phy << MII_PHY_SHIFT) | (reg << MII_REG_SHIFT);
	uint16 value = 0;
	int i;

	mdio_reset(address);
	mdio_idle(address);

	for (i = 16; i-- > 0;) {
		int32 data = SiS900_MII_MDDIR | (cmd & (1 << i) ? SiS900_MII_MDIO : 0);

		write32(address, data);
		mdio_delay(address);
		write32(address, data | SiS900_MII_MDC);
		mdio_delay(address);
	}

	// read the value
	for (i = 16; i-- > 0;) {
		write32(address, 0);
		mdio_delay(address);
		value = (value << 1) | (read32(address) & SiS900_MII_MDIO ? 1 : 0);
		write32(address, SiS900_MII_MDC);
		mdio_delay(address);
	}
	write32(address, 0);

	return value;
}


uint16 
mdio_read(struct sis_info *info, uint16 reg)
{
	return mdio_readFromPHY(info, info->phy, reg);
}


void 
mdio_write(struct sis_info *info, uint16 reg, uint16 value)
{
	mdio_writeToPHY(info,info->phy,reg,value);
}


uint16
mdio_statusFromPHY(struct sis_info *info, uint16 phy)
{
	uint16 status;
	int i = 0;

	// the status must be retrieved two times, because the first
	// one may not work on some PHYs (notably ICS 1893)
	while (i++ < 2)
		status = mdio_readFromPHY(info, phy, MII_STATUS);

	return status;
}


uint16
mdio_status(struct sis_info *info)
{
	return mdio_statusFromPHY(info, info->phy);
}

