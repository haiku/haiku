/* 	ACPI Bus Manger Interface
 *  Copyright 2005, Haiku Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License
 */
 
#ifndef _ACPI_H
#define _ACPI_H

#include <device_manager.h>
#include <bus_manager.h>
#include <KernelExport.h>

typedef struct acpi_module_info acpi_module_info;
typedef struct acpi_object_type acpi_object_type;

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
	status_t			(*get_device) (const char *hid, uint32 index, char *result);
	
	status_t			(*get_device_hid) (const char *path, char *hid);
	uint32				(*get_object_type) (const char *path);
	status_t			(*get_object) (const char *path, acpi_object_type **return_value);
	status_t			(*get_object_typed) (const char *path, acpi_object_type **return_value, uint32 object_type);
	
	/* Control method execution and data acquisition */
	
	status_t			(*evaluate_object) (const char *object, acpi_object_type *return_value, size_t buf_len);
	status_t			(*evaluate_method) (const char *object, const char *method, acpi_object_type *return_value, size_t buf_len, acpi_object_type *args, int num_args);
	/* Power state setting */
	
	status_t			(*enter_sleep_state) (uint8 state);
		/* Sleep state values:
			0: On (Working)
			1: Sleep
			2: Software Off
			3: Mechanical Off
			4: Hibernate
			5: Software Off */
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

/* ACPI control method arg type */

struct acpi_object_type {
	uint32 object_type;
	union {
		uint32 integer;
		struct {
			uint32 len;
			char *string; /* You have to allocate string space yourself */
		} string;
		struct {
			size_t length;
			void *buffer;
		} buffer;
		struct {
			uint32 count;
			acpi_object_type *objects;
		} package;
		struct {
			uint32 cpu_id;
			
			int pblk_address;
			size_t pblk_length;
		} processor;
		struct {
			uint32 min_power_state;
			uint32 resource_order;
		} power_resource;
	} data;
};

#endif

#define B_ACPI_MODULE_NAME	"bus_managers/acpi/v1"

#define ACPI_DEVICE_HID_ITEM "acpi/hid"
#define ACPI_DEVICE_PATH_ITEM "acpi/path"
#define ACPI_DEVICE_TYPE_ITEM "acpi/type"


typedef struct acpi_device_info *acpi_device;


#endif /* _ACPI_H */
