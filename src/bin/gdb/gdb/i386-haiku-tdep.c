/* Target-dependent code for Haiku i386.

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
#include "gdbarch.h"
#include "i386-tdep.h"
#include "osabi.h"

//#define TRACE_I386_HAIKU_NAT
#ifdef TRACE_I386_HAIKU_NAT
	#define TRACE(x)	printf x
#else
	#define TRACE(x)	while (false) {}
#endif

static void
i386_haiku_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
	struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

	// Haiku uses ELF.
	i386_elf_init_abi (info, gdbarch);

	// the offset of the PC in the jmp_buf structure (cf. setjmp(), longjmp())
	tdep->jb_pc_offset = 20;

// TODO: Signal support.
//	tdep->sigtramp_p = i386_haiku_sigtramp_p;
//	tdep->sigcontext_addr = i386_haiku_sigcontext_addr;
//	tdep->sc_reg_offset = i386_haiku_sc_reg_offset;
//	tdep->sc_num_regs = ARRAY_SIZE (i386_haiku_sc_reg_offset);

// We don't need this at the moment. The Haiku runtime loader also relocates
// R_386_JMP_SLOT entries. No lazy resolving is done.
//	set_gdbarch_skip_solib_resolver (gdbarch, haiku_skip_solib_resolver);
}


static enum gdb_osabi
i386_haiku_osabi_sniffer (bfd * abfd)
{ 
	char *targetName = bfd_get_target (abfd);

	if (strcmp (targetName, "elf32-i386") == 0)
		return GDB_OSABI_HAIKU;

	return GDB_OSABI_UNKNOWN;
}


void
_initialize_i386_haiku_tdep (void)
{
	gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_elf_flavour,
		i386_haiku_osabi_sniffer);

	gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_HAIKU,
		i386_haiku_init_abi);
}
