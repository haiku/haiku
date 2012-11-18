/* Target-dependent code for Haiku x86_64.

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
#include "gdbarch.h"
#include "amd64-tdep.h"
#include "osabi.h"


static void
amd64_haiku_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
	struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

	amd64_init_abi (info, gdbarch);

	// the offset of the PC in the jmp_buf structure (cf. setjmp(), longjmp())
	tdep->jb_pc_offset = 0;
}


static enum gdb_osabi
amd64_haiku_osabi_sniffer (bfd * abfd)
{ 
	char *targetName = bfd_get_target (abfd);

	if (strcmp (targetName, "elf64-x86-64") == 0)
		return GDB_OSABI_HAIKU;

	return GDB_OSABI_UNKNOWN;
}


void
_initialize_amd64_haiku_tdep (void)
{
	gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_elf_flavour,
		amd64_haiku_osabi_sniffer);

	gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x86_64, GDB_OSABI_HAIKU,
		amd64_haiku_init_abi);
}
