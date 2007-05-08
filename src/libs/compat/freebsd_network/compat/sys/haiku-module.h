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

typedef int device_probe_t(device_t dev);
typedef int device_attach_t(device_t dev);
typedef int device_detach_t(device_t dev);
typedef int device_resume_t(device_t dev);
typedef int device_suspend_t(device_t dev);

struct device_method {
	const char *name;
	device_method_signature_t method;
};

typedef struct device_method device_method_t;

#define DEVMETHOD(name, func)	{ #name, (device_method_signature_t)&func }

typedef struct {
	const char *name;
	device_method_t *methods;
	size_t softc_size;
} driver_t;

#define BUS_PROBE_LOW_PRIORITY	10
#define BUS_PROBE_DEFAULT		20

#define DRIVER_MODULE_NAME(name, busname) \
	__fbsd_##name##busname

status_t _fbsd_init_hardware(driver_t *);
status_t _fbsd_init_driver(driver_t *);
void _fbsd_uninit_driver(driver_t *);

extern const char gDriverName[];

/* we define the driver methods with HAIKU_FBSD_DRIVER_GLUE to
 * force the rest of the stuff to be linked back with the driver.
 * While gcc 2.95 packs everything from the static library onto
 * the final binary, gcc 4.x rightfuly doesn't. */

#define HAIKU_FBSD_DRIVER_GLUE(publicname, name, busname) \
	extern char *gDevNameList[]; \
	extern device_hooks gDeviceHooks; \
	extern driver_t *DRIVER_MODULE_NAME(name, busname); \
	const char gDriverName[] = #publicname; \
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

extern spinlock __haiku_intr_spinlock;
extern int __haiku_disable_interrupts(device_t dev);

#define HAIKU_CHECK_DISABLE_INTERRUPTS		__haiku_disable_interrupts

#define NO_HAIKU_CHECK_DISABLE_INTERRUPTS() \
	int HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev) { \
		panic("should never be called."); \
		return -1; \
	}

extern int __haiku_driver_requirements;

enum {
	FBSD_TASKQUEUES		= 1 << 0,
	FBSD_FAST_TASKQUEUE	= 1 << 1,
};

#define HAIKU_DRIVER_REQUIREMENTS(flags) \
	int __haiku_driver_requirements = (flags)

#define HAIKU_DRIVER_REQUIRES(flag) (__haiku_driver_requirements & (flag))

#define HAIKU_INTR_REGISTER_ENTER(status) do { \
	status = disable_interrupts(); \
	acquire_spinlock(&__haiku_intr_spinlock); \
} while (0)

#define HAIKU_INTR_REGISTER_LEAVE(status) do { \
	release_spinlock(&__haiku_intr_spinlock); \
	restore_interrupts(status); \
} while (0)

#define DEFINE_CLASS_0(name, driver, methods, size) \
	driver_t driver = { #name, methods, size }

#define DRIVER_MODULE(name, busname, driver, devclass, evh, arg) \
	driver_t *DRIVER_MODULE_NAME(name, busname) = &(driver)


#endif
