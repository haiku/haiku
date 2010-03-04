/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "frequency.h"

#include <arch/x86/arch_cpu.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void
est_get_id16(uint16* _id16)
{
	*_id16 = x86_read_msr(MSR_GET_FREQ_STATE) & 0xffff;
}


status_t
est_set_id16(uint16 id16, bool needCheck)
{
	uint64 msr;

	// Read the current register, mask out the old, set the new id.
	msr = x86_read_msr(MSR_GET_FREQ_STATE);
	msr = (msr & ~0xffff) | id16;
	x86_write_msr(MSR_SET_FREQ_STATE, msr);

	if (needCheck) {
		// Wait a short while for the new setting.  XXX Is this necessary?
		snooze(EST_TRANS_LAT);

		uint16 newID16;
		est_get_id16(&newID16);
		if (newID16 != id16)
			return B_ERROR;
		TRACE("EST: set frequency ok, id %i\n", id16);
	}
	return B_OK;
}


freq_info*
est_get_current(freq_info* list)
{
	// Try a few times to get a valid value.  Sometimes, if the CPU
	// is in the middle of an asynchronous transition (i.e., P4TCC),
	// we get a temporary invalid result.
	for (uint32 i = 0; i < 5; i++) {
		uint16 id16;
		est_get_id16(&id16);

		for (freq_info* info = list; info->id != 0; info++) {
			if (info->id == id16)
				return info;
		}

		snooze(100);
	}
	return NULL;
}


status_t
est_get_info(freq_info** _frequencyInfos)
{
	uint64 msr = x86_read_msr(MSR_GET_FREQ_STATE);
	status_t status = est_table_info(msr, _frequencyInfos);
	if (status != B_OK) {
		TRACE("EST: Get frequency table from model specific register\n");
		status = est_msr_info(msr, _frequencyInfos);
	}

	if (status != B_OK) {
		TRACE("est: CPU supports Enhanced Speedstep, but is not recognized.\n");
		return status;
	}

	return B_OK;
}


status_t
est_table_info(uint64 msr, freq_info** _frequencyInfos)
{
	// Find a table which matches (vendor, id32).
	system_info info;
	if (get_system_info(&info) != B_OK)
		return B_ERROR;

	ss_cpu_info* cpuInfo;
	uint32 id = msr >> 32;
	for (cpuInfo = ESTprocs; cpuInfo->id32 != 0; cpuInfo++) {
		if (cpuInfo->vendor_id == uint32(info.cpu_type & B_CPU_x86_VENDOR_MASK)
			&& cpuInfo->id32 == id)
			break;
	}
	if (cpuInfo->id32 == 0)
		return B_ERROR;

	// Make sure the current setpoint is valid.
	if (est_get_current(cpuInfo->freqtab) == NULL) {
		TRACE("current setting not found in table\n");
		return B_ERROR;
	}

	*_frequencyInfos = cpuInfo->freqtab;
	return B_OK;
}


bool
bus_speed_ok(int bus)
{
	switch (bus) {
		case 100:
		case 133:
		case 166:
		case 333:
			return true;
		default:
			return false;
	}
}


/*!	Flesh out a simple rate table containing the high and low frequencies
	based on the current clock speed and the upper 32 bits of the MSR.
*/
status_t
est_msr_info(uint64 msr, freq_info** _frequencyInfos)
{
	// Figure out the bus clock.
	system_info info;
	if (get_system_info(&info) != B_OK)
		return B_ERROR;

	int32 freq = info.cpu_clock_speed / 1000000;
	uint16 id = msr >> 32;
	int32 bus = 0;
	if (id >> 8)
		bus = freq / (id >> 8);

	TRACE("est: Guessed bus clock (high) of %d MHz\n", int(bus));
	if (!bus_speed_ok(bus)) {
		// We may be running on the low frequency.
		id = msr >> 48;
		if (id >> 8)
			bus = freq / (id >> 8);
		TRACE("est: Guessed bus clock (low) of %d MHz\n", int(bus));
		if (!bus_speed_ok(bus))
			return B_ERROR;

		// Calculate high frequency.
		id = msr >> 32;
		freq = ((id >> 8) & 0xff) * bus;
	}

	// Fill out a new freq table containing just the high and low freqs.
	freq_info* frequencyInfo = (freq_info*)malloc(sizeof(freq_info) * 3);
	if (frequencyInfo == NULL)
		return B_NO_MEMORY;

	memset(frequencyInfo, 0, sizeof(freq_info) * 3);

	// First, the high frequency.
	int32 volts = id & 0xff;
	if (volts != 0) {
		volts <<= 4;
		volts += 700;
	}
	frequencyInfo[0].frequency = freq;
	frequencyInfo[0].volts = volts;
	frequencyInfo[0].id = id;
	frequencyInfo[0].power = CPUFREQ_VAL_UNKNOWN;
	TRACE("Guessed high setting of %d MHz @ %d Mv\n", int(freq), int(volts));

	// Second, the low frequency.
	id = msr >> 48;
	freq = ((id >> 8) & 0xff) * bus;
	volts = id & 0xff;
	if (volts != 0) {
		volts <<= 4;
		volts += 700;
	}
	frequencyInfo[1].frequency = freq;
	frequencyInfo[1].volts = volts;
	frequencyInfo[1].id = id;
	frequencyInfo[1].power = CPUFREQ_VAL_UNKNOWN;
	TRACE("Guessed low setting of %d MHz @ %d Mv\n", int(freq), int(volts));

	// Table is already terminated due to M_ZERO.
	*_frequencyInfos = frequencyInfo;
	return B_OK;
}
