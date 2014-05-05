/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/x86/descriptors.h>

#include <boot/kernel_args.h>
#include <cpu.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>

#include <arch/int.h>
#include <arch/user_debugger.h>


#define IDT_GATES_COUNT	256


enum class DescriptorType : unsigned {
	DataWritable		= 0x2,
	CodeExecuteOnly		= 0x8,
	TSS					= 0x9,
};

class Descriptor {
public:
	constexpr				Descriptor();
	inline					Descriptor(uint32_t first, uint32_t second);
	constexpr				Descriptor(DescriptorType type, bool kernelOnly);

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

class GlobalDescriptorTable {
public:
	constexpr						GlobalDescriptorTable();

	inline	void					Load() const;

			unsigned				SetTSS(unsigned cpu,
										const TSSDescriptor& tss);
private:
	static constexpr	unsigned	kFirstTSS = 5;
	static constexpr	unsigned	kDescriptorCount
										= kFirstTSS + SMP_MAX_CPUS * 2;

	alignas(sizeof(Descriptor))	Descriptor	fTable[kDescriptorCount];
};


static GlobalDescriptorTable	sGDT;

static interrupt_descriptor sIDT[IDT_GATES_COUNT];

typedef void interrupt_handler_function(iframe* frame);
static const uint32 kInterruptHandlerTableSize = IDT_GATES_COUNT;
interrupt_handler_function* gInterruptHandlerTable[kInterruptHandlerTableSize];

extern uint8 isr_array[kInterruptHandlerTableSize][16];


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
Descriptor::Descriptor(DescriptorType type, bool kernelOnly)
	:
	fLimit0(-1),
	fBase0(0),
	fType(static_cast<unsigned>(type)),
	fSystem(1),
	fDPL(kernelOnly ? 0 : 3),
	fPresent(1),
	fLimit1(0xf),
	fUnused(0),
	fLong(is_code_segment(type) ? 1 : 0),
	fDB(is_code_segment(type) ? 0 : 1),
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


constexpr
GlobalDescriptorTable::GlobalDescriptorTable()
	:
	fTable {
		Descriptor(),
		Descriptor(DescriptorType::CodeExecuteOnly, true),
		Descriptor(DescriptorType::DataWritable, true),
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
	auto index = kFirstTSS + cpu * 2;
	ASSERT(index + 1 < kDescriptorCount);
	fTable[index] = tss.GetLower();
	fTable[index + 1] = tss.GetUpper();
	return index;
}


static inline void
load_idt()
{
	struct {
		uint16	limit;
		void*	address;
	} _PACKED idtDescriptor = {
		IDT_GATES_COUNT * sizeof(interrupt_descriptor) - 1,
		sIDT
	};

	asm volatile("lidt %0" : : "m" (idtDescriptor));
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


// #pragma mark -


void
x86_descriptors_preboot_init_percpu(kernel_args* args, int cpu)
{
	new(&sGDT) GlobalDescriptorTable;
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

	load_idt();
}


void
x86_descriptors_init(kernel_args* args)
{
	// Fill out the IDT, pointing each entry to the corresponding entry in the
	// ISR array created in arch_interrupts.S (see there to see how this works).
	for(uint32 i = 0; i < kInterruptHandlerTableSize; i++) {
		// x86_64 removes task gates, therefore we cannot use a separate TSS
		// for the double fault exception. However, instead it adds a new stack
		// switching mechanism, the IST. The IST is a table of stack addresses
		// in the TSS. If the IST field of an interrupt descriptor is non-zero,
		// the CPU will switch to the stack specified by that IST entry when
		// handling that interrupt. So, we use IST entry 1 to store the double
		// fault stack address (set up in x86_descriptors_init_post_vm()).
		uint32 ist = (i == 8) ? 1 : 0;

		// Breakpoint exception can be raised from userland.
		uint32 dpl = (i == 3) ? DPL_USER : DPL_KERNEL;

		set_interrupt_descriptor(&sIDT[i], (addr_t)&isr_array[i],
			GATE_INTERRUPT, KERNEL_CODE_SELECTOR, dpl, ist);
	}

	// Initialize the interrupt handler table.
	interrupt_handler_function** table = gInterruptHandlerTable;
	for (uint32 i = 0; i < ARCH_INTERRUPT_BASE; i++)
		table[i] = x86_invalid_exception;
	for (uint32 i = ARCH_INTERRUPT_BASE; i < kInterruptHandlerTableSize; i++)
		table[i] = x86_hardware_interrupt;

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
	table[12] = x86_fatal_exception;		// Stack Fault Exception (#SS)
	table[13] = x86_64_general_protection_fault; // General Protection Exception (#GP)
	table[14] = x86_page_fault_exception;	// Page-Fault Exception (#PF)
	table[16] = x86_unexpected_exception;	// x87 FPU Floating-Point Error (#MF)
	table[17] = x86_unexpected_exception;	// Alignment Check Exception (#AC)
	table[18] = x86_fatal_exception;		// Machine-Check Exception (#MC)
	table[19] = x86_unexpected_exception;	// SIMD Floating-Point Exception (#XF)
}

