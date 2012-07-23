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
