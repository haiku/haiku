#include "frequency.h"

#include <kernel/arch/x86/arch_cpu.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void
est_get_id16(uint16 *id16_p)
{
	*id16_p = x86_read_msr(MSR_GET_FREQ_STATE) & 0xffff;
}


status_t
est_set_id16(uint16 id16, bool need_check)
{
	uint64 msr;
	   
	// Read the current register, mask out the old, set the new id.
	msr = x86_read_msr(MSR_GET_FREQ_STATE);
	msr = (msr & ~0xffff) | id16;
	x86_write_msr(MSR_SET_FREQ_STATE, msr);
                
	if (need_check) {
		// Wait a short while for the new setting.  XXX Is this necessary?
		snooze(EST_TRANS_LAT);
	
		uint16 new_id16;
		est_get_id16(&new_id16);                
		if (new_id16 != id16)
			return B_ERROR;
		TRACE("EST: set frequency ok, id %i\n", id16);
	}
	return B_OK;
}


freq_info *
est_get_current(freq_info *freq_list)
{
	freq_info *f;
	int i;
	uint16 id16;

	/*
	* Try a few times to get a valid value.  Sometimes, if the CPU
	* is in the middle of an asynchronous transition (i.e., P4TCC),
	* we get a temporary invalid result.
	*/
	for (i = 0; i < 5; i++) {
		est_get_id16(&id16);
		for (f = freq_list; f->id != 0; f++) {
			if (f->id == id16)
				return (f);
		}
		snooze(100);
	}
	return NULL;
}


status_t
est_get_info(freq_info **freqsInfo)
{
	uint64 msr;
	status_t error = B_ERROR;

	msr = x86_read_msr(MSR_GET_FREQ_STATE);
	error = est_table_info(msr, freqsInfo);
	if (error != B_OK) {
		TRACE("EST: Get frequency table from model specific register\n");
		error = est_msr_info(msr, freqsInfo);
	}

	if (error) {
		TRACE("est: CPU supports Enhanced Speedstep, but is not recognized.\n");
		return error;
	}

	return B_OK;
}


status_t
est_table_info(uint64 msr, freq_info **freqs)
{
        ss_cpu_info *p;
        uint32 id;

        /* Find a table which matches (vendor, id32). */
        system_info sysInfo;
		if (get_system_info(&sysInfo) != B_OK)
			return B_ERROR;
        id = msr >> 32;
        for (p = ESTprocs; p->id32 != 0; p++) {
                if (p->vendor_id == uint32(sysInfo.cpu_type & B_CPU_x86_VENDOR_MASK)
                		&& p->id32 == id)
                        break;
        }
        if (p->id32 == 0)
                return B_ERROR;

        /* Make sure the current setpoint is valid. */
        if (est_get_current(p->freqtab) == NULL) {
                TRACE("current setting not found in table\n");
                return B_ERROR;
        }

        *freqs = p->freqtab;
        return B_OK;
}


bool
bus_speed_ok(int bus)
{
	switch (bus) {
		case 100:
		case 133:
		case 333:
			return true;
		default:
			return false;
	}
}

/*
 * Flesh out a simple rate table containing the high and low frequencies
 * based on the current clock speed and the upper 32 bits of the MSR.
 */
status_t
est_msr_info(uint64 msr, freq_info **freqs)
{
	freq_info *fp;
	int32 bus, freq, volts;
	uint16 id;

	// Figure out the bus clock.
	system_info sysInfo;
	if (get_system_info(&sysInfo) != B_OK)
		return B_ERROR;
	
	freq = sysInfo.cpu_clock_speed / 1000000;	
	id = msr >> 32;
	bus = freq / (id >> 8);
	
	TRACE("est: Guessed bus clock (high) of %d MHz\n", int(bus));
	if (!bus_speed_ok(bus)) {
		// We may be running on the low frequency.
		id = msr >> 48;
		bus = freq / (id >> 8);
		TRACE("est: Guessed bus clock (low) of %d MHz\n", int(bus));
		if (!bus_speed_ok(bus))
			return B_ERROR;
                
		// Calculate high frequency.
		id = msr >> 32;
		freq = ((id >> 8) & 0xff) * bus;
	}

	// Fill out a new freq table containing just the high and low freqs.
	fp = (freq_info*)malloc(sizeof(freq_info) * 3);
	memset(fp, 0, sizeof(freq_info) * 3);

	// First, the high frequency.
	volts = id & 0xff;
	if (volts != 0) {
		volts <<= 4;
		volts += 700;
	}
	fp[0].frequency = freq;
	fp[0].volts = volts;
	fp[0].id = id;
	fp[0].power = CPUFREQ_VAL_UNKNOWN;
	TRACE("Guessed high setting of %d MHz @ %d Mv\n", int(freq), int(volts));

	// Second, the low frequency.
	id = msr >> 48;
	freq = ((id >> 8) & 0xff) * bus;
	volts = id & 0xff;
	if (volts != 0) {
		volts <<= 4;
		volts += 700;
	}
	fp[1].frequency = freq;
	fp[1].volts = volts;
	fp[1].id = id;
	fp[1].power = CPUFREQ_VAL_UNKNOWN;
	TRACE("Guessed low setting of %d MHz @ %d Mv\n", int(freq), int(volts));

	// Table is already terminated due to M_ZERO.
	*freqs = fp;
	return B_OK;
}
