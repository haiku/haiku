/*
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include <arch/debug.h>
#include <debug.h>
#include <elf.h>
#include <kernel.h>
#include <signal.h>

#include "disasm_arch.h"


int
disasm_command(int argc, char **argv)
{
	int argi = 1;

	// get back count
	uint64 backCount = 0;
	if (argi < argc && strcmp(argv[argi], "-b") == 0) {
		if (++argi >= argc) {
			print_debugger_command_usage(argv[0]);
			return 0;
		}

		if (!evaluate_debug_expression(argv[argi++], &backCount, false))
			return 0;
	}

	if (argi + 2 < argc) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	// get PC
	uint64 pc;
	if (argi < argc) {
		if (!evaluate_debug_expression(argv[argi++], &pc, false))
			return 0;
	} else {
		pc = (addr_t)arch_debug_get_interrupt_pc(NULL);
		if (pc == 0) {
			kprintf("Failed to get current PC!\n");
			return 0;
		}
	}

	// get count
	uint64 count = 10;
	if (argi < argc) {
		if (!evaluate_debug_expression(argv[argi++], &count, false))
			return 0;
	}

	// TODO: autoincrement

	// if back count is given, compute base address
	addr_t baseAddress = 0;
	if (backCount > 0) {
		status_t error;
		const char *symbol;
		const char *imageName;
		bool exactMatch;

		if (IS_KERNEL_ADDRESS(pc)) {
			error = elf_debug_lookup_symbol_address(pc, &baseAddress, &symbol,
				&imageName, &exactMatch);
		} else {
			error = elf_debug_lookup_user_symbol_address(
				debug_get_debugged_thread()->team, pc, &baseAddress, &symbol,
				&imageName, &exactMatch);
		}

		if (error != B_OK) {
			baseAddress = 0;
			backCount = 0;
		}
	}

	disasm_arch_dump_insns((addr_t)pc, count, baseAddress, backCount);
	return 0;
}


static status_t
std_ops(int32 op, ...)
{
	if (op == B_MODULE_INIT) {
		status_t err = disasm_arch_init();
		if (err != B_OK)
			return err;

		err = add_debugger_command_etc("dis", disasm_command,
			"Print disassembly at address",
			"[ -b <back count> ] [ <address>  [ <count> ] ]\n"
			"Prints disassembly at address.\n"
			"  <address>        - Address at which to start disassembling\n"
			"                     (defaults to current PC).\n"
			"  <count>          - Number of instructions to disassemble\n"
			"                     starting at <address>.\n"
			"  -b <back count>  - Number of instruction to disassemble before\n"
			"                     <address>.\n", 0);
		if (err != B_OK)
			disasm_arch_fini();
		return err;
	} else if (op == B_MODULE_UNINIT) {
		remove_debugger_command("dis", disasm_command);
		return disasm_arch_fini();
	}

	return B_BAD_VALUE;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/disasm/v1",
		B_KEEP_LOADED,
		&std_ops
	},

	NULL,
	NULL,
	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&sModuleInfo,
	NULL
};
