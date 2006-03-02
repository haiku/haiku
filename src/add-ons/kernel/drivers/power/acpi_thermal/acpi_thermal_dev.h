/* ++++++++++
    ACPI Generic Thermal Zone Driver. 
    Obtains general status of passive devices, monitors / sets critical temperatures
    Controls active devices.
+++++ */
#ifndef _ACPI_THERMAL_DEV_H
#define _ACPI_THERMAL_DEV_H

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <ACPI.h>

/* Base Namespace devices are published to */
static const char *basename = "power/thermal/";

typedef struct thermal_dev thermal_dev;
struct thermal_dev
{
	thermal_dev *next;
	
	int num;
	int open;
	char* path;
};

/*
 * builds the device lsit
 */
void enumerate_devices (char* base);

/*
 * Frees allocated memory to device list entries
 */
void cleanup_published_names (void);

static status_t acpi_thermal_control (void* cookie, uint32 op, void* arg, size_t len);

#endif /* _ACPI_THERMAL_DEV_H */
