/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/x86/descriptors.h>

#include <boot/kernel_args.h>
#include <cpu.h>
#include <tls.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>

#include <arch/int.h>
#include <arch/user_debugger.h>


template<typename T, T (*Function)(unsigned), unsigned N, unsigned ...Index>
struct GenerateTable : GenerateTable<T, Function, N - 1,  N - 1, Index...> {
};

template<typename T, T (*Function)(unsigned), unsigned ...Index>
struct GenerateTable<T, Function, 0, Index...> {
	GenerateTable()
		:
		fTable { Function(Index)... }
	{
	}

	T	fTable[sizeof...(Index)];
};

enum class DescriptorType : unsigned {
	DataWritable		= 0x2,
	CodeExecuteOnly		= 0x8,
	TSS					= 0x9,
};

class Descriptor {
public:
	constexpr				Descriptor();
	inline					Descriptor(uint32_t first, uint32_t second);
	constexpr				Descriptor(DescriptorType type, bool kernelOnly,
								bool compatMode = false);

protected:
	union {
		struct [[gnu::packed]] {
			uint16_t		fLimit0;
			unsigned		fBase0			:24;
			unsigned		fType			:4;
			unsigned		fSystem			:1;
			unsigned		fDPL			:2;
			unsigned		fPresent		:1;
			unsigned		fLimit1			:4;
			unsigned 		fUnused			:1;
			unsigned		fLong			:1;
			unsigned		fDB				:1;
			unsigned		fGranularity	:1;
			uint8_t			fBase1;
		};

		uint32_t			fDescriptor[2];
	};
};

class TSSDescriptor : public Descriptor {
public:
	inline						TSSDescriptor(uintptr_t base, size_t limit);

			const Descriptor&	GetLower() const	{ return *this; }
			const Descriptor&	GetUpper() const	{ return fSecond; }

	static	void				LoadTSS(unsigned index);

private:
			Descriptor			fSecond;
};


class UserTLSDescriptor : public Descriptor {
public:
	inline						UserTLSDescriptor(uintptr_t base, size_t limit);

			const Descriptor&	GetDescriptor() const	{ return *this; }
};


class GlobalDescriptorTable {
public:
	constexpr						GlobalDescriptorTable();

	inline	void					Load() const;

			unsigned				SetTSS(unsigned cpu,
										const TSSDescriptor& tss);
			unsigned				SetUserTLS(unsigned cpu,
										addr_t base, size_t limit);
private:
	static constexpr	unsigned	kFirstTSS = 6;
	static constexpr	unsigned	kDescriptorCount
										= kFirstTSS + SMP_MAX_CPUS * 3;

	alignas(uint64_t)	Descriptor	fTable[kDescriptorCount];
};

enum class InterruptDescriptorType : unsigned {
	Interrupt		= 14,
	Trap,
};

class [[gnu::packed]] InterruptDescriptor {
public:
	constexpr						InterruptDescriptor(uintptr_t isr,
										unsigned ist, bool kernelOnly);
	constexpr						InterruptDescriptor(uintptr_t isr);

	static 		InterruptDescriptor	Generate(unsigned index);

private:
						uint16_t	fBase0;
						uint16_t	fSelector;
						unsigned	fIST		:3;
						unsigned	fReserved0	:5;
						unsigned	fType		:4;
						unsigned	fReserved1	:1;
						unsigned	fDPL		:2;
						unsigned	fPresent	:1;
						uint16_t	fBase1;
						uint32_t	fBase2;
						uint32_t	fReserved2;
};

class InterruptDescriptorTable {
public:
	inline				void		Load() const;

	static constexpr	unsigned	kDescriptorCount = 256;

private:
	typedef GenerateTable<InterruptDescriptor, InterruptDescriptor::Generate,
			kDescriptorCount> TableType;
	alignas(uint64_t)	TableType	fTable;
};

class InterruptServiceRoutine {
	alignas(16)	uint8_t	fDummy[16];
};

extern const InterruptServiceRoutine
	isr_array[InterruptDescriptorTable::kDescriptorCount];

static GlobalDescriptorTable	sGDT;
static InterruptDescriptorTable	sIDT;
static uint32 sGDTIDTConstructed = 0;

typedef void interrupt_handler_function(iframe* frame);
interrupt_handler_function*
	gInterruptHandlerTable[InterruptDescriptorTable::kDescriptorCount];


constexpr bool
is_code_segment(DescriptorType type)
{
	return type == DescriptorType::CodeExecuteOnly;
};


constexpr
Descriptor::Descriptor()
	:
	fDescriptor { 0, 0 }
{
	static_assert(sizeof(Descriptor) == sizeof(uint64_t),
		"Invalid Descriptor size.");
}


Descriptor::Descriptor(uint32_t first, uint32_t second)
	:
	fDescriptor { first, second }
{
}


constexpr
Descriptor::Descriptor(DescriptorType type, bool kernelOnly, bool compatMode)
	:
	fLimit0(-1),
	fBase0(0),
	fType(static_cast<unsigned>(type)),
	fSystem(1),
	fDPL(kernelOnly ? 0 : 3),
	fPresent(1),
	fLimit1(0xf),
	fUnused(0),
	fLong(is_code_segment(type) && !compatMode ? 1 : 0),
	fDB(is_code_segment(type) && !compatMode ? 0 : 1),
	fGranularity(1),
	fBase1(0)
{
}


TSSDescriptor::TSSDescriptor(uintptr_t base, size_t limit)
	:
	fSecond(base >> 32, 0)
{
	fLimit0 = static_cast<uint16_t>(limit);
	fBase0 = base & 0xffffff;
	fType = static_cast<unsigned>(DescriptorType::TSS);
	fPresent = 1;
	fLimit1 = (limit >> 16) & 0xf;
	fBase1 = static_cast<uint8_t>(base >> 24);
}


void
TSSDescriptor::LoadTSS(unsigned index)
{
	asm volatile("ltr %w0" : : "r" (index << 3));
}


UserTLSDescriptor::UserTLSDescriptor(uintptr_t base, size_t limit)
	: Descriptor(DescriptorType::DataWritable, false)
{
	fLimit0 = static_cast<uint16_t>(limit);
	fBase0 = base & 0xffffff;
	fLimit1 = (limit >> 16) & 0xf;
	fBase1 = static_cast<uint8_t>(base >> 24);
}


constexpr
GlobalDescriptorTable::GlobalDescriptorTable()
	:
	fTable {
		Descriptor(),
		Descriptor(DescriptorType::CodeExecuteOnly, true),
		Descriptor(DescriptorType::DataWritable, true),
		Descriptor(DescriptorType::CodeExecuteOnly, false, true),
		Descriptor(DescriptorType::DataWritable, false),
		Descriptor(DescriptorType::CodeExecuteOnly, false),
	}
{
	static_assert(kDescriptorCount <= 8192,
		"GDT cannot contain more than 8192 descriptors");
}


void
GlobalDescriptorTable::Load() const
{
	struct [[gnu::packed]] {
		uint16_t	fLimit;
		const void*	fAddress;
	} gdtDescriptor = {
		sizeof(fTable) - 1,
		static_cast<const void*>(fTable),
	};

	asm volatile("lgdt	%0" : : "m" (gdtDescriptor));
}


unsigned
GlobalDescriptorTable::SetTSS(unsigned cpu, const TSSDescriptor& tss)
{
	auto index = kFirstTSS + cpu * 3;
	ASSERT(index + 1 < kDescriptorCount);
	fTable[index] = tss.GetLower();
	fTable[index + 1] = tss.GetUpper();
	return index;
}


unsigned
GlobalDescriptorTable::SetUserTLS(unsigned cpu, uintptr_t base, size_t limit)
{
	auto index = kFirstTSS + cpu * 3 + 2;
	ASSERT(index < kDescriptorCount);
	UserTLSDescriptor desc(base, limit);
	fTable[index] = desc.GetDescriptor();
	return index;
}


constexpr
InterruptDescriptor::InterruptDescriptor(uintptr_t isr, unsigned ist,
	bool kernelOnly)
	:
	fBase0(isr),
	fSelector(KERNEL_CODE_SELECTOR),
	fIST(ist),
	fReserved0(0),
	fType(static_cast<unsigned>(InterruptDescriptorType::Interrupt)),
	fReserved1(0),
	fDPL(kernelOnly ? 0 : 3),
	fPresent(1),
	fBase1(isr >> 16),
	fBase2(isr >> 32),
	fReserved2(0)
{
	static_assert(sizeof(InterruptDescriptor) == sizeof(uint64_t) * 2,
		"Invalid InterruptDescriptor size.");
}


constexpr
InterruptDescriptor::InterruptDescriptor(uintptr_t isr)
	:
	InterruptDescriptor(isr, 0, true)
{
}


void
InterruptDescriptorTable::Load() const
{
	struct [[gnu::packed]] {
		uint16_t	fLimit;
		const void*	fAddress;
	} gdtDescriptor = {
		sizeof(fTable) - 1,
		static_cast<const void*>(fTable.fTable),
	};

	asm volatile("lidt	%0" : : "m" (gdtDescriptor));
}


InterruptDescriptor
InterruptDescriptor::Generate(unsigned index)
{
	return index == 3
		? InterruptDescriptor(uintptr_t(isr_array + index), 0, false)
		: (index == 8
			? InterruptDescriptor(uintptr_t(isr_array + index), 1, true)
			: InterruptDescriptor(uintptr_t(isr_array + index)));
}


//	#pragma mark - Exception handlers


static void
x86_64_general_protection_fault(iframe* frame)
{
	if (debug_debugger_running()) {
		// Handle GPFs if there is a debugger fault handler installed, for
		// non-canonical address accesses.
		cpu_ent* cpu = &gCPU[smp_get_current_cpu()];
		if (cpu->fault_handler != 0) {
			debug_set_page_fault_info(0, frame->ip, DEBUG_PAGE_FAULT_NO_INFO);
			frame->ip = cpu->fault_handler;
			frame->bp = cpu->fault_handler_stack_pointer;
			return;
		}
	}

	x86_unexpected_exception(frame);
}


static void
x86_64_stack_fault_exception(iframe* frame)
{
	// Non-canonical address accesses which reference the stack cause a stack
	// fault exception instead of GPF. However, we can treat it like a GPF.
	x86_64_general_protection_fault(frame);
}


// #pragma mark -


void
x86_descriptors_preboot_init_percpu(kernel_args* args, int cpu)
{
	if (cpu == 0) {
		new(&sGDT) GlobalDescriptorTable;
		new(&sIDT) InterruptDescriptorTable;
	}

	smp_cpu_rendezvous(&sGDTIDTConstructed);
	sGDT.Load();

	memset(&gCPU[cpu].arch.tss, 0, sizeof(struct tss));
	gCPU[cpu].arch.tss.io_map_base = sizeof(struct tss);

	// Set up the double fault IST entry (see x86_descriptors_init()).
	struct tss* tss = &gCPU[cpu].arch.tss;
	size_t stackSize;
	tss->ist1 = (addr_t)x86_get_double_fault_stack(cpu, &stackSize);
	tss->ist1 += stackSize;

	// Set up the descriptor for this TSS.
	auto tssIndex = sGDT.SetTSS(cpu,
			TSSDescriptor(uintptr_t(&gCPU[cpu].arch.tss), sizeof(struct tss)));
	TSSDescriptor::LoadTSS(tssIndex);

	sGDT.SetUserTLS(cpu, 0, TLS_COMPAT_SIZE);

	sIDT.Load();
}


void
x86_descriptors_init(kernel_args* args)
{
	// Initialize the interrupt handler table.
	interrupt_handler_function** table = gInterruptHandlerTable;
	for (uint32 i = 0; i < ARCH_INTERRUPT_BASE; i++)
		table[i] = x86_invalid_exception;
	for (uint32 i = ARCH_INTERRUPT_BASE;
		i < InterruptDescriptorTable::kDescriptorCount; i++) {
		table[i] = x86_hardware_interrupt;
	}

	table[0]  = x86_unexpected_exception;	// Divide Error Exception (#DE)
	table[1]  = x86_handle_debug_exception; // Debug Exception (#DB)
	table[2]  = x86_fatal_exception;		// NMI Interrupt
	table[3]  = x86_handle_breakpoint_exception; // Breakpoint Exception (#BP)
	table[4]  = x86_unexpected_exception;	// Overflow Exception (#OF)
	table[5]  = x86_unexpected_exception;	// BOUND Range Exceeded Exception (#BR)
	table[6]  = x86_unexpected_exception;	// Invalid Opcode Exception (#UD)
	table[7]  = x86_fatal_exception;		// Device Not Available Exception (#NM)
	table[8]  = x86_fatal_exception;		// Double Fault Exception (#DF)
	table[9]  = x86_fatal_exception;		// Coprocessor Segment Overrun
	table[10] = x86_fatal_exception;		// Invalid TSS Exception (#TS)
	table[11] = x86_fatal_exception;		// Segment Not Present (#NP)
	table[12] = x86_64_stack_fault_exception;	// Stack Fault Exception (#SS)
	table[13] = x86_64_general_protection_fault; // General Protection Exception (#GP)
	table[14] = x86_page_fault_exception;	// Page-Fault Exception (#PF)
	table[16] = x86_unexpected_exception;	// x87 FPU Floating-Point Error (#MF)
	table[17] = x86_unexpected_exception;	// Alignment Check Exception (#AC)
	table[18] = x86_fatal_exception;		// Machine-Check Exception (#MC)
	table[19] = x86_unexpected_exception;	// SIMD Floating-Point Exception (#XF)
}


unsigned
x86_64_set_user_tls_segment_base(int cpu, addr_t base)
{
	return sGDT.SetUserTLS(cpu, base, TLS_COMPAT_SIZE);
}
