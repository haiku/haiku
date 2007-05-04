/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */
#ifndef _FBSD_COMPAT_SYS_HAIKU_MODULE_H_
#define _FBSD_COMPAT_SYS_HAIKU_MODULE_H_

#include <Drivers.h> /* for device_hooks */

typedef struct device *device_t;
typedef struct devclass *devclass_t;

typedef int (*device_method_signature_t)(device_t dev);

struct device_method {
	const char *name;
	device_method_signature_t method;
};

typedef struct device_method device_method_t;

#define DEVMETHOD(name, func)	{ #name, (device_method_signature_t)func }

typedef struct {
	const char *name;
	device_method_t *methods;
	size_t size;
} driver_t;

#define BUS_PROBE_DEFAULT		20

#define DRIVER_MODULE_NAME(name, busname) \
	__fbsd_##name##busname

status_t _fbsd_init_hardware(driver_t *);
status_t _fbsd_init_driver(driver_t *);
void _fbsd_uninit_driver(driver_t *);

/* we define the driver methods with HAIKU_FBSD_DRIVER_GLUE to
 * force the rest of the stuff to be linked back with the driver.
 * While gcc 2.95 packs everything from the static library onto
 * the final binary, gcc 4.x rightfuly doesn't. */

#define HAIKU_FBSD_DRIVER_GLUE(name, busname) \
	extern char *gDevNameList[]; \
	extern device_hooks gDeviceHooks; \
	extern driver_t *DRIVER_MODULE_NAME(name, busname); \
	int32 api_version = B_CUR_DRIVER_API_VERSION; \
	status_t init_hardware() \
	{ \
		return _fbsd_init_hardware(DRIVER_MODULE_NAME(name, busname)); \
	} \
	status_t init_driver() \
	{ \
		return _fbsd_init_driver(DRIVER_MODULE_NAME(name, busname)); \
	} \
	void uninit_driver() \
	{ \
		_fbsd_uninit_driver(DRIVER_MODULE_NAME(name, busname)); \
	} \
	const char **publish_devices() { return (const char **)gDevNameList; } \
	device_hooks *find_device(const char *name) { return &gDeviceHooks; }

#define DRIVER_MODULE(name, busname, driver, devclass, evh, arg) \
	driver_t *DRIVER_MODULE_NAME(name, busname) = &(driver)


#endif
