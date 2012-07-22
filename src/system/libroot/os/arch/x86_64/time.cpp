/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <libroot_private.h>
#include <real_time_data.h>
#include <arch_cpu.h>


void
__arch_init_time(real_time_data* data, bool setDefaults)
{
	uint32 conversionFactor;
	uint64 conversionFactorNsecs;

	if (setDefaults) {
		data->arch_data.system_time_offset = 0;
		data->arch_data.system_time_conversion_factor = 100000;
	}

	// TODO: this should only store a pointer to that value
	// When resolving this TODO, also resolve the one in the Jamfile.

	conversionFactor = data->arch_data.system_time_conversion_factor;
	conversionFactorNsecs = (uint64)conversionFactor * 1000;

	// The x86_64 system_time() implementation uses 64-bit multiplication and
	// therefore shifting is not necessary for low frequencies (it's also not
	// too likely that there'll be any x86_64 CPUs clocked under 1GHz).
	__x86_setup_system_time((uint64)conversionFactor << 32,
		conversionFactorNsecs);
}


bigtime_t
__arch_get_system_time_offset(struct real_time_data *data)
{
	//we don't use atomic_get64 because memory is read-only, maybe find another way to lock
	return data->arch_data.system_time_offset;
}

