/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"

#include <OS.h>

#include <elf32.h>
#include <user_runtime.h>
#include <syscalls.h>
#include <vm_types.h>
#include <arch/cpu.h>
#include <sem.h>
#include <runtime_loader.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


//#define TRACE_RLD
#ifdef TRACE_RLD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// ToDo: implement better locking strategy
// ToDo: implement lazy binding

#define	PAGE_MASK (B_PAGE_SIZE - 1)

#define	PAGE_OFFSET(x) ((x) & (PAGE_MASK))
#define	PAGE_BASE(x) ((x) & ~(PAGE_MASK))
#define TO_PAGE_SIZE(x) ((x + (PAGE_MASK)) & ~(PAGE_MASK))

#define RLD_PROGRAM_BASE 0x00200000
	/* keep in sync with app ldscript */

enum {
	RFLAG_RW					= 0x0001,
	RFLAG_ANON					= 0x0002,

	RFLAG_TERMINATED			= 0x0200,
	RFLAG_INITIALIZED			= 0x0400,
	RFLAG_SYMBOLIC				= 0x0800,
	RFLAG_RELOCATED				= 0x1000,
	RFLAG_PROTECTED				= 0x2000,
	RFLAG_DEPENDENCIES_LOADED	= 0x4000,
	RFLAG_REMAPPED				= 0x8000
};


#define IMAGE_TYPE_TO_MASK(type)	(1 << ((type) - 1))
#define ALL_IMAGE_TYPES				(IMAGE_TYPE_TO_MASK(B_APP_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_LIBRARY_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_ADD_ON_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_SYSTEM_IMAGE))
#define APP_OR_LIBRARY_TYPE			(IMAGE_TYPE_TO_MASK(B_APP_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_LIBRARY_IMAGE))


static image_queue_t sLoadedImages = {0, 0};
static image_queue_t sLoadingImages = {0, 0};
static image_queue_t sDisposableImages = {0, 0};
static uint32 sLoadedImageCount = 0;
static image_t *sProgramImage;

// a recursive lock
static sem_id rld_sem;
static thread_id rld_sem_owner;
static int32 rld_sem_count;


#ifdef TRACE_RLD
#	define FATAL(x...) dprintf("runtime_loader: " x);

void
dprintf(const char *format, ...)
{
	char buffer[1024];

	va_list list;
	va_start(list, format);
	
	vsnprintf(buffer, sizeof(buffer), format, list);
	_kern_debug_output(buffer);

	va_end(list);
}
#else
#	define FATAL(x...) printf("runtime_loader: " x);
#endif


static void
rld_unlock()
{
	if (rld_sem_count-- == 1) {
		rld_sem_owner = -1;
		release_sem(rld_sem);
	}
}


static void
rld_lock()
{
	thread_id self = find_thread(NULL);
	if (self != rld_sem_owner) {
		acquire_sem(rld_sem);
		rld_sem_owner = self;
	}
	rld_sem_count++;
}


static void
enqueue_image(image_queue_t *queue, image_t *image)
{
	image->next = 0;

	image->prev = queue->tail;
	if (queue->tail)
		queue->tail->next = image;

	queue->tail = image;
	if (!queue->head)
		queue->head = image;
}


static void
dequeue_image(image_queue_t *queue, image_t *image)
{
	if (image->next)
		image->next->prev = image->prev;
	else
		queue->tail = image->prev;

	if (image->prev)
		image->prev->next = image->next;
	else
		queue->head = image->next;

	image->prev = 0;
	image->next = 0;
}


static uint32
elf_hash(const uchar *name)
{
	uint32 hash = 0;
	uint32 temp;

	while (*name) {
		hash = (hash << 4) + *name++;
		if ((temp = hash & 0xf0000000)) {
			hash ^= temp >> 24;
		}
		hash &= ~temp;
	}
	return hash;
}


static image_t *
find_image_in_queue(image_queue_t *queue, const char *name, bool isPath,
	uint32 typeMask)
{
	image_t *iter;

	if (isPath) {
		for (iter = queue->head; iter; iter = iter->next) {
			if (strncmp(iter->path, name, sizeof(iter->path)) == 0
				&& typeMask & IMAGE_TYPE_TO_MASK(iter->type)) {
				return iter;
			}
		}
	} else {
		for (iter = queue->head; iter; iter = iter->next) {
			if (strncmp(iter->name, name, sizeof(iter->path)) == 0
				&& typeMask & IMAGE_TYPE_TO_MASK(iter->type)) {
				return iter;
			}
		}
	}

	return NULL;
}


static image_t *
find_image(char const *name, uint32 typeMask)
{
	bool isPath = (strchr(name, '/') != NULL);
	image_t *image;

	image = find_image_in_queue(&sLoadedImages, name, isPath, typeMask);

	if (!image)
		image = find_image_in_queue(&sLoadingImages, name, isPath, typeMask);

	return image;
}


static image_t *
find_loaded_image_by_id(image_id id)
{
	image_t *image;

	for (image = sLoadedImages.head; image; image = image->next) {
		if (image->id == id)
			return image;
	}

	return NULL;
}


static status_t
parse_elf_header(struct Elf32_Ehdr *eheader, int32 *_pheaderSize, int32 *_sheaderSize)
{
	if (memcmp(eheader->e_ident, ELF_MAGIC, 4) != 0)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_ident[4] != ELFCLASS32)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_phoff == 0)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_phentsize < sizeof(struct Elf32_Phdr))
		return B_NOT_AN_EXECUTABLE;

	*_pheaderSize = eheader->e_phentsize * eheader->e_phnum;
	*_sheaderSize = eheader->e_shentsize * eheader->e_shnum;

	return *_pheaderSize > 0 && *_sheaderSize > 0 ? B_OK : B_NOT_AN_EXECUTABLE;
}


static int32
count_regions(char const *buff, int phnum, int phentsize)
{
	struct Elf32_Phdr *pheaders;
	int32 count = 0;
	int i;

	for (i = 0; i < phnum; i++) {
		pheaders = (struct Elf32_Phdr *)(buff + i * phentsize);

		switch (pheaders->p_type) {
			case PT_NULL:
				/* NOP header */
				break;
			case PT_LOAD:
				count += 1;
				if (pheaders->p_memsz != pheaders->p_filesz) {
					addr_t A = TO_PAGE_SIZE(pheaders->p_vaddr + pheaders->p_memsz);
					addr_t B = TO_PAGE_SIZE(pheaders->p_vaddr + pheaders->p_filesz);

					if (A != B)
						count += 1;
				}
				break;
			case PT_DYNAMIC:
				/* will be handled at some other place */
				break;
			case PT_INTERP:
				/* should check here for appropiate interpreter */
				break;
			case PT_NOTE:
				/* unsupported */
				break;
			case PT_SHLIB:
				/* undefined semantics */
				break;
			case PT_PHDR:
				/* we don't use it */
				break;
			default:
				FATAL("unhandled pheader type 0x%lx\n", pheaders[i].p_type);
				return B_BAD_DATA;
		}
	}

	return count;
}


/*
 * create_image() & destroy_image()
 *
 * 	Create and destroy image_t structures. The destroyer makes sure that the
 * 	memory buffers are full of garbage before freeing.
 */

static image_t *
create_image(const char *name, const char *path, int num_regions)
{
	size_t allocSize;
	image_t *image;
	const char *lastSlash;

	allocSize = sizeof(image_t) + (num_regions - 1) * sizeof(elf_region_t);

	image = rldalloc(allocSize);
	memset(image, 0, allocSize);

	strlcpy(image->path, path, sizeof(image->path));

	// Make the last component of the supplied name the image name.
	// If present, DT_SONAME will replace this name.
	if ((lastSlash = strrchr(name, '/')))
		strlcpy(image->name, lastSlash + 1, sizeof(image->name));
	else
		strlcpy(image->name, name, sizeof(image->name));

	image->ref_count = 1;
	image->num_regions = num_regions;

	return image;
}


static void
delete_image_struct(image_t *image)
{
#ifdef DEBUG
	size_t size = sizeof(image_t) + (image->num_regions - 1) * sizeof(elf_region_t);
	memset(image->needed, 0xa5, sizeof(image->needed[0]) * image->num_needed);
#endif
	rldfree(image->needed);

#ifdef DEBUG
	memset(image, 0xa5, size);
#endif
	rldfree(image);
}


static void
delete_image(image_t *image)
{
	_kern_unregister_image(image->id);
		// registered in load_container()

	delete_image_struct(image);
}


static status_t
parse_program_headers(image_t *image, char *buff, int phnum, int phentsize)
{
	struct Elf32_Phdr *pheader;
	int regcount;
	int i;

	regcount = 0;
	for (i = 0; i < phnum; i++) {
		pheader = (struct Elf32_Phdr *)(buff + i * phentsize);

		switch (pheader->p_type) {
			case PT_NULL:
				/* NOP header */
				break;
			case PT_LOAD:
				if (pheader->p_memsz == pheader->p_filesz) {
					/*
					 * everything in one area
					 */
					image->regions[regcount].start = pheader->p_vaddr;
					image->regions[regcount].size = pheader->p_memsz;
					image->regions[regcount].vmstart = PAGE_BASE(pheader->p_vaddr);
					image->regions[regcount].vmsize = TO_PAGE_SIZE(pheader->p_memsz
						+ PAGE_OFFSET(pheader->p_vaddr));
					image->regions[regcount].fdstart = pheader->p_offset;
					image->regions[regcount].fdsize = pheader->p_filesz;
					image->regions[regcount].delta = 0;
					image->regions[regcount].flags = 0;
					if (pheader->p_flags & PF_WRITE) {
						// this is a writable segment
						image->regions[regcount].flags |= RFLAG_RW;
					}
				} else {
					/*
					 * may require splitting
					 */
					addr_t A = TO_PAGE_SIZE(pheader->p_vaddr + pheader->p_memsz);
					addr_t B = TO_PAGE_SIZE(pheader->p_vaddr + pheader->p_filesz);

					image->regions[regcount].start = pheader->p_vaddr;
					image->regions[regcount].size = pheader->p_filesz;
					image->regions[regcount].vmstart = PAGE_BASE(pheader->p_vaddr);
					image->regions[regcount].vmsize = TO_PAGE_SIZE(pheader->p_filesz
						+ PAGE_OFFSET(pheader->p_vaddr));
					image->regions[regcount].fdstart = pheader->p_offset;
					image->regions[regcount].fdsize = pheader->p_filesz;
					image->regions[regcount].delta = 0;
					image->regions[regcount].flags = 0;
					if (pheader->p_flags & PF_WRITE) {
						// this is a writable segment
						image->regions[regcount].flags |= RFLAG_RW;
					}

					if (A != B) {
						/*
						 * yeah, it requires splitting
						 */
						regcount += 1;
						image->regions[regcount].start = pheader->p_vaddr;
						image->regions[regcount].size = pheader->p_memsz - pheader->p_filesz;
						image->regions[regcount].vmstart = image->regions[regcount-1].vmstart + image->regions[regcount-1].vmsize;
						image->regions[regcount].vmsize = TO_PAGE_SIZE(pheader->p_memsz + PAGE_OFFSET(pheader->p_vaddr))
							- image->regions[regcount-1].vmsize;
						image->regions[regcount].fdstart = 0;
						image->regions[regcount].fdsize = 0;
						image->regions[regcount].delta = 0;
						image->regions[regcount].flags = RFLAG_ANON;
						if (pheader->p_flags & PF_WRITE) {
							// this is a writable segment
							image->regions[regcount].flags |= RFLAG_RW;
						}
					}
				}
				regcount += 1;
				break;
			case PT_DYNAMIC:
				image->dynamic_ptr = pheader->p_vaddr;
				break;
			case PT_INTERP:
				/* should check here for appropiate interpreter */
				break;
			case PT_NOTE:
				/* unsupported */
				break;
			case PT_SHLIB:
				/* undefined semantics */
				break;
			case PT_PHDR:
				/* we don't use it */
				break;
			default:
				FATAL("unhandled pheader type 0x%lx\n", pheader[i].p_type);
				return B_BAD_DATA;
		}
	}

	return B_OK;
}


static bool
assert_dynamic_loadable(image_t *image)
{
	uint32 i;

	if (!image->dynamic_ptr)
		return true;

	for (i = 0; i < image->num_regions; i++) {
		if (image->dynamic_ptr >= image->regions[i].start
			&& image->dynamic_ptr < image->regions[i].start + image->regions[i].size)
			return true;
	}

	return false;
}


/**	This function will change the protection of all read-only segments
 *	to really be read-only.
 *	The areas have to be read/write first, so that they can be relocated.
 */

static void
remap_images(void)
{
	image_t *image;
	uint32 i;

	for (image = sLoadedImages.head; image != NULL; image = image->next) {
		for (i = 0; i < image->num_regions; i++) {
			if ((image->regions[i].flags & RFLAG_RW) == 0
				&& (image->regions[i].flags & RFLAG_REMAPPED) == 0) {
				// we only need to do this once, so we remember those we've already mapped
				if (_kern_set_area_protection(image->regions[i].id,
						B_READ_AREA | B_EXECUTE_AREA) == B_OK)
					image->regions[i].flags |= RFLAG_REMAPPED;
			}
		}
	}
}


static status_t
map_image(int fd, char const *path, image_t *image, bool fixed)
{
	status_t status = B_OK;
	const char *baseName;
	uint32 i;

	(void)(fd);

	// cut the file name from the path as base name for the created areas
	baseName = strrchr(path, '/');
	if (baseName != NULL)
		baseName++;
	else
		baseName = path;

	for (i = 0; i < image->num_regions; i++) {
		char regionName[B_OS_NAME_LENGTH];
		addr_t loadAddress;
		uint32 addressSpecifier;

		// for BeOS compatibility: if we load an old BeOS executable, we
		// have to relocate it, if possible - we recognize it because the
		// vmstart is set to 0 (hopefully always)
		if (fixed && image->regions[i].vmstart == 0)
			fixed = false;

		snprintf(regionName, sizeof(regionName), "%s_seg%lu%s",
			baseName, i, (image->regions[i].flags & RFLAG_RW) ? "rw" : "ro");

		if (image->dynamic_ptr && !fixed) {
			// relocatable image... we can afford to place wherever
			if (i == 0) {
				// but only the first segment gets a free ride
				loadAddress = RLD_PROGRAM_BASE;
				addressSpecifier = B_BASE_ADDRESS;
			} else {
				loadAddress = image->regions[i].vmstart + image->regions[i-1].delta;
				addressSpecifier = B_EXACT_ADDRESS;
			}
		} else {
			// not relocatable, put it where it asks or die trying
			loadAddress = image->regions[i].vmstart;
			addressSpecifier = B_EXACT_ADDRESS;
		}

		if (image->regions[i].flags & RFLAG_ANON) {
			image->regions[i].id = _kern_create_area(regionName, (void **)&loadAddress,
				addressSpecifier, image->regions[i].vmsize, B_NO_LOCK,
				B_READ_AREA | B_WRITE_AREA);

			if (image->regions[i].id < 0) {
				status = image->regions[i].id;
				goto error;
			}

			image->regions[i].delta = loadAddress - image->regions[i].vmstart;
			image->regions[i].vmstart = loadAddress;
		} else {
			image->regions[i].id = sys_vm_map_file(regionName, (void **)&loadAddress,
				addressSpecifier, image->regions[i].vmsize, B_READ_AREA | B_WRITE_AREA,
				REGION_PRIVATE_MAP, path, PAGE_BASE(image->regions[i].fdstart));

			if (image->regions[i].id < 0) {
				status = image->regions[i].id;
				goto error;
			}

			TRACE(("\"%s\" at %p, 0x%lx bytes (%s)\n", path,
				(void *)loadAddress, image->regions[i].vmsize,
				image->regions[i].flags & RFLAG_RW ? "rw" : "read-only"));

			image->regions[i].delta = loadAddress - image->regions[i].vmstart;
			image->regions[i].vmstart = loadAddress;

			// handle trailer bits in data segment
			if (image->regions[i].flags & RFLAG_RW) {
				addr_t startClearing;
				addr_t toClear;

				startClearing = image->regions[i].vmstart
					+ PAGE_OFFSET(image->regions[i].start)
					+ image->regions[i].size;
				toClear = image->regions[i].vmsize
					- PAGE_OFFSET(image->regions[i].start)
					- image->regions[i].size;

				TRACE(("cleared 0x%lx and the following 0x%lx bytes\n", startClearing, toClear));
				memset((void *)startClearing, 0, toClear);
			}
		}
	}

	if (image->dynamic_ptr)
		image->dynamic_ptr += image->regions[0].delta;

	return B_OK;

error:
	return status;
}


static void
unmap_image(image_t *image)
{
	uint32 i;

	for (i = 0; i < image->num_regions; i++) {
		_kern_delete_area(image->regions[i].id);

		image->regions[i].id = -1;
	}
}


static bool
parse_dynamic_segment(image_t *image)
{
	struct Elf32_Dyn *d;
	int i;
	int sonameOffset = -1;

	image->symhash = 0;
	image->syms = 0;
	image->strtab = 0;

	d = (struct Elf32_Dyn *)image->dynamic_ptr;
	if (!d)
		return true;

	for (i = 0; d[i].d_tag != DT_NULL; i++) {
		switch (d[i].d_tag) {
			case DT_NEEDED:
				image->num_needed += 1;
				break;
			case DT_HASH:
				image->symhash = (uint32 *)(d[i].d_un.d_ptr + image->regions[0].delta);
				break;
			case DT_STRTAB:
				image->strtab = (char *)(d[i].d_un.d_ptr + image->regions[0].delta);
				break;
			case DT_SYMTAB:
				image->syms = (struct Elf32_Sym *)(d[i].d_un.d_ptr + image->regions[0].delta);
				break;
			case DT_REL:
				image->rel = (struct Elf32_Rel *)(d[i].d_un.d_ptr + image->regions[0].delta);
				break;
			case DT_RELSZ:
				image->rel_len = d[i].d_un.d_val;
				break;
			case DT_RELA:
				image->rela = (struct Elf32_Rela *)(d[i].d_un.d_ptr + image->regions[0].delta);
				break;
			case DT_RELASZ:
				image->rela_len = d[i].d_un.d_val;
				break;
			// TK: procedure linkage table
			case DT_JMPREL:
				image->pltrel = (struct Elf32_Rel *)(d[i].d_un.d_ptr + image->regions[0].delta);
				break;
			case DT_PLTRELSZ:
				image->pltrel_len = d[i].d_un.d_val;
				break;
			case DT_INIT:
				image->init_routine = (d[i].d_un.d_ptr + image->regions[0].delta);
				break;
			case DT_FINI:
				image->term_routine = (d[i].d_un.d_ptr + image->regions[0].delta);
				break;
			case DT_SONAME:
				sonameOffset = d[i].d_un.d_val;
				break;
			default:
				continue;
		}
	}

	// lets make sure we found all the required sections
	if (!image->symhash || !image->syms || !image->strtab)
		return false;

	if (sonameOffset >= 0)
		strlcpy(image->name, STRING(image, sonameOffset), sizeof(image->name));

	return true;
}


static struct Elf32_Sym *
find_symbol(image_t *image, const char *name, int32 type)
{
	uint32 hash, i;

	// ToDo: "type" is currently ignored!
	(void)type;

	if (image->dynamic_ptr == NULL)
		return NULL;

	hash = elf_hash(name) % HASHTABSIZE(image);

	for (i = HASHBUCKETS(image)[hash]; i != STN_UNDEF; i = HASHCHAINS(image)[i]) {
		struct Elf32_Sym *symbol = &image->syms[i];

		if (symbol->st_shndx != SHN_UNDEF
			&& ((ELF32_ST_BIND(symbol->st_info)== STB_GLOBAL)
				|| (ELF32_ST_BIND(symbol->st_info) == STB_WEAK))
			&& !strcmp(SYMNAME(image, symbol), name)) {
			// check if the type matches
			if ((type == B_SYMBOL_TYPE_TEXT && ELF32_ST_TYPE(symbol->st_info) != STT_FUNC)
				|| (type == B_SYMBOL_TYPE_DATA && ELF32_ST_TYPE(symbol->st_info) != STT_OBJECT))
				continue;

			return symbol;
		}
	}

	return NULL;
}


static struct Elf32_Sym *
find_symbol_in_loaded_images(image_t **_image, const char *name)
{
	image_t *image;

	for (image = sLoadedImages.head; image; image = image->next) {
		struct Elf32_Sym *symbol;

		if (image->dynamic_ptr == NULL)
			continue;

		symbol = find_symbol(image, name, B_SYMBOL_TYPE_ANY);
		if (symbol) {
			*_image = image;
			return symbol;
		}
	}

	return NULL;
}


int
resolve_symbol(image_t *image, struct Elf32_Sym *sym, addr_t *sym_addr)
{
	struct Elf32_Sym *sym2;
	char *symname;
	image_t *shimg;

	switch (sym->st_shndx) {
		case SHN_UNDEF:
			// patch the symbol name
			symname = SYMNAME(image, sym);

			// it's undefined, must be outside this image, try the other image
			sym2 = find_symbol_in_loaded_images(&shimg, symname);
			if (!sym2) {
				printf("elf_resolve_symbol: could not resolve symbol '%s'\n", symname);
				return B_MISSING_SYMBOL;
			}

			// make sure they're the same type
			if (ELF32_ST_TYPE(sym->st_info) != STT_NOTYPE
				&& ELF32_ST_TYPE(sym->st_info) != ELF32_ST_TYPE(sym2->st_info)) {
				printf("elf_resolve_symbol: found symbol '%s' in shared image but wrong type\n", symname);
				return B_MISSING_SYMBOL;
			}

			if (ELF32_ST_BIND(sym2->st_info) != STB_GLOBAL
				&& ELF32_ST_BIND(sym2->st_info) != STB_WEAK) {
				printf("elf_resolve_symbol: found symbol '%s' but not exported\n", symname);
				return B_MISSING_SYMBOL;
			}

			*sym_addr = sym2->st_value + shimg->regions[0].delta;
			return B_NO_ERROR;

		case SHN_ABS:
			*sym_addr = sym->st_value + image->regions[0].delta;
			return B_NO_ERROR;

		case SHN_COMMON:
			// ToDo: finish this
			printf("elf_resolve_symbol: COMMON symbol, finish me!\n");
			return B_ERROR; //ERR_NOT_IMPLEMENTED_YET;

		default:
			// standard symbol
			*sym_addr = sym->st_value + image->regions[0].delta;
			return B_NO_ERROR;
	}
}


static void
register_image(image_t *image, int fd, const char *path)
{
	struct stat stat;
	image_info info;

	// ToDo: set these correctly
	info.id = 0;
	info.type = image->type;
	info.sequence = 0;
	info.init_order = 0;
	info.init_routine = (void *)image->init_routine;
	info.term_routine = (void *)image->term_routine;
	
	if (_kern_read_stat(fd, NULL, false, &stat, sizeof(struct stat)) == B_OK) {
		info.device = stat.st_dev;
		info.node = stat.st_ino;
	} else {
		info.device = -1;
		info.node = -1;
	}

	strlcpy(info.name, path, sizeof(info.name));
	info.text = (void *)image->regions[0].vmstart;
	info.text_size = image->regions[0].vmsize;
	info.data = (void *)image->regions[1].vmstart;
	info.data_size = image->regions[1].vmsize;
	image->id = _kern_register_image(&info, sizeof(image_info));
}


static status_t
relocate_image(image_t *image)
{
	status_t status = arch_relocate_image(image);
	if (status < B_OK) {
		FATAL("troubles relocating: 0x%lx\n", status);
		return status;
	}

	_kern_image_relocated(image->id);
	return B_OK;
}


static status_t
load_container(char const *name, image_type type, const char *rpath, image_t **_image)
{
	int32 pheaderSize, sheaderSize;
	char path[PATH_MAX];
	ssize_t length;
	char ph_buff[4096];
	int32 numRegions;
	image_t *found;
	image_t *image;
	status_t status;
	int fd;

	struct Elf32_Ehdr eheader;

	// Have we already loaded that image? Don't check for add-ons -- we always
	// reload them.
	if (type != B_ADD_ON_IMAGE) {
		found = find_image(name, APP_OR_LIBRARY_TYPE);
		if (found) {
			atomic_add(&found->ref_count, 1);
			*_image = found;
			return B_OK;
		}
	}

	strlcpy(path, name, sizeof(path));

	// Try to load explicit image path first
	fd = open_executable(path, type, rpath);
	if (fd < 0) {
		FATAL("cannot open file %s\n", path);
		return fd;
	}

	// If the path is not absolute, we prepend the CWD to make it one.
	if (path[0] != '/') {
		char relativePath[PATH_MAX];
		strcpy(relativePath, path);

		// get the CWD
		status = _kern_getcwd(path, sizeof(path));
		if (status < B_OK) {
			FATAL("_kern_getcwd() failed\n");
			goto err1;
		}

		if (strlcat(path, "/", sizeof(path)) >= sizeof(path)
			|| strlcat(path, relativePath, sizeof(path)) >= sizeof(path)) {
			status = B_NAME_TOO_LONG;
			FATAL("Absolute path of image %s is too "
				"long!\n", relativePath);
			goto err1;
		}
	}

	length = _kern_read(fd, 0, &eheader, sizeof(eheader));
	if (length != sizeof(eheader)) {
		status = B_NOT_AN_EXECUTABLE;
		FATAL("troubles reading ELF header\n");
		goto err1;
	}

	status = parse_elf_header(&eheader, &pheaderSize, &sheaderSize);
	if (status < B_OK) {
		FATAL("incorrect ELF header\n");
		goto err1;
	}

	// ToDo: what to do about this restriction??
	if (pheaderSize > (int)sizeof(ph_buff)) {
		FATAL("Cannot handle program headers bigger than %lu\n", sizeof(ph_buff));
		status = B_UNSUPPORTED;
		goto err1;
	}

	length = _kern_read(fd, eheader.e_phoff, ph_buff, pheaderSize);
	if (length != pheaderSize) {
		FATAL("Could not read program headers: %s\n", strerror(length));
		status = B_BAD_DATA;
		goto err1;
	}

	numRegions = count_regions(ph_buff, eheader.e_phnum, eheader.e_phentsize);
	if (numRegions <= 0) {
		FATAL("Troubles parsing Program headers, numRegions = %ld\n", numRegions);
		status = B_BAD_DATA;
		goto err1;
	}

	image = create_image(name, path, numRegions);
	if (image == NULL) {
		FATAL("Failed to allocate image_t object\n");
		status = B_NO_MEMORY;
		goto err1;
	}

	status = parse_program_headers(image, ph_buff, eheader.e_phnum, eheader.e_phentsize);
	if (status < B_OK)
		goto err2;

	if (!assert_dynamic_loadable(image)) {
		FATAL("Dynamic segment must be loadable (implementation restriction)\n");
		status = B_UNSUPPORTED;
		goto err2;
	}

	status = map_image(fd, path, image, type == B_APP_IMAGE);
	if (status < B_OK) {
		FATAL("Could not map image: %s\n", strerror(status));
		status = B_ERROR;
		goto err2;
	}

	if (!parse_dynamic_segment(image)) {
		FATAL("Troubles handling dynamic section\n");
		status = B_BAD_DATA;
		goto err3;
	}

	if (eheader.e_entry != NULL)
		image->entry_point = eheader.e_entry + image->regions[0].delta;

	image->type = type;
	register_image(image, fd, path);

	_kern_close(fd);

	enqueue_image(&sLoadedImages, image);

	*_image = image;
	return B_OK;

err3:
	unmap_image(image);
err2:
	delete_image_struct(image);
err1:
	_kern_close(fd);
	return status;
}


static const char *
find_dt_rpath(image_t *image)
{
	int i;
	struct Elf32_Dyn *d = (struct Elf32_Dyn *)image->dynamic_ptr;

	for (i = 0; d[i].d_tag != DT_NULL; i++) {
		if (d[i].d_tag == DT_RPATH)
			return STRING(image, d[i].d_un.d_val);
	}

	return NULL;
}


static status_t
load_dependencies(image_t *image)
{
	struct Elf32_Dyn *d = (struct Elf32_Dyn *)image->dynamic_ptr;
	int neededOffset;
	status_t status;
	uint32 i, j;
	const char *rpath;

	if (!d || (image->flags & RFLAG_DEPENDENCIES_LOADED))
		return B_OK;

	image->flags |= RFLAG_DEPENDENCIES_LOADED;

	rpath = find_dt_rpath(image);

	image->needed = rldalloc(image->num_needed * sizeof(image_t *));
	if (image->needed == NULL) {
		FATAL("failed to allocate needed struct\n");
		return B_NO_MEMORY;
	}
	memset(image->needed, 0, image->num_needed * sizeof(image_t *));

	for (i = 0, j = 0; d[i].d_tag != DT_NULL; i++) {
		switch (d[i].d_tag) {
			case DT_NEEDED:
				neededOffset = d[i].d_un.d_val;

				status = load_container(STRING(image, neededOffset),
					B_LIBRARY_IMAGE, rpath, &image->needed[j]);
				if (status < B_OK)
					return status;

				j += 1;
				break;

			default:
				// ignore any other tag
				continue;
		}
	}

	if (j != image->num_needed) {
		FATAL("Internal error at load_dependencies()");
		return B_ERROR;
	}

	return B_OK;
}


static uint32
topological_sort(image_t *image, uint32 slot, image_t **initList,
	uint32 sortFlag)
{
	uint32 i;

	if (image->flags & sortFlag)
		return slot;

	image->flags |= sortFlag; /* make sure we don't visit this one */
	for (i = 0; i < image->num_needed; i++)
		slot = topological_sort(image->needed[i], slot, initList, sortFlag);

	initList[slot] = image;
	return slot + 1;
}


static uint32
get_sorted_image_list(image_t *image, image_t ***_list, uint32 sortFlag)
{
	image_t **list;

	list = rldalloc(sLoadedImageCount * sizeof(image_t *));
	if (list == NULL) {
		FATAL("memory shortage in get_sorted_image_list()");
		return 0; // B_NO_MEMORY
	}
	memset(list, 0, sLoadedImageCount * sizeof(image_t *));

	*_list = list;
	return topological_sort(image, 0, list, sortFlag);
}


static status_t
relocate_dependencies(image_t *image)
{
	uint32 count, i;
	image_t **list;

	count = get_sorted_image_list(image, &list, RFLAG_RELOCATED);

	for (i = 0; i < count; i++) {
		status_t status = relocate_image(list[i]);
		if (status < B_OK)
			return status;
	}

	rldfree(list);
	return B_OK;
}


static void
init_dependencies(image_t *image, bool initHead)
{
	image_t **initList;
	uint32 count, i;

	count = get_sorted_image_list(image, &initList, RFLAG_INITIALIZED);
	if (count == 0)
		return;

	if (!initHead) {
		// this removes the "calling" image
		image->flags &= ~RFLAG_INITIALIZED;
		initList[--count] = NULL;
	}

	TRACE(("%ld: init dependencies\n", find_thread(NULL)));
	for (i = 0; i < count; i++) {
		TRACE(("%ld:  init: %s\n", find_thread(NULL), initList[i]->name));
		arch_call_init(initList[i]);
	}
	TRACE(("%ld:  init done.\n", find_thread(NULL)));

	rldfree(initList);
}


static void
put_image(image_t *image)
{
	// If all references to the image are gone, add it to the disposable list
	// and remove all dependencies

	if (atomic_add(&image->ref_count, -1) == 1) {
		size_t i;

		dequeue_image(&sLoadedImages, image);
		enqueue_image(&sDisposableImages, image);

		for (i = 0; i < image->num_needed; i++) {
			put_image(image->needed[i]);
		}
	}
}


//	#pragma mark -
//	Exported functions (to libroot.so)


image_id
load_program(char const *path, void **_entry)
{
	status_t status;
	image_t *image;

	rld_lock();
		// for now, just do stupid simple global locking

	TRACE(("rld: load %s\n", path));

	status = load_container(path, B_APP_IMAGE, NULL, &sProgramImage);
	if (status < B_OK) {
		_kern_loading_app_failed(status);
		rld_unlock();
		return status;
	}

	for (image = sLoadedImages.head; image != NULL; image = image->next) {
		status = load_dependencies(image);
		if (status < B_OK)
			goto err;
	}

	status = relocate_dependencies(sProgramImage);
	if (status < B_OK)
		goto err;

	// We patch any exported __gRuntimeLoader symbols to point to our private API
	{
		struct Elf32_Sym *symbol = find_symbol_in_loaded_images(&image, "__gRuntimeLoader");
		if (symbol != NULL) {
			void **_export = (void **)(symbol->st_value + image->regions[0].delta);
			*_export = &gRuntimeLoader;
		}
	}

	init_dependencies(sLoadedImages.head, true);
	remap_images();
		// ToDo: once setup_system_time() is fixed, move this one line higher!

	// Since the images are initialized now, we no longer should use our
	// getenv(), but use the one from libroot.so
	{
		struct Elf32_Sym *symbol = find_symbol_in_loaded_images(&image, "getenv");
		if (symbol != NULL)
			gGetEnv = (void *)(symbol->st_value + image->regions[0].delta);
	}

	if (sProgramImage->entry_point == NULL) {
		status = B_NOT_AN_EXECUTABLE;
		goto err;
	}

	*_entry = (void *)(sProgramImage->entry_point);

	rld_unlock();
	return sProgramImage->id;

err:
	_kern_loading_app_failed(status);
	delete_image(sProgramImage);
	rld_unlock();
	return status;
}


image_id
load_library(char const *path, uint32 flags, bool addOn)
{
	image_t *image = NULL;
	image_t *iter;
	image_type type = (addOn ? B_ADD_ON_IMAGE : B_LIBRARY_IMAGE);
	status_t status;

	// ToDo: implement flags
	(void)flags;

	rld_lock();
		// for now, just do stupid simple global locking

	// have we already loaded this library?
	// Checking it at this stage saves loading its dependencies again
	if (!addOn) {
		image = find_image(path, APP_OR_LIBRARY_TYPE);
		if (image) {
			atomic_add(&image->ref_count, 1);
			rld_unlock();
			return image->id;
		}
	}

	status = load_container(path, type, NULL, &image);
	if (status < B_OK) {
		rld_unlock();
		return status;
	}

	for (iter = sLoadedImages.head; iter; iter = iter->next) {
		status = load_dependencies(iter);
		if (status < B_OK)
			goto err;
	}

	status = relocate_dependencies(image);
	if (status < B_OK)
		goto err;

	remap_images();
	init_dependencies(image, true);

	rld_unlock();
	return image->id;

err:
	delete_image(image);
	rld_unlock();
	return status;
}


status_t
unload_library(image_id imageID, bool addOn)
{
	status_t status = B_BAD_IMAGE_ID;
	image_t *image;
	image_type type = addOn ? B_ADD_ON_IMAGE : B_LIBRARY_IMAGE;

	rld_lock();
		// for now, just do stupid simple global locking

	/*
	 * we only check images that have been already initialized
	 */

	for (image = sLoadedImages.head; image; image = image->next) {
		if (image->id == imageID) {
			/*
			 * do the unloading
			 */
			if (type == image->type) {
				put_image(image);
				status = B_OK;
			} else
				status = B_BAD_VALUE;
			break;
		}
	}

	if (status == B_OK) {
		while ((image = sDisposableImages.head) != NULL) {
			// call image fini here...
			if (image->term_routine)
				arch_call_term(image);

			dequeue_image(&sDisposableImages, image);
			unmap_image(image);
	
			delete_image(image);
		}
	}

	rld_unlock();
	return status;
}


status_t
get_nth_symbol(image_id imageID, int32 num, char *nameBuffer, int32 *_nameLength,
	int32 *_type, void **_location)
{
	int32 count = 0, j;
	uint32 i;
	image_t *image;

	rld_lock();

	// get the image from those who have been already initialized
	image = find_loaded_image_by_id(imageID);
	if (image == NULL) {
		rld_unlock();
		return B_BAD_IMAGE_ID;
	}

	// iterate through all the hash buckets until we've found the one
	for (i = 0; i < HASHTABSIZE(image); i++) {
		for (j = HASHBUCKETS(image)[i]; j != STN_UNDEF; j = HASHCHAINS(image)[j]) {
			struct Elf32_Sym *symbol = &image->syms[i];

			if (count == num) {
				strlcpy(nameBuffer, SYMNAME(image, symbol), *_nameLength);
				*_nameLength = strlen(SYMNAME(image, symbol));

				// ToDo: check with the return types of that BeOS function
				if (ELF32_ST_TYPE(symbol->st_info) == STT_FUNC)
					*_type = B_SYMBOL_TYPE_TEXT;
				else if (ELF32_ST_TYPE(symbol->st_info) == STT_OBJECT)
					*_type = B_SYMBOL_TYPE_DATA;
				else
					*_type = B_SYMBOL_TYPE_ANY;

				*_location = (void *)(symbol->st_value + image->regions[0].delta);
				goto out;
			}
			count++;
		}
	}
out:
	rld_unlock();	

	if (num != count)
		return B_BAD_INDEX;

	return B_OK;
}


status_t
get_symbol(image_id imageID, char const *symbolName, int32 symbolType, void **_location)
{
	status_t status = B_OK;
	image_t *image;

	rld_lock();
		// for now, just do stupid simple global locking

	// get the image from those who have been already initialized
	image = find_loaded_image_by_id(imageID);
	if (image != NULL) {
		struct Elf32_Sym *symbol;

		// get the symbol in the image
		symbol = find_symbol(image, symbolName, symbolType);
		if (symbol)
			*_location = (void *)(symbol->st_value + image->regions[0].delta);
		else
			status = B_ENTRY_NOT_FOUND;
	} else
		status = B_BAD_IMAGE_ID;

	rld_unlock();
	return status;
}


//	#pragma mark -


/** Read and verify the ELF header */

status_t
elf_verify_header(void *header, int32 length)
{
	int32 programSize, sectionSize;

	if (length < (int32)sizeof(struct Elf32_Ehdr))
		return B_BAD_VALUE;

	return parse_elf_header((struct Elf32_Ehdr *)header, &programSize, &sectionSize);
}


void
terminate_program(void)
{
	image_t **termList;
	uint32 count, i;

	count = get_sorted_image_list(sProgramImage, &termList, RFLAG_TERMINATED);
	if (count == 0)
		return;

	TRACE(("%ld: terminate dependencies\n", find_thread(NULL)));
	for (i = count; i-- > 0;) {
		TRACE(("%ld:  term: %s\n", find_thread(NULL), termList[i]->name));
		arch_call_term(termList[i]);
	}
	TRACE(("%ld:  term done.\n", find_thread(NULL)));

	rldfree(termList);
	
}


void
rldelf_init(void)
{
	rld_sem = create_sem(1, "rld_lock");
	rld_sem_owner = -1;
	rld_sem_count = 0;

	// create the debug area
	{
		int32 size = TO_PAGE_SIZE(sizeof(runtime_loader_debug_area));

		runtime_loader_debug_area *area;
		area_id areaID = _kern_create_area(RUNTIME_LOADER_DEBUG_AREA_NAME,
			(void **)&area, B_ANY_ADDRESS, size, B_NO_LOCK,
			B_READ_AREA | B_WRITE_AREA);
		if (areaID < B_OK) {
			FATAL("Failed to create debug area.\n");
			_kern_loading_app_failed(areaID);
		}

		area->loaded_images = &sLoadedImages;
	}
}
