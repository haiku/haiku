/* 	ACPI Bus Manger Interface
 *  Copyright 2005, Haiku Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License
 */
 
#ifndef _ACPI_H
#define _ACPI_H

#include <bus_manager.h>
#include <KernelExport.h>

typedef struct acpi_module_info acpi_module_info;

struct acpi_module_info {
	bus_manager_info	binfo;
	
	void				(*enable_fixed_event) (uint32 event);
	void				(*disable_fixed_event) (uint32 event);
	
	uint32				(*fixed_event_status) (uint32 event);
						/* Returns 1 if event set, 0 otherwise */
	void				(*reset_fixed_event) (uint32 event);
	
	status_t			(*install_fixed_event_handler)	(uint32 event, interrupt_handler *handler, void *data); 
	status_t			(*remove_fixed_event_handler)	(uint32 event, interrupt_handler *handler); 
};

#define B_ACPI_MODULE_NAME	"bus_managers/acpi/v1"

#ifndef __ACTYPES_H__

/* ACPI fixed event types */

#define ACPI_EVENT_PMTIMER              0
#define ACPI_EVENT_GLOBAL               1
#define ACPI_EVENT_POWER_BUTTON         2
#define ACPI_EVENT_SLEEP_BUTTON         3
#define ACPI_EVENT_RTC                  4

#endif

#endif /* _ACPI_H */
