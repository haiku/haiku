/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <drivers/bios.h>

#include <KernelExport.h>

#include <AutoDeleter.h>

#include <arch/x86/arch_cpu.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "x86emu.h"


struct BIOSState {
	BIOSState()
		:
		mapped_address(0),
		bios_area(-1),
		ram_area(-1),
		allocated_size(0)
	{
	}

	~BIOSState()
	{
		if (bios_area >= 0)
			delete_area(bios_area);
		if (ram_area >= 0)
			delete_area(ram_area);
	}

	addr_t		mapped_address;
	area_id		bios_area;
	area_id		ram_area;
	size_t		allocated_size;
};


// BIOS memory layout definitions.
static const uint32 kBDABase = 0;
static const uint32 kBDASize = 0x1000;
static const uint32 kEBDABase = 0x90000;
static const uint32 kEBDASize = 0x70000;
static const uint32 kRAMBase = 0x1000;
static const uint32 kRAMSize = 0x8f000;
static const uint32 kStackSize = 0x1000;
static const uint32 kTotalSize = 0x100000;


static sem_id sBIOSLock;
static BIOSState* sCurrentBIOSState;


//	#pragma mark - X86EMU hooks.


static x86emuu8
x86emu_pio_inb(X86EMU_pioAddr port)
{
	return in8(port);
}


static x86emuu16
x86emu_pio_inw(X86EMU_pioAddr port)
{
	return in16(port);
}


static x86emuu32
x86emu_pio_inl(X86EMU_pioAddr port)
{
	return in32(port);
}


static void
x86emu_pio_outb(X86EMU_pioAddr port, x86emuu8 data)
{
	out8(data, port);
}


static void
x86emu_pio_outw(X86EMU_pioAddr port, x86emuu16 data)
{
	out16(data, port);
}


static void
x86emu_pio_outl(X86EMU_pioAddr port, x86emuu32 data)
{
	out32(data, port);
}


static X86EMU_pioFuncs x86emu_pio_funcs = {
	x86emu_pio_inb,
	x86emu_pio_inw,
	x86emu_pio_inl,
	x86emu_pio_outb,
	x86emu_pio_outw,
	x86emu_pio_outl,
};


static x86emuu8
x86emu_mem_rdb(x86emuu32 addr)
{
	return *(x86emuu8*)((addr_t)addr + sCurrentBIOSState->mapped_address);
}


static x86emuu16
x86emu_mem_rdw(x86emuu32 addr)
{
	return *(x86emuu16*)((addr_t)addr + sCurrentBIOSState->mapped_address);
}


static x86emuu32
x86emu_mem_rdl(x86emuu32 addr)
{
	return *(x86emuu32*)((addr_t)addr + sCurrentBIOSState->mapped_address);
}


static void
x86emu_mem_wrb(x86emuu32 addr, x86emuu8 val)
{
	*(x86emuu8*)((addr_t)addr + sCurrentBIOSState->mapped_address) = val;
}


static void
x86emu_mem_wrw(x86emuu32 addr, x86emuu16 val)
{
	*(x86emuu16*)((addr_t)addr + sCurrentBIOSState->mapped_address) = val;
}


static void
x86emu_mem_wrl(x86emuu32 addr, x86emuu32 val)
{
	*(x86emuu32*)((addr_t)addr + sCurrentBIOSState->mapped_address) = val;
}


static X86EMU_memFuncs x86emu_mem_funcs = {
	x86emu_mem_rdb,
	x86emu_mem_rdw,
	x86emu_mem_rdl,
	x86emu_mem_wrb,
	x86emu_mem_wrw,
	x86emu_mem_wrl,
};


//	#pragma mark -


static void*
bios_allocate_mem(bios_state* state, size_t size)
{
	// Simple allocator for memory to pass to the BIOS. No need for a complex
	// allocator here, there is only a few allocations per BIOS usage.

	size = ROUNDUP(size, 4);

	if (state->allocated_size + size > kRAMSize)
		return NULL;

	void* address
		= (void*)(state->mapped_address + kRAMBase + state->allocated_size);
	state->allocated_size += size;

	return address;
}


static uint32
bios_physical_address(bios_state* state, void* virtualAddress)
{
	return (uint32)((addr_t)virtualAddress - state->mapped_address);
}


static void*
bios_virtual_address(bios_state* state, uint32 physicalAddress)
{
	return (void*)((addr_t)physicalAddress + state->mapped_address);
}


static status_t
bios_prepare(bios_state** _state)
{
	status_t status;

	BIOSState* state = new(std::nothrow) BIOSState;
	if (state == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<BIOSState> stateDeleter(state);

	// Reserve a chunk of address space to map at.
	status = vm_reserve_address_range(VMAddressSpace::KernelID(),
		(void**)&state->mapped_address, B_ANY_KERNEL_ADDRESS,
		kTotalSize, 0);
	if (status != B_OK)
		return status;

	// Map RAM for for the BIOS.
	state->ram_area = create_area("bios ram", (void**)&state->mapped_address,
		B_EXACT_ADDRESS, kBDASize + kRAMSize, B_NO_LOCK, B_KERNEL_READ_AREA
			| B_KERNEL_WRITE_AREA);
	if (state->ram_area < B_OK) {
		vm_unreserve_address_range(VMAddressSpace::KernelID(),
			(void*)state->mapped_address, kTotalSize);
		return state->ram_area;
	}

	// Copy the interrupt vectors and the BIOS data area.
	status = vm_memcpy_from_physical((void*)state->mapped_address, kBDABase,
		kBDASize, false);
	if (status != B_OK) {
		vm_unreserve_address_range(VMAddressSpace::KernelID(),
			(void*)state->mapped_address, kTotalSize);
		return status;
	}

	// Map the extended BIOS data area and VGA memory.
	void* address = (void*)(state->mapped_address + kEBDABase);
	state->bios_area = map_physical_memory("bios", kEBDABase, kEBDASize,
		B_EXACT_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, &address);
	if (state->bios_area < B_OK) {
		vm_unreserve_address_range(VMAddressSpace::KernelID(),
			(void*)state->mapped_address, kTotalSize);
		return state->bios_area;
	}

	// Attempt to acquire exclusive access to the BIOS.
	acquire_sem(sBIOSLock);

	stateDeleter.Detach();
	*_state = state;
	return B_OK;
}


static status_t
bios_interrupt(bios_state* state, uint8 vector, bios_regs* regs)
{
	sCurrentBIOSState = state;

	// Allocate a stack.
	void* stack = bios_allocate_mem(state, kStackSize);
	if (stack == NULL)
		return B_NO_MEMORY;
	uint32 stackTop = bios_physical_address(state, stack) + kStackSize;

	// X86EMU finishes when it encounters a HLT instruction, allocate a
	// byte to store one in and set the return address of the interrupt to
	// point to it.
	void* halt = bios_allocate_mem(state, 1);
	if (halt == NULL)
		return B_NO_MEMORY;
	*(uint8*)halt = 0xF4;

	// Copy in the registers.
	memset(&M, 0, sizeof(M));
	M.x86.R_EAX = regs->eax;
	M.x86.R_EBX = regs->ebx;
	M.x86.R_ECX = regs->ecx;
	M.x86.R_EDX = regs->edx;
	M.x86.R_EDI = regs->edi;
	M.x86.R_ESI = regs->esi;
	M.x86.R_EBP = regs->ebp;
	M.x86.R_EFLG = regs->eflags | X86_EFLAGS_INTERRUPT | X86_EFLAGS_RESERVED1;
	M.x86.R_EIP = bios_physical_address(state, halt);
	M.x86.R_CS = 0x0;
	M.x86.R_DS = regs->ds;
	M.x86.R_ES = regs->es;
	M.x86.R_FS = regs->fs;
	M.x86.R_GS = regs->gs;
	M.x86.R_SS = stackTop >> 4;
	M.x86.R_ESP = stackTop - (M.x86.R_SS << 4);

	/* Run the interrupt. */
	X86EMU_setupPioFuncs(&x86emu_pio_funcs);
	X86EMU_setupMemFuncs(&x86emu_mem_funcs);
	X86EMU_prepareForInt(vector);
	X86EMU_exec();

	// Copy back modified data.
	regs->eax = M.x86.R_EAX;
	regs->ebx = M.x86.R_EBX;
	regs->ecx = M.x86.R_ECX;
	regs->edx = M.x86.R_EDX;
	regs->edi = M.x86.R_EDI;
	regs->esi = M.x86.R_ESI;
	regs->ebp = M.x86.R_EBP;
	regs->eflags = M.x86.R_EFLG;
	regs->ds = M.x86.R_DS;
	regs->es = M.x86.R_ES;
	regs->fs = M.x86.R_FS;
	regs->gs = M.x86.R_GS;

	return B_OK;
}


static void
bios_finish(bios_state* state)
{
	release_sem(sBIOSLock);

	delete state;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			sBIOSLock = create_sem(1, "bios lock");
			if (sBIOSLock < B_OK)
				return sBIOSLock;

			return B_OK;
		case B_MODULE_UNINIT:
			delete_sem(sBIOSLock);
			return B_OK;
		default:
			return B_ERROR;
	}
}


static bios_module_info sBIOSModule = {
	{
		B_BIOS_MODULE_NAME,
		0,
		std_ops
	},

	bios_prepare,
	bios_interrupt,
	bios_finish,
	bios_allocate_mem,
	bios_physical_address,
	bios_virtual_address
};


module_info *modules[] = {
	(module_info*)&sBIOSModule,
	NULL
};
