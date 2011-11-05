/*
 * Copyright 2009, Colin Günther. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_HAIKU_MODULE_H_
#define _FBSD_COMPAT_SYS_HAIKU_MODULE_H_


#include <Drivers.h>
#include <KernelExport.h>

#include <kernel/lock.h>
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

#define DRIVER_MODULE_NAME(name, busname) \
	__fbsd_ ## name ## _ ## busname

status_t _fbsd_init_hardware(driver_t *driver[]);
status_t _fbsd_init_drivers(driver_t *driver[]);
status_t _fbsd_uninit_drivers(driver_t *driver[]);

extern const char *gDriverName;
driver_t *__haiku_select_miibus_driver(device_t dev);
driver_t *__haiku_probe_miibus(device_t dev, driver_t *drivers[]);
status_t __haiku_handle_fbsd_drivers_list(status_t (*handler)(driver_t *[]));

status_t init_wlan_stack(void);
void uninit_wlan_stack(void);
status_t start_wlan(device_t);
status_t stop_wlan(device_t);
status_t wlan_control(void*, uint32, void*, size_t);
status_t wlan_close(void*);
status_t wlan_if_l2com_alloc(void*);

/* we define the driver methods with HAIKU_FBSD_DRIVERS_GLUE to
 * force the rest of the stuff to be linked back with the driver.
 * While gcc 2.95 packs everything from the static library onto
 * the final binary, gcc 4.x rightfuly doesn't. */

#define HAIKU_FBSD_DRIVERS_GLUE(publicname)								\
	extern const char *gDeviceNameList[];								\
	extern device_hooks gDeviceHooks;									\
	const char *gDriverName = #publicname;								\
	int32 api_version = B_CUR_DRIVER_API_VERSION;						\
	status_t init_hardware()											\
	{																	\
		return __haiku_handle_fbsd_drivers_list(_fbsd_init_hardware);	\
	}																	\
	status_t init_driver()												\
	{																	\
		return __haiku_handle_fbsd_drivers_list(_fbsd_init_drivers);	\
	}																	\
	void uninit_driver()												\
	{																	\
		__haiku_handle_fbsd_drivers_list(_fbsd_uninit_drivers);			\
	}																	\
	const char **publish_devices()										\
		{ return gDeviceNameList; }										\
	device_hooks *find_device(const char *name)							\
		{ return &gDeviceHooks; }										\
	status_t init_wlan_stack(void)										\
		{ return B_OK; } 												\
	void uninit_wlan_stack(void) {}										\
	status_t start_wlan(device_t dev)									\
		{ return B_OK; }												\
	status_t stop_wlan(device_t dev)									\
		{ return B_OK; }												\
	status_t wlan_control(void *cookie, uint32 op, void *arg, 			\
		size_t length)													\
		{ return B_BAD_VALUE; }											\
	status_t wlan_close(void* cookie)									\
		{ return B_OK; }												\
	status_t wlan_if_l2com_alloc(void* ifp)								\
		{ return B_OK; }

#define HAIKU_FBSD_DRIVER_GLUE(publicname, name, busname)				\
	extern driver_t *DRIVER_MODULE_NAME(name, busname);					\
	status_t __haiku_handle_fbsd_drivers_list(status_t (*proc)(driver_t *[])) {\
		driver_t *drivers[] = {											\
			DRIVER_MODULE_NAME(name, busname),							\
			NULL														\
		};																\
		return (*proc)(drivers);										\
	}																	\
	HAIKU_FBSD_DRIVERS_GLUE(publicname);

#define HAIKU_FBSD_WLAN_DRIVERS_GLUE(publicname)						\
	extern const char *gDeviceNameList[];								\
	extern device_hooks gDeviceHooks;									\
	const char *gDriverName = #publicname;								\
	int32 api_version = B_CUR_DRIVER_API_VERSION;						\
	status_t init_hardware()											\
	{																	\
		return __haiku_handle_fbsd_drivers_list(_fbsd_init_hardware);	\
	}																	\
	status_t init_driver()												\
	{																	\
		return __haiku_handle_fbsd_drivers_list(_fbsd_init_drivers);	\
	}																	\
	void uninit_driver()												\
	{																	\
		__haiku_handle_fbsd_drivers_list(_fbsd_uninit_drivers);			\
	}																	\
	const char **publish_devices()										\
		{ return gDeviceNameList; }										\
	device_hooks *find_device(const char *name)							\
		{ return &gDeviceHooks; }

#define HAIKU_FBSD_WLAN_DRIVER_GLUE(publicname, name, busname)			\
	extern driver_t *DRIVER_MODULE_NAME(name, busname);					\
	status_t __haiku_handle_fbsd_drivers_list(status_t (*proc)(driver_t *[])) {\
		driver_t *drivers[] = {											\
			DRIVER_MODULE_NAME(name, busname),							\
			NULL														\
		};																\
		return (*proc)(drivers);										\
	}																	\
	HAIKU_FBSD_WLAN_DRIVERS_GLUE(publicname);

#define HAIKU_FBSD_RETURN_MII_DRIVER(drivers)					\
	driver_t *__haiku_select_miibus_driver(device_t dev)		\
	{															\
		return __haiku_probe_miibus(dev, drivers);				\
	}

#define HAIKU_FBSD_MII_DRIVER(name)								\
	extern driver_t *DRIVER_MODULE_NAME(name, miibus);			\
	driver_t *__haiku_select_miibus_driver(device_t dev)		\
	{															\
		driver_t *drivers[] = {									\
			DRIVER_MODULE_NAME(name, miibus),					\
			NULL												\
		};														\
		return __haiku_probe_miibus(dev, drivers);				\
	}

#define NO_HAIKU_FBSD_MII_DRIVER()								\
	HAIKU_FBSD_RETURN_MII_DRIVER(NULL)

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
	FBSD_WLAN			= 1 << 3,
};

#define HAIKU_DRIVER_REQUIREMENTS(flags) \
	int __haiku_driver_requirements = (flags)

#define HAIKU_DRIVER_REQUIRES(flag) (__haiku_driver_requirements & (flag))


/* #pragma mark - firmware loading */


/*
 * Only needed to be specified in the glue code of drivers which actually need
 * to load firmware. See iprowifi2100 for an example.
 */

extern const uint __haiku_firmware_version;

/* Use 0 if driver doesn't care about firmware version. */
#define HAIKU_FIRMWARE_VERSION(version) \
	const uint __haiku_firmware_version = (version)

extern const uint __haiku_firmware_parts_count;
extern const char* __haiku_firmware_name_map[][2];

#define HAIKU_FIRMWARE_NAME_MAP(firmwarePartsCount) \
	const uint __haiku_firmware_parts_count = firmwarePartsCount; \
	const char* __haiku_firmware_name_map[firmwarePartsCount][2]

#define NO_HAIKU_FIRMWARE_NAME_MAP() \
	const uint __haiku_firmware_parts_count = 0; \
	const char* __haiku_firmware_name_map[0][2] = {}


/* #pragma mark - synchronization */


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
	devclass_t *__class_ ## name ## _ ## busname ## _ ## devclass = &(devclass)


#endif	/* _FBSD_COMPAT_SYS_HAIKU_MODULE_H_ */
