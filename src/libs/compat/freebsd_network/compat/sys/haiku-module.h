/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_HAIKU_MODULE_H_
#define _FBSD_COMPAT_SYS_HAIKU_MODULE_H_


#include <Drivers.h>
#include <KernelExport.h>

#include <lock.h>
#include <net_stack.h>

#undef ASSERT
	/* private/kernel/debug.h sets it */

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
driver_t *__haiku_select_miibus_driver(device_t dev);
driver_t *__haiku_probe_miibus(device_t dev, driver_t *drivers[], int count);
void __haiku_scan_miibus(device_t dev);

/* we define the driver methods with HAIKU_FBSD_DRIVER_GLUE to
 * force the rest of the stuff to be linked back with the driver.
 * While gcc 2.95 packs everything from the static library onto
 * the final binary, gcc 4.x rightfuly doesn't. */

#define HAIKU_FBSD_DRIVER_GLUE(publicname, name, busname)				\
	extern const char *gDevNameList[];									\
	extern device_hooks gDeviceHooks;									\
	extern driver_t *DRIVER_MODULE_NAME(name, busname);					\
	const char gDriverName[] = #publicname;								\
	int32 api_version = B_CUR_DRIVER_API_VERSION;						\
	status_t init_hardware()											\
	{																	\
		return _fbsd_init_hardware(DRIVER_MODULE_NAME(name, busname));	\
	}																	\
	status_t init_driver()												\
	{																	\
		return _fbsd_init_driver(DRIVER_MODULE_NAME(name, busname));	\
	}																	\
	void uninit_driver()												\
		{ _fbsd_uninit_driver(DRIVER_MODULE_NAME(name, busname)); }		\
	const char **publish_devices()										\
		{ return gDevNameList; }										\
	device_hooks *find_device(const char *name)							\
		{ return &gDeviceHooks; }

#define HAIKU_FBSD_RETURN_MII_DRIVER(drivers, count)			\
	driver_t *__haiku_select_miibus_driver(device_t dev)		\
	{															\
		return __haiku_probe_miibus(dev, drivers, count);		\
	}

#define HAIKU_FBSD_MII_DRIVER(name)								\
	extern driver_t *DRIVER_MODULE_NAME(name, miibus);			\
	HAIKU_FBSD_RETURN_MII_DRIVER(								\
		&DRIVER_MODULE_NAME(name, miibus), 1)

#define NO_HAIKU_FBSD_MII_DRIVER()								\
	HAIKU_FBSD_RETURN_MII_DRIVER(NULL, 0)

extern spinlock __haiku_intr_spinlock;
extern int __haiku_disable_interrupts(device_t dev);
extern void __haiku_reenable_interrupts(device_t dev);

#define HAIKU_CHECK_DISABLE_INTERRUPTS		__haiku_disable_interrupts
#define HAIKU_REENABLE_INTERRUPTS			__haiku_reenable_interrupts

#define NO_HAIKU_CHECK_DISABLE_INTERRUPTS()				\
	int HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev) {	\
		panic("should never be called.");				\
		return -1; \
	}

#define NO_HAIKU_REENABLE_INTERRUPTS() \
	void HAIKU_REENABLE_INTERRUPTS(device_t dev) {}

extern int __haiku_driver_requirements;

enum {
	FBSD_TASKQUEUES		= 1 << 0,
	FBSD_FAST_TASKQUEUE	= 1 << 1,
	FBSD_SWI_TASKQUEUE	= 1 << 2,
};

#define HAIKU_DRIVER_REQUIREMENTS(flags) \
	int __haiku_driver_requirements = (flags)

#define HAIKU_DRIVER_REQUIRES(flag) (__haiku_driver_requirements & (flag))

#define HAIKU_INTR_REGISTER_STATE \
	cpu_status __haiku_cpu_state = 0

#define HAIKU_INTR_REGISTER_ENTER() do {		\
	__haiku_cpu_state = disable_interrupts();	\
	acquire_spinlock(&__haiku_intr_spinlock);	\
} while (0)

#define HAIKU_INTR_REGISTER_LEAVE() do {		\
	release_spinlock(&__haiku_intr_spinlock);	\
	restore_interrupts(__haiku_cpu_state);		\
} while (0)

#define HAIKU_PROTECT_INTR_REGISTER(x) do {		\
	HAIKU_INTR_REGISTER_STATE;					\
	HAIKU_INTR_REGISTER_ENTER();				\
	x;											\
	HAIKU_INTR_REGISTER_LEAVE();				\
} while (0)

#define DEFINE_CLASS_0(name, driver, methods, size) \
	driver_t driver = { #name, methods, size }

#define DRIVER_MODULE(name, busname, driver, devclass, evh, arg) \
	driver_t *DRIVER_MODULE_NAME(name, busname) = &(driver); \
	devclass_t *__class_ ## busname ## _ ## devclass = &(devclass)

#endif	/* _FBSD_COMPAT_SYS_HAIKU_MODULE_H_ */
