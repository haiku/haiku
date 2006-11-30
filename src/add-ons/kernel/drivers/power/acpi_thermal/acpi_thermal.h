/* ++++++++++
     Public interface to the ACPI thermal device driver.
+++++ */
#ifndef _ACPI_THERMAL_H
#define _ACPI_THERMAL_H

#include <KernelExport.h>
#include <ACPI.h>

enum { /* ioctl op-codes */
	drvOpGetThermalType = B_DEVICE_OP_CODES_END + 10001,
}; 


typedef struct acpi_thermal_active_object acpi_thermal_active_object;
typedef struct acpi_thermal_type acpi_thermal_type;

struct acpi_thermal_type {
	/* Required fields for thermal devices */
	uint32 critical_temp;
	uint32 current_temp;
	
	/* Optional HOT temp, S4 sleep threshold */
	uint32 hot_temp;
	
	/* acpi_objects_type evaluated from _PSL 
	   if != NULL, client must free()
	*/
	acpi_object_type *passive_package;
	
	/* List of acpi_thermal_active_objects that can actively manage this thermal device */
	uint32 active_count;
	acpi_thermal_active_object *active_devices;
};

struct acpi_thermal_active_object {
	uint32 threshold_temp;
	acpi_object_type *active_package;
};

#endif /* _ACPI_THERMAL_H */
