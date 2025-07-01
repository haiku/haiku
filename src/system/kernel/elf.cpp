/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*!	Contains the ELF loader */


#include <elf.h>

#include <OS.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>


#include <algorithm>

#include <AutoDeleter.h>
#include <BytePointer.h>
#include <commpage.h>
#include <driver_settings.h>
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
#include <StackOrHeapArray.h>
#include <vfs.h>
#include <vm/vm.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>

#include <arch/cpu.h>
#include <arch/elf.h>
#include <elf_priv.h> // For elf_image_info struct
#include <boot/elf.h>

// Define conceptual constants for ELF validation (can be tuned)
#define ELF_MAX_PROGRAM_HEADERS   256
#define ELF_MAX_PHENTSIZE_FACTOR  4
#define ELF_MAX_TOTAL_PH_SIZE     (64 * 1024) // 64KB
#define ELF_MAX_SEGMENT_MEMSZ     (256 * 1024 * 1024) // 256MB
#define MAX_DYN_SECTION_MEMSZ     (1 * 1024 * 1024)   // 1MB for dynamic section itself
#define ELF_MAX_DT_STRSZ          (16 * 1024 * 1024)  // 16MB for DT_STRSZ
#define ELF_MAX_REL_TABLE_SIZE    (32 * 1024 * 1024)  // 32MB for relocation tables
#define ELF_MAX_VER_COUNT         1024                // Max version definitions/needed


//#define TRACE_ELF
#ifdef TRACE_ELF
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


namespace {

#define IMAGE_HASH_SIZE 16

struct ImageHashDefinition {
	typedef struct elf_image_info ValueType;
	typedef image_id KeyType;

	size_t Hash(ValueType* entry) const
		{ return HashKey(entry->id); }
	ValueType*& GetLink(ValueType* entry) const
		{ return entry->next; }

	size_t HashKey(KeyType key) const
	{
		return (size_t)key;
	}

	bool Compare(KeyType key, ValueType* entry) const
	{
		return key == entry->id;
	}
};

typedef BOpenHashTable<ImageHashDefinition> ImageHash;

} // namespace


#ifndef ELF32_COMPAT

static ImageHash *sImagesHash;

static struct elf_image_info *sKernelImage = NULL;
static mutex sImageMutex = MUTEX_INITIALIZER("kimages_lock");
	// guards sImagesHash
static mutex sImageLoadMutex = MUTEX_INITIALIZER("kimages_load_lock");
	// serializes loading/unloading add-ons locking order
	// sImageLoadMutex -> sImageMutex
static bool sLoadElfSymbols = false;
static bool sInitialized = false;


static elf_sym *elf_find_symbol(struct elf_image_info *image, const char *name,
	const elf_version_info *version, bool lookupDefault);


static void
unregister_elf_image(struct elf_image_info *image)
{
	unregister_image(team_get_kernel_team(), image->id);
	sImagesHash->Remove(image);
}


static void
register_elf_image(struct elf_image_info *image)
{
	extended_image_info imageInfo;

	memset(&imageInfo, 0, sizeof(imageInfo));
	imageInfo.basic_info.id = image->id;
	imageInfo.basic_info.type = B_SYSTEM_IMAGE;
	strlcpy(imageInfo.basic_info.name, image->name,
		sizeof(imageInfo.basic_info.name));

	imageInfo.basic_info.text = (void *)image->text_region.start;
	imageInfo.basic_info.text_size = image->text_region.size;
	imageInfo.basic_info.data = (void *)image->data_region.start;
	imageInfo.basic_info.data_size = image->data_region.size;

	if (image->text_region.id >= 0) {
		// evaluate the API/ABI version symbols

		// Haiku API version
		imageInfo.basic_info.api_version = 0;
		elf_sym* symbol = elf_find_symbol(image,
			B_SHARED_OBJECT_HAIKU_VERSION_VARIABLE_NAME, NULL, true);
		if (symbol != NULL && symbol->st_shndx != SHN_UNDEF
			&& symbol->st_value > 0
			&& symbol->Type() == STT_OBJECT
			&& symbol->st_size >= sizeof(uint32)) {
			addr_t symbolAddress = symbol->st_value + image->text_region.delta;
			if (symbolAddress >= image->text_region.start
				&& symbolAddress - image->text_region.start + sizeof(uint32)
					<= image->text_region.size) {
				imageInfo.basic_info.api_version = *(uint32*)symbolAddress;
			}
		}

		// Haiku ABI
		imageInfo.basic_info.abi = 0;
		symbol = elf_find_symbol(image,
			B_SHARED_OBJECT_HAIKU_ABI_VARIABLE_NAME, NULL, true);
		if (symbol != NULL && symbol->st_shndx != SHN_UNDEF
			&& symbol->st_value > 0
			&& symbol->Type() == STT_OBJECT
			&& symbol->st_size >= sizeof(uint32)) {
			addr_t symbolAddress = symbol->st_value + image->text_region.delta;
			if (symbolAddress >= image->text_region.start
				&& symbolAddress - image->text_region.start + sizeof(uint32)
					<= image->text_region.size) {
				imageInfo.basic_info.api_version = *(uint32*)symbolAddress;
			}
		}
	} else {
		// in-memory image -- use the current values
		imageInfo.basic_info.api_version = B_HAIKU_VERSION;
		imageInfo.basic_info.abi = B_HAIKU_ABI;
	}

	image->id = register_image(team_get_kernel_team(), &imageInfo,
		sizeof(imageInfo));
	sImagesHash->Insert(image);
}


/*!	Note, you must lock the image mutex when you call this function. */
static struct elf_image_info *
find_image_at_address(addr_t address)
{
#if KDEBUG
	if (!debug_debugger_running())
		ASSERT_LOCKED_MUTEX(&sImageMutex);
#endif

	ImageHash::Iterator iterator(sImagesHash);

	// get image that may contain the address

	while (iterator.HasNext()) {
		struct elf_image_info* image = iterator.Next();
		if ((address >= image->text_region.start && address
				<= (image->text_region.start + image->text_region.size))
			|| (address >= image->data_region.start
				&& address
					<= (image->data_region.start + image->data_region.size)))
			return image;
	}

	return NULL;
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
	return sImagesHash->Lookup(id);
}


static struct elf_image_info *
find_image_by_vnode(void *vnode)
{
	MutexLocker locker(sImageMutex);

	ImageHash::Iterator iterator(sImagesHash);
	while (iterator.HasNext()) {
		struct elf_image_info* image = iterator.Next();
		if (image->vnode == vnode)
			return image;
	}

	return NULL;
}


#endif // ELF32_COMPAT


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
	image->dynamic_section_memsz = 0;
	image->string_table_size = 0;
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


static const char *
get_symbol_type_string(elf_sym *symbol)
{
	switch (symbol->Type()) {
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
get_symbol_bind_string(elf_sym *symbol)
{
	switch (symbol->Bind()) {
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


#ifndef ELF32_COMPAT


/*!	Searches a symbol (pattern) in all kernel images */
static int
dump_symbol(int argc, char **argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <symbol-name>\n", argv[0]);
		return 0;
	}

	struct elf_image_info *image = NULL;
	const char *pattern = argv[1];

	void* symbolAddress = NULL;

	ImageHash::Iterator iterator(sImagesHash);
	while (iterator.HasNext()) {
		image = iterator.Next();
		if (image->num_debug_symbols > 0) {
			// search extended debug symbol table (contains static symbols)
			for (uint32 i = 0; i < image->num_debug_symbols; i++) {
				elf_sym *symbol = &image->debug_symbols[i];
				const char *name = image->debug_string_table + symbol->st_name;

				if (symbol->st_value > 0 && strstr(name, pattern) != 0) {
					symbolAddress
						= (void*)(symbol->st_value + image->text_region.delta);
					kprintf("%p %5lu %s:%s\n", symbolAddress,
						(long unsigned int)(symbol->st_size),
						image->name, name);
				}
			}
		} else {
			// search standard symbol lookup table
			for (uint32 i = 0; i < HASHTABSIZE(image); i++) {
				for (uint32 j = HASHBUCKETS(image)[i]; j != STN_UNDEF;
						j = HASHCHAINS(image)[j]) {
					elf_sym *symbol = &image->syms[j];
					const char *name = SYMNAME(image, symbol);

					if (symbol->st_value > 0 && strstr(name, pattern) != 0) {
						symbolAddress = (void*)(symbol->st_value
							+ image->text_region.delta);
						kprintf("%p %5lu %s:%s\n", symbolAddress,
							(long unsigned int)(symbol->st_size),
							image->name, name);
					}
				}
			}
		}
	}

	if (symbolAddress != NULL)
		set_debug_variable("_", (addr_t)symbolAddress);

	return 0;
}


static int
dump_symbols(int argc, char **argv)
{
	struct elf_image_info *image = NULL;
	uint32 i;

	// if the argument looks like a hex number, treat it as such
	if (argc > 1) {
		if (isdigit(argv[1][0])) {
			addr_t num = strtoul(argv[1], NULL, 0);

			if (IS_KERNEL_ADDRESS(num)) {
				// find image at address

				ImageHash::Iterator iterator(sImagesHash);
				while (iterator.HasNext()) {
					elf_image_info* current = iterator.Next();
					if (current->text_region.start <= num
						&& current->text_region.start
							+ current->text_region.size	>= num) {
						image = current;
						break;
					}
				}

				if (image == NULL) {
					kprintf("No image covers %#" B_PRIxADDR " in the kernel!\n",
						num);
				}
			} else {
				image = sImagesHash->Lookup(num);
				if (image == NULL) {
					kprintf("image %#" B_PRIxADDR " doesn't exist in the "
						"kernel!\n", num);
				}
			}
		} else {
			// look for image by name
			ImageHash::Iterator iterator(sImagesHash);
			while (iterator.HasNext()) {
				elf_image_info* current = iterator.Next();
				if (!strcmp(current->name, argv[1])) {
					image = current;
					break;
				}
			}

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

	kprintf("Symbols of image %" B_PRId32 " \"%s\":\n", image->id, image->name);
	kprintf("%-*s Type       Size Name\n", B_PRINTF_POINTER_WIDTH, "Address");

	if (image->num_debug_symbols > 0) {
		// search extended debug symbol table (contains static symbols)
		for (i = 0; i < image->num_debug_symbols; i++) {
			elf_sym *symbol = &image->debug_symbols[i];

			if (symbol->st_value == 0 || symbol->st_size
					>= image->text_region.size + image->data_region.size)
				continue;

			kprintf("%0*lx %s/%s %5ld %s\n", B_PRINTF_POINTER_WIDTH,
				symbol->st_value + image->text_region.delta,
				get_symbol_type_string(symbol), get_symbol_bind_string(symbol),
				(long unsigned int)(symbol->st_size),
				image->debug_string_table + symbol->st_name);
		}
	} else {
		int32 j;

		// search standard symbol lookup table
		for (i = 0; i < HASHTABSIZE(image); i++) {
			for (j = HASHBUCKETS(image)[i]; j != STN_UNDEF;
					j = HASHCHAINS(image)[j]) {
				elf_sym *symbol = &image->syms[j];

				if (symbol->st_value == 0 || symbol->st_size
						>= image->text_region.size + image->data_region.size)
					continue;

				kprintf("%08lx %s/%s %5ld %s\n",
					symbol->st_value + image->text_region.delta,
					get_symbol_type_string(symbol),
					get_symbol_bind_string(symbol),
					(long unsigned int)(symbol->st_size),
					SYMNAME(image, symbol));
			}
		}
	}

	return 0;
}


static void
dump_elf_region(struct elf_region *region, const char *name)
{
	kprintf("   %s.id %" B_PRId32 "\n", name, region->id);
	kprintf("   %s.start %#" B_PRIxADDR "\n", name, region->start);
	kprintf("   %s.size %#" B_PRIxSIZE "\n", name, region->size);
	kprintf("   %s.delta %ld\n", name, region->delta);
}


static void
dump_image_info(struct elf_image_info *image)
{
	kprintf("elf_image_info at %p:\n", image);
	kprintf(" next %p\n", image->next);
	kprintf(" id %" B_PRId32 "\n", image->id);
	dump_elf_region(&image->text_region, "text");
	dump_elf_region(&image->data_region, "data");
	kprintf(" dynamic_section %#" B_PRIxADDR "\n", image->dynamic_section);
	kprintf(" needed %p\n", image->needed);
	kprintf(" symhash %p\n", image->symhash);
	kprintf(" syms %p\n", image->syms);
	kprintf(" strtab %p\n", image->strtab);
	kprintf(" rel %p\n", image->rel);
	kprintf(" rel_len %#x\n", image->rel_len);
	kprintf(" rela %p\n", image->rela);
	kprintf(" rela_len %#x\n", image->rela_len);
	kprintf(" pltrel %p\n", image->pltrel);
	kprintf(" pltrel_len %#x\n", image->pltrel_len);

	kprintf(" debug_symbols %p (%" B_PRIu32 ")\n",
		image->debug_symbols, image->num_debug_symbols);
}


static int
dump_image(int argc, char **argv)
{
	struct elf_image_info *image;

	// if the argument looks like a hex number, treat it as such
	if (argc > 1) {
		addr_t num = strtoul(argv[1], NULL, 0);

		if (IS_KERNEL_ADDRESS(num)) {
			// semi-hack
			dump_image_info((struct elf_image_info *)num);
		} else {
			image = sImagesHash->Lookup(num);
			if (image == NULL) {
				kprintf("image %#" B_PRIxADDR " doesn't exist in the kernel!\n",
					num);
			} else
				dump_image_info(image);
		}
		return 0;
	}

	kprintf("loaded kernel images:\n");

	ImageHash::Iterator iterator(sImagesHash);

	while (iterator.HasNext()) {
		image = iterator.Next();
		kprintf("%p (%" B_PRId32 ") %s\n", image, image->id, image->name);
	}

	return 0;
}


// Currently unused
/*static
void dump_symbol(struct elf_image_info *image, elf_sym *sym)
{

	kprintf("symbol at %p, in image %p\n", sym, image);

	kprintf(" name index %d, '%s'\n", sym->st_name, SYMNAME(image, sym));
	kprintf(" st_value 0x%x\n", sym->st_value);
	kprintf(" st_size %d\n", sym->st_size);
	kprintf(" st_info 0x%x\n", sym->st_info);
	kprintf(" st_other 0x%x\n", sym->st_other);
	kprintf(" st_shndx %d\n", sym->st_shndx);
}
*/


#endif // ELF32_COMPAT


static uint32
elf_hash(const char* _name)
{
	const uint8* name = (const uint8*)_name;

	uint32 h = 0;
	while (*name != '\0') {
		h = (h << 4) + *name++;
		h ^= (h >> 24) & 0xf0;
	}
	return (h & 0x0fffffff);
}


static elf_sym *
elf_find_symbol(struct elf_image_info *image, const char *name,
	const elf_version_info *lookupVersion, bool lookupDefault)
{
	if (image->dynamic_section == 0 || HASHTABSIZE(image) == 0)
		return NULL;

	elf_sym* versionedSymbol = NULL;
	uint32 versionedSymbolCount = 0;

	uint32 hash = elf_hash(name) % HASHTABSIZE(image);
	for (uint32 i = HASHBUCKETS(image)[hash]; i != STN_UNDEF;
			i = HASHCHAINS(image)[i]) {
		elf_sym* symbol = &image->syms[i];

		// consider only symbols with the right name and binding
		if (symbol->st_shndx == SHN_UNDEF
			|| ((symbol->Bind() != STB_GLOBAL) && (symbol->Bind() != STB_WEAK))
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
	elf_dyn *d;
	ssize_t neededOffset = -1;

	TRACE(("top of elf_parse_dynamic_section\n"));

	image->symhash = 0;
	image->syms = 0;
	image->strtab = 0;

	d = (elf_dyn *)image->dynamic_section;
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
				image->syms = (elf_sym *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_REL:
				image->rel = (elf_rel *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_RELSZ:
				image->rel_len = d[i].d_un.d_val;
				break;
			case DT_RELA:
				image->rela = (elf_rela *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_RELASZ:
				image->rela_len = d[i].d_un.d_val;
				break;
			case DT_JMPREL:
				image->pltrel = (elf_rel *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_PLTRELSZ:
				image->pltrel_len = d[i].d_un.d_val;
				break;
			case DT_PLTREL:
				image->pltrel_type = d[i].d_un.d_val;
				break;
			case DT_VERSYM:
				image->symbol_versions = (elf_versym*)
					(d[i].d_un.d_ptr + image->text_region.delta);
				break;
			case DT_VERDEF:
				image->version_definitions = (elf_verdef*)
					(d[i].d_un.d_ptr + image->text_region.delta);
				break;
			case DT_VERDEFNUM:
				image->num_version_definitions = d[i].d_un.d_val;
				break;
			case DT_VERNEED:
				image->needed_versions = (elf_verneed*)
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


#ifndef ELF32_COMPAT


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
	BytePointer<elf_verdef> definition(image->version_definitions);
	for (uint32 i = 0; i < image->num_version_definitions; i++) {
		uint32 versionIndex = VER_NDX(definition->vd_ndx);
		elf_version_info& info = image->versions[versionIndex];

		if (neededVersion.hash == info.hash
			&& strcmp(neededVersion.name, info.name) == 0) {
			return B_OK;
		}

		definition += definition->vd_next;
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
		BytePointer<elf_verdef> definition(image->version_definitions);
		for (uint32 i = 0; i < image->num_version_definitions; i++) {
			if (definition->vd_version != 1) {
				dprintf("Unsupported version definition revision: %u\n",
					definition->vd_version);
				return B_BAD_VALUE;
			}

			uint32 versionIndex = VER_NDX(definition->vd_ndx);
			if (versionIndex > maxIndex)
				maxIndex = versionIndex;

			definition += definition->vd_next;
		}
	}

	if (image->needed_versions != NULL) {
		BytePointer<elf_verneed> needed(image->needed_versions);
		for (uint32 i = 0; i < image->num_needed_versions; i++) {
			if (needed->vn_version != 1) {
				dprintf("Unsupported version needed revision: %u\n",
					needed->vn_version);
				return B_BAD_VALUE;
			}

			BytePointer<elf_vernaux> vernaux(needed + needed->vn_aux);
			for (uint32 k = 0; k < needed->vn_cnt; k++) {
				uint32 versionIndex = VER_NDX(vernaux->vna_other);
				if (versionIndex > maxIndex)
					maxIndex = versionIndex;

				vernaux += vernaux->vna_next;
			}

			needed += needed->vn_next;
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
		BytePointer<elf_verdef> definition(image->version_definitions);
		for (uint32 i = 0; i < image->num_version_definitions; i++) {
			if (definition->vd_cnt > 0
				&& (definition->vd_flags & VER_FLG_BASE) == 0) {
				BytePointer<elf_verdaux> verdaux(definition
					+ definition->vd_aux);

				uint32 versionIndex = VER_NDX(definition->vd_ndx);
				elf_version_info& info = image->versions[versionIndex];
				info.hash = definition->vd_hash;
				info.name = STRING(image, verdaux->vda_name);
				info.file_name = NULL;
			}

			definition += definition->vd_next;
		}
	}

	// needed versions
	if (image->needed_versions != NULL) {
		BytePointer<elf_verneed> needed(image->needed_versions);
		for (uint32 i = 0; i < image->num_needed_versions; i++) {
			const char* fileName = STRING(image, needed->vn_file);

			BytePointer<elf_vernaux> vernaux(needed + needed->vn_aux);
			for (uint32 k = 0; k < needed->vn_cnt; k++) {
				uint32 versionIndex = VER_NDX(vernaux->vna_other);
				elf_version_info& info = image->versions[versionIndex];
				info.hash = vernaux->vna_hash;
				info.name = STRING(image, vernaux->vna_name);
				info.file_name = fileName;

				vernaux += vernaux->vna_next;
			}

			needed += needed->vn_next;
		}
	}

	return B_OK;
}


static status_t
check_needed_image_versions(elf_image_info* image)
{
	if (image->needed_versions == NULL)
		return B_OK;

	BytePointer<elf_verneed> needed(image->needed_versions);
	for (uint32 i = 0; i < image->num_needed_versions; i++) {
		elf_image_info* dependency = sKernelImage;

		BytePointer<elf_vernaux> vernaux(needed + needed->vn_aux);
		for (uint32 k = 0; k < needed->vn_cnt; k++) {
			uint32 versionIndex = VER_NDX(vernaux->vna_other);
			elf_version_info& info = image->versions[versionIndex];

			status_t error = assert_defined_image_version(image, dependency,
				info, (vernaux->vna_flags & VER_FLG_WEAK) != 0);
			if (error != B_OK)
				return error;

			vernaux += vernaux->vna_next;
		}

		needed += needed->vn_next;
	}

	return B_OK;
}


#endif // ELF32_COMPAT


/*!	Resolves the \a symbol by linking against \a sharedImage if necessary.
	Returns the resolved symbol's address in \a _symbolAddress.
*/
status_t
elf_resolve_symbol(struct elf_image_info *image, elf_sym *symbol,
	struct elf_image_info *sharedImage, elf_addr *_symbolAddress)
{
	// Local symbols references are always resolved to the given symbol.
	if (symbol->Bind() == STB_LOCAL) {
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
	elf_sym* foundSymbol = elf_find_symbol(firstImage, symbolName, versionInfo,
		false);
	if (foundSymbol == NULL
		|| foundSymbol->Bind() == STB_WEAK) {
		// Not found or found a weak definition -- try to resolve in the other
		// image.
		elf_sym* secondSymbol = elf_find_symbol(secondImage, symbolName,
			versionInfo, false);
		// If we found a symbol -- take it in case we didn't have a symbol
		// before or the new symbol is not weak.
		if (secondSymbol != NULL
			&& (foundSymbol == NULL
				|| secondSymbol->Bind() != STB_WEAK)) {
			foundImage = secondImage;
			foundSymbol = secondSymbol;
		}
	}

	if (foundSymbol == NULL) {
		// Weak undefined symbols get a value of 0, if unresolved.
		if (symbol->Bind() == STB_WEAK) {
			*_symbolAddress = 0;
			return B_OK;
		}

		dprintf("\"%s\": could not resolve symbol '%s'\n", image->name,
			symbolName);
		return B_MISSING_SYMBOL;
	}

	// make sure they're the same type
	if (symbol->Type() != foundSymbol->Type()) {
		dprintf("elf_resolve_symbol: found symbol '%s' in image '%s' "
			"(requested by image '%s') but wrong type (%d vs. %d)\n",
			symbolName, foundImage->name, image->name,
			foundSymbol->Type(), symbol->Type());
		return B_MISSING_SYMBOL;
	}

	*_symbolAddress = foundSymbol->st_value + foundImage->text_region.delta;
	return B_OK;
}


/*! Until we have shared library support, just this links against the kernel */
static int
elf_relocate(struct elf_image_info* image, struct elf_image_info* resolveImage)
{
	int status = B_NO_ERROR;

	TRACE(("elf_relocate(%p (\"%s\"))\n", image, image->name));

	// deal with the rels first
	if (image->rel) {
		TRACE(("total %i rel relocs\n", image->rel_len / (int)sizeof(elf_rel)));

		status = arch_elf_relocate_rel(image, resolveImage, image->rel,
			image->rel_len);
		if (status < B_OK)
			return status;
	}

	if (image->pltrel) {
		if (image->pltrel_type == DT_REL) {
			TRACE(("total %i plt-relocs\n",
				image->pltrel_len / (int)sizeof(elf_rel)));
			status = arch_elf_relocate_rel(image, resolveImage, image->pltrel,
				image->pltrel_len);
		} else {
			TRACE(("total %i plt-relocs\n",
				image->pltrel_len / (int)sizeof(elf_rela)));
			status = arch_elf_relocate_rela(image, resolveImage,
				(elf_rela *)image->pltrel, image->pltrel_len);
		}
		if (status < B_OK)
			return status;
	}

	if (image->rela) {
		TRACE(("total %i rel relocs\n",
			image->rela_len / (int)sizeof(elf_rela)));

		status = arch_elf_relocate_rela(image, resolveImage, image->rela,
			image->rela_len);
		if (status < B_OK)
			return status;
	}

	return status;
}


static int
verify_eheader(elf_ehdr *elfHeader)
{
	if (memcmp(elfHeader->e_ident, ELFMAG, 4) != 0)
		return B_NOT_AN_EXECUTABLE;

	if (elfHeader->e_ident[4] != ELF_CLASS)
		return B_NOT_AN_EXECUTABLE;

	if (elfHeader->e_phoff == 0)
		return B_NOT_AN_EXECUTABLE;

	if (elfHeader->e_phentsize < sizeof(elf_phdr))
		return B_NOT_AN_EXECUTABLE;

	return 0;
}


#ifndef ELF32_COMPAT


static void
unload_elf_image(struct elf_image_info *image)
{
	if (atomic_add(&image->ref_count, -1) > 1)
		return;

	TRACE(("unload image %" B_PRId32 ", %s\n", image->id, image->name));

	unregister_elf_image(image);
	delete_elf_image(image);
}


static status_t
load_elf_symbol_table(int fd, struct elf_image_info *image)
{
	elf_ehdr *elfHeader = image->elf_header;
	elf_sym *symbolTable = NULL;
	elf_shdr *stringHeader = NULL;
	uint32 numSymbols = 0;
	char *stringTable = NULL; // Initialize to NULL
	status_t status = B_OK;   // Initialize status
	ssize_t length;
	int32 i;
	elf_shdr *sectionHeaders = NULL; // Initialize to NULL

	// get section headers
	uint64 calculated_sh_size = (uint64)elfHeader->e_shnum * elfHeader->e_shentsize;
	const uint64 practical_max_sh_size = (uint64)0xffff * 256; // Max sections * generous entry size

	if (elfHeader->e_shnum > 0 && elfHeader->e_shentsize == 0) {
		TRACE(("ELF: e_shentsize is 0 with e_shnum > 0.\n"));
		status = B_BAD_DATA;
		goto error0; // No allocations yet to free for this function's scope
	}
	if (calculated_sh_size > practical_max_sh_size || calculated_sh_size > SSIZE_MAX) {
		TRACE(("ELF: Section headers table size (%" B_PRIu64 ") is too large.\n", calculated_sh_size));
		status = B_BAD_DATA; // Or B_NO_MEMORY
		goto error0;
	}

	size_t section_headers_alloc_size = (size_t)calculated_sh_size;
	if (section_headers_alloc_size > 0) {
		sectionHeaders = (elf_shdr *)malloc(section_headers_alloc_size);
		if (sectionHeaders == NULL) {
			dprintf("error allocating space for section headers\n");
			status = B_NO_MEMORY;
			goto error0;
		}
	} else {
		sectionHeaders = NULL; // Explicitly NULL if size is 0
	}

	if (section_headers_alloc_size > 0) {
		length = read_pos(fd, elfHeader->e_shoff, sectionHeaders, section_headers_alloc_size);
		if (length < 0) { // read_pos returns ssize_t, error is < 0
			TRACE(("error reading in section headers: %s\n", strerror(length)));
			status = length; // actual error code
			goto error1;
		}
		if ((size_t)length < section_headers_alloc_size) {
			TRACE(("short read while reading in section headers\n"));
			status = B_ERROR; // Or B_IO_ERROR
			goto error1;
		}
	}

	// find symbol table in section headers
	for (i = 0; i < elfHeader->e_shnum; i++) { // This loop won't run if e_shnum is 0
		if (sectionHeaders[i].sh_type == SHT_SYMTAB) {
			// Validate sh_link before using it as an index
			if (sectionHeaders[i].sh_link >= elfHeader->e_shnum) {
				TRACE(("ELF: Invalid sh_link in SYMTAB section header.\n"));
				status = B_BAD_DATA;
				goto error1;
			}
			stringHeader = &sectionHeaders[sectionHeaders[i].sh_link];

			if (stringHeader->sh_type != SHT_STRTAB) {
				TRACE(("SHT_SYMTAB section does not link to a SHT_STRTAB section.\n"));
				status = B_BAD_DATA;
				goto error1;
			}

			// read in symbol table
			size_t symtab_size = sectionHeaders[i].sh_size;
			if (symtab_size == 0) { // Empty symbol table is valid
				numSymbols = 0;
				symbolTable = NULL; // Ensure it's NULL if size is 0
				break;
			}
			if (symtab_size > practical_max_sh_size) { // Reuse practical_max_sh_size as a general sanity limit
				TRACE(("ELF: Symbol table size (%" B_PRIuSIZE ") is too large.\n", symtab_size));
				status = B_BAD_DATA;
				goto error1;
			}
			if (symtab_size % sizeof(elf_sym) != 0) {
				TRACE(("ELF: Symbol table size is not a multiple of elf_sym size.\n"));
				status = B_BAD_DATA;
				goto error1;
			}

			symbolTable = (elf_sym *)malloc(symtab_size);
			if (symbolTable == NULL) {
				status = B_NO_MEMORY;
				goto error1;
			}

			length = read_pos(fd, sectionHeaders[i].sh_offset, symbolTable, symtab_size);
			if (length < 0) {
				TRACE(("error reading in symbol table: %s\n", strerror(length)));
				status = length;
				goto error2;
			}
			if ((size_t)length < symtab_size) {
				TRACE(("short read while reading in symbol table\n"));
				status = B_ERROR;
				goto error2;
			}

			numSymbols = symtab_size / sizeof(elf_sym);
			break;
		}
	}

	if (symbolTable == NULL && elfHeader->e_shnum > 0 && i == elfHeader->e_shnum) {
		TRACE(("no symbol table (SHT_SYMTAB) found\n"));
		status = B_BAD_VALUE;
		goto error1;
	}
	if (symbolTable == NULL && numSymbols == 0) {
		goto success_no_symbols;
	}


	// read in string table (only if stringHeader was found and validated)
	if (stringHeader == NULL) {
		TRACE(("ELF: stringHeader is NULL, implies no valid SHT_SYMTAB or SHT_STRTAB found.\n"));
		if (symbolTable != NULL) {
			status = B_BAD_DATA;
			goto error2;
		}
		goto success_no_symbols;
	}


	size_t strtab_size = stringHeader->sh_size;
	if (strtab_size == 0) {
		stringTable = (char*)malloc(1);
		if (stringTable == NULL) {
			status = B_NO_MEMORY;
			goto error2;
		}
		stringTable[0] = '\0';
	} else {
		if (strtab_size > practical_max_sh_size) {
			TRACE(("ELF: String table size (%" B_PRIuSIZE ") is too large.\n", strtab_size));
			status = B_BAD_DATA;
			goto error2;
		}
		stringTable = (char *)malloc(strtab_size);
		if (stringTable == NULL) {
			status = B_NO_MEMORY;
			goto error2;
		}

		length = read_pos(fd, stringHeader->sh_offset, stringTable, strtab_size);
		if (length < 0) {
			TRACE(("error reading in string table: %s\n", strerror(length)));
			status = length;
			goto error3;
		}
		if ((size_t)length < strtab_size) {
			TRACE(("short read while reading in string table\n"));
			status = B_ERROR;
			goto error3;
		}
	}

success_no_symbols:

	TRACE(("loaded %" B_PRId32 " debug symbols\n", numSymbols));

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
insert_preloaded_image(preloaded_elf_image *preloadedImage, bool kernel)
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

		status = elf_relocate(image, sKernelImage);
		if (status != B_OK)
			goto error1;
	} else
		sKernelImage = image;

	// copy debug symbols to the kernel heap
	if (preloadedImage->debug_symbols != NULL) {
		int32 debugSymbolsSize = sizeof(elf_sym)
			* preloadedImage->num_debug_symbols;
		image->debug_symbols = (elf_sym*)malloc(debugSymbolsSize);
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
		fTeam = team;
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
		struct image *image;
		status_t error = _FindImageAtAddress(address, image);
		if (error != B_OK)
			return error;

		if (image->info.basic_info.text == fTeam->commpage_address) {
			// commpage requires special treatment since kernel stores symbol
			// information
			if (*_imageName != NULL)
				*_imageName = "commpage";
			address -= (addr_t)fTeam->commpage_address;
			error = elf_debug_lookup_symbol_address(address, _baseAddress,
				_symbolName, NULL, _exactMatch);
			if (_baseAddress != NULL)
				*_baseAddress += (addr_t)fTeam->commpage_address;
			return error;
		}

		const extended_image_info& info = image->info;
		const uint32 *symhash = (uint32 *)info.symbol_hash;
		elf_sym *syms = (elf_sym *)info.symbol_table;

		strlcpy(fImageName, info.basic_info.name, sizeof(fImageName));

		// symbol hash table size
		uint32 hashTabSize;
		if (!_Read(symhash, hashTabSize))
			return B_BAD_ADDRESS;

		// remote pointers to hash buckets and chains
		const uint32* hashBuckets = symhash + 2;
		const uint32* hashChains = symhash + 2 + hashTabSize;

		const addr_t loadDelta = (addr_t)info.basic_info.text;

		// search the image for the symbol
		elf_sym symbolFound;
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

				elf_sym symbol;
				if (!_Read(syms + j, symbol))
					continue;

				// The symbol table contains not only symbols referring to
				// functions and data symbols within the shared object, but also
				// referenced symbols of other shared objects, as well as
				// section and file references. We ignore everything but
				// function and data symbols that have an st_value != 0 (0
				// seems to be an indication for a symbol defined elsewhere
				// -- couldn't verify that in the specs though).
				if ((symbol.Type() != STT_FUNC && symbol.Type() != STT_OBJECT)
					|| symbol.st_value == 0
					|| (symbol.st_value + symbol.st_size) > (elf_addr)info.basic_info.text_size) {
					continue;
				}

				// skip symbols starting after the given address
				addr_t symbolAddress = symbol.st_value + loadDelta;
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
				if (_ReadString(info, symbolFound.st_name, fSymbolName,
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
				*_baseAddress = symbolFound.st_value + loadDelta;
			else
				*_baseAddress = loadDelta;
		}

		if (_exactMatch)
			*_exactMatch = exactMatch;

		return B_OK;
	}

	status_t _FindImageAtAddress(addr_t address, struct image*& _image)
	{
		for (struct image* image = fTeam->image_list.First();
				image != NULL; image = fTeam->image_list.GetNext(image)) {
			image_info *info = &image->info.basic_info;

			if ((address < (addr_t)info->text
					|| address >= (addr_t)info->text + info->text_size)
				&& (address < (addr_t)info->data
					|| address >= (addr_t)info->data + info->data_size))
				continue;

			// found image
			_image = image;
			return B_OK;
		}

		return B_ENTRY_NOT_FOUND;
	}

	bool _ReadString(const extended_image_info& info, uint32 offset, char* buffer,
		size_t bufferSize)
	{
		const char* address = (char *)info.string_table + offset;

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
	Team*						fTeam;
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
	elf_sym *symbol;
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
	elf_sym *symbolFound = NULL;
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
				elf_sym *symbol = &image->debug_symbols[i];

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
					elf_sym *symbol = &image->syms[j];

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

	ImageHash::Iterator iterator(sImagesHash);
	while (iterator.HasNext()) {
		image = iterator.Next();
		if (image->num_debug_symbols > 0) {
			// search extended debug symbol table (contains static symbols)
			for (uint32 i = 0; i < image->num_debug_symbols; i++) {
				elf_sym *symbol = &image->debug_symbols[i];
				const char *name = image->debug_string_table + symbol->st_name;

				if (symbol->st_value > 0 && !strcmp(name, searchName))
					return symbol->st_value + image->text_region.delta;
			}
		} else {
			// search standard symbol lookup table
			for (uint32 i = 0; i < HASHTABSIZE(image); i++) {
				for (uint32 j = HASHBUCKETS(image)[i]; j != STN_UNDEF;
						j = HASHCHAINS(image)[j]) {
					elf_sym *symbol = &image->syms[j];
					const char *name = SYMNAME(image, symbol);

					if (symbol->st_value > 0 && !strcmp(name, searchName))
						return symbol->st_value + image->text_region.delta;
				}
			}
		}
	}

	return 0;
}


status_t
elf_lookup_kernel_symbol(const char* name, elf_symbol_info* info)
{
	// find the symbol
	elf_sym* foundSymbol = elf_find_symbol(sKernelImage, name, NULL, false);
	if (foundSymbol == NULL)
		return B_MISSING_SYMBOL;

	info->address = foundSymbol->st_value + sKernelImage->text_region.delta;
	info->size = foundSymbol->st_size;
	return B_OK;
}


#endif // ELF32_COMPAT


status_t
elf_load_user_image(const char *path, Team *team, uint32 flags, addr_t *entry)
{
	elf_ehdr elfHeader;
	char baseName[B_OS_NAME_LENGTH];
	status_t status;
	ssize_t length;

	TRACE(("elf_load: entry path '%s', team %p\n", path, team));

	int fd = _kern_open(-1, path, O_RDONLY, 0);
	if (fd < 0)
		return fd;
	FileDescriptorCloser fdCloser(fd);

	struct stat st;
	status = _kern_read_stat(fd, NULL, false, &st, sizeof(st));
	if (status != B_OK)
		return status;

	// read and verify the ELF header

	length = _kern_read(fd, 0, &elfHeader, sizeof(elfHeader));
	if (length < B_OK)
		return length;

	if (length != sizeof(elfHeader)) {
		// short read
		return B_NOT_AN_EXECUTABLE;
	}
	status = verify_eheader(&elfHeader);
	if (status < B_OK)
		return status;

#ifdef ELF_LOAD_USER_IMAGE_TEST_EXECUTABLE
	if ((flags & ELF_LOAD_USER_IMAGE_TEST_EXECUTABLE) != 0)
		return B_OK;
#endif

	struct elf_image_info* image;
	image = create_image_struct();
	if (image == NULL)
		return B_NO_MEMORY;
	CObjectDeleter<elf_image_info, void, delete_elf_image> imageDeleter(image);

	struct ElfHeaderUnsetter {
		ElfHeaderUnsetter(elf_image_info* image)
			: fImage(image)
		{
		}
		~ElfHeaderUnsetter()
		{
			fImage->elf_header = NULL;
		}

		elf_image_info* fImage;
	} headerUnsetter(image);
	image->elf_header = &elfHeader;

	// read program header
	// Validate ELF program header count and entry size before allocation
	if (elfHeader.e_phnum > 256) { // ELF_MAX_PROGRAM_HEADERS
		TRACE(("%s: ELF has too many program headers (%" B_PRIu16 ").\n", baseName, elfHeader.e_phnum));
		return B_BAD_DATA;
	}
	// verify_eheader ensures e_phentsize >= sizeof(elf_phdr).
	if (elfHeader.e_phnum > 0 && elfHeader.e_phentsize > (sizeof(elf_phdr) * 4)) { // ELF_MAX_PHENTSIZE_FACTOR
		TRACE(("%s: ELF program header entry size (e_phentsize %" B_PRIu16 ") is unusually large.\n", baseName, elfHeader.e_phentsize));
		return B_BAD_DATA;
	}

	size_t programHeadersSize = 0;
	if (elfHeader.e_phnum > 0) {
		if (elfHeader.e_phentsize > SIZE_MAX / elfHeader.e_phnum) {
			TRACE(("%s: ELF program headers size calculation would overflow (e_phnum %" B_PRIu16 ", e_phentsize %" B_PRIu16 ").\n",
				baseName, elfHeader.e_phnum, elfHeader.e_phentsize));
			return B_BAD_DATA;
		}
		programHeadersSize = (size_t)elfHeader.e_phnum * elfHeader.e_phentsize;

		if (programHeadersSize == 0) { // Should not happen if e_phnum > 0
			TRACE(("%s: Calculated programHeadersSize is 0 with e_phnum > 0.\n", baseName));
			return B_BAD_DATA;
		}
		if (programHeadersSize > (64 * 1024)) { // ELF_MAX_TOTAL_PH_SIZE
			TRACE(("%s: ELF program headers total size (%" B_PRIuSIZE ") is too large.\n", baseName, programHeadersSize));
			return B_BAD_DATA;
		}
	}

	elf_phdr *programHeaders = (elf_phdr *)malloc(programHeadersSize);
	if (programHeaders == NULL) {
		dprintf("%s: error allocating space for program headers (size %" B_PRIuSIZE ")\n", baseName, programHeadersSize);
		return B_NO_MEMORY;
	}
	MemoryDeleter headersDeleter(programHeaders);

	if (elfHeader.e_phnum > 0) {
		if (elfHeader.e_phoff < 0) {
			TRACE(("%s: ELF program header offset (e_phoff %" B_PRIdOFF ") is negative.\n", baseName, elfHeader.e_phoff));
			return B_BAD_DATA;
		}
		if ((unsigned long long)elfHeader.e_phoff + programHeadersSize > (unsigned long long)st.st_size) {
			TRACE(("%s: ELF program headers (offset %" B_PRIdOFF ", size %" B_PRIuSIZE ") exceed file size (%" B_PRIdOFF ").\n",
				baseName, elfHeader.e_phoff, programHeadersSize, st.st_size));
			return B_BAD_DATA;
		}
		if (programHeadersSize > SSIZE_MAX) {
			TRACE(("%s: ELF program headers size (%" B_PRIuSIZE ") exceeds SSIZE_MAX for read.\n", baseName, programHeadersSize));
			return B_BAD_DATA;
		}

		TRACE(("reading in program headers at offset %" B_PRIdOFF ", size %" B_PRIuSIZE "\n",
			elfHeader.e_phoff, programHeadersSize));
		length = _kern_read(fd, elfHeader.e_phoff, programHeaders, programHeadersSize);
		if (length < B_OK) {
			dprintf("error reading in program headers: %s\n", strerror(length));
			return length;
		}
		if ((size_t)length != programHeadersSize) {
			dprintf("short read while reading in program headers (read %" B_PRIdSSIZE ", expected %" B_PRIuSIZE ").\n",
				length, programHeadersSize);
			return B_ERROR;
		}
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
		if (length > B_OS_NAME_LENGTH - 16)
			snprintf(baseName, B_OS_NAME_LENGTH, "...%s", leaf + length + 16 - B_OS_NAME_LENGTH);
		else
			strcpy(baseName, leaf);
	}

	// map the program's segments into memory, initially with rw access
	// correct area protection will be set after relocation

	BStackOrHeapArray<area_id, 8> mappedAreas(elfHeader.e_phnum);
	if (!mappedAreas.IsValid())
		return B_NO_MEMORY;

	extended_image_info imageInfo;
	memset(&imageInfo, 0, sizeof(imageInfo));

	addr_t delta = 0;
	uint32 addressSpec = B_RANDOMIZED_BASE_ADDRESS;
	for (int i = 0; i < elfHeader.e_phnum; i++) {
		elf_phdr* phdr = &programHeaders[i];
		char regionName[B_OS_NAME_LENGTH];
		char *regionAddress;
		char *originalRegionAddress;
		area_id id;

		mappedAreas[i] = -1;

		if (phdr->p_type == PT_DYNAMIC) {
			image->dynamic_section = phdr->p_vaddr;
			// Store dynamic section size for later parsing validation
			image->dynamic_section_memsz = phdr->p_memsz;
			continue;
		}

		if (phdr->p_type != PT_LOAD)
			continue;

		// II.1: p_filesz vs p_memsz
		if (phdr->p_filesz > phdr->p_memsz) {
			TRACE(("%s: Segment %d p_filesz (%" B_PRIuELFADDR ") > p_memsz (%" B_PRIuELFADDR ").\n",
				baseName, i, phdr->p_filesz, phdr->p_memsz));
			return B_BAD_DATA;
		}

		// II.2: Region Size Calculation (common for text/data logic below)
		elf_xword memsz = phdr->p_memsz;
		elf_xword vaddr_page_offset = phdr->p_vaddr % B_PAGE_SIZE;
		elf_xword required_raw_size;
		size_t segmentSize; // Rounded final size for memory mapping/area

		// const elf_xword MAX_REASONABLE_SEGMENT_MEMSZ_USER = (1024UL * 1024 * 1024 * 2); // 2GB Example
		// if (memsz > MAX_REASONABLE_SEGMENT_MEMSZ_USER) {
		// 	TRACE(("%s: Segment %d p_memsz (%" B_PRIuELFXWORD ") too large.\n", baseName, i, memsz));
		// 	return B_BAD_DATA;
		// }
		if (memsz > (elf_xword)-1 - vaddr_page_offset) {
			TRACE(("%s: Segment %d: p_memsz + (p_vaddr %% PAGE_SIZE) overflows.\n", baseName, i));
			return B_BAD_DATA;
		}
		required_raw_size = memsz + vaddr_page_offset;

		if (required_raw_size == 0 && memsz > 0) {
			TRACE(("%s: Segment %d: required_raw_size is 0 but memsz > 0.\n", baseName, i));
			return B_BAD_DATA;
		}
		if (required_raw_size > (elf_xword)-1 - (B_PAGE_SIZE - 1)) {
			 TRACE(("%s: Segment %d: ROUNDUP calculation for segment size would overflow.\n", baseName, i));
			 return B_BAD_DATA;
		}
		segmentSize = ROUNDUP(required_raw_size, B_PAGE_SIZE);
		if (segmentSize == 0 && required_raw_size > 0) {
			 TRACE(("%s: Segment %d: segmentSize rounded to 0 incorrectly.\n", baseName, i));
			 return B_BAD_DATA;
		}

		// II.3: File Read Boundaries (common check before mapping)
		if (phdr->p_filesz > 0) {
			if (phdr->p_offset < 0) {
				TRACE(("%s: Segment %d p_offset (%" B_PRIuELFADDR ") is negative.\n", baseName, i, phdr->p_offset));
				return B_BAD_DATA;
			}
			if ((unsigned long long)phdr->p_offset + phdr->p_filesz > (unsigned long long)st.st_size) {
				TRACE(("%s: Segment %d file region (offset %" B_PRIuELFADDR ", filesz %" B_PRIuELFADDR ") exceeds file size (%" B_PRIdOFF ").\n",
					baseName, i, phdr->p_offset, phdr->p_filesz, st.st_size));
				return B_BAD_DATA;
			}
		}

		regionAddress = (char *)(ROUNDDOWN(phdr->p_vaddr, B_PAGE_SIZE) + delta);
		originalRegionAddress = regionAddress;

		// II.4: Virtual Address Sanity (check before vm_map_file or create_area)
		// Note: For user images, regionAddress can be large. The OS manages user VAS.
		// The primary check is that regionAddress + segmentSize doesn't wrap addr_t.
		if ((addr_t)regionAddress > (addr_t)-1 - segmentSize) {
			TRACE(("%s: Segment %d: target virtual address range would wrap (addr %p, size %#" B_PRIxSIZE ").\n",
				baseName, i, regionAddress, segmentSize));
			return B_BAD_DATA;
		}


		if (phdr->p_flags & PF_WRITE) {
			// rw/data segment
			size_t file_map_size = ROUNDUP((phdr->p_vaddr % B_PAGE_SIZE) + phdr->p_filesz, B_PAGE_SIZE);
			// file_map_size calculation also needs overflow checks if done from scratch:
			// elf_xword file_raw_size;
			// if (phdr->p_filesz > (elf_xword)-1 - vaddr_page_offset) { /* overflow */ return B_BAD_DATA; }
			// file_raw_size = phdr->p_filesz + vaddr_page_offset;
			// if (file_raw_size > (elf_xword)-1 - (B_PAGE_SIZE-1)) { /* overflow for roundup */ return B_BAD_DATA; }
			// file_map_size = ROUNDUP(file_raw_size, B_PAGE_SIZE);
			// if (file_map_size == 0 && file_raw_size > 0) return B_BAD_DATA;
			// Ensure file_map_size <= segmentSize (as p_filesz <= p_memsz)
			if (file_map_size > segmentSize) { // Should be guaranteed by p_filesz <= p_memsz
				TRACE(("%s: Segment %d data: file_map_size %#" B_PRIxSIZE " > segmentSize %#" B_PRIxSIZE ".\n",
					baseName, i, file_map_size, segmentSize));
				return B_BAD_DATA; // Should not happen if p_filesz <= p_memsz
			}


			snprintf(regionName, B_OS_NAME_LENGTH, "%s_seg%drw", baseName, i);

			id = vm_map_file(team->id, regionName, (void **)&regionAddress,
				addressSpec, file_map_size, // Map only up to file content size initially
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
				REGION_PRIVATE_MAP, false, fd,
				ROUNDDOWN(phdr->p_offset, B_PAGE_SIZE));
			if (id < B_OK) {
				dprintf("%s: error mapping file data for \"%s\": %s!\n", baseName, regionName, strerror(id));
				return B_NOT_AN_EXECUTABLE; // Or id
			}
			mappedAreas[i] = id;

			imageInfo.basic_info.data = regionAddress;
			imageInfo.basic_info.data_size = segmentSize; // Full mem size

			image->data_region.start = (addr_t)regionAddress;
			image->data_region.size = segmentSize; // Full mem size

			// clean garbage brought by mmap for the part of last page not in file
			// This is only for the mapped file part. BSS is handled next.
			if (phdr->p_filesz > 0 && file_map_size > 0) { // Only if there's file content mapped
				addr_t start_zero = (addr_t)regionAddress + vaddr_page_offset + phdr->p_filesz;
				addr_t end_mapped_file_page = (addr_t)regionAddress + file_map_size;
				if (start_zero < end_mapped_file_page) {
					size_t amount_to_zero = end_mapped_file_page - start_zero;
					memset((void *)start_zero, 0, amount_to_zero);
				}
			}


			if (segmentSize > file_map_size) { // If BSS part exists
				size_t bssSize = segmentSize - file_map_size;
				char* bssRegionAddress = regionAddress + file_map_size;

				snprintf(regionName, B_OS_NAME_LENGTH, "%s_bss%d", baseName, i);

				virtual_address_restrictions virtualRestrictions = {};
				virtualRestrictions.address = bssRegionAddress;
				virtualRestrictions.address_specification = B_EXACT_ADDRESS;
				physical_address_restrictions physicalRestrictions = {};
				id = create_area_etc(team->id, regionName, bssSize, B_NO_LOCK,
					B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0, 0, &virtualRestrictions,
					&physicalRestrictions, (void**)&bssRegionAddress); // Re-assigns bssRegionAddress
				if (id < B_OK) {
					dprintf("%s: error allocating bss area for \"%s\": %s!\n", baseName, regionName, strerror(id));
					// mappedAreas[i] is already set, need to clean up if this fails.
					// This function doesn't have goto error paths for cleanup. Consider adding.
					return B_NOT_AN_EXECUTABLE; // Or id
				}
				// mappedAreas needs to track this BSS area too if it needs separate cleanup.
				// Current mappedAreas only tracks one ID per phdr.
			}
		} else { // Text segment (PF_EXECUTE and !PF_WRITE, or PF_EXECUTE and only one exec header)
			snprintf(regionName, B_OS_NAME_LENGTH, "%s_seg%drx", baseName, i);

			// segmentSize is already calculated and validated.
			// p_offset and p_filesz are already validated against st.st_size.
			// Here, vm_map_file maps the whole segmentSize, and OS handles zeroing parts not in file.

			id = vm_map_file(team->id, regionName, (void **)&regionAddress,
				addressSpec, segmentSize, // Map entire segmentSize
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, // Initially writable for relocations
				REGION_PRIVATE_MAP, false, fd,
				ROUNDDOWN(phdr->p_offset, B_PAGE_SIZE));
			if (id < B_OK) {
				dprintf("%s: error mapping file text for \"%s\": %s!\n", baseName, regionName, strerror(id));
				return B_NOT_AN_EXECUTABLE; // Or id
			}
			mappedAreas[i] = id;

			imageInfo.basic_info.text = regionAddress;
			imageInfo.basic_info.text_size = segmentSize;

			image->text_region.start = (addr_t)regionAddress;
			image->text_region.size = segmentSize;
		}

		if (addressSpec != B_EXACT_ADDRESS) {
			addressSpec = B_EXACT_ADDRESS;
			delta = regionAddress - originalRegionAddress;
		}
	}

	image->data_region.delta = delta;
	image->text_region.delta = delta;

	// modify the dynamic ptr by the delta of the regions
	image->dynamic_section += image->text_region.delta;

	status = elf_parse_dynamic_section(image);
	if (status != B_OK)
		return status;

	status = elf_relocate(image, image);
	if (status != B_OK)
		return status;

	// set correct area protection
	for (int i = 0; i < elfHeader.e_phnum; i++) {
		if (mappedAreas[i] == -1)
			continue;

		uint32 protection = 0;
		if (programHeaders[i].p_flags & PF_EXECUTE)
			protection |= B_EXECUTE_AREA;
		if (programHeaders[i].p_flags & PF_WRITE)
			protection |= B_WRITE_AREA;
		if (programHeaders[i].p_flags & PF_READ)
			protection |= B_READ_AREA;

		status = vm_set_area_protection(team->id, mappedAreas[i], protection,
			true);
		if (status != B_OK)
			return status;
	}

	// register the loaded image
	imageInfo.basic_info.type = B_LIBRARY_IMAGE;
	imageInfo.basic_info.device = st.st_dev;
	imageInfo.basic_info.node = st.st_ino;
	strlcpy(imageInfo.basic_info.name, path, sizeof(imageInfo.basic_info.name));

	imageInfo.basic_info.api_version = B_HAIKU_VERSION;
	imageInfo.basic_info.abi = B_HAIKU_ABI;
		// TODO: Get the actual values for the shared object. Currently only
		// the runtime loader is loaded, so this is good enough for the time
		// being.

	imageInfo.text_delta = delta;
	imageInfo.symbol_table = image->syms;
	imageInfo.symbol_hash = image->symhash;
	imageInfo.string_table = image->strtab;

	imageInfo.basic_info.id = register_image(team, &imageInfo,
		sizeof(imageInfo));
	if (imageInfo.basic_info.id >= 0 && team_get_current_team_id() == team->id)
		user_debug_image_created(&imageInfo.basic_info);
		// Don't care, if registering fails. It's not crucial.

	TRACE(("elf_load: done!\n"));

	*entry = elfHeader.e_entry + delta;
	return B_OK;
}


#ifndef ELF32_COMPAT

image_id
load_kernel_add_on(const char *path)
{
	elf_phdr *programHeaders;
	elf_ehdr *elfHeader;
	struct elf_image_info *image;
	const char *fileName;
	void *reservedAddress;
	size_t reservedSize;
	status_t status;
	ssize_t length;
	bool textSectionWritable = false;
	int executableHeaderCount = 0;
	struct stat st;

	TRACE(("elf_load_kspace: entry path '%s'\n", path));

	int fd = _kern_open(-1, path, O_RDONLY, 0);
	if (fd < 0)
		return fd;

	status = _kern_read_stat(fd, NULL, false, &st, sizeof(st));
	if (status != B_OK)
		goto error0;

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

	elfHeader = (elf_ehdr *)malloc(sizeof(*elfHeader));
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

	// Validate ELF program header count and entry size before allocation
	if (elfHeader->e_phnum > 256) { // ELF_MAX_PROGRAM_HEADERS
		TRACE(("%s: ELF has too many program headers (%" B_PRIu16 ").\n", fileName, elfHeader->e_phnum));
		status = B_BAD_DATA;
		goto error2;
	}
	// verify_eheader ensures e_phentsize >= sizeof(elf_phdr).
	// Check for excessively large e_phentsize.
	if (elfHeader->e_phnum > 0 && elfHeader->e_phentsize > (sizeof(elf_phdr) * 4)) { // ELF_MAX_PHENTSIZE_FACTOR
		TRACE(("%s: ELF program header entry size (e_phentsize %" B_PRIu16 ") is unusually large.\n", fileName, elfHeader->e_phentsize));
		status = B_BAD_DATA;
		goto error2;
	}

	size_t programHeadersSize = 0;
	if (elfHeader->e_phnum > 0) {
		if (elfHeader->e_phentsize > SIZE_MAX / elfHeader->e_phnum) {
			TRACE(("%s: ELF program headers size calculation would overflow (e_phnum %" B_PRIu16 ", e_phentsize %" B_PRIu16 ").\n",
				fileName, elfHeader->e_phnum, elfHeader->e_phentsize));
			status = B_BAD_DATA;
			goto error2;
		}
		programHeadersSize = (size_t)elfHeader->e_phnum * elfHeader->e_phentsize;

		if (programHeadersSize == 0) { // Should not happen if e_phnum > 0 and e_phentsize is validated
			TRACE(("%s: Calculated programHeadersSize is 0 with e_phnum > 0.\n", fileName));
			status = B_BAD_DATA;
			goto error2;
		}
		if (programHeadersSize > (64 * 1024)) { // ELF_MAX_TOTAL_PH_SIZE
			TRACE(("%s: ELF program headers total size (%" B_PRIuSIZE ") is too large.\n", fileName, programHeadersSize));
			status = B_BAD_DATA;
			goto error2;
		}
	}

	programHeaders = (elf_phdr *)malloc(programHeadersSize);
	if (programHeaders == NULL) {
		dprintf("%s: error allocating space for program headers (size %" B_PRIuSIZE ")\n", fileName, programHeadersSize);
		status = B_NO_MEMORY;
		goto error2;
	}

	if (elfHeader->e_phnum > 0) {
		// Validate read offset and size against file size and SSIZE_MAX
		if (elfHeader->e_phoff < 0) { // elf_off is off_t, can be negative
			TRACE(("%s: ELF program header offset (e_phoff %" B_PRIdOFF ") is negative.\n", fileName, elfHeader->e_phoff));
			status = B_BAD_DATA;
			goto error3;
		}
		if ((unsigned long long)elfHeader->e_phoff + programHeadersSize > (unsigned long long)st.st_size) {
			TRACE(("%s: ELF program headers (offset %" B_PRIdOFF ", size %" B_PRIuSIZE ") exceed file size (%" B_PRIdOFF ").\n",
				fileName, elfHeader->e_phoff, programHeadersSize, st.st_size));
			status = B_BAD_DATA;
			goto error3;
		}
		if (programHeadersSize > SSIZE_MAX) {
			TRACE(("%s: ELF program headers size (%" B_PRIuSIZE ") exceeds SSIZE_MAX for read.\n", fileName, programHeadersSize));
			status = B_BAD_DATA;
			goto error3;
		}

		TRACE(("reading in program headers at offset %" B_PRIdOFF ", size %" B_PRIuSIZE "\n",
			elfHeader->e_phoff, programHeadersSize));

		length = _kern_read(fd, elfHeader->e_phoff, programHeaders, programHeadersSize);
		if (length < B_OK) {
			status = length;
			TRACE(("%s: error reading in program headers: %s\n", fileName, strerror(status)));
			goto error3;
		}
		if ((size_t)length != programHeadersSize) {
			TRACE(("%s: short read while reading in program headers (read %" B_PRIdSSIZE ", expected %" B_PRIuSIZE ").\n",
				fileName, length, programHeadersSize));
			status = B_ERROR;
			goto error3;
		}
	}

	// determine how much space we need for all loaded segments

	reservedSize = 0;
	// 'length' here is actually accumulating total rounded segment sizes, perhaps rename for clarity later.
	size_t totalSegmentsMappedSize = 0;


	for (int32 i = 0; i < elfHeader->e_phnum; i++) {
		elf_phdr* phdr = &programHeaders[i];
		size_t currentSegmentRoundedSize;
		size_t segmentVirtualEndRounded;

		if (phdr->p_type != PT_LOAD)
			continue;

		// Validate p_filesz vs p_memsz (as per Part II.1)
		if (phdr->p_filesz > phdr->p_memsz) {
			TRACE(("%s: Segment %" B_PRId32 " p_filesz (%" B_PRIuELFADDR ") > p_memsz (%" B_PRIuELFADDR ").\n",
				fileName, i, phdr->p_filesz, phdr->p_memsz));
			status = B_BAD_DATA;
			goto error3; // Ensure programHeaders is freed
		}

		// Validate for 'totalSegmentsMappedSize' accumulation (as per Part II.2)
		elf_xword current_memsz = phdr->p_memsz;
		elf_xword current_vaddr_offset = phdr->p_vaddr % B_PAGE_SIZE;
		elf_xword current_required_raw_size;

		if (current_memsz > (elf_xword)-1 - current_vaddr_offset) { // Using (elf_xword)-1 as MAX_elf_xword
			TRACE(("%s: Segment %" B_PRId32 " p_memsz + (p_vaddr %% PAGE_SIZE) overflows for mapped size calc.\n", fileName, i));
			status = B_BAD_DATA;
			goto error3;
		}
		current_required_raw_size = current_memsz + current_vaddr_offset;

		if (current_required_raw_size > (elf_xword)-1 - (B_PAGE_SIZE - 1)) {
			 TRACE(("%s: Segment %" B_PRId32 " ROUNDUP calculation for mapped size would overflow.\n", fileName, i));
			 status = B_BAD_DATA;
			 goto error3;
		}
		currentSegmentRoundedSize = ROUNDUP(current_required_raw_size, B_PAGE_SIZE);
		if (currentSegmentRoundedSize == 0 && current_required_raw_size > 0) {
			 TRACE(("%s: Segment %" B_PRId32 " mapped size rounded to 0 incorrectly.\n", fileName, i));
			 status = B_BAD_DATA;
			 goto error3;
		}
		if (currentSegmentRoundedSize > (elf_xword)-1 - totalSegmentsMappedSize) { // Check before adding to total
			TRACE(("%s: Accumulating segment mapped sizes would overflow.\n", fileName));
			status = B_BAD_DATA;
			goto error3;
		}
		totalSegmentsMappedSize += currentSegmentRoundedSize;


		// Validate for 'reservedSize' (highest extent) calculation (Part II.5)
		if (phdr->p_vaddr > (elf_addr)-1 - phdr->p_memsz) {
			TRACE(("%s: Segment %" B_PRId32 " p_vaddr + p_memsz overflows for reservedSize calc.\n", fileName, i));
			status = B_BAD_DATA;
			goto error3;
		}
		elf_addr segmentActualEnd = phdr->p_vaddr + phdr->p_memsz;

		if (segmentActualEnd > (elf_addr)-1 - (B_PAGE_SIZE - 1) && segmentActualEnd !=0) {
			TRACE(("%s: Segment %" B_PRId32 " ROUNDUP for segmentActualEnd would overflow (reservedSize calc).\n", fileName, i));
			status = B_BAD_DATA;
			goto error3;
		}
		segmentVirtualEndRounded = ROUNDUP(segmentActualEnd, B_PAGE_SIZE);
		if (segmentVirtualEndRounded == 0 && segmentActualEnd > 0) {
			TRACE(("%s: Segment %" B_PRId32 " segment end rounded to 0 incorrectly (reservedSize calc).\n", fileName, i));
			status = B_BAD_DATA;
			goto error3;
		}

		if (segmentVirtualEndRounded > reservedSize)
			reservedSize = segmentVirtualEndRounded;

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

		TRACE(("looking at program header %" B_PRId32 "\n", i));

		switch (programHeaders[i].p_type) {
			case PT_LOAD:
				break;
			case PT_DYNAMIC:
				image->dynamic_section = programHeaders[i].p_vaddr;
				image->dynamic_section_memsz = programHeaders[i].p_memsz; // Store memsz
				continue;
			case PT_INTERP:
				// should check here for appropriate interpreter
				continue;
			case PT_PHDR:
			case PT_STACK:
				// we don't use it
				continue;
			case PT_EH_FRAME:
				// not implemented yet, but can be ignored
				continue;
			case PT_ARM_UNWIND:
				continue;
			case PT_RISCV_ATTRIBUTES:
				// TODO: check ABI compatibility attributes
				continue;
			default:
				dprintf("%s: unhandled pheader type %#" B_PRIx32 "\n", fileName,
					programHeaders[i].p_type);
				continue;
		}

		// we're here, so it must be a PT_LOAD segment
		elf_phdr* phdr = &programHeaders[i];

		// II.2 Region Size Calculation
		elf_xword memsz = phdr->p_memsz;
		elf_xword vaddr_page_offset = phdr->p_vaddr % B_PAGE_SIZE;
		elf_xword required_raw_size;

		// const elf_xword MAX_REASONABLE_SEGMENT_MEMSZ_KADDON = (256 * 1024 * 1024); // Example
		// if (memsz > MAX_REASONABLE_SEGMENT_MEMSZ_KADDON) {
		// TRACE(("%s: Segment %" B_PRId32 " p_memsz (%" B_PRIuELFXWORD ") too large for kernel add-on.\n", fileName, i, memsz));
		// status = B_BAD_DATA;
		// goto error4;
		// }

		if (memsz > (elf_xword)-1 - vaddr_page_offset) {
			TRACE(("%s: Segment %" B_PRId32 " p_memsz + (p_vaddr %% PAGE_SIZE) overflows.\n", fileName, i));
			status = B_BAD_DATA;
			goto error4;
		}
		required_raw_size = memsz + vaddr_page_offset;

		if (required_raw_size == 0 && memsz > 0) {
			TRACE(("%s: Segment %" B_PRId32 " required_raw_size is 0 but memsz > 0.\n", fileName, i));
			status = B_BAD_DATA;
			goto error4;
		}
		if (required_raw_size > (elf_xword)-1 - (B_PAGE_SIZE - 1)) {
			 TRACE(("%s: Segment %" B_PRId32 " ROUNDUP calculation for segment size would overflow.\n", fileName, i));
			 status = B_BAD_DATA;
			 goto error4;
		}
		size_t segmentSize = ROUNDUP(required_raw_size, B_PAGE_SIZE);
		if (segmentSize == 0 && required_raw_size > 0) {
			 TRACE(("%s: Segment %" B_PRId32 " segmentSize rounded to 0 incorrectly.\n", fileName, i));
			 status = B_BAD_DATA;
			 goto error4;
		}


		// Usually add-ons have two PT_LOAD headers: one for .data one or .text.
		// x86 and PPC may differ in permission bits for .data's PT_LOAD header
		// x86 is usually RW, PPC is RWE

		// Some add-ons may have .text and .data concatenated in a single
		// PT_LOAD RWE header and we must map that to .text.
		if (phdr->IsReadWrite()
			&& (!phdr->IsExecutable()
				|| executableHeaderCount > 1)) {
			// this is the writable segment
			if (image->data_region.size != 0) {
				// we've already created this segment
				continue;
			}
			region = &image->data_region;
			snprintf(regionName, B_OS_NAME_LENGTH, "%s_data", fileName);
		} else if (phdr->IsExecutable()) {
			// this is the non-writable segment
			if (image->text_region.size != 0) {
				// we've already created this segment
				continue;
			}
			region = &image->text_region;
			textSectionWritable = phdr->IsReadWrite();
			snprintf(regionName, B_OS_NAME_LENGTH, "%s_text", fileName);
		} else {
			dprintf("%s: weird program header flags %#" B_PRIx32 "\n", fileName,
				phdr->p_flags);
			continue;
		}

		region->start = (addr_t)reservedAddress + ROUNDDOWN(phdr->p_vaddr, B_PAGE_SIZE);
		region->size = segmentSize; // Use validated segmentSize

		// II.4 Virtual Address Sanity
		if (region->start > (addr_t)-1 - region->size) { // Check before create_area uses start+size
			TRACE(("%s: Segment %" B_PRId32 " virtual address range (start %#" B_PRIxADDR ", size %#" B_PRIxSIZE ") would wrap for area creation.\n",
				fileName, i, region->start, region->size));
			status = B_BAD_DATA;
			goto error4;
		}

		region->id = create_area(regionName, (void **)&region->start,
			B_EXACT_ADDRESS, region->size, B_FULL_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (region->id < B_OK) {
			dprintf("%s: error allocating area \"%s\": %s\n", fileName, regionName,
				strerror(region->id));
			status = region->id; // Propagate actual error
			goto error4;
		}
		region->delta = -ROUNDDOWN(phdr->p_vaddr, B_PAGE_SIZE);

		TRACE(("elf_load_kspace: created area \"%s\" (id %" B_PRId32 ") at %p, size %#" B_PRIxSIZE "\n",
			regionName, region->id, (void *)region->start, region->size));

		// II.3 File Read Boundaries
		if (phdr->p_filesz > 0) {
			if (phdr->p_offset < 0) {
				TRACE(("%s: Segment %" B_PRId32 " p_offset (%" B_PRIuELFADDR ") is negative.\n", fileName, i, phdr->p_offset));
				status = B_BAD_DATA;
				goto error5; // Error after area creation needs to clean up area via error5
			}
			if ((unsigned long long)phdr->p_offset + phdr->p_filesz > (unsigned long long)st.st_size) {
				TRACE(("%s: Segment %" B_PRId32 " file region (offset %" B_PRIuELFADDR ", filesz %" B_PRIuELFADDR ") exceeds file size (%" B_PRIdOFF ").\n",
					fileName, i, phdr->p_offset, phdr->p_filesz, st.st_size));
				status = B_BAD_DATA;
				goto error5;
			}
			// Ensure read does not exceed allocated region size (vaddr_page_offset + p_filesz vs region->size)
			if (vaddr_page_offset + phdr->p_filesz > region->size) {
				TRACE(("%s: Segment %" B_PRId32 " file content (vaddr_offset %" B_PRIuELFXWORD " + filesz %" B_PRIuELFADDR ") exceeds allocated region size (%" B_PRIuSIZE ").\n",
					fileName, i, vaddr_page_offset, phdr->p_filesz, region->size));
				status = B_BAD_DATA;
				goto error5;
			}


			length = _kern_read(fd, phdr->p_offset,
				(void *)(region->start + vaddr_page_offset), phdr->p_filesz);
			if (length < B_OK) {
				status = length;
				dprintf("%s: error reading in segment %" B_PRId32 " of \"%s\": %s\n", fileName, i, regionName, strerror(status));
				goto error5;
			}
			if ((elf_xword)length != phdr->p_filesz) {
				dprintf("%s: short read for segment %" B_PRId32 " of \"%s\" (read %" B_PRIdSSIZE ", expected %" B_PRIuELFADDR ").\n",
					fileName, i, regionName, length, phdr->p_filesz);
				status = B_ERROR;
				goto error5;
			}
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

	status = elf_relocate(image, sKernelImage);
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

	if (sLoadElfSymbols)
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
	elf_sym* symbolTable = (elf_sym*)malloc(0);
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
	elf_sym* symbolTable = (elf_sym*)realloc(
		(elf_sym*)image->debug_symbols, sizeof(elf_sym) * symbolCount);
	if (symbolTable == NULL)
		return B_NO_MEMORY;
	image->debug_symbols = symbolTable;

	// enter the symbol
	elf_sym& symbol = symbolTable[symbolCount - 1];
	symbol.SetInfo(STB_GLOBAL,
		type == B_SYMBOL_TYPE_DATA ? STT_OBJECT : STT_FUNC);
	symbol.st_name = stringIndex;
	symbol.st_value = address;
	symbol.st_size = size;
	symbol.st_other = 0;
	symbol.st_shndx = 0;
	image->num_debug_symbols++;

	return B_OK;
}


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
elf_read_kernel_image_symbols(image_id id, elf_sym* symbolTable,
	int32* _symbolCount, char* stringTable, size_t* _stringTableSize,
	addr_t* _imageDelta, bool kernel)
{
	// check params
	if (_symbolCount == NULL || _stringTableSize == NULL)
		return B_BAD_VALUE;
	if (!kernel) {
		if (!IS_USER_ADDRESS(_symbolCount) || !IS_USER_ADDRESS(_stringTableSize)
			|| (_imageDelta != NULL && !IS_USER_ADDRESS(_imageDelta))
			|| (symbolTable != NULL && !IS_USER_ADDRESS(symbolTable))
			|| (stringTable != NULL && !IS_USER_ADDRESS(stringTable))) {
			return B_BAD_ADDRESS;
		}
	}

	// get buffer sizes
	int32 maxSymbolCount;
	size_t maxStringTableSize;
	if (kernel) {
		maxSymbolCount = *_symbolCount;
		maxStringTableSize = *_stringTableSize;
	} else {
		if (user_memcpy(&maxSymbolCount, _symbolCount, sizeof(maxSymbolCount))
				!= B_OK
			|| user_memcpy(&maxStringTableSize, _stringTableSize,
				sizeof(maxStringTableSize)) != B_OK) {
			return B_BAD_ADDRESS;
		}
	}

	// find the image
	MutexLocker _(sImageMutex);
	struct elf_image_info* image = find_image(id);
	if (image == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the tables and infos
	addr_t imageDelta = image->text_region.delta;
	const elf_sym* symbols;
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
		if (kernel) {
			memcpy(symbolTable, symbols, sizeof(elf_sym) * symbolsToCopy);
		} else if (user_memcpy(symbolTable, symbols,
				sizeof(elf_sym) * symbolsToCopy) != B_OK) {
			return B_BAD_ADDRESS;
		}
	}

	// copy string table
	size_t stringsToCopy = min_c(stringTableSize, maxStringTableSize);
	if (stringTable != NULL && stringsToCopy > 0) {
		if (kernel) {
			memcpy(stringTable, strings, stringsToCopy);
		} else {
			if (user_memcpy(stringTable, strings, stringsToCopy)
					!= B_OK) {
				return B_BAD_ADDRESS;
			}
		}
	}

	// copy sizes
	if (kernel) {
		*_symbolCount = symbolCount;
		*_stringTableSize = stringTableSize;
		if (_imageDelta != NULL)
			*_imageDelta = imageDelta;
	} else {
		if (user_memcpy(_symbolCount, &symbolCount, sizeof(symbolCount)) != B_OK
			|| user_memcpy(_stringTableSize, &stringTableSize,
					sizeof(stringTableSize)) != B_OK
			|| (_imageDelta != NULL && user_memcpy(_imageDelta, &imageDelta,
					sizeof(imageDelta)) != B_OK)) {
			return B_BAD_ADDRESS;
		}
	}

	return B_OK;
}


status_t
elf_init(kernel_args* args)
{
	struct preloaded_image* image;

	image_init();

	if (void* handle = load_driver_settings("kernel")) {
		sLoadElfSymbols = get_driver_boolean_parameter(handle, "load_symbols",
			false, false);

		unload_driver_settings(handle);
	}

	sImagesHash = new(std::nothrow) ImageHash();
	if (sImagesHash == NULL)
		return B_NO_MEMORY;
	status_t init = sImagesHash->Init(IMAGE_HASH_SIZE);
	if (init != B_OK)
		return init;

	// Build a image structure for the kernel, which has already been loaded.
	// The preloaded_images were already prepared by the VM.
	image = args->kernel_image;
	if (insert_preloaded_image(static_cast<preloaded_elf_image *>(image),
			true) < B_OK)
		panic("could not create kernel image.\n");

	// Build image structures for all preloaded images.
	for (image = args->preloaded_images; image != NULL; image = image->next)
		insert_preloaded_image(static_cast<preloaded_elf_image *>(image),
			false);

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
_user_read_kernel_image_symbols(image_id id, elf_sym* symbolTable,
	int32* _symbolCount, char* stringTable, size_t* _stringTableSize,
	addr_t* _imageDelta)
{
	return elf_read_kernel_image_symbols(id, symbolTable, _symbolCount,
		stringTable, _stringTableSize, _imageDelta, false);
}

#endif // ELF32_COMPAT

[end of src/system/kernel/elf.cpp]
