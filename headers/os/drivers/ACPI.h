/* 	ACPI Bus Manger Interface
 *  Copyright 2005, Haiku Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License
 */
 
#ifndef _ACPI_H
#define _ACPI_H

#include <bus_manager.h>

typedef struct acpi_module_info acpi_module_info;

struct acpi_module_info {
	bus_manager_info	binfo;

	uint32				(*read_acpi_reg) (uint32 register_id);
	void				(*write_acpi_reg) (uint32 register_id, uint32 value);
};

#define B_ACPI_MODULE_NAME	"bus_managers/acpi/v1"

#ifndef __ACTYPES_H__

/* ACPI register_id arguments */

#define ACPI_BITREG_TIMER_STATUS                0x00
#define ACPI_BITREG_BUS_MASTER_STATUS           0x01
#define ACPI_BITREG_GLOBAL_LOCK_STATUS          0x02
#define ACPI_BITREG_POWER_BUTTON_STATUS         0x03
#define ACPI_BITREG_SLEEP_BUTTON_STATUS         0x04
#define ACPI_BITREG_RT_CLOCK_STATUS             0x05
#define ACPI_BITREG_WAKE_STATUS                 0x06

#define ACPI_BITREG_TIMER_ENABLE                0x07
#define ACPI_BITREG_GLOBAL_LOCK_ENABLE          0x08
#define ACPI_BITREG_POWER_BUTTON_ENABLE         0x09
#define ACPI_BITREG_SLEEP_BUTTON_ENABLE         0x0A
#define ACPI_BITREG_RT_CLOCK_ENABLE             0x0B
#define ACPI_BITREG_WAKE_ENABLE                 0x0C

#define ACPI_BITREG_SCI_ENABLE                  0x0D
#define ACPI_BITREG_BUS_MASTER_RLD              0x0E
#define ACPI_BITREG_GLOBAL_LOCK_RELEASE         0x0F
#define ACPI_BITREG_SLEEP_TYPE_A                0x10
#define ACPI_BITREG_SLEEP_TYPE_B                0x11
#define ACPI_BITREG_SLEEP_ENABLE                0x12

#define ACPI_BITREG_ARB_DISABLE                 0x13

#endif

#endif /* _ACPI_H */
