/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <kernel/arch/x86/arch_cpu.h>

#include <ACPI.h>
#include "enhanced_speedstep.h"
#include <condition_variable.h>

#include "frequency.h"


#define EST_MODULE_NAME "drivers/power/enhanced_speedstep/driver_v1"

#define EST_DEVICE_MODULE_NAME "drivers/power/enhanced_speedstep/device_v1"

/* Base Namespace devices are published to */
#define EST_BASENAME "power/enhanced_speedstep/%d"

// name of pnp generator of path ids
#define EST_PATHID_GENERATOR "enhanced_speedstep/path_id"

static device_manager_info *sDeviceManager;
static ConditionVariable sFrequencyCondition;
static vint32 sCurrentID;


static status_t
est_read(void* _cookie, off_t position, void *buffer, size_t* numBytes)
{
	if (*numBytes < 1)
		return B_IO_ERROR;

	est_cookie *device = (est_cookie *)_cookie;

	if (position == 0) {
		size_t max_len = *numBytes;
		char *str = (char *)buffer;

		snprintf(str, max_len, "CPU Frequency states:\n");
		max_len-= strlen(str);
		str += strlen(str);

		freq_info *freqsInfo = device->available_states;
		freq_info *f;
		for (f = freqsInfo; f->frequency != 0; f++) {
			snprintf(str, max_len, " Frequency %hu, Volts %hu, Power %i, "
				"Latency %i, id %hu\n", f->frequency, f->volts, f->power, f->id,
				EST_TRANS_LAT);
			max_len-= strlen(str);
			str += strlen(str);
		}

		freq_info *f2 = est_get_current(freqsInfo);
		if (f2) {
			snprintf(str, max_len, "\nCurrent State: Frequency %hu, Volts %hu, "
				"Power %i, Latency %i\n", f2->frequency, f2->volts, f2->power,
				EST_TRANS_LAT);
		}

		*numBytes = strlen((char *)buffer);
	} else {
		*numBytes = 0;
	}

	return B_OK;
}


static status_t
est_write(void* cookie, off_t position, const void* buffer, size_t* numBytes)
{
	return B_ERROR;
}


status_t
est_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	est_cookie* device = (est_cookie*)_cookie;
	status_t err = B_ERROR;

	uint32* magicId;
	uint16* id;
	freq_info* freqInfo = NULL;
	switch (op) {
		case IDENTIFY_DEVICE:
			if (len < sizeof(uint32))
				return B_IO_ERROR;
			magicId = (uint32*)arg;
			*magicId = kMagicFreqID;
			err = B_OK;
			break;

		case GET_CPU_FREQ_STATES:
			if (len < sizeof(freq_info) * (device->number_states + 1))
				return B_IO_ERROR;
			freqInfo = (freq_info*)arg;
			user_memcpy(freqInfo, device->available_states,
				sizeof(freq_info) * (device->number_states + 1));
			err = B_OK;
			break;

		case GET_CURENT_CPU_FREQ_STATE:
			if (len < sizeof(uint16))
				return B_IO_ERROR;
			freqInfo = est_get_current(device->available_states);
			if (!freqInfo)
				return B_ERROR;
			atomic_set(&sCurrentID, freqInfo->id);
			*((uint16*)arg) = freqInfo->id;
			err = B_OK;
			break;

		case SET_CPU_FREQ_STATE:
			if (len < sizeof(uint16))
				return B_IO_ERROR;
			id = (uint16*)arg;
			err = est_set_id16(*id);
			if (err == B_OK) {
				atomic_set(&sCurrentID, *id);
				sFrequencyCondition.NotifyAll();
			}
			break;

		case WATCH_CPU_FREQ:
			if (len < sizeof(uint16))
				return B_IO_ERROR;
			sFrequencyCondition.Wait();
			if (atomic_get(&(device->stop_watching))) {
				atomic_set(&(device->stop_watching), 0);
				err = B_ERROR;
			} else {
				*((uint16*)arg) = atomic_get(&sCurrentID);
				err = B_OK;
			}
			break;

		case STOP_WATCHING_CPU_FREQ:
			atomic_set(&(device->stop_watching), 1);
			sFrequencyCondition.NotifyAll();
			err = B_OK;
			break;
	}
	return err;
}


static status_t
est_open(void *initCookie, const char *path, int flags, void** cookie)
{
	TRACE("est: open\n");
	est_cookie *device;
	device = (est_cookie *)calloc(1, sizeof(est_cookie));
	if (device == NULL)
		return B_NO_MEMORY;

	*cookie = device;

	device_node *node = (device_node *)initCookie;
	device->node = node;

	device_node *parent;
	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);
	sDeviceManager->put_node(parent);

	device->stop_watching = 0;

	// enable enhanced speedstep
	uint64 msrMisc = x86_read_msr(MSR_MISC);
	if ((msrMisc & MSR_EST_ENABLED) == 0) {
		TRACE("est: enable enhanced speedstep\n");
		x86_write_msr(MSR_MISC, msrMisc | MSR_EST_ENABLED);

		uint64 msrMisc = x86_read_msr(MSR_MISC);
		if ((msrMisc & MSR_EST_ENABLED) == 0) {
			TRACE("est: enable enhanced speedstep failed\n");
			return B_ERROR;
		}
	}

	// get freq_info
	if (est_get_info(&(device->available_states)) != B_OK)
		return B_ERROR;
	freq_info *freqsInfo = device->available_states;

	// count number of states
	TRACE("est: frequency info:\n");
	freq_info *f;
	device->number_states = 0;
	for (f = freqsInfo; f->frequency != 0; f++) {
		TRACE("est: Frequency %u, Volts %u, Power %i, Latency %u, id %u\n",
			f->frequency, f->volts, f->power, f->id, EST_TRANS_LAT);
		device->number_states++;
	}

	// print current frequency
	freq_info *f2 = est_get_current(freqsInfo);
	if (f2) {
		TRACE("est: Current Frequency %u, Volts %u, Power %i, Latency %u\n",
			f2->frequency, f2->volts, f2->power, EST_TRANS_LAT);
	}

	return B_OK;
}


static status_t
est_close(void* cookie)
{
	est_cookie *device = (est_cookie*)cookie;
	free(device);
	return B_OK;
}


static status_t
est_free(void* cookie)
{
	return B_OK;
}


//	#pragma mark - driver module API


static float
est_support(device_node *parent)
{
	// make sure parent is really the ACPI bus manager
	const char *bus;
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return 0.0;

	// check whether it's really a cpu Device
	uint32 deviceType;
	if (sDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM,
			&deviceType, false) != B_OK
		|| deviceType != ACPI_TYPE_PROCESSOR) {
		return 0.0;
	}

	// check if cpu support est
	uint32 cpuNum = 0;
	system_info sysInfo;
	if (get_system_info(&sysInfo) != B_OK)
		return 0.0;
	TRACE("est: cpu_type: %u vendor %u model %u\n", sysInfo.cpu_type,
		sysInfo.cpu_type & B_CPU_x86_VENDOR_MASK, sysInfo.cpu_type & 0x00FF);
	if ((sysInfo.cpu_type & B_CPU_x86_VENDOR_MASK) != B_CPU_INTEL_x86)
		return 0.0;

	// TODO: Make the code SMP safe!
	if (sysInfo.cpu_count > 1)
		return 0.0;

	cpuid_info info;
	if (get_cpuid(&info, 1, cpuNum) != B_OK)
		return 0.0;

	TRACE("est: extended_features: %i\n", int(info.eax_1.extended_features));

	// check for enhanced speedstep
	if ((info.eax_1.extended_features & IA32_FEATURE_EXT_EST) == 0)
		return 0.0;

	TRACE("est: supported\n");
	return 0.6;
}


static status_t
est_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: "ACPI Enhanced Speedstep" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, EST_MODULE_NAME, attrs,
		NULL, NULL);
}


static status_t
est_init_driver(device_node *node, void **_driverCookie)
{
	*_driverCookie = node;

	sFrequencyCondition.Init(NULL, "frequency cv");
	sCurrentID = -1;

	return B_OK;
}


static void
est_uninit_driver(void *driverCookie)
{
}


static status_t
est_register_child_devices(void *_cookie)
{
	device_node *node = (device_node*)_cookie;

	int pathID = sDeviceManager->create_id(EST_PATHID_GENERATOR);
	if (pathID < 0) {
		TRACE("est_register_child_devices: couldn't create a path_id\n");
		return B_ERROR;
	}

	char name[128];
	snprintf(name, sizeof(name), EST_BASENAME, pathID);

	return sDeviceManager->publish_device(node, name, EST_DEVICE_MODULE_NAME);
}


static status_t
est_init_device(void *driverCookie, void **cookie)
{
	// driverCookie is the device node
	*cookie = driverCookie;
	return B_OK;
}


static void
est_uninit_device(void *_cookie)
{

}



module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{}
};


driver_module_info est_driver_module = {
	{
		EST_MODULE_NAME,
		0,
		NULL
	},

	est_support,
	est_register_device,
	est_init_driver,
	est_uninit_driver,
	est_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info est_device_module = {
	{
		EST_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	est_init_device,
	est_uninit_device,
	NULL,

	est_open,
	est_close,
	est_free,
	est_read,
	est_write,
	NULL,
	est_control,

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&est_driver_module,
	(module_info *)&est_device_module,
	NULL
};
