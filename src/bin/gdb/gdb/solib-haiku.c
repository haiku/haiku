/* Shared library support for Haiku.

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

#include <string.h>

#include "defs.h"
#include "haiku-nat.h"	// TODO: Needs to be removed. See there!
#include "inferior.h"
#include "objfiles.h"
#include "solib-haiku.h"
#include "solist.h"
#include "symfile.h"
#include "symtab.h"
#include "target.h"
#include "elf/common.h"
#include "elf/internal.h"

//#define TRACE_SOLIB_HAIKU
#ifdef TRACE_SOLIB_HAIKU
	#define TRACE(x)	printf x
#else
	#define TRACE(x)	while (false) {}
#endif


struct lm_info {
	CORE_ADDR	text_address;
	CORE_ADDR	unrelocated_text_address;
	bool		unrelocated_text_address_initialized;
};


static haiku_image_info *
haiku_get_app_image()
{
	haiku_image_info *image;
	int lastID = -1;

	while ((image = haiku_get_next_image_info(lastID)) != NULL) {
		if (image->is_app_image)
			return image;

		lastID = image->id;
	}

	return NULL;
}


static Elf_Internal_Phdr *
read_phdrs(bfd *abfd, int *_count)
{
	long size;
	int count;
	Elf_Internal_Phdr *phdrs;

	// get the phdrs size
	size = bfd_get_elf_phdr_upper_bound(abfd);
	if (size <= 0)
		return NULL;

	// alloc memory
	phdrs = (Elf_Internal_Phdr *)xmalloc(size);
	if (!phdrs)
		return NULL;

	// read the phdrs
	count = bfd_get_elf_phdrs(abfd, phdrs);
	if (count < 0) {
		xfree(phdrs);
		return NULL;
	}

	*_count = count;
	return phdrs;
}


static CORE_ADDR
get_bfd_vma(bfd *abfd)
{
	int count;
	Elf_Internal_Phdr *phdrs;
	int i;
	CORE_ADDR result = 0;

	// get the phdrs array
	phdrs = read_phdrs(abfd, &count);
	if (!phdrs)
		return 0;

	// iterate through phdrs array and find the first one to load
	for (i = 0; i < count; i++) {
		Elf_Internal_Phdr *phdr = phdrs + i;
		if (phdr->p_type != PT_LOAD)
			continue;

		// found the first segment to load
		result = phdr->p_vaddr & ~(B_PAGE_SIZE - 1);
TRACE(("get_bfd_vma(): found first segment: %p\n", (void*)result));
		break;
	}

	xfree(phdrs);

	return result;
}


static CORE_ADDR
get_unrelocated_text_address(struct so_list *so)
{
	if (!so->lm_info->unrelocated_text_address_initialized) {
		so->lm_info->unrelocated_text_address = get_bfd_vma(so->abfd);
		so->lm_info->unrelocated_text_address_initialized = true;
	}

	return so->lm_info->unrelocated_text_address;
}


static void
relocate_main_executable (void)
{
	haiku_image_info *appImageInfo = haiku_get_app_image();

	TRACE(("relocate_main_executable()\n"));

TRACE(("relocate_main_executable(): symfile_objfile: %p\n",
symfile_objfile));
TRACE(("relocate_main_executable(): symfile_objfile->obfd: %p\n",
(symfile_objfile ? symfile_objfile->obfd : NULL)));
TRACE(("relocate_main_executable(): app image: %p\n", appImageInfo));

	// Relocate the executable here.
	if (symfile_objfile && symfile_objfile->obfd && appImageInfo) {
		CORE_ADDR unrelocatedAddress = get_bfd_vma(symfile_objfile->obfd);
		CORE_ADDR displacement = (CORE_ADDR)appImageInfo->text_address
			- unrelocatedAddress;

TRACE(("relocate_main_executable(): image text address: %p, "
"unrelocated address: %p\n", (void*)appImageInfo->text_address,
(void*)unrelocatedAddress));

		if (displacement != 0) {
			struct cleanup *old_chain;
			struct section_offsets *new_offsets;
			int i, changed;

			changed = 0;

			new_offsets = xcalloc (symfile_objfile->num_sections,
				sizeof (struct section_offsets));
			old_chain = make_cleanup (xfree, new_offsets);

			for (i = 0; i < symfile_objfile->num_sections; i++) {
				if (displacement
					!= ANOFFSET (symfile_objfile->section_offsets, i)) {
					changed = 1;
				}
				new_offsets->offsets[i] = displacement;
			}

			if (changed)
				objfile_relocate (symfile_objfile, new_offsets);

			do_cleanups (old_chain);
		}
	}
}


// #pragma mark -

static void
haiku_relocate_section_addresses (struct so_list *so, struct section_table *sec)
{
	CORE_ADDR unrelocatedAddress = get_unrelocated_text_address(so);
	long relocation = so->lm_info->text_address - unrelocatedAddress;

//	TRACE(("haiku_relocate_section_addresses()\n"));

	sec->addr += relocation;
	sec->endaddr += relocation;
}


static void
haiku_free_so (struct so_list *so)
{
	xfree (so->lm_info);
}


static void
haiku_clear_solib (void)
{
}


static void
haiku_solib_create_inferior_hook (void)
{
	relocate_main_executable();
}


static void
haiku_special_symbol_handling (void)
{
}


static struct so_list *
haiku_current_sos (void)
{
	int lastID = -1;
	haiku_image_info *image;
	struct so_list *head = 0;
	struct so_list **link_ptr = &head;

	TRACE(("haiku_current_sos()\n"));

	while ((image = haiku_get_next_image_info(lastID)) != NULL) {
		struct so_list *object
			= (struct so_list *) xmalloc (sizeof (struct so_list));
		struct cleanup *old_chain = make_cleanup (xfree, object);

		lastID = image->id;

		memset (object, 0, sizeof (*object));

		object->lm_info = XMALLOC(struct lm_info);
		make_cleanup (xfree, object->lm_info);
		object->lm_info->text_address = image->text_address;
		object->lm_info->unrelocated_text_address = 0;
		object->lm_info->unrelocated_text_address_initialized = false;

		// Note: I don't know why, but the other solib implementations seem
		// to ignore the executable's shared object. We'll just do the same
		// here.
		if (image->is_app_image) {
			free_so (object);

			// Others don't do that, but it helps a lot to relocate the
			// executable here. Otherwise, when attaching gdb to a running
			// process it would never be done.
			relocate_main_executable();
		} else {
			strncpy (object->so_name, image->path, SO_NAME_MAX_PATH_SIZE);
			object->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
			strncpy (object->so_original_name, image->name,
				SO_NAME_MAX_PATH_SIZE);
			object->so_original_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';

			object->next = 0;
			*link_ptr = object;
			link_ptr = &object->next;
		}

		discard_cleanups (old_chain);
	}

	return head;
}


static int
haiku_open_symbol_file_object (void *from_ttyp)
{
	// Note: I have never seen this function being called. The Sun OS
	// implementation is a no-op.
	haiku_image_info *appImage = haiku_get_app_image();

	TRACE(("haiku_open_symbol_file_object(%p)\n", from_ttyp));

	if (!appImage) {
		TRACE(("haiku_open_symbol_file_object(): No app image!\n"));
		return 0;
	}

	symbol_file_add_main (appImage->path, *(int *)from_ttyp);

	return 1;
}


static int
haiku_in_dynsym_resolve_code (CORE_ADDR pc)
{
	// No dynamic resolving implemented in Haiku yet.
	return 0;
}


// #pragma mark -

static struct target_so_ops haiku_so_ops;

extern initialize_file_ftype _initialize_haiku_solib;

void
_initialize_haiku_solib (void)
{
	haiku_so_ops.relocate_section_addresses = haiku_relocate_section_addresses;
	haiku_so_ops.free_so = haiku_free_so;
	haiku_so_ops.clear_solib = haiku_clear_solib;
	haiku_so_ops.solib_create_inferior_hook = haiku_solib_create_inferior_hook;
	haiku_so_ops.special_symbol_handling = haiku_special_symbol_handling;
	haiku_so_ops.current_sos = haiku_current_sos;
	haiku_so_ops.open_symbol_file_object = haiku_open_symbol_file_object;
	haiku_so_ops.in_dynsym_resolve_code = haiku_in_dynsym_resolve_code;

	/* FIXME: Don't do this here.  *_gdbarch_init() should set so_ops. */
	current_target_so_ops = &haiku_so_ops;
}
