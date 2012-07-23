/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include "x86_signals.h"

#include <string.h>

#include <KernelExport.h>

#include <commpage.h>
#include <cpu.h>
#include <elf.h>
#include <smp.h>

#include "syscall_numbers.h"


void
x86_initialize_commpage_signal_handler()
{
	// TODO x86_64
}

