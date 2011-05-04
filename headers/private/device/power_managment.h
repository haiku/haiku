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

	// CPU Frequence:
	// get a list of freq_info, the list is terminated with a element with
	// frequency = 0
	GET_CPU_FREQ_STATES = B_DEVICE_OP_CODES_END + 20005,
	// get and set a freq_info
	GET_CURENT_CPU_FREQ_STATE,
	SET_CPU_FREQ_STATE,
	// start watching for frequency changes, ioctl blocks until the frequency
	// has changed
	WATCH_CPU_FREQ,
	// stop all watching ioctl, ioctl return B_ERROR
	STOP_WATCHING_CPU_FREQ,

	GET_BATTERY_INFO,
	GET_EXTENDED_BATTERY_INFO,
	WATCH_BATTERY,
	STOP_WATCHING_BATTERY
};


// CPU Frequence:
// magic id returned by IDENTIFY_DEVICE
const uint32 kMagicFreqID = 48921;


#define MAX_CPU_FREQUENCY_STATES 10


typedef struct {
	uint16			frequency;	// [Mhz]
	uint16			volts;
	uint16			id;
	int				power;
} freq_info;


// ACPI Battery:
// magic id returned by IDENTIFY_DEVICE
const uint32 kMagicACPIBatteryID = 17822;


// Our known battery states
#define BATTERY_DISCHARGING		0x01
#define BATTERY_CHARGING		0x02
#define BATTERY_CRITICAL_STATE	0x04


typedef struct {
	int		state;
	int		current_rate;
	int 	capacity;
	int		voltage;
} acpi_battery_info;


typedef struct {
	int		power_unit;
	int		design_capacity;
	int		last_full_charge;
	int		technology;
	int 	design_voltage;
	int		design_capacity_warning;
	int		design_capacity_low;
	int		capacity_granularity_1;
	int		capacity_granularity_2;
	char	model_number[32];
	char	serial_number[32];
	char	type[32];
	char	oem_info[32];
} acpi_extended_battery_info;


#endif
