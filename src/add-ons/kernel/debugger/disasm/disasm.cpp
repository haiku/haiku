/*
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include <debug.h>
#include <signal.h>

#include "disasm_arch.h"

int
disasm_command(int argc, char **argv)
{
	uint64 pc;
	size_t len = 10;

	if (argc < 2) {
		kprintf("Usage: dis addr [count]\n");
		return 1;
	}

	// TODO: use iframe pc by default
	// TODO: autoincrement
	pc = parse_expression(argv[1]);
	if (argc > 2)
		len = (size_t)parse_expression(argv[2]);
	disasm_arch_dump_insns((addr_t)pc, 200, 10);
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
		return add_debugger_command("dis", disasm_command, "output disassembly at address");
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
