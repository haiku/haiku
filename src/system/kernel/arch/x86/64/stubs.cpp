/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


// This file contains stubs for everything that's not been implemented yet on
// x86_64.


#include <cpu.h>
#include <commpage.h>
#include <debug.h>
#include <debugger.h>
#include <elf.h>
#include <elf_priv.h>
#include <real_time_clock.h>
#include <real_time_data.h>
#include <smp.h>
#include <timer.h>
#include <team.h>
#include <thread.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include <boot/kernel_args.h>

#include <arch/debug.h>
#include <arch/vm_translation_map.h>
#include <arch/vm.h>
#include <arch/user_debugger.h>
#include <arch/timer.h>
#include <arch/thread.h>
#include <arch/system_info.h>
#include <arch/smp.h>
#include <arch/real_time_clock.h>
#include <arch/elf.h>


status_t
arch_commpage_init(void)
{
	return B_OK;
}


status_t
arch_commpage_init_post_cpus(void)
{
	return B_OK;
}


ssize_t
arch_cpu_user_strlcpy(char* to, const char* from, size_t size,
	addr_t* faultHandler)
{
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memcpy(void* to, const void* from, size_t size,
	addr_t* faultHandler)
{
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memset(void* s, char c, size_t count, addr_t* faultHandler)
{
	return B_BAD_ADDRESS;
}


void
arch_debug_save_registers(struct arch_debug_registers* registers)
{

}


struct stack_frame {
	struct stack_frame	*previous;
	addr_t				return_address;
};


static bool
is_iframe(addr_t frame)
{
	addr_t previousFrame = *(addr_t*)frame;
	return ((previousFrame & ~(addr_t)IFRAME_TYPE_MASK) == 0
		&& previousFrame != 0);
}


static void
print_iframe(struct iframe* frame)
{
	bool isUser = IFRAME_IS_USER(frame);

	kprintf("%s iframe at %p (end = %p)\n", isUser ? "user" : "kernel", frame,
		isUser ? (uint64*)(frame + 1) : &frame->user_sp);

	kprintf(" rax 0x%-16lx    rbx 0x%-16lx    rcx 0x%lx\n", frame->ax,
		frame->bx, frame->cx);
	kprintf(" rdx 0x%-16lx    rsi 0x%-16lx    rdi 0x%lx\n", frame->dx,
		frame->si, frame->di);
	kprintf(" rbp 0x%-16lx     r8 0x%-16lx     r9 0x%lx\n", frame->bp,
		frame->r8, frame->r9);
	kprintf(" r10 0x%-16lx    r11 0x%-16lx    r12 0x%lx\n", frame->r10,
		frame->r11, frame->r12);
	kprintf(" r13 0x%-16lx    r14 0x%-16lx    r15 0x%lx\n", frame->r13,
		frame->r14, frame->r15);
	kprintf(" rip 0x%-16lx rflags 0x%-16lx", frame->ip, frame->flags);

	if (isUser) {
		// from user space
		kprintf("user rsp 0x%lx", frame->user_sp);
	}
	kprintf("\n");
	kprintf(" vector: 0x%lx, error code: 0x%lx\n", frame->vector,
		frame->error_code);
}


static status_t
lookup_symbol(addr_t address, addr_t* _baseAddress, const char** _symbolName,
	const char** _imageName, bool* _exactMatch)
{
	status_t status = B_ENTRY_NOT_FOUND;

	if (IS_KERNEL_ADDRESS(address)) {
		status = elf_debug_lookup_symbol_address(address, _baseAddress,
			_symbolName, _imageName, _exactMatch);
	}

	return status;
}


static void
print_stack_frame(addr_t rip, addr_t rbp, addr_t nextRbp, int32 callIndex)
{
	const char* symbol;
	const char* image;
	addr_t baseAddress;
	bool exactMatch;
	status_t status;
	addr_t diff;

	diff = nextRbp - rbp;

	// kernel space/user space switch
	if (diff & (1L << 63))
		diff = 0;

	status = lookup_symbol(rip, &baseAddress, &symbol, &image, &exactMatch);

	kprintf("%2d %016lx (+%4ld) %016lx   ", callIndex, rbp, diff, rip);

	if (status == B_OK) {
		if (symbol != NULL)
			kprintf("<%s>:%s%s", image, symbol, exactMatch ? "" : " (nearest)");
		else
			kprintf("<%s@%p>:unknown", image, (void*)baseAddress);

		kprintf(" + 0x%04lx\n", rip - baseAddress);
	} else {
		VMArea *area = VMAddressSpace::Kernel()->LookupArea(rip);
		if (area != NULL) {
			kprintf("%d:%s@%p + %#lx\n", area->id, area->name,
				(void*)area->Base(), rip - area->Base());
		} else
			kprintf("\n");
	}
}


void
arch_debug_stack_trace(void)
{
	addr_t rbp = x86_get_stack_frame();

	kprintf("frame                       caller             <image>:function"
		" + offset\n");

	for (int32 callIndex = 0;; callIndex++) {
		if (rbp == 0)
			break;

		if (is_iframe(rbp)) {
			struct iframe* frame = (struct iframe*)rbp;
			print_iframe(frame);
			print_stack_frame(frame->ip, rbp, frame->bp, callIndex);

			rbp = frame->bp;
		} else {
			stack_frame* frame = (stack_frame*)rbp;
			if (frame->return_address == 0)
				break;

			print_stack_frame(frame->return_address, rbp,
				(frame->previous != NULL) ? (addr_t)frame->previous : rbp,
				callIndex);

			rbp = (addr_t)frame->previous;
		}
	}
}


bool
arch_debug_contains_call(Thread *thread, const char *symbol,
	addr_t start, addr_t end)
{
	return false;
}


void *
arch_debug_get_caller(void)
{
	return NULL;
}


int32
arch_debug_get_stack_trace(addr_t* returnAddresses, int32 maxCount,
	int32 skipIframes, int32 skipFrames, uint32 flags)
{
	return 0;
}


void*
arch_debug_get_interrupt_pc(bool* _isSyscall)
{
	return NULL;
}


void
arch_debug_unset_current_thread(void)
{

}


bool
arch_is_debug_variable_defined(const char* variableName)
{
	return false;
}


status_t
arch_set_debug_variable(const char* variableName, uint64 value)
{
	return B_OK;
}


status_t
arch_get_debug_variable(const char* variableName, uint64* value)
{
	return B_OK;
}


ssize_t
arch_debug_gdb_get_registers(char* buffer, size_t bufferSize)
{
	return B_ERROR;
}


status_t
arch_debug_init(kernel_args *args)
{
	return B_OK;
}

void
arch_debug_call_with_fault_handler(cpu_ent* cpu, jmp_buf jumpBuffer,
	void (*function)(void*), void* parameter)
{
	// To be implemented in asm, not here.

	function(parameter);
}


status_t
arch_rtc_init(struct kernel_args *args, struct real_time_data *data)
{
	return B_OK;
}


uint32
arch_rtc_get_hw_time(void)
{
	return 0;
}


void
arch_rtc_set_hw_time(uint32 seconds)
{

}


void
arch_rtc_set_system_time_offset(struct real_time_data *data, bigtime_t offset)
{

}


bigtime_t
arch_rtc_get_system_time_offset(struct real_time_data *data)
{
	return 0;
}


status_t
arch_smp_init(kernel_args *args)
{
	return B_OK;
}


status_t
arch_smp_per_cpu_init(kernel_args *args, int32 cpu)
{
	return B_OK;
}


void
arch_smp_send_broadcast_ici(void)
{

}


void
arch_smp_send_ici(int32 target_cpu)
{

}


status_t
arch_get_system_info(system_info *info, size_t size)
{
	return B_OK;
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	return B_OK;
}


// The software breakpoint instruction (int3).
const uint8 kX86SoftwareBreakpoint[1] = { 0xcc };


void
arch_clear_team_debug_info(struct arch_team_debug_info *info)
{

}


void
arch_destroy_team_debug_info(struct arch_team_debug_info *info)
{

}


void
arch_clear_thread_debug_info(struct arch_thread_debug_info *info)
{

}


void
arch_destroy_thread_debug_info(struct arch_thread_debug_info *info)
{

}


void
arch_update_thread_single_step()
{

}


void
arch_set_debug_cpu_state(const debug_cpu_state *cpuState)
{

}


void
arch_get_debug_cpu_state(debug_cpu_state *cpuState)
{

}


status_t
arch_set_breakpoint(void *address)
{
	return B_OK;
}


status_t
arch_clear_breakpoint(void *address)
{
	return B_OK;
}


status_t
arch_set_watchpoint(void *address, uint32 type, int32 length)
{
	return B_OK;
}


status_t
arch_clear_watchpoint(void *address)
{
	return B_OK;
}


bool
arch_has_breakpoints(struct arch_team_debug_info *info)
{
	return false;
}


void
x86_init_user_debug()
{

}


status_t
_user_read_kernel_image_symbols(image_id id, struct Elf32_Sym* symbolTable,
	int32* _symbolCount, char* stringTable, size_t* _stringTableSize,
	addr_t* _imageDelta)
{
	return B_ERROR;
}
