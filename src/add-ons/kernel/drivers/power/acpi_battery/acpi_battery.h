/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */

#ifndef ACPI_BATTERY_H
#define ACPI_BATTERY_H

#include <KernelExport.h>
#include <ACPI.h>

#include "device/power_managment.h"

//#define TRACE_BATTERY
#ifdef TRACE_BATTERY
#	define TRACE(x...) dprintf("acpi_battery: "x)
#else
#	define TRACE(x...)
#endif


#define ACPI_NAME_BATTERY "PNP0C0A"

struct battery_driver_cookie {
	device_node*				node;
	acpi_device_module_info*	acpi;
	acpi_device					acpi_cookie;
};


struct battery_device_cookie {
	battery_driver_cookie*		driver_cookie;
	vint32						stop_watching;
};


void battery_notify_handler(acpi_handle device, uint32 value, void *context);

#endif
