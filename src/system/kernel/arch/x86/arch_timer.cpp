/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <arch/timer.h>
#include <arch_system_info.h>

#include <cpu.h>
#include <timer.h>
#include <interrupts.h>

#include <arch/x86/smp_priv.h>
#include <arch/x86/timer.h>

#include <x86intrin.h>


//#define TRACE_TIMER
#ifdef TRACE_TIMER
#	define TRACE(x...) dprintf("arch_timer: " x)
#else
#	define TRACE(x...) ;
#endif


extern timer_info gPITTimer;
extern timer_info gAPICTimer;
extern timer_info gHPETTimer;

static timer_info *sTimers[] = {
	&gHPETTimer,
	&gAPICTimer,
	&gPITTimer,
	NULL
};

static timer_info *sTimer = NULL;


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	TRACE("arch_timer_set_hardware_timer: timeout %" B_PRIdBIGTIME "\n", timeout);
	sTimer->set_hardware_timer(timeout);
}


void
arch_timer_clear_hardware_timer(void)
{
	TRACE("arch_timer_clear_hardware_timer\n");
	sTimer->clear_hardware_timer();
}


static void
sort_timers(timer_info *timers[], int numTimers)
{
	timer_info *tempPtr;
	int max = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i < numTimers - 1; i++) {
		max = i;
		for (j = i + 1; j < numTimers; j++) {
			if (timers[j]->get_priority() > timers[max]->get_priority())
				max = j;
		}
		if (max != i) {
			tempPtr = timers[max];
			timers[max] = timers[i];
			timers[i] = tempPtr;
		}
	}

#if 0
	for (i = 0; i < numTimers; i++)
		dprintf(" %s: priority %d\n", timers[i]->name, timers[i]->get_priority());
#endif
}


int
arch_init_timer(kernel_args *args)
{
	// Sort timers by priority
	sort_timers(sTimers, (sizeof(sTimers) / sizeof(sTimers[0])) - 1);

	timer_info *timer = NULL;
	cpu_status state = disable_interrupts();

	for (int i = 0; (timer = sTimers[i]) != NULL; i++) {
		if (timer->init(args) == B_OK)
			break;
	}

	sTimer = timer;

	if (sTimer != NULL) {
		dprintf("arch_init_timer: using %s timer.\n", sTimer->name);
	} else {
		panic("No system timers were found usable.\n");
	}

	restore_interrupts(state);

	return 0;
}


// #pragma mark -


uint32 gSystemTimeConversionFactor;
int64 gCPUClockSpeed;


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


#ifdef __SIZEOF_INT128__
typedef unsigned __int128 uint128;
#else
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
#endif


static inline uint64_t
rdtsc_fenced()
{
	// RDTSC is not serializing, nor does it drain the instruction stream.
	// RDTSCP does, but is not available everywhere. Other OSes seem to use
	// "CPUID" rather than MFENCE/LFENCE for serializing here during boot.
	asm volatile ("cpuid" : : : "eax", "ebx", "ecx", "edx");

	return __rdtsc();
}


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
	uint64 startTSC = rdtsc_fenced();

	// Wait for the PIT to count down to our desired value
	uint8 endLow;
	uint8 endHigh;
	do {
		out8(select | PIT_ACCESS_LATCH_COUNTER, PIT_CONTROL);
		endLow = in8(channelPort);
		endHigh = in8(channelPort);
	} while (endHigh > desiredHighByte);

	// And read the second TSC value
	uint64 endTSC = rdtsc_fenced();

	tscDelta = endTSC - startTSC;
	expired = ((startHigh << 8) | startLow) - ((endHigh << 8) | endLow);
	conversionFactor = (double)tscDelta / (double)expired;
}


static void
calculate_cpu_conversion_factor(uint8 channel, uint32* conversionFactor)
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
	*conversionFactor = ((uint128(expired) * uint32(1000000)) << 32)
		/ uint128(tscDeltaSlow);

#ifdef TRACE_TIMER
	if (clockSpeed > 1000000000LL) {
		dprintf("CPU at %lld.%03Ld GHz\n", clockSpeed / 1000000000LL,
			(clockSpeed % 1000000000LL) / 1000000LL);
	} else {
		dprintf("CPU at %lld.%03Ld MHz\n", clockSpeed / 1000000LL,
			(clockSpeed % 1000000LL) / 1000LL);
	}
#endif

	gCPUClockSpeed = clockSpeed;
	//dprintf("factors: %lu %llu\n", gTimeConversionFactor, clockSpeed);

#ifdef TRACE_TIMER
	if (quickSampleCount > 1) {
		dprintf("needed %" B_PRIu32 " quick samples for TSC calibration\n",
			quickSampleCount);
	}

	if (slowSampleCount > 1) {
		dprintf("needed %" B_PRIu32 " slow samples for TSC calibration\n",
			slowSampleCount);
	}
#endif

	if (channel == 2) {
		// Set the gate low again
		out8(in8(PIT_CHANNEL_2_CONTROL) & ~PIT_CHANNEL_2_GATE_HIGH,
			PIT_CHANNEL_2_CONTROL);
	}

	dprintf("CPU: TSC frequency calibrated via PIT: %" B_PRIu32 "\n", *conversionFactor);
}


static void
init_tsc_with_cpuid(kernel_args* args, uint32* conversionFactor)
{
	cpu_ent* cpu = get_cpu_struct();
	if (cpu->arch.vendor != VENDOR_INTEL)
		return;

	uint32 model = (cpu->arch.extended_model << 4) | cpu->arch.model;
	cpuid_info cpuid;
	get_current_cpuid(&cpuid, 0, 0);
	uint32 maxBasicLeaf = cpuid.eax_0.max_eax;
	if (maxBasicLeaf < IA32_CPUID_LEAF_TSC)
		return;

	uint32 frequency = 0;
	if (maxBasicLeaf >= IA32_CPUID_LEAF_FREQUENCY) {
		get_current_cpuid(&cpuid, IA32_CPUID_LEAF_FREQUENCY, 0);
		frequency = cpuid.regs.eax;
		gCPUClockSpeed = frequency * 1000000LL;
	}

	get_current_cpuid(&cpuid, IA32_CPUID_LEAF_TSC, 0);
	if (cpuid.regs.eax == 0 || cpuid.regs.ebx == 0)
		return;
	uint32 khz = cpuid.regs.ecx / 1000;
	uint32 denominator = cpuid.regs.eax;
	uint32 numerator = cpuid.regs.ebx;
	if (khz == 0 && model == 0x5f) {
		// CPUID_LEAF_FREQUENCY isn't supported, hardcoding
		khz = 25000;
	}

	if (khz == 0 && frequency != 0) {
		// for these CPUs the base frequency is also the tsc frequency
		khz = frequency * 1000 * denominator / numerator;
	}
	if (khz == 0)
		return;

	dprintf("CPU: using TSC frequency from CPUID\n");
	// compute for microseconds as follows (1000000 << 32) / (tsc freq in Hz),
	// or (1000 << 32) / (tsc freq in kHz)
	*conversionFactor = (1000ULL << 32) / (khz * numerator / denominator);
}


static void
init_tsc_with_msr(kernel_args* args, uint32* conversionFactor)
{
	cpu_ent* cpuEnt = get_cpu_struct();
	if (cpuEnt->arch.vendor != VENDOR_AMD)
		return;

	uint32 family = cpuEnt->arch.family + cpuEnt->arch.extended_family;
	if (family < 0x10)
		return;
	uint64 value = x86_read_msr(MSR_F10H_HWCR);
	if ((value & HWCR_TSCFREQSEL) == 0)
		return;

	value = x86_read_msr(MSR_F10H_PSTATEDEF(0));
	if ((value & PSTATEDEF_EN) == 0)
		return;
	if (family != 0x17 && family != 0x19)
		return;

	uint64 khz = 200 * 1000;
	uint32 denominator = (value >> 8) & 0x3f;
	if (denominator < 0x8 || denominator > 0x2c)
		return;
	if (denominator > 0x1a && (denominator % 2) == 1)
		return;
	uint32 numerator = value & 0xff;
	if (numerator < 0x10)
		return;

	dprintf("CPU: using TSC frequency from MSR %" B_PRIu64 "\n", khz * numerator / denominator);
	// compute for microseconds as follows (1000000 << 32) / (tsc freq in Hz),
	// or (1000 << 32) / (tsc freq in kHz)
	*conversionFactor = (1000ULL << 32) / (khz * numerator / denominator);
}


static void
init_tsc_on_hypervisor(kernel_args* args, uint32* conversionFactor)
{
	cpuid_info info;
	if (get_current_cpuid(&info, 1, 0) != B_OK
			|| (info.regs.ecx & IA32_FEATURE_EXT_HYPERVISOR) == 0)
		return;

	get_current_cpuid(&info, 0x40000000, 0);
	const uint32 maxVMM = info.regs.eax;

	if (maxVMM >= 0x40000010) {
		get_current_cpuid(&info, 0x40000010, 0);

		uint64 clockSpeed = uint64(info.regs.eax) * 1000;
		*conversionFactor = (uint64(1000) << 32) / info.regs.eax;

		gCPUClockSpeed = clockSpeed;

		dprintf("CPU: TSC frequency read from hypervisor CPUID leaf\n");
		return;
	}

	if (maxVMM >= 0x40000003) {
		// Hyper-V does not implement the standard hypervisor timing interface
		// "Hv#1" here indicates Hyper-V
		get_current_cpuid(&info, 0x40000001, 0);
		if (info.regs.eax == 0x31237648) {
			// Check for HV_X64_MSR_TSC_FREQUENCY presence
			// TSC frequency is represented in Hz
			get_current_cpuid(&info, 0x40000003, 0);
			if ((info.regs.eax & (1 << 11)) != 0) {
				uint64 clockSpeed = x86_read_msr(0x40000022);
				if (clockSpeed > 0) {
					*conversionFactor = (uint64(1000000) << 32) / clockSpeed;

					gCPUClockSpeed = clockSpeed;

					dprintf("CPU: TSC frequency read from Hyper-V MSR\n");
					return;
				}
			}
		}
	}
}


void
x86_init_tsc(kernel_args* args)
{
	// init the TSC -> system_time() conversion factors

	// try to find the TSC frequency with CPUID
	uint32 conversionFactor = 0;
	init_tsc_with_cpuid(args, &conversionFactor);
	init_tsc_with_msr(args, &conversionFactor);
	init_tsc_on_hypervisor(args, &conversionFactor);
	if (conversionFactor == 0)
		calculate_cpu_conversion_factor(2, &conversionFactor);

	gSystemTimeConversionFactor = conversionFactor;

	uint64 conversionFactorNsecs = (uint64)conversionFactor * 1000;

#ifdef __x86_64__
	// The x86_64 system_time() implementation uses 64-bit multiplication and
	// therefore shifting is not necessary for low frequencies (it's also not
	// too likely that there'll be any x86_64 CPUs clocked under 1GHz).
	__x86_setup_system_time((uint64)conversionFactor << 32,
		conversionFactorNsecs);
#else
	if (conversionFactorNsecs >> 32 != 0) {
		// the TSC frequency is < 1 GHz, which forces us to shift the factor
		__x86_setup_system_time(conversionFactor, conversionFactorNsecs >> 16,
			true);
	} else {
		// the TSC frequency is >= 1 GHz
		__x86_setup_system_time(conversionFactor, conversionFactorNsecs, false);
	}
#endif
}
