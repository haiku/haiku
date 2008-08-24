/*
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include <arch/debug.h>
#include <debug.h>
#include <signal.h>

#include "disasm_arch.h"


int
disasm_command(int argc, char **argv)
{
	if (argc > 3) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	// get PC
	uint64 pc;
	if (argc >= 2) {
		if (!evaluate_debug_expression(argv[1], &pc, false))
			return 0;
	} else {
		pc = (addr_t)arch_debug_get_interrupt_pc();
		if (pc == 0) {
			kprintf("Failed to get current PC!\n");
			return 0;
		}
	}

	// get count
	uint64 count = 10;
	if (argc >= 3) {
		if (!evaluate_debug_expression(argv[2], &count, false))
			return 0;
	}

	// TODO: autoincrement

	disasm_arch_dump_insns((addr_t)pc, count);
	return 0;
}


static status_t
std_ops(int32 op, ...)
{
	status_t err;

	if (op == B_MODULE_INIT) {
		err = disasm_arch_init();
		if (err < 0)
			return err;
		return add_debugger_command_etc("dis", disasm_command,
			"Print disassembly at address",
			"[ <address>  [ <count> ] ]\n"
			"Prints disassembly at address.\n"
			"  <address>   - Address at which to start disassembling\n"
			"                (defaults to current PC).\n"
			"  <count>     - Number of instructions to disassemble.\n", 0);
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
