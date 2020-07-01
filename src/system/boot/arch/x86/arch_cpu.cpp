/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * calculate_cpu_conversion_factor() was written by Travis Geiselbrecht and
 * licensed under the NewOS license.
 */


#include <OS.h>

#include <boot/arch/x86/arch_cpu.h>
#include <boot/kernel_args.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include <arch/cpu.h>
#include <arch/x86/arch_cpu.h>
#include <arch_kernel.h>
#include <arch_system_info.h>

#include <string.h>

#if __GNUC__ > 2
#include <x86intrin.h>
#else
static inline uint64_t __rdtsc()
{
	uint64 tsc;

	asm volatile ("rdtsc\n" : "=A"(tsc));

	return tsc;
}
#endif


uint32 gTimeConversionFactor;

// PIT definitions
#define TIMER_CLKNUM_HZ					(14318180 / 12)

// PIT IO Ports
#define PIT_CHANNEL_PORT_BASE			0x40
#define PIT_CONTROL						0x43

// Channel selection
#define PIT_SELECT_CHANNEL_SHIFT		6

// Access mode
#define PIT_ACCESS_LATCH_COUNTER		(0 << 4)
#define PIT_ACCESS_LOW_BYTE_ONLY		(1 << 4)
#define PIT_ACCESS_HIGH_BYTE_ONLY		(2 << 4)
#define PIT_ACCESS_LOW_THEN_HIGH_BYTE	(3 << 4)

// Operating modes
#define PIT_MODE_INTERRUPT_ON_0			(0 << 1)
#define PIT_MODE_HARDWARE_COUNTDOWN		(1 << 1)
#define PIT_MODE_RATE_GENERATOR			(2 << 1)
#define PIT_MODE_SQUARE_WAVE_GENERATOR	(3 << 1)
#define PIT_MODE_SOFTWARE_STROBE		(4 << 1)
#define PIT_MODE_HARDWARE_STROBE		(5 << 1)

// BCD/Binary mode
#define PIT_BINARY_MODE					0
#define PIT_BCD_MODE					1

// Channel 2 control (speaker)
#define PIT_CHANNEL_2_CONTROL			0x61
#define PIT_CHANNEL_2_GATE_HIGH			0x01
#define PIT_CHANNEL_2_SPEAKER_OFF_MASK	~0x02

// Maximum values
#define MAX_QUICK_SAMPLES				20
#define MAX_SLOW_SAMPLES				20
	// TODO: These are arbitrary. They are here to avoid spinning indefinitely
	// if the TSC just isn't stable and we can't get our desired error range.


struct uint128 {
	uint128(uint64 low, uint64 high = 0)
		:
		low(low),
		high(high)
	{
	}

	bool operator<(const uint128& other) const
	{
		return high < other.high || (high == other.high && low < other.low);
	}

	bool operator<=(const uint128& other) const
	{
		return !(other < *this);
	}

	uint128 operator<<(int count) const
	{
		if (count == 0)
			return *this;

		if (count >= 128)
			return 0;

		if (count >= 64)
			return uint128(0, low << (count - 64));

		return uint128(low << count, (high << count) | (low >> (64 - count)));
	}

	uint128 operator>>(int count) const
	{
		if (count == 0)
			return *this;

		if (count >= 128)
			return 0;

		if (count >= 64)
			return uint128(high >> (count - 64), 0);

		return uint128((low >> count) | (high << (64 - count)), high >> count);
	}

	uint128 operator+(const uint128& other) const
	{
		uint64 resultLow = low + other.low;
		return uint128(resultLow,
			high + other.high + (resultLow < low ? 1 : 0));
	}

	uint128 operator-(const uint128& other) const
	{
		uint64 resultLow = low - other.low;
		return uint128(resultLow,
			high - other.high - (resultLow > low ? 1 : 0));
	}

	uint128 operator*(uint32 other) const
	{
		uint64 resultMid = (low >> 32) * other;
		uint64 resultLow = (low & 0xffffffff) * other + (resultMid << 32);
		return uint128(resultLow,
			high * other + (resultMid >> 32)
				+ (resultLow < resultMid << 32 ? 1 : 0));
	}

	uint128 operator/(const uint128& other) const
	{
		int shift = 0;
		uint128 shiftedDivider = other;
		while (shiftedDivider.high >> 63 == 0 && shiftedDivider < *this) {
			shiftedDivider = shiftedDivider << 1;
			shift++;
		}

		uint128 result = 0;
		uint128 temp = *this;
		for (; shift >= 0; shift--, shiftedDivider = shiftedDivider >> 1) {
			if (shiftedDivider <= temp) {
				result = result + (uint128(1) << shift);
				temp = temp - shiftedDivider;
			}
		}

		return result;
	}

	operator uint64() const
	{
		return low;
	}

private:
	uint64	low;
	uint64	high;
};


static inline void
calibration_loop(uint8 desiredHighByte, uint8 channel, uint64& tscDelta,
	double& conversionFactor, uint16& expired)
{
	uint8 select = channel << PIT_SELECT_CHANNEL_SHIFT;
	out8(select | PIT_ACCESS_LOW_THEN_HIGH_BYTE | PIT_MODE_INTERRUPT_ON_0
		| PIT_BINARY_MODE, PIT_CONTROL);

	// Fill in count of 0xffff, low then high byte
	uint8 channelPort = PIT_CHANNEL_PORT_BASE + channel;
	out8(0xff, channelPort);
	out8(0xff, channelPort);

	// Read the count back once to delay the start. This ensures that we've
	// waited long enough for the counter to actually start counting down, as
	// this only happens on the next clock cycle after reload.
	in8(channelPort);
	in8(channelPort);

	// We're expecting the PIT to be at the starting position (high byte 0xff)
	// as we just programmed it, but if it isn't we wait for it to wrap.
	uint8 startLow;
	uint8 startHigh;
	do {
		out8(select | PIT_ACCESS_LATCH_COUNTER, PIT_CONTROL);
		startLow = in8(channelPort);
		startHigh = in8(channelPort);
	} while (startHigh != 255);

	// Read in the first TSC value
	uint64 startTSC = __rdtsc();

	// Wait for the PIT to count down to our desired value
	uint8 endLow;
	uint8 endHigh;
	do {
		out8(select | PIT_ACCESS_LATCH_COUNTER, PIT_CONTROL);
		endLow = in8(channelPort);
		endHigh = in8(channelPort);
	} while (endHigh > desiredHighByte);

	// And read the second TSC value
	uint64 endTSC = __rdtsc();

	tscDelta = endTSC - startTSC;
	expired = ((startHigh << 8) | startLow) - ((endHigh << 8) | endLow);
	conversionFactor = (double)tscDelta / (double)expired;
}


void
calculate_cpu_conversion_factor(uint8 channel)
{
	// When using channel 2, enable the input and disable the speaker.
	if (channel == 2) {
		uint8 control = in8(PIT_CHANNEL_2_CONTROL);
		control &= PIT_CHANNEL_2_SPEAKER_OFF_MASK;
		control |= PIT_CHANNEL_2_GATE_HIGH;
		out8(control, PIT_CHANNEL_2_CONTROL);
	}

	uint64 tscDeltaQuick, tscDeltaSlower, tscDeltaSlow;
	double conversionFactorQuick, conversionFactorSlower, conversionFactorSlow;
	uint16 expired;

	uint32 quickSampleCount = 1;
	uint32 slowSampleCount = 1;

quick_sample:
	calibration_loop(224, channel, tscDeltaQuick, conversionFactorQuick,
		expired);

slower_sample:
	calibration_loop(192, channel, tscDeltaSlower, conversionFactorSlower,
		expired);

	double deviation = conversionFactorQuick / conversionFactorSlower;
	if (deviation < 0.99 || deviation > 1.01) {
		// We might have been hit by a SMI or were otherwise stalled
		if (quickSampleCount++ < MAX_QUICK_SAMPLES)
			goto quick_sample;
	}

	// Slow sample
	calibration_loop(128, channel, tscDeltaSlow, conversionFactorSlow,
		expired);

	deviation = conversionFactorSlower / conversionFactorSlow;
	if (deviation < 0.99 || deviation > 1.01) {
		// We might have been hit by a SMI or were otherwise stalled
		if (slowSampleCount++ < MAX_SLOW_SAMPLES)
			goto slower_sample;
	}

	// Scale the TSC delta to timer units
	tscDeltaSlow *= TIMER_CLKNUM_HZ;

	uint64 clockSpeed = tscDeltaSlow / expired;
	gTimeConversionFactor = ((uint128(expired) * uint32(1000000)) << 32)
		/ uint128(tscDeltaSlow);

#ifdef TRACE_CPU
	if (clockSpeed > 1000000000LL) {
		dprintf("CPU at %Ld.%03Ld GHz\n", clockSpeed / 1000000000LL,
			(clockSpeed % 1000000000LL) / 1000000LL);
	} else {
		dprintf("CPU at %Ld.%03Ld MHz\n", clockSpeed / 1000000LL,
			(clockSpeed % 1000000LL) / 1000LL);
	}
#endif

	gKernelArgs.arch_args.system_time_cv_factor = gTimeConversionFactor;
	gKernelArgs.arch_args.cpu_clock_speed = clockSpeed;
	//dprintf("factors: %lu %llu\n", gTimeConversionFactor, clockSpeed);

	if (quickSampleCount > 1) {
		dprintf("needed %" B_PRIu32 " quick samples for TSC calibration\n",
			quickSampleCount);
	}

	if (slowSampleCount > 1) {
		dprintf("needed %" B_PRIu32 " slow samples for TSC calibration\n",
			slowSampleCount);
	}

	if (channel == 2) {
		// Set the gate low again
		out8(in8(PIT_CHANNEL_2_CONTROL) & ~PIT_CHANNEL_2_GATE_HIGH,
			PIT_CHANNEL_2_CONTROL);
	}
}

extern int open_maybe_packaged(BootVolume& volume, const char* path,
	int openMode);

void
ucode_load(BootVolume& volume)
{
	cpuid_info info;
	if (get_current_cpuid(&info, 0, 0) != B_OK
		|| strncmp(info.eax_0.vendor_id, "GenuineIntel", 12) != 0)
		return;

	if (get_current_cpuid(&info, 1, 0) != B_OK)
		return;

	char path[128];
	int family = info.eax_1.family;
	int model = info.eax_1.model;
	if (family == 0x6 || family == 0xf) {
		family += info.eax_1.extended_family;
		model += (info.eax_1.extended_model << 4);
	}
	snprintf(path, sizeof(path), "system/data/firmware/intel-ucode/"
		"%02x-%02x-%02x", family, model, info.eax_1.stepping);
	dprintf("ucode_load: %s\n", path);

	int fd = open_maybe_packaged(volume, path, O_RDONLY);
	if (fd < B_OK) {
		dprintf("ucode_load: couldn't find microcode\n");
		return;
	}
	struct stat stat;
	if (fstat(fd, &stat) < 0) {
		dprintf("ucode_load: couldn't stat microcode file\n");
		close(fd);
		return;
	}

	ssize_t length = stat.st_size;
	const uint32 alignment = 16;
#define ALIGN(size, align)	(((size) + align - 1) & ~(align - 1))
	void *buffer = kernel_args_malloc(length + alignment - 1);
	if (buffer != NULL) {
		buffer = (void*)ALIGN((addr_t)buffer, alignment);
		if (read(fd, buffer, length) != length) {
			dprintf("ucode_load: couldn't read microcode file\n");
			kernel_args_free(buffer);
		} else {
			gKernelArgs.ucode_data = buffer;
			gKernelArgs.ucode_data_size = length;
			dprintf("ucode_load: microcode file read in memory\n");
		}
	}

	close(fd);
}


extern "C" bigtime_t
system_time()
{
	uint64 tsc = __rdtsc();
	uint64 lo = (uint32)tsc;
	uint64 hi = tsc >> 32;
	return ((lo * gTimeConversionFactor) >> 32) + hi * gTimeConversionFactor;
}


extern "C" void
spin(bigtime_t microseconds)
{
	bigtime_t time = system_time();

	while ((system_time() - time) < microseconds)
		asm volatile ("pause;");
}


extern "C" status_t
boot_arch_cpu_init()
{
    // Nothing really to init on x86
    return B_OK;
}


extern "C" void
arch_ucode_load(BootVolume& volume)
{
    ucode_load(volume);
}
