/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Yongcong Du <ycdu.vmcore@gmail.com>
 */

#include <stdio.h>
#include <string.h>
#include <smp.h>

#include <Drivers.h>
#include <KernelExport.h>

#include <cpuidle.h>

#define DEVICE_NAME "cpuidle"

int32 api_version = B_CUR_DRIVER_API_VERSION;

static GenCpuidle *sGenCpuidle;

static status_t
cpuidle_open(const char *name, uint32 flags, void **cookie)
{
	*cookie = NULL;
	return B_OK;
}


static status_t
cpuidle_close(void *cookie)
{
	return B_OK;
}


static status_t
cpuidle_free(void *cookie)
{
	return B_OK;
}


static status_t
cpuidle_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	return B_OK;
}


static status_t
cpuidle_read(void *cookie, off_t pos, void *buffer, size_t *length)
{
	if (pos != 0) {
		*length = 0;
		return B_OK;
	}
	char *str = (char *)buffer;
	size_t max_len = *length;
	int32 stateCount = sGenCpuidle->GetIdleStateCount();
	int32 bytes = snprintf(str, max_len, "C-STATE COUNT: %"B_PRId32"\n", stateCount);
	max_len-= bytes;
	str += bytes;
	int32 cpu = smp_get_num_cpus();

	for (int32 i = 0; i < cpu; i++) {
		bytes = snprintf(str, max_len, "CPU%"B_PRId32"\n", i);
		max_len-= bytes;
		str += bytes;
		for (int32 j = 1; j < stateCount; j++) {
			CpuidleStat stat;
			bytes = snprintf(str, max_len, "%s\n", sGenCpuidle->GetIdleStateName(j));
			max_len-= bytes;
			str += bytes;
			sGenCpuidle->GetIdleStateInfo(i, j, &stat);
			bytes = snprintf(str, max_len, "%lld %lldus\n",
							stat.usageCount, stat.usageTime);
			max_len-= bytes;
			str += bytes;
		}
	}
	*length = strlen((char *)buffer);

	return B_OK;
}


static status_t
cpuidle_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	return B_OK;
}


status_t
init_hardware(void)
{
	return B_OK;
}


const char **
publish_devices(void)
{
	static const char *devices[] = {
		DEVICE_NAME,
		NULL
	};

	return devices;
}


device_hooks *
find_device(const char *name)
{
	static device_hooks hooks = {
		&cpuidle_open,
		&cpuidle_close,
		&cpuidle_free,
		&cpuidle_ioctl,
		&cpuidle_read,
		&cpuidle_write,
	};


	return &hooks;
}


status_t
init_driver(void)
{
	status_t err;

	err = get_module(B_CPUIDLE_MODULE_NAME, (module_info **)&sGenCpuidle);
	if (err != B_OK) {
		dprintf("can't load "B_CPUIDLE_MODULE_NAME"\n");
	}
	return B_OK;
}


void
uninit_driver(void)
{
	put_module(B_CPUIDLE_MODULE_NAME);
}
