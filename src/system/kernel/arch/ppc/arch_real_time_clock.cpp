/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <arch/real_time_clock.h>

#include <arch_platform.h>
#include <boot/kernel_args.h>
#include <real_time_data.h>
#include <smp.h>


static spinlock sSetArchDataLock;

status_t
arch_rtc_init(kernel_args *args, struct real_time_data *data)
{
	// init the platform RTC service
	status_t error = PPCPlatform::Default()->InitRTC(args, data);
	if (error != B_OK)
		return error;

	// init the arch specific part of the real_time_data
	data->arch_data.data[0].system_time_offset = 0;
	// cvFactor = 2^32 * 1000000 / tbFreq
	// => (tb * cvFactor) >> 32 = (tb * 2^32 * 1000000 / tbFreq) >> 32
	//    = tb / tbFreq * 1000000 = time in us
	data->arch_data.system_time_conversion_factor
		= uint32((uint64(1) << 32) * 1000000
			/ args->arch_args.time_base_frequency);
	data->arch_data.version = 0;

	// init spinlock
	sSetArchDataLock = 0;

	// init system_time() conversion factor
	__ppc_setup_system_time(&data->arch_data.system_time_conversion_factor);

	return B_OK;
}


uint32
arch_rtc_get_hw_time(void)
{
	return PPCPlatform::Default()->GetHardwareRTC();
}


void
arch_rtc_set_hw_time(uint32 seconds)
{
	PPCPlatform::Default()->SetHardwareRTC(seconds);
}


void
arch_rtc_set_system_time_offset(struct real_time_data *data, bigtime_t offset)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&sSetArchDataLock);

	int32 version = data->arch_data.version + 1;
	data->arch_data.data[version % 2].system_time_offset = offset;
	data->arch_data.version = version;

	release_spinlock(&sSetArchDataLock);
	restore_interrupts(state);
}


bigtime_t
arch_rtc_get_system_time_offset(struct real_time_data *data)
{
	int32 version;
	bigtime_t offset;
	do {
		version = data->arch_data.version;
		offset = data->arch_data.data[version % 2].system_time_offset;
	} while (version != data->arch_data.version);

	return offset;
}
