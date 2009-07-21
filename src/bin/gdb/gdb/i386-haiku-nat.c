/* Native-dependent code for Haiku i386.

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
#include "i386-tdep.h"
#include "i387-tdep.h"
#include "inferior.h"
#include "regcache.h"
#include "target.h"

/* Offset in `struct debug_cpu_state' where MEMBER is stored.  */
#define REG_OFFSET(member) offsetof (struct x86_debug_cpu_state, member)

/* At kHaikuI386RegOffset[REGNUM] you'll find the offset in `struct
   debug_cpu_state' where the GDB register REGNUM is stored. */
static int kHaikuI386RegOffset[] = {
	REG_OFFSET (eax),
	REG_OFFSET (ecx),
	REG_OFFSET (edx),
	REG_OFFSET (ebx),
	REG_OFFSET (user_esp),
	REG_OFFSET (ebp),
	REG_OFFSET (esi),
	REG_OFFSET (edi),
	REG_OFFSET (eip),
	REG_OFFSET (eflags),
	REG_OFFSET (cs),
	REG_OFFSET (user_ss),
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
	} else if (reg < I386_ST0_REGNUM) {
		int offset = kHaikuI386RegOffset[reg];
		regcache_raw_supply (current_regcache, reg, (char*)cpuState + offset);
	} else {
		i387_supply_fxsave (current_regcache, -1,
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
	} else if (reg < I386_ST0_REGNUM) {
		int offset = kHaikuI386RegOffset[reg];
		regcache_raw_collect (current_regcache, reg, (char*)cpuState + offset);
	} else {
		i387_collect_fxsave (current_regcache, -1,
			&cpuState->extended_registers);
	}
}

