/*
 * Copyright 2004-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef POWER_MANAGMENT_H
#define POWER_MANAGMENT_H


#include <Drivers.h>


// io controls
enum {
	// ioctl response with kMagicFreqID
	IDENTIFY_DEVICE = B_DEVICE_OP_CODES_END + 20001,

	GET_BATTERY_INFO,
	GET_EXTENDED_BATTERY_INFO,
	WATCH_BATTERY,
	STOP_WATCHING_BATTERY
};


// ACPI Battery:
// magic id returned by IDENTIFY_DEVICE
const uint32 kMagicACPIBatteryID = 17822;


// Our known battery states
#define BATTERY_DISCHARGING		0x01
#define BATTERY_CHARGING		0x02
#define BATTERY_CRITICAL_STATE	0x04

#define BATTERY_MAX_STRING_LENGTH	32


typedef struct {
	uint32	state;
	uint32	current_rate;
	uint32 	capacity;
	uint32	voltage;
} acpi_battery_info;


typedef struct {
	uint32	power_unit;
#define ACPI_BATTERY_UNIT_MW	0
#define ACPI_BATTERY_UNIT_MA	1
	uint32	design_capacity;
	uint32	last_full_charge;
	uint32	technology;
	uint32 	design_voltage;
	uint32	design_capacity_warning;
	uint32	design_capacity_low;
	uint32	capacity_granularity_1;
	uint32	capacity_granularity_2;
	char	model_number[BATTERY_MAX_STRING_LENGTH];
	char	serial_number[BATTERY_MAX_STRING_LENGTH];
	char	type[BATTERY_MAX_STRING_LENGTH];
	char	oem_info[BATTERY_MAX_STRING_LENGTH];
	// ACPI 4.0 and later
	uint16	revision;
#define ACPI_BATTERY_REVISION_0		0
#define ACPI_BATTERY_REVISION_1		1
#define ACPI_BATTERY_REVISION_BIF	0xffff
	uint32	cycles;
	uint32	accuracy;
	uint32	max_sampling_time;
	uint32	min_sampling_time;
	uint32	max_average_interval;
	uint32	min_average_interval;
	// ACPI 6.0 and later
	uint32	swapping_capability;
#define ACPI_BATTERY_SWAPPING_NO	0
#define ACPI_BATTERY_SWAPPING_COLD	1
#define ACPI_BATTERY_SWAPPING_HOT	2
} acpi_extended_battery_info;


#endif
