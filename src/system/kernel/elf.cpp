/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*!	Contains the ELF loader */

#ifndef __x86_64__

#include <elf.h>

#include <OS.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <algorithm>

#include <AutoDeleter.h>
#include <boot/kernel_args.h>
#include <debug.h>
#include <image_defs.h>
#include <kernel.h>
#include <kimage.h>
#include <syscalls.h>
#include <team.h>
#include <thread.h>
#include <runtime_loader.h>
#include <util/AutoLock.h>
#include <util/khash.h>
#include <vfs.h>
#include <vm/vm.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>

#include <arch/cpu.h>
#include <arch/elf.h>
#include <elf_priv.h>
#include <boot/elf.h>

//#define TRACE_ELF
#ifdef TRACE_ELF
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define IMAGE_HASH_SIZE 16

static hash_table *sImagesHash;

static struct elf_image_info *sKernelImage = NULL;
static mutex sImageMutex = MUTEX_INITIALIZER("kimages_lock");
	// guards sImagesHash
static mutex sImageLoadMutex = MUTEX_INITIALIZER("kimages_load_lock");
	// serializes loading/unloading add-ons locking order
	// sImageLoadMutex -> sImageMutex
static bool sInitialized = false;


static struct Elf32_Sym *elf_find_symbol(struct elf_image_info *image,
	const char *name, const elf_version_info *version, bool lookupDefault);


/*! Calculates hash for an image using its ID */
static uint32
image_hash(void *_image, const void *_key, uint32 range)
{
	struct elf_image_info *image = (struct elf_image_info *)_image;
	image_id id = (image_id)_key;

	if (image != NULL)
		return image->id % range;

	return (uint32)id % range;
}


/*!	Compares an image to a given ID */
static int
image_compare(void *_image, const void *_key)
{
	struct elf_image_info *image = (struct elf_image_info *)_image;
	image_id id = (image_id)_key;

	return id - image->id;
}


static void
unregister_elf_image(struct elf_image_info *image)
{
	unregister_image(team_get_kernel_team(), image->id);
	hash_remove(sImagesHash, image);
}


static void
register_elf_image(struct elf_image_info *image)
{
	image_info imageInfo;

	memset(&imageInfo, 0, sizeof(image_info));
	imageInfo.id = image->id;
	imageInfo.type = B_SYSTEM_IMAGE;
	strlcpy(imageInfo.name, image->name, sizeof(imageInfo.name));

	imageInfo.text = (void *)image->text_region.start;
	imageInfo.text_size = image->text_region.size;
	imageInfo.data = (void *)image->data_region.start;
	imageInfo.data_size = image->data_region.size;

	if (image->text_region.id >= 0) {
		// evaluate the API/ABI version symbols

		// Haiku API version
		imageInfo.api_version = 0;
		struct Elf32_Sym* symbol = elf_find_symbol(image,
			B_SHARED_OBJECT_HAIKU_VERSION_VARIABLE_NAME, NULL, true);
		if (symbol != NULL && symbol->st_shndx != SHN_UNDEF
			&& symbol->st_value > 0
			&& ELF32_ST_TYPE(symbol->st_info) == STT_OBJECT
			&& symbol->st_size >= sizeof(uint32)) {
			addr_t symbolAddress = symbol->st_value + image->text_region.delta;
			if (symbolAddress >= image->text_region.start
				&& symbolAddress - image->text_region.start + sizeof(uint32)
					<= image->text_region.size) {
				imageInfo.api_version = *(uint32*)symbolAddress;
			}
		}

		// Haiku ABI
		imageInfo.abi = 0;
		symbol = elf_find_symbol(image,
			B_SHARED_OBJECT_HAIKU_ABI_VARIABLE_NAME, NULL, true);
		if (symbol != NULL && symbol->st_shndx != SHN_UNDEF
			&& symbol->st_value > 0
			&& ELF32_ST_TYPE(symbol->st_info) == STT_OBJECT
			&& symbol->st_size >= sizeof(uint32)) {
			addr_t symbolAddress = symbol->st_value + image->text_region.delta;
			if (symbolAddress >= image->text_region.start
				&& symbolAddress - image->text_region.start + sizeof(uint32)
					<= image->text_region.size) {
				imageInfo.api_version = *(uint32*)symbolAddress;
			}
		}
	} else {
		// in-memory image -- use the current values
		imageInfo.api_version = B_HAIKU_VERSION;
		imageInfo.abi = B_HAIKU_ABI;
	}

	image->id = register_image(team_get_kernel_team(), &imageInfo,
		sizeof(image_info));
	hash_insert(sImagesHash, image);
}


/*!	Note, you must lock the image mutex when you call this function. */
static struct elf_image_info *
find_image_at_address(addr_t address)
{
	struct hash_iterator iterator;
	struct elf_image_info *image;

#if KDEBUG
	if (!debug_debugger_running())
		ASSERT_LOCKED_MUTEX(&sImageMutex);
#endif

	hash_open(sImagesHash, &iterator);

	// get image that may contain the address

	while ((image = (elf_image_info *)hash_next(sImagesHash, &iterator))
			!= NULL) {
		if ((address >= image->text_region.start && address
				<= (image->text_region.start + image->text_region.size))
			|| (address >= image->data_region.start
				&& address
					<= (image->data_region.start + image->data_region.size)))
			break;
	}

	hash_close(sImagesHash, &iterator, false);
	return image;
}


static int
dump_address_info(int argc, char **argv)
{
	const char *symbol, *imageName;
	bool exactMatch;
	addr_t address, baseAddress;

	if (argc < 2) {
		kprintf("usage: ls <address>\n");
		return 0;
	}

	address = strtoul(argv[1], NULL, 16);

	status_t error;

	if (IS_KERNEL_ADDRESS(address)) {
		error = elf_debug_lookup_symbol_address(address, &baseAddress, &symbol,
			&imageName, &exactMatch);
	} else {
		error = elf_debug_lookup_user_symbol_address(
			debug_get_debugged_thread()->team, address, &baseAddress, &symbol,
			&imageName, &exactMatch);
	}

	if (error == B_OK) {
		kprintf("%p = %s + 0x%lx (%s)%s\n", (void*)address, symbol,
			address - baseAddress, imageName, exactMatch ? "" : " (nearest)");
	} else
		kprintf("There is no image loaded at this address!\n");

	return 0;
}


static struct elf_image_info *
find_image(image_id id)
{
	return (elf_image_info *)hash_lookup(sImagesHash, (void *)id);
}


static struct elf_image_info *
find_image_by_vnode(void *vnode)
{
	struct hash_iterator iterator;
	struct elf_image_info *image;

	mutex_lock(&sImageMutex);
	hash_open(sImagesHash, &iterator);

	while ((image = (elf_image_info *)hash_next(sImagesHash, &iterator))
			!= NULL) {
		if (image->vnode == vnode)
			break;
	}

	hash_close(sImagesHash, &iterator, false);
	mutex_unlock(&sImageMutex);

	return image;
}


static struct elf_image_info *
create_image_struct()
{
	struct elf_image_info *image
		= (struct elf_image_info *)malloc(sizeof(struct elf_image_info));
	if (image == NULL)
		return NULL;

	memset(image, 0, sizeof(struct elf_image_info));

	image->text_region.id = -1;
	image->data_region.id = -1;
	image->ref_count = 1;

	return image;
}


static void
delete_elf_image(struct elf_image_info *image)
{
	if (image->text_region.id >= 0)
		delete_area(image->text_region.id);

	if (image->data_region.id >= 0)
		delete_area(image->data_region.id);

	if (image->vnode)
		vfs_put_vnode(image->vnode);

	free(image->versions);
	free(image->debug_symbols);
	free((void*)image->debug_string_table);
	free(image->elf_header);
	free(image->name);
	free(image);
}


static uint32
elf_hash(const char *name)
{
	uint32 hash = 0;
	uint32 temp;

	while (*name) {
		hash = (hash << 4) + (uint8)*name++;
		if ((temp = hash & 0xf0000000) != 0)
			hash ^= temp >> 24;
		hash &= ~temp;
	}
	return hash;
}


static const char *
get_symbol_type_string(struct Elf32_Sym *symbol)
{
	switch (ELF32_ST_TYPE(symbol->st_info)) {
		case STT_FUNC:
			return "func";
		case STT_OBJECT:
			return " obj";
		case STT_FILE:
			return "file";
		default:
			return "----";
	}
}


static const char *
get_symbol_bind_string(struct Elf32_Sym *symbol)
{
	switch (ELF32_ST_BIND(symbol->st_info)) {
		case STB_LOCAL:
			return "loc ";
		case STB_GLOBAL:
			return "glob";
		case STB_WEAK:
			return "weak";
		default:
			return "----";
	}
}


/*!	Searches a symbol (pattern) in all kernel images */
static int
dump_symbol(int argc, char **argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <symbol-name>\n", argv[0]);
		return 0;
	}

	struct elf_image_info *image = NULL;
	struct hash_iterator iterator;
	const char *pattern = argv[1];

	void* symbolAddress = NULL;

	hash_open(sImagesHash, &iterator);
	while ((image = (elf_image_info *)hash_next(sImagesHash, &iterator))
			!= NULL) {
		if (image->num_debug_symbols > 0) {
			// search extended debug symbol table (contains static symbols)
			for (uint32 i = 0; i < image->num_debug_symbols; i++) {
				struct Elf32_Sym *symbol = &image->debug_symbols[i];
				const char *name = image->debug_string_table + symbol->st_name;

				if (symbol->st_value > 0 && strstr(name, pattern) != 0) {
					symbolAddress
						= (void*)(symbol->st_value + image->text_region.delta);
					kprintf("%p %5lu %s:%s\n", symbolAddress, symbol->st_size,
						image->name, name);
				}
			}
		} else {
			// search standard symbol lookup table
			for (uint32 i = 0; i < HASHTABSIZE(image); i++) {
				for (uint32 j = HASHBUCKETS(image)[i]; j != STN_UNDEF;
						j = HASHCHAINS(image)[j]) {
					struct Elf32_Sym *symbol = &image->syms[j];
					const char *name = SYMNAME(image, symbol);

					if (symbol->st_value > 0 && strstr(name, pattern) != 0) {
						symbolAddress = (void*)(symbol->st_value
							+ image->text_region.delta);
						kprintf("%p %5lu %s:%s\n", symbolAddress,
							symbol->st_size, image->name, name);
					}
				}
			}
		}
	}
	hash_close(sImagesHash, &iterator, false);

	if (symbolAddress != NULL)
		set_debug_variable("_", (addr_t)symbolAddress);

	return 0;
}


static int
dump_symbols(int argc, char **argv)
{
	struct elf_image_info *image = NULL;
	struct hash_iterator iterator;
	uint32 i;

	// if the argument looks like a hex number, treat it as such
	if (argc > 1) {
		if (isdigit(argv[1][0])) {
			uint32 num = strtoul(argv[1], NULL, 0);

			if (IS_KERNEL_ADDRESS(num)) {
				// find image at address

				hash_open(sImagesHash, &iterator);
				while ((image = (elf_image_info *)hash_next(sImagesHash,
						&iterator)) != NULL) {
					if (image->text_region.start <= num
						&& image->text_region.start + image->text_region.size
							>= num)
						break;
				}
				hash_close(sImagesHash, &iterator, false);

				if (image == NULL)
					kprintf("No image covers 0x%lx in the kernel!\n", num);
			} else {
				image = (elf_image_info *)hash_lookup(sImagesHash, (void *)num);
				if (image == NULL)
					kprintf("image 0x%lx doesn't exist in the kernel!\n", num);
			}
		} else {
			// look for image by name
			hash_open(sImagesHash, &iterator);
			while ((image = (elf_image_info *)hash_next(sImagesHash,
					&iterator)) != NULL) {
				if (!strcmp(image->name, argv[1]))
					break;
			}
			hash_close(sImagesHash, &iterator, false);

			if (image == NULL)
				kprintf("No image \"%s\" found in kernel!\n", argv[1]);
		}
	} else {
		kprintf("usage: %s image_name/image_id/address_in_image\n", argv[0]);
		return 0;
	}

	if (image == NULL)
		return -1;

	// dump symbols

	kprintf("Symbols of image %ld \"%s\":\nAddress  Type       Size Name\n",
		image->id, image->name);

	if (image->num_debug_symbols > 0) {
		// search extended debug symbol table (contains static symbols)
		for (i = 0; i < image->num_debug_symbols; i++) {
			struct Elf32_Sym *symbol = &image->debug_symbols[i];

			if (symbol->st_value == 0 || symbol->st_size
					>= image->text_region.size + image->data_region.size)
				continue;

			kprintf("%08lx %s/%s %5ld %s\n",
				symbol->st_value + image->text_region.delta,
				get_symbol_type_string(symbol), get_symbol_bind_string(symbol),
				symbol->st_size, image->debug_string_table + symbol->st_name);
		}
	} else {
		int32 j;

		// search standard symbol lookup table
		for (i = 0; i < HASHTABSIZE(image); i++) {
			for (j = HASHBUCKETS(image)[i]; j != STN_UNDEF;
					j = HASHCHAINS(image)[j]) {
				struct Elf32_Sym *symbol = &image->syms[j];

				if (symbol->st_value == 0 || symbol->st_size
						>= image->text_region.size + image->data_region.size)
					continue;

				kprintf("%08lx %s/%s %5ld %s\n",
					symbol->st_value + image->text_region.delta,
					get_symbol_type_string(symbol),
					get_symbol_bind_string(symbol),
					symbol->st_size, SYMNAME(image, symbol));
			}
		}
	}

	return 0;
}


static void
dump_elf_region(struct elf_region *region, const char *name)
{
	kprintf("   %s.id %ld\n", name, region->id);
	kprintf("   %s.start 0x%lx\n", name, region->start);
	kprintf("   %s.size 0x%lx\n", name, region->size);
	kprintf("   %s.delta %ld\n", name, region->delta);
}


static void
dump_image_info(struct elf_image_info *image)
{
	kprintf("elf_image_info at %p:\n", image);
	kprintf(" next %p\n", image->next);
	kprintf(" id %ld\n", image->id);
	dump_elf_region(&image->text_region, "text");
	dump_elf_region(&image->data_region, "data");
	kprintf(" dynamic_section 0x%lx\n", image->dynamic_section);
	kprintf(" needed %p\n", image->needed);
	kprintf(" symhash %p\n", image->symhash);
	kprintf(" syms %p\n", image->syms);
	kprintf(" strtab %p\n", image->strtab);
	kprintf(" rel %p\n", image->rel);
	kprintf(" rel_len 0x%x\n", image->rel_len);
	kprintf(" rela %p\n", image->rela);
	kprintf(" rela_len 0x%x\n", image->rela_len);
	kprintf(" pltrel %p\n", image->pltrel);
	kprintf(" pltrel_len 0x%x\n", image->pltrel_len);

	kprintf(" debug_symbols %p (%ld)\n",
		image->debug_symbols, image->num_debug_symbols);
}


static int
dump_image(int argc, char **argv)
{
	struct hash_iterator iterator;
	struct elf_image_info *image;

	// if the argument looks like a hex number, treat it as such
	if (argc > 1) {
		uint32 num = strtoul(argv[1], NULL, 0);

		if (IS_KERNEL_ADDRESS(num)) {
			// semi-hack
			dump_image_info((struct elf_image_info *)num);
		} else {
			image = (elf_image_info *)hash_lookup(sImagesHash, (void *)num);
			if (image == NULL)
				kprintf("image 0x%lx doesn't exist in the kernel!\n", num);
			else
				dump_image_info(image);
		}
		return 0;
	}

	kprintf("loaded kernel images:\n");

	hash_open(sImagesHash, &iterator);

	while ((image = (elf_image_info *)hash_next(sImagesHash, &iterator))
			!= NULL) {
		kprintf("%p (%ld) %s\n", image, image->id, image->name);
	}

	hash_close(sImagesHash, &iterator, false);
	return 0;
}


// Currently unused
#if 0
static
void dump_symbol(struct elf_image_info *image, struct Elf32_Sym *sym)
{

	kprintf("symbol at %p, in image %p\n", sym, image);

	kprintf(" name index %d, '%s'\n", sym->st_name, SYMNAME(image, sym));
	kprintf(" st_value 0x%x\n", sym->st_value);
	kprintf(" st_size %d\n", sym->st_size);
	kprintf(" st_info 0x%x\n", sym->st_info);
	kprintf(" st_other 0x%x\n", sym->st_other);
	kprintf(" st_shndx %d\n", sym->st_shndx);
}
#endif


static struct Elf32_Sym *
elf_find_symbol(struct elf_image_info *image, const char *name,
	const elf_version_info *lookupVersion, bool lookupDefault)
{
	if (image->dynamic_section == 0 || HASHTABSIZE(image) == 0)
		return NULL;

	Elf32_Sym* versionedSymbol = NULL;
	uint32 versionedSymbolCount = 0;

	uint32 hash = elf_hash(name) % HASHTABSIZE(image);
	for (uint32 i = HASHBUCKETS(image)[hash]; i != STN_UNDEF;
			i = HASHCHAINS(image)[i]) {
		Elf32_Sym* symbol = &image->syms[i];

		// consider only symbols with the right name and binding
		if (symbol->st_shndx == SHN_UNDEF
			|| ((ELF32_ST_BIND(symbol->st_info) != STB_GLOBAL)
				&& (ELF32_ST_BIND(symbol->st_info) != STB_WEAK))
			|| strcmp(SYMNAME(image, symbol), name) != 0) {
			continue;
		}

		// check the version

		// Handle the simple cases -- the image doesn't have version
		// information -- first.
		if (image->symbol_versions == NULL) {
			if (lookupVersion == NULL) {
				// No specific symbol version was requested either, so the
				// symbol is just fine.
				return symbol;
			}

			// A specific version is requested. Since the only possible
			// dependency is the kernel itself, the add-on was obviously linked
			// against a newer kernel.
			dprintf("Kernel add-on requires version support, but the kernel "
				"is too old.\n");
			return NULL;
		}

		// The image has version information. Let's see what we've got.
		uint32 versionID = image->symbol_versions[i];
		uint32 versionIndex = VER_NDX(versionID);
		elf_version_info& version = image->versions[versionIndex];

		// skip local versions
		if (versionIndex == VER_NDX_LOCAL)
			continue;

		if (lookupVersion != NULL) {
			// a specific version is requested

			// compare the versions
			if (version.hash == lookupVersion->hash
				&& strcmp(version.name, lookupVersion->name) == 0) {
				// versions match
				return symbol;
			}

			// The versions don't match. We're still fine with the
			// base version, if it is public and we're not looking for
			// the default version.
			if ((versionID & VER_NDX_FLAG_HIDDEN) == 0
				&& versionIndex == VER_NDX_GLOBAL
				&& !lookupDefault) {
				// TODO: Revise the default version case! That's how
				// FreeBSD implements it, but glibc doesn't handle it
				// specially.
				return symbol;
			}
		} else {
			// No specific version requested, but the image has version
			// information. This can happen in either of these cases:
			//
			// * The dependent object was linked against an older version
			//   of the now versioned dependency.
			// * The symbol is looked up via find_image_symbol() or dlsym().
			//
			// In the first case we return the base version of the symbol
			// (VER_NDX_GLOBAL or VER_NDX_INITIAL), or, if that doesn't
			// exist, the unique, non-hidden versioned symbol.
			//
			// In the second case we want to return the public default
			// version of the symbol. The handling is pretty similar to the
			// first case, with the exception that we treat VER_NDX_INITIAL
			// as regular version.

			// VER_NDX_GLOBAL is always good, VER_NDX_INITIAL is fine, if
			// we don't look for the default version.
			if (versionIndex == VER_NDX_GLOBAL
				|| (!lookupDefault && versionIndex == VER_NDX_INITIAL)) {
				return symbol;
			}

			// If not hidden, remember the version -- we'll return it, if
			// it is the only one.
			if ((versionID & VER_NDX_FLAG_HIDDEN) == 0) {
				versionedSymbolCount++;
				versionedSymbol = symbol;
			}
		}
	}

	return versionedSymbolCount == 1 ? versionedSymbol : NULL;
}


static status_t
elf_parse_dynamic_section(struct elf_image_info *image)
{
	struct Elf32_Dyn *d;
	int32 neededOffset = -1;

	TRACE(("top of elf_parse_dynamic_section\n"));

	image->symhash = 0;
	image->syms = 0;
	image->strtab = 0;

	d = (struct Elf32_Dyn *)image->dynamic_section;
	if (!d)
		return B_ERROR;

	for (int32 i = 0; d[i].d_tag != DT_NULL; i++) {
		switch (d[i].d_tag) {
			case DT_NEEDED:
				neededOffset = d[i].d_un.d_ptr + image->text_region.delta;
				break;
			case DT_HASH:
				image->symhash = (uint32 *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_STRTAB:
				image->strtab = (char *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_SYMTAB:
				image->syms = (struct Elf32_Sym *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_REL:
				image->rel = (struct Elf32_Rel *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_RELSZ:
				image->rel_len = d[i].d_un.d_val;
				break;
			case DT_RELA:
				image->rela = (struct Elf32_Rela *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_RELASZ:
				image->rela_len = d[i].d_un.d_val;
				break;
			case DT_JMPREL:
				image->pltrel = (struct Elf32_Rel *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_PLTRELSZ:
				image->pltrel_len = d[i].d_un.d_val;
				break;
			case DT_PLTREL:
				image->pltrel_type = d[i].d_un.d_val;
				break;
			case DT_VERSYM:
				image->symbol_versions = (Elf32_Versym*)
					(d[i].d_un.d_ptr + image->text_region.delta);
				break;
			case DT_VERDEF:
				image->version_definitions = (Elf32_Verdef*)
					(d[i].d_un.d_ptr + image->text_region.delta);
				break;
			case DT_VERDEFNUM:
				image->num_version_definitions = d[i].d_un.d_val;
				break;
			case DT_VERNEED:
				image->needed_versions = (Elf32_Verneed*)
					(d[i].d_un.d_ptr + image->text_region.delta);
				break;
			case DT_VERNEEDNUM:
				image->num_needed_versions = d[i].d_un.d_val;
				break;
			case DT_SYMBOLIC:
				image->symbolic = true;
				break;
			case DT_FLAGS:
			{
				uint32 flags = d[i].d_un.d_val;
				if ((flags & DF_SYMBOLIC) != 0)
					image->symbolic = true;
				break;
			}

			default:
				continue;
		}
	}

	// lets make sure we found all the required sections
	if (!image->symhash || !image->syms || !image->strtab)
		return B_ERROR;

	TRACE(("needed_offset = %ld\n", neededOffset));

	if (neededOffset >= 0)
		image->needed = STRING(image, neededOffset);

	return B_OK;
}


static status_t
assert_defined_image_version(elf_image_info* dependentImage,
	elf_image_info* image, const elf_version_info& neededVersion, bool weak)
{
	// If the image doesn't have version definitions, we print a warning and
	// succeed. Weird, but that's how glibc does it. Not unlikely we'll fail
	// later when resolving versioned symbols.
	if (image->version_definitions == NULL) {
		dprintf("%s: No version information available (required by %s)\n",
			image->name, dependentImage->name);
		return B_OK;
	}

	// iterate through the defined versions to find the given one
	Elf32_Verdef* definition = image->version_definitions;
	for (uint32 i = 0; i < image->num_version_definitions; i++) {
		uint32 versionIndex = VER_NDX(definition->vd_ndx);
		elf_version_info& info = image->versions[versionIndex];

		if (neededVersion.hash == info.hash
			&& strcmp(neededVersion.name, info.name) == 0) {
			return B_OK;
		}

		definition = (Elf32_Verdef*)
			((uint8*)definition + definition->vd_next);
	}

	// version not found -- fail, if not weak
	if (!weak) {
		dprintf("%s: version \"%s\" not found (required by %s)\n", image->name,
			neededVersion.name, dependentImage->name);
		return B_MISSING_SYMBOL;
	}

	return B_OK;
}


static status_t
init_image_version_infos(elf_image_info* image)
{
	// First find out how many version infos we need -- i.e. get the greatest
	// version index from the defined and needed versions (they use the same
	// index namespace).
	uint32 maxIndex = 0;

	if (image->version_definitions != NULL) {
		Elf32_Verdef* definition = image->version_definitions;
		for (uint32 i = 0; i < image->num_version_definitions; i++) {
			if (definition->vd_version != 1) {
				dprintf("Unsupported version definition revision: %u\n",
					definition->vd_version);
				return B_BAD_VALUE;
			}

			uint32 versionIndex = VER_NDX(definition->vd_ndx);
			if (versionIndex > maxIndex)
				maxIndex = versionIndex;

			definition = (Elf32_Verdef*)
				((uint8*)definition	+ definition->vd_next);
		}
	}

	if (image->needed_versions != NULL) {
		Elf32_Verneed* needed = image->needed_versions;
		for (uint32 i = 0; i < image->num_needed_versions; i++) {
			if (needed->vn_version != 1) {
				dprintf("Unsupported version needed revision: %u\n",
					needed->vn_version);
				return B_BAD_VALUE;
			}

			Elf32_Vernaux* vernaux
				= (Elf32_Vernaux*)((uint8*)needed + needed->vn_aux);
			for (uint32 k = 0; k < needed->vn_cnt; k++) {
				uint32 versionIndex = VER_NDX(vernaux->vna_other);
				if (versionIndex > maxIndex)
					maxIndex = versionIndex;

				vernaux = (Elf32_Vernaux*)((uint8*)vernaux + vernaux->vna_next);
			}

			needed = (Elf32_Verneed*)((uint8*)needed + needed->vn_next);
		}
	}

	if (maxIndex == 0)
		return B_OK;

	// allocate the version infos
	image->versions
		= (elf_version_info*)malloc(sizeof(elf_version_info) * (maxIndex + 1));
	if (image->versions == NULL) {
		dprintf("Memory shortage in init_image_version_infos()\n");
		return B_NO_MEMORY;
	}
	image->num_versions = maxIndex + 1;

	// init the version infos

	// version definitions
	if (image->version_definitions != NULL) {
		Elf32_Verdef* definition = image->version_definitions;
		for (uint32 i = 0; i < image->num_version_definitions; i++) {
			if (definition->vd_cnt > 0
				&& (definition->vd_flags & VER_FLG_BASE) == 0) {
				Elf32_Verdaux* verdaux
					= (Elf32_Verdaux*)((uint8*)definition + definition->vd_aux);

				uint32 versionIndex = VER_NDX(definition->vd_ndx);
				elf_version_info& info = image->versions[versionIndex];
				info.hash = definition->vd_hash;
				info.name = STRING(image, verdaux->vda_name);
				info.file_name = NULL;
			}

			definition = (Elf32_Verdef*)
				((uint8*)definition + definition->vd_next);
		}
	}

	// needed versions
	if (image->needed_versions != NULL) {
		Elf32_Verneed* needed = image->needed_versions;
		for (uint32 i = 0; i < image->num_needed_versions; i++) {
			const char* fileName = STRING(image, needed->vn_file);

			Elf32_Vernaux* vernaux
				= (Elf32_Vernaux*)((uint8*)needed + needed->vn_aux);
			for (uint32 k = 0; k < needed->vn_cnt; k++) {
				uint32 versionIndex = VER_NDX(vernaux->vna_other);
				elf_version_info& info = image->versions[versionIndex];
				info.hash = vernaux->vna_hash;
				info.name = STRING(image, vernaux->vna_name);
				info.file_name = fileName;

				vernaux = (Elf32_Vernaux*)((uint8*)vernaux + vernaux->vna_next);
			}

			needed = (Elf32_Verneed*)((uint8*)needed + needed->vn_next);
		}
	}

	return B_OK;
}


static status_t
check_needed_image_versions(elf_image_info* image)
{
	if (image->needed_versions == NULL)
		return B_OK;

	Elf32_Verneed* needed = image->needed_versions;
	for (uint32 i = 0; i < image->num_needed_versions; i++) {
		elf_image_info* dependency = sKernelImage;

		Elf32_Vernaux* vernaux
			= (Elf32_Vernaux*)((uint8*)needed + needed->vn_aux);
		for (uint32 k = 0; k < needed->vn_cnt; k++) {
			uint32 versionIndex = VER_NDX(vernaux->vna_other);
			elf_version_info& info = image->versions[versionIndex];

			status_t error = assert_defined_image_version(image, dependency,
				info, (vernaux->vna_flags & VER_FLG_WEAK) != 0);
			if (error != B_OK)
				return error;

			vernaux = (Elf32_Vernaux*)((uint8*)vernaux + vernaux->vna_next);
		}

		needed = (Elf32_Verneed*)((uint8*)needed + needed->vn_next);
	}

	return B_OK;
}


/*!	Resolves the \a symbol by linking against \a sharedImage if necessary.
	Returns the resolved symbol's address in \a _symbolAddress.
*/
status_t
elf_resolve_symbol(struct elf_image_info *image, struct Elf32_Sym *symbol,
	struct elf_image_info *sharedImage, addr_t *_symbolAddress)
{
	// Local symbols references are always resolved to the given symbol.
	if (ELF32_ST_BIND(symbol->st_info) == STB_LOCAL) {
		*_symbolAddress = symbol->st_value + image->text_region.delta;
		return B_OK;
	}

	// Non-local symbols we try to resolve to the kernel image first. Unless
	// the image is linked symbolically, then vice versa.
	elf_image_info* firstImage = sharedImage;
	elf_image_info* secondImage = image;
	if (image->symbolic)
		std::swap(firstImage, secondImage);

	const char *symbolName = SYMNAME(image, symbol);

	// get the version info
	const elf_version_info* versionInfo = NULL;
	if (image->symbol_versions != NULL) {
		uint32 index = symbol - image->syms;
		uint32 versionIndex = VER_NDX(image->symbol_versions[index]);
		if (versionIndex >= VER_NDX_INITIAL)
			versionInfo = image->versions + versionIndex;
	}

	// find the symbol
	elf_image_info* foundImage = firstImage;
	struct Elf32_Sym* foundSymbol = elf_find_symbol(firstImage, symbolName,
		versionInfo, false);
	if (foundSymbol == NULL
		|| ELF32_ST_BIND(foundSymbol->st_info) == STB_WEAK) {
		// Not found or found a weak definition -- try to resolve in the other
		// image.
		Elf32_Sym* secondSymbol = elf_find_symbol(secondImage, symbolName,
			versionInfo, false);
		// If we found a symbol -- take it in case we didn't have a symbol
		// before or the new symbol is not weak.
		if (secondSymbol != NULL
			&& (foundSymbol == NULL
				|| ELF32_ST_BIND(secondSymbol->st_info) != STB_WEAK)) {
			foundImage = secondImage;
			foundSymbol = secondSymbol;
		}
	}

	if (foundSymbol == NULL) {
		// Weak undefined symbols get a value of 0, if unresolved.
		if (ELF32_ST_BIND(symbol->st_info) == STB_WEAK) {
			*_symbolAddress = 0;
			return B_OK;
		}

		dprintf("\"%s\": could not resolve symbol '%s'\n", image->name,
			symbolName);
		return B_MISSING_SYMBOL;
	}

	// make sure they're the same type
	if (ELF32_ST_TYPE(symbol->st_info) != ELF32_ST_TYPE(foundSymbol->st_info)) {
		dprintf("elf_resolve_symbol: found symbol '%s' in image '%s' "
			"(requested by image '%s') but wrong type (%d vs. %d)\n",
			symbolName, foundImage->name, image->name,
			ELF32_ST_TYPE(foundSymbol->st_info),
			ELF32_ST_TYPE(symbol->st_info));
		return B_MISSING_SYMBOL;
	}

	*_symbolAddress = foundSymbol->st_value + foundImage->text_region.delta;
	return B_OK;
}


/*! Until we have shared library support, just this links against the kernel */
static int
elf_relocate(struct elf_image_info *image)
{
	int status = B_NO_ERROR;

	TRACE(("elf_relocate(%p (\"%s\"))\n", image, image->name));

	// deal with the rels first
	if (image->rel) {
		TRACE(("total %i relocs\n",
			image->rel_len / (int)sizeof(struct Elf32_Rel)));

		status = arch_elf_relocate_rel(image, sKernelImage, image->rel,
			image->rel_len);
		if (status < B_OK)
			return status;
	}

	if (image->pltrel) {
		TRACE(("total %i plt-relocs\n",
			image->pltrel_len / (int)sizeof(struct Elf32_Rel)));

		if (image->pltrel_type == DT_REL) {
			status = arch_elf_relocate_rel(image, sKernelImage, image->pltrel,
				image->pltrel_len);
		} else {
			status = arch_elf_relocate_rela(image, sKernelImage,
				(struct Elf32_Rela *)image->pltrel, image->pltrel_len);
		}
		if (status < B_OK)
			return status;
	}

	if (image->rela) {
		status = arch_elf_relocate_rela(image, sKernelImage, image->rela,
			image->rela_len);
		if (status < B_OK)
			return status;
	}

	return status;
}


static int
verify_eheader(struct Elf32_Ehdr *elfHeader)
{
	if (memcmp(elfHeader->e_ident, ELF_MAGIC, 4) != 0)
		return B_NOT_AN_EXECUTABLE;

	if (elfHeader->e_ident[4] != ELFCLASS32)
		return B_NOT_AN_EXECUTABLE;

	if (elfHeader->e_phoff == 0)
		return B_NOT_AN_EXECUTABLE;

	if (elfHeader->e_phentsize < sizeof(struct Elf32_Phdr))
		return B_NOT_AN_EXECUTABLE;

	return 0;
}


static void
unload_elf_image(struct elf_image_info *image)
{
	if (atomic_add(&image->ref_count, -1) > 1)
		return;

	TRACE(("unload image %ld, %s\n", image->id, image->name));

	unregister_elf_image(image);
	delete_elf_image(image);
}


static status_t
load_elf_symbol_table(int fd, struct elf_image_info *image)
{
	struct Elf32_Ehdr *elfHeader = image->elf_header;
	struct Elf32_Sym *symbolTable = NULL;
	struct Elf32_Shdr *stringHeader = NULL;
	uint32 numSymbols = 0;
	char *stringTable;
	status_t status;
	ssize_t length;
	int32 i;

	// get section headers

	ssize_t size = elfHeader->e_shnum * elfHeader->e_shentsize;
	struct Elf32_Shdr *sectionHeaders = (struct Elf32_Shdr *)malloc(size);
	if (sectionHeaders == NULL) {
		dprintf("error allocating space for section headers\n");
		return B_NO_MEMORY;
	}

	length = read_pos(fd, elfHeader->e_shoff, sectionHeaders, size);
	if (length < size) {
		TRACE(("error reading in program headers\n"));
		status = B_ERROR;
		goto error1;
	}

	// find symbol table in section headers

	for (i = 0; i < elfHeader->e_shnum; i++) {
		if (sectionHeaders[i].sh_type == SHT_SYMTAB) {
			stringHeader = &sectionHeaders[sectionHeaders[i].sh_link];

			if (stringHeader->sh_type != SHT_STRTAB) {
				TRACE(("doesn't link to string table\n"));
				status = B_BAD_DATA;
				goto error1;
			}

			// read in symbol table
			symbolTable
				= (struct Elf32_Sym *)malloc(size = sectionHeaders[i].sh_size);
			if (symbolTable == NULL) {
				status = B_NO_MEMORY;
				goto error1;
			}

			length
				= read_pos(fd, sectionHeaders[i].sh_offset, symbolTable, size);
			if (length < size) {
				TRACE(("error reading in symbol table\n"));
				status = B_ERROR;
				goto error2;
			}

			numSymbols = size / sizeof(struct Elf32_Sym);
			break;
		}
	}

	if (symbolTable == NULL) {
		TRACE(("no symbol table\n"));
		status = B_BAD_VALUE;
		goto error1;
	}

	// read in string table

	stringTable = (char *)malloc(size = stringHeader->sh_size);
	if (stringTable == NULL) {
		status = B_NO_MEMORY;
		goto error2;
	}

	length = read_pos(fd, stringHeader->sh_offset, stringTable, size);
	if (length < size) {
		TRACE(("error reading in string table\n"));
		status = B_ERROR;
		goto error3;
	}

	TRACE(("loaded debug %ld symbols\n", numSymbols));

	// insert tables into image
	image->debug_symbols = symbolTable;
	image->num_debug_symbols = numSymbols;
	image->debug_string_table = stringTable;

	free(sectionHeaders);
	return B_OK;

error3:
	free(stringTable);
error2:
	free(symbolTable);
error1:
	free(sectionHeaders);

	return status;
}


static status_t
insert_preloaded_image(struct preloaded_elf32_image *preloadedImage, bool kernel)
{
	status_t status;

	status = verify_eheader(&preloadedImage->elf_header);
	if (status != B_OK)
		return status;

	elf_image_info *image = create_image_struct();
	if (image == NULL)
		return B_NO_MEMORY;

	image->name = strdup(preloadedImage->name);
	image->dynamic_section = preloadedImage->dynamic_section.start;

	image->text_region.id = preloadedImage->text_region.id;
	image->text_region.start = preloadedImage->text_region.start;
	image->text_region.size = preloadedImage->text_region.size;
	image->text_region.delta = preloadedImage->text_region.delta;
	image->data_region.id = preloadedImage->data_region.id;
	image->data_region.start = preloadedImage->data_region.start;
	image->data_region.size = preloadedImage->data_region.size;
	image->data_region.delta = preloadedImage->data_region.delta;

	status = elf_parse_dynamic_section(image);
	if (status != B_OK)
		goto error1;

	status = init_image_version_infos(image);
	if (status != B_OK)
		goto error1;

	if (!kernel) {
		status = check_needed_image_versions(image);
		if (status != B_OK)
			goto error1;

		status = elf_relocate(image);
		if (status != B_OK)
			goto error1;
	} else
		sKernelImage = image;

	// copy debug symbols to the kernel heap
	if (preloadedImage->debug_symbols != NULL) {
		int32 debugSymbolsSize = sizeof(Elf32_Sym)
			* preloadedImage->num_debug_symbols;
		image->debug_symbols = (Elf32_Sym*)malloc(debugSymbolsSize);
		if (image->debug_symbols != NULL) {
			memcpy(image->debug_symbols, preloadedImage->debug_symbols,
				debugSymbolsSize);
		}
	}
	image->num_debug_symbols = preloadedImage->num_debug_symbols;

	// copy debug string table to the kernel heap
	if (preloadedImage->debug_string_table != NULL) {
		image->debug_string_table = (char*)malloc(
			preloadedImage->debug_string_table_size);
		if (image->debug_string_table != NULL) {
			memcpy((void*)image->debug_string_table,
				preloadedImage->debug_string_table,
				preloadedImage->debug_string_table_size);
		}
	}

	register_elf_image(image);
	preloadedImage->id = image->id;
		// modules_init() uses this information to get the preloaded images

	// we now no longer need to write to the text area anymore
	set_area_protection(image->text_region.id,
		B_KERNEL_READ_AREA | B_KERNEL_EXECUTE_AREA);

	return B_OK;

error1:
	delete_elf_image(image);

	preloadedImage->id = -1;

	return status;
}


//	#pragma mark - userland symbol lookup


class UserSymbolLookup {
public:
	static UserSymbolLookup& Default()
	{
		return sLookup;
	}

	status_t Init(Team* team)
	{
		// find the runtime loader debug area
		VMArea* area;
		for (VMAddressSpace::AreaIterator it
					= team->address_space->GetAreaIterator();
				(area = it.Next()) != NULL;) {
			if (strcmp(area->name, RUNTIME_LOADER_DEBUG_AREA_NAME) == 0)
				break;
		}

		if (area == NULL)
			return B_ERROR;

		// copy the runtime loader data structure
		if (!_Read((runtime_loader_debug_area*)area->Base(), fDebugArea))
			return B_BAD_ADDRESS;

		return B_OK;
	}

	status_t LookupSymbolAddress(addr_t address, addr_t *_baseAddress,
		const char **_symbolName, const char **_imageName, bool *_exactMatch)
	{
		// Note, that this function doesn't find all symbols that we would like
		// to find. E.g. static functions do not appear in the symbol table
		// as function symbols, but as sections without name and size. The
		// .symtab section together with the .strtab section, which apparently
		// differ from the tables referred to by the .dynamic section, also
		// contain proper names and sizes for those symbols. Therefore, to get
		// completely satisfying results, we would need to read those tables
		// from the shared object.

		// get the image for the address
		image_t image;
		status_t error = _FindImageAtAddress(address, image);
		if (error != B_OK)
			return error;

		strlcpy(fImageName, image.name, sizeof(fImageName));

		// symbol hash table size
		uint32 hashTabSize;
		if (!_Read(image.symhash, hashTabSize))
			return B_BAD_ADDRESS;

		// remote pointers to hash buckets and chains
		const uint32* hashBuckets = image.symhash + 2;
		const uint32* hashChains = image.symhash + 2 + hashTabSize;

		const elf_region_t& textRegion = image.regions[0];

		// search the image for the symbol
		Elf32_Sym symbolFound;
		addr_t deltaFound = INT_MAX;
		bool exactMatch = false;

		// to get rid of the erroneous "uninitialized" warnings
		symbolFound.st_name = 0;
		symbolFound.st_value = 0;

		for (uint32 i = 0; i < hashTabSize; i++) {
			uint32 bucket;
			if (!_Read(&hashBuckets[i], bucket))
				return B_BAD_ADDRESS;

			for (uint32 j = bucket; j != STN_UNDEF;
					_Read(&hashChains[j], j) ? 0 : j = STN_UNDEF) {

				Elf32_Sym symbol;
				if (!_Read(image.syms + j, symbol))
					continue;

				// The symbol table contains not only symbols referring to
				// functions and data symbols within the shared object, but also
				// referenced symbols of other shared objects, as well as
				// section and file references. We ignore everything but
				// function and data symbols that have an st_value != 0 (0
				// seems to be an indication for a symbol defined elsewhere
				// -- couldn't verify that in the specs though).
				if ((ELF32_ST_TYPE(symbol.st_info) != STT_FUNC
						&& ELF32_ST_TYPE(symbol.st_info) != STT_OBJECT)
					|| symbol.st_value == 0
					|| symbol.st_value + symbol.st_size + textRegion.delta
						> textRegion.vmstart + textRegion.size) {
					continue;
				}

				// skip symbols starting after the given address
				addr_t symbolAddress = symbol.st_value + textRegion.delta;
				if (symbolAddress > address)
					continue;
				addr_t symbolDelta = address - symbolAddress;

				if (symbolDelta < deltaFound) {
					deltaFound = symbolDelta;
					symbolFound = symbol;

					if (symbolDelta >= 0 && symbolDelta < symbol.st_size) {
						// exact match
						exactMatch = true;
						break;
					}
				}
			}
		}

		if (_imageName)
			*_imageName = fImageName;

		if (_symbolName) {
			*_symbolName = NULL;

			if (deltaFound < INT_MAX) {
				if (_ReadString(image, symbolFound.st_name, fSymbolName,
						sizeof(fSymbolName))) {
					*_symbolName = fSymbolName;
				} else {
					// we can't get its name, so forget the symbol
					deltaFound = INT_MAX;
				}
			}
		}

		if (_baseAddress) {
			if (deltaFound < INT_MAX)
				*_baseAddress = symbolFound.st_value + textRegion.delta;
			else
				*_baseAddress = textRegion.vmstart;
		}

		if (_exactMatch)
			*_exactMatch = exactMatch;

		return B_OK;
	}

	status_t _FindImageAtAddress(addr_t address, image_t& image)
	{
		image_queue_t imageQueue;
		if (!_Read(fDebugArea.loaded_images, imageQueue))
			return B_BAD_ADDRESS;

		image_t* imageAddress = imageQueue.head;
		while (imageAddress != NULL) {
			if (!_Read(imageAddress, image))
				return B_BAD_ADDRESS;

			if (image.regions[0].vmstart <= address
				&& address < image.regions[0].vmstart + image.regions[0].size) {
				return B_OK;
			}

			imageAddress = image.next;
		}

		return B_ENTRY_NOT_FOUND;
	}

	bool _ReadString(const image_t& image, uint32 offset, char* buffer,
		size_t bufferSize)
	{
		const char* address = image.strtab + offset;

		if (!IS_USER_ADDRESS(address))
			return false;

		if (debug_debugger_running()) {
			return debug_strlcpy(B_CURRENT_TEAM, buffer, address, bufferSize)
				>= 0;
		}
		return user_strlcpy(buffer, address, bufferSize) >= 0;
	}

	template<typename T> bool _Read(const T* address, T& data);
		// gcc 2.95.3 doesn't like it defined in-place

private:
	runtime_loader_debug_area	fDebugArea;
	char						fImageName[B_OS_NAME_LENGTH];
	char						fSymbolName[256];
	static UserSymbolLookup		sLookup;
};


template<typename T>
bool
UserSymbolLookup::_Read(const T* address, T& data)
{
	if (!IS_USER_ADDRESS(address))
		return false;

	if (debug_debugger_running())
		return debug_memcpy(B_CURRENT_TEAM, &data, address, sizeof(T)) == B_OK;
	return user_memcpy(&data, address, sizeof(T)) == B_OK;
}


UserSymbolLookup UserSymbolLookup::sLookup;
	// doesn't need construction, but has an Init() method


//	#pragma mark - public kernel API


status_t
get_image_symbol(image_id id, const char *name, int32 symbolClass,
	void **_symbol)
{
	struct elf_image_info *image;
	struct Elf32_Sym *symbol;
	status_t status = B_OK;

	TRACE(("get_image_symbol(%s)\n", name));

	mutex_lock(&sImageMutex);

	image = find_image(id);
	if (image == NULL) {
		status = B_BAD_IMAGE_ID;
		goto done;
	}

	symbol = elf_find_symbol(image, name, NULL, true);
	if (symbol == NULL || symbol->st_shndx == SHN_UNDEF) {
		status = B_ENTRY_NOT_FOUND;
		goto done;
	}

	// TODO: support the "symbolClass" parameter!

	TRACE(("found: %lx (%lx + %lx)\n",
		symbol->st_value + image->text_region.delta,
		symbol->st_value, image->text_region.delta));

	*_symbol = (void *)(symbol->st_value + image->text_region.delta);

done:
	mutex_unlock(&sImageMutex);
	return status;
}


//	#pragma mark - kernel private API


/*!	Looks up a symbol by address in all images loaded in kernel space.
	Note, if you need to call this function outside a debugger, make
	sure you fix locking and the way it returns its information, first!
*/
status_t
elf_debug_lookup_symbol_address(addr_t address, addr_t *_baseAddress,
	const char **_symbolName, const char **_imageName, bool *_exactMatch)
{
	struct elf_image_info *image;
	struct Elf32_Sym *symbolFound = NULL;
	const char *symbolName = NULL;
	addr_t deltaFound = INT_MAX;
	bool exactMatch = false;
	status_t status;

	TRACE(("looking up %p\n", (void *)address));

	if (!sInitialized)
		return B_ERROR;

	//mutex_lock(&sImageMutex);

	image = find_image_at_address(address);
		// get image that may contain the address

	if (image != NULL) {
		addr_t symbolDelta;
		uint32 i;
		int32 j;

		TRACE((" image %p, base = %p, size = %p\n", image,
			(void *)image->text_region.start, (void *)image->text_region.size));

		if (image->debug_symbols != NULL) {
			// search extended debug symbol table (contains static symbols)

			TRACE((" searching debug symbols...\n"));

			for (i = 0; i < image->num_debug_symbols; i++) {
				struct Elf32_Sym *symbol = &image->debug_symbols[i];

				if (symbol->st_value == 0 || symbol->st_size
						>= image->text_region.size + image->data_region.size)
					continue;

				symbolDelta
					= address - (symbol->st_value + image->text_region.delta);
				if (symbolDelta >= 0 && symbolDelta < symbol->st_size)
					exactMatch = true;

				if (exactMatch || symbolDelta < deltaFound) {
					deltaFound = symbolDelta;
					symbolFound = symbol;
					symbolName = image->debug_string_table + symbol->st_name;

					if (exactMatch)
						break;
				}
			}
		} else {
			// search standard symbol lookup table

			TRACE((" searching standard symbols...\n"));

			for (i = 0; i < HASHTABSIZE(image); i++) {
				for (j = HASHBUCKETS(image)[i]; j != STN_UNDEF;
						j = HASHCHAINS(image)[j]) {
					struct Elf32_Sym *symbol = &image->syms[j];

					if (symbol->st_value == 0
						|| symbol->st_size >= image->text_region.size
							+ image->data_region.size)
						continue;

					symbolDelta = address - (long)(symbol->st_value
						+ image->text_region.delta);
					if (symbolDelta >= 0 && symbolDelta < symbol->st_size)
						exactMatch = true;

					if (exactMatch || symbolDelta < deltaFound) {
						deltaFound = symbolDelta;
						symbolFound = symbol;
						symbolName = SYMNAME(image, symbol);

						if (exactMatch)
							goto symbol_found;
					}
				}
			}
		}
	}
symbol_found:

	if (symbolFound != NULL) {
		if (_symbolName)
			*_symbolName = symbolName;
		if (_imageName)
			*_imageName = image->name;
		if (_baseAddress)
			*_baseAddress = symbolFound->st_value + image->text_region.delta;
		if (_exactMatch)
			*_exactMatch = exactMatch;

		status = B_OK;
	} else if (image != NULL) {
		TRACE(("symbol not found!\n"));

		if (_symbolName)
			*_symbolName = NULL;
		if (_imageName)
			*_imageName = image->name;
		if (_baseAddress)
			*_baseAddress = image->text_region.start;
		if (_exactMatch)
			*_exactMatch = false;

		status = B_OK;
	} else {
		TRACE(("image not found!\n"));
		status = B_ENTRY_NOT_FOUND;
	}

	// Note, theoretically, all information we return back to our caller
	// would have to be locked - but since this function is only called
	// from the debugger, it's safe to do it this way

	//mutex_unlock(&sImageMutex);

	return status;
}


/*!	Tries to find a matching user symbol for the given address.
	Note that the given team's address space must already be in effect.
*/
status_t
elf_debug_lookup_user_symbol_address(Team* team, addr_t address,
	addr_t *_baseAddress, const char **_symbolName, const char **_imageName,
	bool *_exactMatch)
{
	if (team == NULL || team == team_get_kernel_team())
		return B_BAD_VALUE;

	UserSymbolLookup& lookup = UserSymbolLookup::Default();
	status_t error = lookup.Init(team);
	if (error != B_OK)
		return error;

	return lookup.LookupSymbolAddress(address, _baseAddress, _symbolName,
		_imageName, _exactMatch);
}


/*!	Looks up a symbol in all kernel images. Note, this function is thought to
	be used in the kernel debugger, and therefore doesn't perform any locking.
*/
addr_t
elf_debug_lookup_symbol(const char* searchName)
{
	struct elf_image_info *image = NULL;
	struct hash_iterator iterator;

	hash_open(sImagesHash, &iterator);
	while ((image = (elf_image_info *)hash_next(sImagesHash, &iterator))
			!= NULL) {
		if (image->num_debug_symbols > 0) {
			// search extended debug symbol table (contains static symbols)
			for (uint32 i = 0; i < image->num_debug_symbols; i++) {
				struct Elf32_Sym *symbol = &image->debug_symbols[i];
				const char *name = image->debug_string_table + symbol->st_name;

				if (symbol->st_value > 0 && !strcmp(name, searchName))
					return symbol->st_value + image->text_region.delta;
			}
		} else {
			// search standard symbol lookup table
			for (uint32 i = 0; i < HASHTABSIZE(image); i++) {
				for (uint32 j = HASHBUCKETS(image)[i]; j != STN_UNDEF;
						j = HASHCHAINS(image)[j]) {
					struct Elf32_Sym *symbol = &image->syms[j];
					const char *name = SYMNAME(image, symbol);

					if (symbol->st_value > 0 && !strcmp(name, searchName))
						return symbol->st_value + image->text_region.delta;
				}
			}
		}
	}
	hash_close(sImagesHash, &iterator, false);

	return 0;
}


status_t
elf_lookup_kernel_symbol(const char* name, elf_symbol_info* info)
{
	// find the symbol
	Elf32_Sym* foundSymbol = elf_find_symbol(sKernelImage, name, NULL, false);
	if (foundSymbol == NULL)
		return B_MISSING_SYMBOL;

	info->address = foundSymbol->st_value + sKernelImage->text_region.delta;
	info->size = foundSymbol->st_size;
	return B_OK;
}


status_t
elf_load_user_image(const char *path, Team *team, int flags, addr_t *entry)
{
	struct Elf32_Ehdr elfHeader;
	struct Elf32_Phdr *programHeaders = NULL;
	char baseName[B_OS_NAME_LENGTH];
	status_t status;
	ssize_t length;
	int fd;
	int i;

	TRACE(("elf_load: entry path '%s', team %p\n", path, team));

	fd = _kern_open(-1, path, O_RDONLY, 0);
	if (fd < 0)
		return fd;

	struct stat st;
	status = _kern_read_stat(fd, NULL, false, &st, sizeof(st));
	if (status != B_OK)
		return status;

	// read and verify the ELF header

	length = _kern_read(fd, 0, &elfHeader, sizeof(elfHeader));
	if (length < B_OK) {
		status = length;
		goto error;
	}

	if (length != sizeof(elfHeader)) {
		// short read
		status = B_NOT_AN_EXECUTABLE;
		goto error;
	}
	status = verify_eheader(&elfHeader);
	if (status < B_OK)
		goto error;

	// read program header

	programHeaders = (struct Elf32_Phdr *)malloc(
		elfHeader.e_phnum * elfHeader.e_phentsize);
	if (programHeaders == NULL) {
		dprintf("error allocating space for program headers\n");
		status = B_NO_MEMORY;
		goto error;
	}

	TRACE(("reading in program headers at 0x%lx, length 0x%x\n",
		elfHeader.e_phoff, elfHeader.e_phnum * elfHeader.e_phentsize));
	length = _kern_read(fd, elfHeader.e_phoff, programHeaders,
		elfHeader.e_phnum * elfHeader.e_phentsize);
	if (length < B_OK) {
		status = length;
		dprintf("error reading in program headers\n");
		goto error;
	}
	if (length != elfHeader.e_phnum * elfHeader.e_phentsize) {
		dprintf("short read while reading in program headers\n");
		status = -1;
		goto error;
	}

	// construct a nice name for the region we have to create below
	{
		int32 length;

		const char *leaf = strrchr(path, '/');
		if (leaf == NULL)
			leaf = path;
		else
			leaf++;

		length = strlen(leaf);
		if (length > B_OS_NAME_LENGTH - 8)
			sprintf(baseName, "...%s", leaf + length + 8 - B_OS_NAME_LENGTH);
		else
			strcpy(baseName, leaf);
	}

	// map the program's segments into memory

	image_info imageInfo;
	memset(&imageInfo, 0, sizeof(image_info));

	for (i = 0; i < elfHeader.e_phnum; i++) {
		char regionName[B_OS_NAME_LENGTH];
		char *regionAddress;
		area_id id;

		if (programHeaders[i].p_type != PT_LOAD)
			continue;

		regionAddress = (char *)ROUNDDOWN(programHeaders[i].p_vaddr,
			B_PAGE_SIZE);
		if (programHeaders[i].p_flags & PF_WRITE) {
			// rw/data segment
			uint32 memUpperBound = (programHeaders[i].p_vaddr % B_PAGE_SIZE)
				+ programHeaders[i].p_memsz;
			uint32 fileUpperBound = (programHeaders[i].p_vaddr % B_PAGE_SIZE)
				+ programHeaders[i].p_filesz;

			memUpperBound = ROUNDUP(memUpperBound, B_PAGE_SIZE);
			fileUpperBound = ROUNDUP(fileUpperBound, B_PAGE_SIZE);

			sprintf(regionName, "%s_seg%drw", baseName, i);

			id = vm_map_file(team->id, regionName, (void **)&regionAddress,
				B_EXACT_ADDRESS, fileUpperBound,
				B_READ_AREA | B_WRITE_AREA, REGION_PRIVATE_MAP, false,
				fd, ROUNDDOWN(programHeaders[i].p_offset, B_PAGE_SIZE));
			if (id < B_OK) {
				dprintf("error mapping file data: %s!\n", strerror(id));
				status = B_NOT_AN_EXECUTABLE;
				goto error;
			}

			imageInfo.data = regionAddress;
			imageInfo.data_size = memUpperBound;

			// clean garbage brought by mmap (the region behind the file,
			// at least parts of it are the bss and have to be zeroed)
			uint32 start = (uint32)regionAddress
				+ (programHeaders[i].p_vaddr % B_PAGE_SIZE)
				+ programHeaders[i].p_filesz;
			uint32 amount = fileUpperBound
				- (programHeaders[i].p_vaddr % B_PAGE_SIZE)
				- (programHeaders[i].p_filesz);
			memset((void *)start, 0, amount);

			// Check if we need extra storage for the bss - we have to do this if
			// the above region doesn't already comprise the memory size, too.

			if (memUpperBound != fileUpperBound) {
				size_t bssSize = memUpperBound - fileUpperBound;

				snprintf(regionName, B_OS_NAME_LENGTH, "%s_bss%d", baseName, i);

				regionAddress += fileUpperBound;
				virtual_address_restrictions virtualRestrictions = {};
				virtualRestrictions.address = regionAddress;
				virtualRestrictions.address_specification = B_EXACT_ADDRESS;
				physical_address_restrictions physicalRestrictions = {};
				id = create_area_etc(team->id, regionName, bssSize, B_NO_LOCK,
					B_READ_AREA | B_WRITE_AREA, 0, &virtualRestrictions,
					&physicalRestrictions, (void**)&regionAddress);
				if (id < B_OK) {
					dprintf("error allocating bss area: %s!\n", strerror(id));
					status = B_NOT_AN_EXECUTABLE;
					goto error;
				}
			}
		} else {
			// assume ro/text segment
			snprintf(regionName, B_OS_NAME_LENGTH, "%s_seg%dro", baseName, i);

			size_t segmentSize = ROUNDUP(programHeaders[i].p_memsz
				+ (programHeaders[i].p_vaddr % B_PAGE_SIZE), B_PAGE_SIZE);

			id = vm_map_file(team->id, regionName, (void **)&regionAddress,
				B_EXACT_ADDRESS, segmentSize,
				B_READ_AREA | B_EXECUTE_AREA, REGION_PRIVATE_MAP, false,
				fd, ROUNDDOWN(programHeaders[i].p_offset, B_PAGE_SIZE));
			if (id < B_OK) {
				dprintf("error mapping file text: %s!\n", strerror(id));
				status = B_NOT_AN_EXECUTABLE;
				goto error;
			}

			imageInfo.text = regionAddress;
			imageInfo.text_size = segmentSize;
		}
	}

	// register the loaded image
	imageInfo.type = B_LIBRARY_IMAGE;
    imageInfo.device = st.st_dev;
    imageInfo.node = st.st_ino;
	strlcpy(imageInfo.name, path, sizeof(imageInfo.name));

	imageInfo.api_version = B_HAIKU_VERSION;
	imageInfo.abi = B_HAIKU_ABI;
		// TODO: Get the actual values for the shared object. Currently only
		// the runtime loader is loaded, so this is good enough for the time
		// being.

	imageInfo.id = register_image(team, &imageInfo, sizeof(image_info));
	if (imageInfo.id >= 0 && team_get_current_team_id() == team->id)
		user_debug_image_created(&imageInfo);
		// Don't care, if registering fails. It's not crucial.

	TRACE(("elf_load: done!\n"));

	*entry = elfHeader.e_entry;
	status = B_OK;

error:
	free(programHeaders);
	_kern_close(fd);

	return status;
}


image_id
load_kernel_add_on(const char *path)
{
	struct Elf32_Phdr *programHeaders;
	struct Elf32_Ehdr *elfHeader;
	struct elf_image_info *image;
	const char *fileName;
	void *reservedAddress;
	size_t reservedSize;
	status_t status;
	ssize_t length;
	bool textSectionWritable = false;
	int executableHeaderCount = 0;

	TRACE(("elf_load_kspace: entry path '%s'\n", path));

	int fd = _kern_open(-1, path, O_RDONLY, 0);
	if (fd < 0)
		return fd;

	struct vnode *vnode;
	status = vfs_get_vnode_from_fd(fd, true, &vnode);
	if (status < B_OK)
		goto error0;

	// get the file name
	fileName = strrchr(path, '/');
	if (fileName == NULL)
		fileName = path;
	else
		fileName++;

	// Prevent someone else from trying to load this image
	mutex_lock(&sImageLoadMutex);

	// make sure it's not loaded already. Search by vnode
	image = find_image_by_vnode(vnode);
	if (image) {
		atomic_add(&image->ref_count, 1);
		goto done;
	}

	elfHeader = (struct Elf32_Ehdr *)malloc(sizeof(*elfHeader));
	if (!elfHeader) {
		status = B_NO_MEMORY;
		goto error;
	}

	length = _kern_read(fd, 0, elfHeader, sizeof(*elfHeader));
	if (length < B_OK) {
		status = length;
		goto error1;
	}
	if (length != sizeof(*elfHeader)) {
		// short read
		status = B_NOT_AN_EXECUTABLE;
		goto error1;
	}
	status = verify_eheader(elfHeader);
	if (status < B_OK)
		goto error1;

	image = create_image_struct();
	if (!image) {
		status = B_NO_MEMORY;
		goto error1;
	}
	image->vnode = vnode;
	image->elf_header = elfHeader;
	image->name = strdup(path);
	vnode = NULL;

	programHeaders = (struct Elf32_Phdr *)malloc(elfHeader->e_phnum
		* elfHeader->e_phentsize);
	if (programHeaders == NULL) {
		dprintf("%s: error allocating space for program headers\n", fileName);
		status = B_NO_MEMORY;
		goto error2;
	}

	TRACE(("reading in program headers at 0x%lx, length 0x%x\n",
		elfHeader->e_phoff, elfHeader->e_phnum * elfHeader->e_phentsize));

	length = _kern_read(fd, elfHeader->e_phoff, programHeaders,
		elfHeader->e_phnum * elfHeader->e_phentsize);
	if (length < B_OK) {
		status = length;
		TRACE(("%s: error reading in program headers\n", fileName));
		goto error3;
	}
	if (length != elfHeader->e_phnum * elfHeader->e_phentsize) {
		TRACE(("%s: short read while reading in program headers\n", fileName));
		status = B_ERROR;
		goto error3;
	}

	// determine how much space we need for all loaded segments

	reservedSize = 0;
	length = 0;

	for (int32 i = 0; i < elfHeader->e_phnum; i++) {
		size_t end;

		if (programHeaders[i].p_type != PT_LOAD)
			continue;

		length += ROUNDUP(programHeaders[i].p_memsz
			+ (programHeaders[i].p_vaddr % B_PAGE_SIZE), B_PAGE_SIZE);

		end = ROUNDUP(programHeaders[i].p_memsz + programHeaders[i].p_vaddr,
			B_PAGE_SIZE);
		if (end > reservedSize)
			reservedSize = end;

		if (programHeaders[i].IsExecutable())
			executableHeaderCount++;
	}

	// Check whether the segments have an unreasonable amount of unused space
	// inbetween.
	if ((ssize_t)reservedSize > length + 8 * 1024) {
		status = B_BAD_DATA;
		goto error1;
	}

	// reserve that space and allocate the areas from that one
	if (vm_reserve_address_range(VMAddressSpace::KernelID(), &reservedAddress,
			B_ANY_KERNEL_ADDRESS, reservedSize, 0) < B_OK) {
		status = B_NO_MEMORY;
		goto error3;
	}

	image->data_region.size = 0;
	image->text_region.size = 0;

	for (int32 i = 0; i < elfHeader->e_phnum; i++) {
		char regionName[B_OS_NAME_LENGTH];
		elf_region *region;

		TRACE(("looking at program header %ld\n", i));

		switch (programHeaders[i].p_type) {
			case PT_LOAD:
				break;
			case PT_DYNAMIC:
				image->dynamic_section = programHeaders[i].p_vaddr;
				continue;
			default:
				dprintf("%s: unhandled pheader type 0x%lx\n", fileName,
					programHeaders[i].p_type);
				continue;
		}

		// we're here, so it must be a PT_LOAD segment

		// Usually add-ons have two PT_LOAD headers: one for .data one or .text.
		// x86 and PPC may differ in permission bits for .data's PT_LOAD header
		// x86 is usually RW, PPC is RWE

		// Some add-ons may have .text and .data concatenated in a single
		// PT_LOAD RWE header and we must map that to .text.
		if (programHeaders[i].IsReadWrite()
			&& (!programHeaders[i].IsExecutable()
				|| executableHeaderCount > 1)) {
			// this is the writable segment
			if (image->data_region.size != 0) {
				// we've already created this segment
				continue;
			}
			region = &image->data_region;

			snprintf(regionName, B_OS_NAME_LENGTH, "%s_data", fileName);
		} else if (programHeaders[i].IsExecutable()) {
			// this is the non-writable segment
			if (image->text_region.size != 0) {
				// we've already created this segment
				continue;
			}
			region = &image->text_region;

			// some programs may have .text and .data concatenated in a
			// single PT_LOAD section which is readable/writable/executable
			textSectionWritable = programHeaders[i].IsReadWrite();
			snprintf(regionName, B_OS_NAME_LENGTH, "%s_text", fileName);
		} else {
			dprintf("%s: weird program header flags 0x%lx\n", fileName,
				programHeaders[i].p_flags);
			continue;
		}

		region->start = (addr_t)reservedAddress + ROUNDDOWN(
			programHeaders[i].p_vaddr, B_PAGE_SIZE);
		region->size = ROUNDUP(programHeaders[i].p_memsz
			+ (programHeaders[i].p_vaddr % B_PAGE_SIZE), B_PAGE_SIZE);
		region->id = create_area(regionName, (void **)&region->start,
			B_EXACT_ADDRESS, region->size, B_FULL_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (region->id < B_OK) {
			dprintf("%s: error allocating area: %s\n", fileName,
				strerror(region->id));
			status = B_NOT_AN_EXECUTABLE;
			goto error4;
		}
		region->delta = -ROUNDDOWN(programHeaders[i].p_vaddr, B_PAGE_SIZE);

		TRACE(("elf_load_kspace: created area \"%s\" at %p\n",
			regionName, (void *)region->start));

		length = _kern_read(fd, programHeaders[i].p_offset,
			(void *)(region->start + (programHeaders[i].p_vaddr % B_PAGE_SIZE)),
			programHeaders[i].p_filesz);
		if (length < B_OK) {
			status = length;
			dprintf("%s: error reading in segment %ld\n", fileName, i);
			goto error5;
		}
	}

	image->data_region.delta += image->data_region.start;
	image->text_region.delta += image->text_region.start;

	// modify the dynamic ptr by the delta of the regions
	image->dynamic_section += image->text_region.delta;

	status = elf_parse_dynamic_section(image);
	if (status < B_OK)
		goto error5;

	status = init_image_version_infos(image);
	if (status != B_OK)
		goto error5;

	status = check_needed_image_versions(image);
	if (status != B_OK)
		goto error5;

	status = elf_relocate(image);
	if (status < B_OK)
		goto error5;

	// We needed to read in the contents of the "text" area, but
	// now we can protect it read-only/execute, unless this is a
	// special image with concatenated .text and .data, when it
	// will also need write access.
	set_area_protection(image->text_region.id,
		B_KERNEL_READ_AREA | B_KERNEL_EXECUTE_AREA
		| (textSectionWritable ? B_KERNEL_WRITE_AREA : 0));

	// There might be a hole between the two segments, and we don't need to
	// reserve this any longer
	vm_unreserve_address_range(VMAddressSpace::KernelID(), reservedAddress,
		reservedSize);

	// ToDo: this should be enabled by kernel settings!
	if (1)
		load_elf_symbol_table(fd, image);

	free(programHeaders);
	mutex_lock(&sImageMutex);
	register_elf_image(image);
	mutex_unlock(&sImageMutex);

done:
	_kern_close(fd);
	mutex_unlock(&sImageLoadMutex);

	return image->id;

error5:
error4:
	vm_unreserve_address_range(VMAddressSpace::KernelID(), reservedAddress,
		reservedSize);
error3:
	free(programHeaders);
error2:
	delete_elf_image(image);
	elfHeader = NULL;
error1:
	free(elfHeader);
error:
	mutex_unlock(&sImageLoadMutex);
error0:
	dprintf("Could not load kernel add-on \"%s\": %s\n", path,
		strerror(status));

	if (vnode)
		vfs_put_vnode(vnode);
	_kern_close(fd);

	return status;
}


status_t
unload_kernel_add_on(image_id id)
{
	MutexLocker _(sImageLoadMutex);
	MutexLocker _2(sImageMutex);

	elf_image_info *image = find_image(id);
	if (image == NULL)
		return B_BAD_IMAGE_ID;

	unload_elf_image(image);
	return B_OK;
}


struct elf_image_info*
elf_get_kernel_image()
{
	return sKernelImage;
}


status_t
elf_get_image_info_for_address(addr_t address, image_info* info)
{
	MutexLocker _(sImageMutex);
	struct elf_image_info* elfInfo = find_image_at_address(address);
	if (elfInfo == NULL)
		return B_ENTRY_NOT_FOUND;

	info->id = elfInfo->id;
    info->type = B_SYSTEM_IMAGE;
    info->sequence = 0;
    info->init_order = 0;
    info->init_routine = NULL;
    info->term_routine = NULL;
    info->device = -1;
    info->node = -1;
		// TODO: We could actually fill device/node in.
	strlcpy(info->name, elfInfo->name, sizeof(info->name));
    info->text = (void*)elfInfo->text_region.start;
    info->data = (void*)elfInfo->data_region.start;
    info->text_size = elfInfo->text_region.size;
    info->data_size = elfInfo->data_region.size;

	return B_OK;
}


image_id
elf_create_memory_image(const char* imageName, addr_t text, size_t textSize,
	addr_t data, size_t dataSize)
{
	// allocate the image
	elf_image_info* image = create_image_struct();
	if (image == NULL)
		return B_NO_MEMORY;
	MemoryDeleter imageDeleter(image);

	// allocate symbol and string tables -- we allocate an empty symbol table,
	// so that elf_debug_lookup_symbol_address() won't try the dynamic symbol
	// table, which we don't have.
	Elf32_Sym* symbolTable = (Elf32_Sym*)malloc(0);
	char* stringTable = (char*)malloc(1);
	MemoryDeleter symbolTableDeleter(symbolTable);
	MemoryDeleter stringTableDeleter(stringTable);
	if (symbolTable == NULL || stringTable == NULL)
		return B_NO_MEMORY;

	// the string table always contains the empty string
	stringTable[0] = '\0';

	image->debug_symbols = symbolTable;
	image->num_debug_symbols = 0;
	image->debug_string_table = stringTable;

	// dup image name
	image->name = strdup(imageName);
	if (image->name == NULL)
		return B_NO_MEMORY;

	// data and text region
	image->text_region.id = -1;
	image->text_region.start = text;
	image->text_region.size = textSize;
	image->text_region.delta = 0;

	image->data_region.id = -1;
	image->data_region.start = data;
	image->data_region.size = dataSize;
	image->data_region.delta = 0;

	mutex_lock(&sImageMutex);
	register_elf_image(image);
	image_id imageID = image->id;
	mutex_unlock(&sImageMutex);

	// keep the allocated memory
	imageDeleter.Detach();
	symbolTableDeleter.Detach();
	stringTableDeleter.Detach();

	return imageID;
}


status_t
elf_add_memory_image_symbol(image_id id, const char* name, addr_t address,
	size_t size, int32 type)
{
	MutexLocker _(sImageMutex);

	// get the image
	struct elf_image_info* image = find_image(id);
	if (image == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the current string table size
	size_t stringTableSize = 1;
	if (image->num_debug_symbols > 0) {
		for (int32 i = image->num_debug_symbols - 1; i >= 0; i--) {
			int32 nameIndex = image->debug_symbols[i].st_name;
			if (nameIndex != 0) {
				stringTableSize = nameIndex
					+ strlen(image->debug_string_table + nameIndex) + 1;
				break;
			}
		}
	}

	// enter the name in the string table
	char* stringTable = (char*)image->debug_string_table;
	size_t stringIndex = 0;
	if (name != NULL) {
		size_t nameSize = strlen(name) + 1;
		stringIndex = stringTableSize;
		stringTableSize += nameSize;
		stringTable = (char*)realloc((char*)image->debug_string_table,
			stringTableSize);
		if (stringTable == NULL)
			return B_NO_MEMORY;
		image->debug_string_table = stringTable;
		memcpy(stringTable + stringIndex, name, nameSize);
	}

	// resize the symbol table
	int32 symbolCount = image->num_debug_symbols + 1;
	Elf32_Sym* symbolTable = (Elf32_Sym*)realloc(
		(Elf32_Sym*)image->debug_symbols, sizeof(Elf32_Sym) * symbolCount);
	if (symbolTable == NULL)
		return B_NO_MEMORY;
	image->debug_symbols = symbolTable;

	// enter the symbol
	Elf32_Sym& symbol = symbolTable[symbolCount - 1];
	uint32 symbolType = type == B_SYMBOL_TYPE_DATA ? STT_OBJECT : STT_FUNC;
	symbol.st_name = stringIndex;
	symbol.st_value = address;
	symbol.st_size = size;
	symbol.st_info = ELF32_ST_INFO(STB_GLOBAL, symbolType);
	symbol.st_other = 0;
	symbol.st_shndx = 0;
	image->num_debug_symbols++;

	return B_OK;
}


status_t
elf_init(kernel_args *args)
{
	struct preloaded_image *image;

	image_init();

	sImagesHash = hash_init(IMAGE_HASH_SIZE, 0, image_compare, image_hash);
	if (sImagesHash == NULL)
		return B_NO_MEMORY;

	// Build a image structure for the kernel, which has already been loaded.
	// The preloaded_images were already prepared by the VM.
	image = args->kernel_image;
	if (insert_preloaded_image(static_cast<struct preloaded_elf32_image *>(
			image), true) < B_OK)
		panic("could not create kernel image.\n");

	// Build image structures for all preloaded images.
	for (image = args->preloaded_images; image != NULL; image = image->next)
		insert_preloaded_image(static_cast<struct preloaded_elf32_image *>(
			image), false);

	add_debugger_command("ls", &dump_address_info,
		"lookup symbol for a particular address");
	add_debugger_command("symbols", &dump_symbols, "dump symbols for image");
	add_debugger_command("symbol", &dump_symbol, "search symbol in images");
	add_debugger_command_etc("image", &dump_image, "dump image info",
		"Prints info about the specified image.\n"
		"  <image>  - pointer to the semaphore structure, or ID\n"
		"           of the image to print info for.\n", 0);

	sInitialized = true;
	return B_OK;
}


// #pragma mark -


/*!	Reads the symbol and string table for the kernel image with the given ID.
	\a _symbolCount and \a _stringTableSize are both in- and output parameters.
	When called they call the size of the buffers given by \a symbolTable and
	\a stringTable respectively. When the function returns successfully, they
	will contain the actual sizes (which can be greater than the original ones).
	The function will copy as much as possible into the buffers. For only
	getting the required buffer sizes, it can be invoked with \c NULL buffers.
	On success \a _imageDelta will contain the offset to be added to the symbol
	values in the table to get the actual symbol addresses.
*/
status_t
_user_read_kernel_image_symbols(image_id id, struct Elf32_Sym* symbolTable,
	int32* _symbolCount, char* stringTable, size_t* _stringTableSize,
	addr_t* _imageDelta)
{
	// check params
	if (_symbolCount == NULL || _stringTableSize == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(_symbolCount) || !IS_USER_ADDRESS(_stringTableSize)
		|| (_imageDelta != NULL && !IS_USER_ADDRESS(_imageDelta))
		|| (symbolTable != NULL && !IS_USER_ADDRESS(symbolTable))
		|| (stringTable != NULL && !IS_USER_ADDRESS(stringTable))) {
		return B_BAD_ADDRESS;
	}

	// get buffer sizes
	int32 maxSymbolCount;
	size_t maxStringTableSize;
	if (user_memcpy(&maxSymbolCount, _symbolCount, sizeof(maxSymbolCount))
			!= B_OK
		|| user_memcpy(&maxStringTableSize, _stringTableSize,
			sizeof(maxStringTableSize)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// find the image
	MutexLocker _(sImageMutex);
	struct elf_image_info* image = find_image(id);
	if (image == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the tables and infos
	addr_t imageDelta = image->text_region.delta;
	const Elf32_Sym* symbols;
	int32 symbolCount;
	const char* strings;

	if (image->debug_symbols != NULL) {
		symbols = image->debug_symbols;
		symbolCount = image->num_debug_symbols;
		strings = image->debug_string_table;
	} else {
		symbols = image->syms;
		symbolCount = image->symhash[1];
		strings = image->strtab;
	}

	// The string table size isn't stored in the elf_image_info structure. Find
	// out by iterating through all symbols.
	size_t stringTableSize = 0;
	for (int32 i = 0; i < symbolCount; i++) {
		size_t index = symbols[i].st_name;
		if (index > stringTableSize)
			stringTableSize = index;
	}
	stringTableSize += strlen(strings + stringTableSize) + 1;
		// add size of the last string

	// copy symbol table
	int32 symbolsToCopy = min_c(symbolCount, maxSymbolCount);
	if (symbolTable != NULL && symbolsToCopy > 0) {
		if (user_memcpy(symbolTable, symbols, sizeof(Elf32_Sym) * symbolsToCopy)
				!= B_OK) {
			return B_BAD_ADDRESS;
		}
	}

	// copy string table
	size_t stringsToCopy = min_c(stringTableSize, maxStringTableSize);
	if (stringTable != NULL && stringsToCopy > 0) {
		if (user_memcpy(stringTable, strings, stringsToCopy)
				!= B_OK) {
			return B_BAD_ADDRESS;
		}
	}

	// copy sizes
	if (user_memcpy(_symbolCount, &symbolCount, sizeof(symbolCount)) != B_OK
		|| user_memcpy(_stringTableSize, &stringTableSize,
				sizeof(stringTableSize)) != B_OK
		|| (_imageDelta != NULL && user_memcpy(_imageDelta, &imageDelta,
				sizeof(imageDelta)) != B_OK)) {
		return B_BAD_ADDRESS;
	}

	return B_OK;
}
#endif
