/* 	ACPI Bus Manger Interface
 *  Copyright 2005, Haiku Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License
 */
 
#ifndef _ACPI_H
#define _ACPI_H

#include <bus_manager.h>
#include <KernelExport.h>

typedef struct acpi_module_info acpi_module_info;
typedef struct acpi_object_info acpi_object_info;

struct acpi_module_info {
	bus_manager_info	binfo;
	
	/* Fixed Event Management */
	
	void				(*enable_fixed_event) (uint32 event);
	void				(*disable_fixed_event) (uint32 event);
	
	uint32				(*fixed_event_status) (uint32 event);
						/* Returns 1 if event set, 0 otherwise */
	void				(*reset_fixed_event) (uint32 event);
	
	status_t			(*install_fixed_event_handler)	(uint32 event, interrupt_handler *handler, void *data); 
	status_t			(*remove_fixed_event_handler)	(uint32 event, interrupt_handler *handler); 

	/* Namespace Access */
	
	status_t			(*get_next_entry) (uint32 object_type, const char *base, char *result, size_t len, void **counter);
	status_t			(*get_device_hid) (const char *path, char *hid);
	uint32				(*get_object_type) (const char *path);
	
	/* Control method execution and data acquisition */
	
	status_t			(*evaluate_object) (const char *object, void *return_value, size_t buf_len);
};


#ifndef __ACTYPES_H__

/* ACPI fixed event types */

enum {
	ACPI_EVENT_PMTIMER = 0,
	ACPI_EVENT_GLOBAL,
	ACPI_EVENT_POWER_BUTTON,
	ACPI_EVENT_SLEEP_BUTTON,
	ACPI_EVENT_RTC
};

/* ACPI Object Types */

enum {
	ACPI_TYPE_ANY = 0,
	ACPI_TYPE_INTEGER,
	ACPI_TYPE_STRING,
	ACPI_TYPE_BUFFER,
	ACPI_TYPE_PACKAGE,
	ACPI_TYPE_FIELD_UNIT,
	ACPI_TYPE_DEVICE,
	ACPI_TYPE_EVENT,
	ACPI_TYPE_METHOD,
	ACPI_TYPE_MUTEX,
	ACPI_TYPE_REGION,
	ACPI_TYPE_POWER,
	ACPI_TYPE_PROCESSOR,
	ACPI_TYPE_THERMAL,
	ACPI_TYPE_BUFFER_FIELD
};

#endif

#define B_ACPI_MODULE_NAME	"bus_managers/acpi/v1"

#endif /* _ACPI_H */
