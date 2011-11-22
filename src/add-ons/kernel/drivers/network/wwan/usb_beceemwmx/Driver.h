/*
 *	Beceem WiMax USB Driver
 *	Copyright 2010-2011 Haiku, Inc. All rights reserved.
 *	Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 *
 *	Partially using:
 *		USB Ethernet Control Model devices
 *			(c) 2008 by Michael Lotz, <mmlr@mlotz.ch>
 *		ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver
 *			(c) 2008 by S.Zharski, <imker@gmx.li>
 */
#ifndef _USB_BECEEM_DRIVER_H_
#define _USB_BECEEM_DRIVER_H_


#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <USB3.h>
#include <ether_driver.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <directories.h>
#include <util/kernel_cpp.h>


#define DRIVER_NAME	"usb_beceemwmx"
#define MAX_DEVICES	8

#define FIRM_BIN	\
	kSystemDataDirectory "/firmware/macxvi200/macxvi200.bin"
	// location on file system of device firmware
#define FIRM_CFG	\
	kSystemDataDirectory "/firmware/macxvi200/macxvi.cfg"
	// location on file system of vendor configuration

#define SYS_CFG			0x0F000C00

#define FIRM_BEGIN_ADDR		0xBFC00000
	// register we push firmware to
#define CONF_BEGIN_ADDR		0xBF60B004
	// register we push vendor configuration to

#define EEPROM_COMMAND_Q_REG			0x0F003018
#define EEPROM_READ_DATA_Q_REG			0x0F003020
#define EEPROM_CAL_DATA_INTERNAL_LOC	0x1FB00008

#define DSD_START_OFFSET		0x200
	// DSD start offset
#define GPIO_START_OFFSET		0x3
	// offset of GPIO start
#define GPIO_PARAM_POINTER		0x0218
	// offset on DSD containing GPIO paramaters (flash map <5)
#define GPIO_PARAM_POINTER_MAP5	0x0220
	// offset on DSD containing GPIO paramaters (flash map 5>)
#define GPIO_MODE_REGISTER		0x0F000034
	// register to set GPIO mode (input or output)
#define GPIO_OUTPUT_SET_REG		0x0F000040
	// register to set active GPIO pin, 1 enables, 0 does nothing
#define GPIO_OUTPUT_CLR_REG		0x0F000044
	// register to clear active GPIO pin, 1 enables, 0 does nothing
#define GPIO_DISABLE_VAL		0xFF
	// value of a disabled GPIO pin
#define NUM_OF_LEDS				4
	// number of possible GPIO leds

#define CLOCK_RESET_CNTRL_REG_1	0x0F00000C
#define HPM_CONFIG_LDO145		0x0F000D54

#define CHIP_ID_REG				0x0F000000
	// register containing chipset ID

//Beceem chip models	|Chip_ID	|NVM type
//UNKNOWN				0xbece0110
//UNKNOWN				0xbece0120
//UNKNOWN				0xbece0121
//UNKNOWN				0xbece0130
#define T3				0xbece0300	// EEPROM
#define T3B				0xbece0310
//UNKNOWN				0xbece3200
#define T3LPB			0xbece3300
#define BCS250_BC		0xbece3301	// FLASH
#define BCS220_2		0xbece3311
#define BCS220_2BC		0xbece3310
#define BCS220_3		0xbece3321

// Driver states for pwmxdevice->driverState
#define STATE_INIT		0x1		// DRIVER_INIT
#define STATE_FWPUSH	0x2		// FW_DOWNLOAD
#define STATE_FWPUSH_OK	0x4		// FW_DOWNLOAD_DONE
#define STATE_NONET		0x8		// NO_NETWORK_ENTRY
#define STATE_NORMAL	0x10	// NORMAL_OPERATION
#define STATE_IDLEENTER	0x20	// IDLEMODE_ENTER
#define STATE_IDLECONT	0x40	// IDLEMODE_CONTINUE
#define STATE_IDLEEXIT	0x80	// IDLEMODE_EXIT
#define STATE_HALT		0x00	// SHUTDOWN_EXIT


const uint8 kInvalidRequest = 0xff;

// version of bcm driver this was based upon + end .1
const char* const kVersion = "ver.5.2.45.1";

extern usb_module_info *gUSBModule;


extern "C" {
status_t	usb_beceem_device_added(usb_device device, void **cookie);
status_t	usb_beceem_device_removed(void *cookie);

status_t	init_hardware();
void		uninit_driver();

const char **publish_devices();
device_hooks *find_device(const char *name);
}


#endif //_USB_BECEEM_DRIVER_H_

