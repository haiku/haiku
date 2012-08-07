/* Native-dependent code for Haiku x86_64.

   Copyright 2012 Alex Smith <alex@alex-smith.me.uk>.
   Copyright 2005 Ingo Weinhold <bonefish@cs.tu-berlin.de>.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "haiku-nat.h"
#include "amd64-tdep.h"
#include "i387-tdep.h"
#include "inferior.h"
#include "regcache.h"
#include "target.h"

/* Offset in `struct debug_cpu_state' where MEMBER is stored.  */
#define REG_OFFSET(member) offsetof (struct x86_64_debug_cpu_state, member)

/* At kHaikuAMD64RegOffset[REGNUM] you'll find the offset in `struct
   debug_cpu_state' where the GDB register REGNUM is stored. */
static int kHaikuAMD64RegOffset[] = {
	REG_OFFSET (rax),
	REG_OFFSET (rbx),
	REG_OFFSET (rcx),
	REG_OFFSET (rdx),
	REG_OFFSET (rsi),
	REG_OFFSET (rdi),
	REG_OFFSET (rbp),
	REG_OFFSET (rsp),
	REG_OFFSET (r8),
	REG_OFFSET (r9),
	REG_OFFSET (r10),
	REG_OFFSET (r11),
	REG_OFFSET (r12),
	REG_OFFSET (r13),
	REG_OFFSET (r14),
	REG_OFFSET (r15),
	REG_OFFSET (rip),
	REG_OFFSET (rflags),
	REG_OFFSET (cs),
	REG_OFFSET (ss),
	REG_OFFSET (ds),
	REG_OFFSET (es),
	REG_OFFSET (fs),
	REG_OFFSET (gs)
};


void
haiku_supply_registers(int reg, const debug_cpu_state *cpuState)
{
	if (reg == -1) {
		int i;
		for (i = 0; i < NUM_REGS; i++)
			haiku_supply_registers(i, cpuState);
	} else if (reg < AMD64_ST0_REGNUM) {
		int offset = kHaikuAMD64RegOffset[reg];
		regcache_raw_supply (current_regcache, reg, (char*)cpuState + offset);
	} else {
		amd64_supply_fxsave (current_regcache, -1,
			&cpuState->extended_registers);
	}
}


void
haiku_collect_registers(int reg, debug_cpu_state *cpuState)
{
	if (reg == -1) {
		int i;
		for (i = 0; i < NUM_REGS; i++)
			haiku_collect_registers(i, cpuState);
	} else if (reg < AMD64_ST0_REGNUM) {
		int offset = kHaikuAMD64RegOffset[reg];
		regcache_raw_collect (current_regcache, reg, (char*)cpuState + offset);
	} else {
		amd64_collect_fxsave (current_regcache, -1,
			&cpuState->extended_registers);
	}
}

