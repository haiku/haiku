/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifdef _KERNEL_MODE
#	undef _KERNEL_MODE
#endif

#include <OS.h>

#include <elf32.h>
#include <user_runtime.h>
#include <syscalls.h>
#include <arch/cpu.h>
#include <sem.h>

#include <string.h>
#include <stdio.h>

#include "rld_priv.h"

// ToDo: implement better locking strategy
// ToDo: implement unload_program()
// ToDo: implement load_addon()/unload_addon(): at the very least, we will have to make
//	sure that B_ADD_ON_IMAGE is set correctly
// ToDo: implement search paths $LIBRARY_PATH, $ADDON_PATH

#define	PAGE_MASK (B_PAGE_SIZE - 1)
#define	PAGE_OFFS(y) ((y) & (PAGE_MASK))
#define	PAGE_BASE(y) ((y) & ~(PAGE_MASK))

/* david - added '_' to avoid macro's in kernel.h */
#define	_ROUNDOWN(x,y) ((x)&~((y)-1))
#define	_ROUNDUP(x,y) ROUNDOWN(x+y-1,y)


enum {
	RFLAG_RW				= 0x0001,
	RFLAG_ANON				= 0x0002,

	RFLAG_SORTED			= 0x0400,
	RFLAG_SYMBOLIC			= 0x0800,
	RFLAG_RELOCATED			= 0x1000,
	RFLAG_PROTECTED			= 0x2000,
	RFLAG_INITIALIZED		= 0x4000,
	RFLAG_NEEDAGIRLFRIEND	= 0x8000
};


typedef
struct elf_region_t {
	region_id	id;
	addr_t		start;
	addr_t		size;
	addr_t		vmstart;
	addr_t		vmsize;
	addr_t		fdstart;
	addr_t		fdsize;
	long		delta;
	uint32		flags;
} elf_region_t;


typedef struct image_t {
	// image identification
	char				name[SYS_MAX_OS_NAME_LEN];
	image_id			id;
	image_type			type;

	struct image_t		*next;
	struct image_t		*prev;
	int32				ref_count;
	uint32				flags;

	addr_t 				entry_point;
	addr_t				init_routine;
	addr_t				term_routine;
	addr_t 				dynamic_ptr; 	// pointer to the dynamic section

	// pointer to symbol participation data structures
	uint32				*symhash;
	struct Elf32_Sym	*syms;
	char				*strtab;
	struct Elf32_Rel	*rel;
	int					rel_len;
	struct Elf32_Rela	*rela;
	int					rela_len;
	struct Elf32_Rel	*pltrel;
	int					pltrel_len;

	uint32				num_needed;
	struct image_t		**needed;

	// describes the text and data regions
	uint32				num_regions;
	elf_region_t		regions[1];
} image_t;


typedef
struct image_queue_t {
	image_t *head;
	image_t *tail;
} image_queue_t;


static image_queue_t gLoadedImages = {0, 0};
static image_queue_t gLoadingImages = {0, 0};
static image_queue_t gDisposableImages = {0, 0};
static uint32 gLoadedImageCount = 0;

// a recursive lock
static sem_id rld_sem;
static thread_id rld_sem_owner;
static int32 rld_sem_count;

static struct uspace_program_args const *gProgramArgs;


#define STRING(image, offset) ((char *)(&(image)->strtab[(offset)]))
#define SYMNAME(image, sym) STRING(image, (sym)->st_name)
#define SYMBOL(image, num) ((struct Elf32_Sym *)&(image)->syms[num])
#define HASHTABSIZE(image) ((image)->symhash[0])
#define HASHBUCKETS(image) ((unsigned int *)&(image)->symhash[2])
#define HASHCHAINS(image) ((unsigned int *)&(image)->symhash[2+HASHTABSIZE(image)])


/*
 * This macro is non ISO compliant, but a gcc extension
 */
#define	FATAL(x,y...) \
	if (x) { \
		printf("rld.so: " y); \
		_kern_exit(0); \
	}


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
find_image(char const *name)
{
	image_t *iter;

	for (iter = gLoadedImages.head; iter; iter = iter->next) {
		if (strncmp(iter->name, name, sizeof(iter->name)) == 0)
			return iter;
	}

	for (iter = gLoadingImages.head; iter; iter = iter->next) {
		if (strncmp(iter->name, name, sizeof(iter->name)) == 0)
			return iter;
	}

	return NULL;
}


static image_t *
find_loaded_image_by_id(image_id id)
{
	image_t *image;

	for (image = gLoadedImages.head; image; image = image->next) {
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


static int
count_regions(char const *buff, int phnum, int phentsize)
{
	int i;
	int retval = 0;
	struct Elf32_Phdr *pheaders;

	for (i = 0; i < phnum; i++) {
		pheaders = (struct Elf32_Phdr *)(buff + i * phentsize);

		switch (pheaders->p_type) {
			case PT_NULL:
				/* NOP header */
				break;
			case PT_LOAD:
				retval += 1;
				if (pheaders->p_memsz!= pheaders->p_filesz) {
					unsigned A = pheaders->p_vaddr + pheaders->p_memsz;
					unsigned B = pheaders->p_vaddr + pheaders->p_filesz - 1;

					A = PAGE_BASE(A);
					B = PAGE_BASE(B);

					if (A != B)
						retval += 1;
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
				FATAL(true, "unhandled pheader type 0x%lx\n", pheaders[i].p_type);
				break;
		}
	}

	return retval;
}


/*
 * create_image() & destroy_image()
 *
 * 	Create and destroy image_t structures. The destroyer makes sure that the
 * 	memory buffers are full of garbage before freeing.
 */

static image_t *
create_image(char const *name, int num_regions)
{
	size_t allocSize;
	image_t *image;

	allocSize = sizeof(image_t) + (num_regions - 1) * sizeof(elf_region_t);

	image = rldalloc(allocSize);
	memset(image, 0, allocSize);

	strlcpy(image->name, name, sizeof(image->name));
	image->ref_count = 1;
	image->num_regions = num_regions;

	return image;
}


static void
delete_image(image_t *image)
{
	size_t size = sizeof(image_t) + (image->num_regions - 1) * sizeof(elf_region_t);

	_kern_unregister_image(image->id);
		// registered in load_container()

	memset(image->needed, 0xa5, sizeof(image->needed[0]) * image->num_needed);
	rldfree(image->needed);

	memset(image, 0xa5, size);
	rldfree(image);
}


static void
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
					image->regions[regcount].vmstart = _ROUNDOWN(pheader->p_vaddr, B_PAGE_SIZE);
					image->regions[regcount].vmsize = _ROUNDUP(pheader->p_memsz +
						(pheader->p_vaddr % B_PAGE_SIZE), B_PAGE_SIZE);
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
					unsigned A = pheader->p_vaddr + pheader->p_memsz;
					unsigned B = pheader->p_vaddr + pheader->p_filesz - 1;

					A = PAGE_BASE(A);
					B = PAGE_BASE(B);

					image->regions[regcount].start = pheader->p_vaddr;
					image->regions[regcount].size = pheader->p_filesz;
					image->regions[regcount].vmstart = _ROUNDOWN(pheader->p_vaddr, B_PAGE_SIZE);
					image->regions[regcount].vmsize = _ROUNDUP (pheader->p_filesz +
						(pheader->p_vaddr % B_PAGE_SIZE), B_PAGE_SIZE);
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
						image->regions[regcount].vmsize = _ROUNDUP(pheader->p_memsz
								+ (pheader->p_vaddr % B_PAGE_SIZE), B_PAGE_SIZE)
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
				FATAL(true, "unhandled pheader type 0x%lx\n", pheader[i].p_type);
				break;
		}
	}
}


static bool
assert_dynamic_loadable(image_t *image)
{
	unsigned i;

	if (!image->dynamic_ptr)
		return true;

	for (i = 0; i < image->num_regions; i++) {
		if (image->dynamic_ptr >= image->regions[i].start
			&& image->dynamic_ptr < image->regions[i].start + image->regions[i].size)
			return true;
	}

	return false;
}


static bool
map_image(int fd, char const *path, image_t *image, bool fixed)
{
	unsigned i;

	(void)(fd);

	for (i = 0; i < image->num_regions; i++) {
		char regionName[B_OS_NAME_LENGTH];
		addr_t load_address;
		unsigned addr_specifier;

		// for BeOS compatibility: if we load an old BeOS executable, we
		// have to relocate it, if possible - we recognize it because the
		// vmstart is set to 0 (hopefully always)
		if (fixed && image->regions[i].vmstart == 0)
			fixed = false;

		snprintf(regionName, sizeof(regionName), "%s_seg%d%s",
			path, i, (image->regions[i].flags & RFLAG_RW) ? "rw" : "ro");

		if (image->dynamic_ptr && !fixed) {
			/*
			 * relocatable image... we can afford to place wherever
			 */
			if (i == 0) {
				/*
				 * but only the first segment gets a free ride
				 */
				load_address = 0;
				addr_specifier = B_ANY_ADDRESS;
			} else {
				load_address = image->regions[i].vmstart + image->regions[i-1].delta;
				addr_specifier = B_EXACT_ADDRESS;
			}
		} else {
			/*
			 * not relocatable, put it where it asks or die trying
			 */
			load_address = image->regions[i].vmstart;
			addr_specifier = B_EXACT_ADDRESS;
		}

		if (image->regions[i].flags & RFLAG_ANON) {
			image->regions[i].id = _kern_create_area(regionName, (void **)&load_address,
				addr_specifier, image->regions[i].vmsize, B_NO_LOCK,
				B_READ_AREA | B_WRITE_AREA);

			if (image->regions[i].id < 0)
				goto error;

			image->regions[i].delta = load_address - image->regions[i].vmstart;
			image->regions[i].vmstart = load_address;
		} else {
			image->regions[i].id = sys_vm_map_file(regionName, (void **)&load_address,
				addr_specifier, image->regions[i].vmsize, B_READ_AREA | B_WRITE_AREA,
				REGION_PRIVATE_MAP, path,
				_ROUNDOWN(image->regions[i].fdstart, B_PAGE_SIZE));

			if (image->regions[i].id < 0)
				goto error;

			image->regions[i].delta = load_address - image->regions[i].vmstart;
			image->regions[i].vmstart = load_address;

			/*
			 * handle trailer bits in data segment
			 */
			if (image->regions[i].flags & RFLAG_RW) {
				unsigned start_clearing;
				unsigned to_clear;

				start_clearing = image->regions[i].vmstart
					+ PAGE_OFFS(image->regions[i].start)
					+ image->regions[i].size;
				to_clear = image->regions[i].vmsize
					- PAGE_OFFS(image->regions[i].start)
					- image->regions[i].size;
				memset((void*)start_clearing, 0, to_clear);
			}
		}
	}

	if (image->dynamic_ptr)
		image->dynamic_ptr += image->regions[0].delta;

	return true;

error:
	return false;
}


static void
unmap_image(image_t *image)
{
	unsigned i;

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
			default:
				continue;
		}
	}

	// lets make sure we found all the required sections
	if (!image->symhash || !image->syms || !image->strtab)
		return false;

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

	for (image = gLoadedImages.head; image; image = image->next) {
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


static int
resolve_symbol(image_t *image, struct Elf32_Sym *sym, addr_t *sym_addr)
{
	struct Elf32_Sym *sym2;
	char             *symname;
	image_t          *shimg;

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


#include "arch/rldreloc.inc"


static void
register_image(image_t *image, const char *path)
{
	image_info info;

	// ToDo: set these correctly
	info.id = 0;
	info.type = image->type;
	info.sequence = 0;
	info.init_order = 0;
	info.init_routine = (void *)image->init_routine;
	info.term_routine = (void *)image->term_routine;
	info.device = 0;
	info.node = 0;
	strlcpy(info.name, path, sizeof(info.name));
	info.text = NULL;
	info.text_size = 0;
	info.data = NULL;
	info.data_size = 0;
	image->id = _kern_register_image(&info, sizeof(image_info));
}


static image_t *
load_container(char const *path, char const *name, image_type type)
{
	int32 pheaderSize, sheaderSize;
	int fd;
	int len;
	char ph_buff[4096];
	int num_regions;
	bool map_success;
	bool dynamic_success;
	image_t *found;
	image_t *image;

	struct Elf32_Ehdr eheader;

	// have we already loaded that image?
	found = find_image(name);
	if (found) {
		atomic_add(&found->ref_count, 1);
		return found;
	}

	fd = _kern_open(path, 0);
	FATAL((fd < 0), "cannot open file %s\n", path);

	len = _kern_read(fd, 0, &eheader, sizeof(eheader));
	FATAL((len != sizeof(eheader)), "troubles reading ELF header\n");

	if (parse_elf_header(&eheader, &pheaderSize, &sheaderSize) < B_OK) {
		FATAL(1, "incorrect ELF header\n");
	}
	// ToDo: what to do about this restriction??
	FATAL((pheaderSize > (int)sizeof(ph_buff)), "cannot handle Program headers bigger than %lu\n", (long unsigned)sizeof(ph_buff));

	len = _kern_read(fd, eheader.e_phoff, ph_buff, pheaderSize);
	FATAL((len != pheaderSize), "troubles reading Program headers\n");

	num_regions = count_regions(ph_buff, eheader.e_phnum, eheader.e_phentsize);
	FATAL((num_regions <= 0), "troubles parsing Program headers, num_regions= %d\n", num_regions);

	image = create_image(name, num_regions);
	FATAL((!image), "failed to allocate image_t control block\n");

	parse_program_headers(image, ph_buff, eheader.e_phnum, eheader.e_phentsize);
	FATAL(!assert_dynamic_loadable(image), "dynamic segment must be loadable (implementation restriction)\n");

	map_success = map_image(fd, path, image, type == B_APP_IMAGE);
	FATAL(!map_success, "troubles reading image\n");

	dynamic_success = parse_dynamic_segment(image);
	FATAL(!dynamic_success, "troubles handling dynamic section\n");

	image->entry_point = eheader.e_entry + image->regions[0].delta;
	image->type = type;
	register_image(image, path);

	_kern_close(fd);

	enqueue_image(&gLoadedImages, image);

	return image;
}


static void
load_dependencies(image_t *image)
{
	struct Elf32_Dyn *d = (struct Elf32_Dyn *)image->dynamic_ptr;
	addr_t   needed_offset;
	char   path[256];
	uint32 i, j;

	if (!d)
		return;

	image->needed = rldalloc(image->num_needed * sizeof(image_t *));
	FATAL((!image->needed), "failed to allocate needed struct\n");
	memset(image->needed, 0, image->num_needed * sizeof(image_t *));

	for (i = 0, j = 0; d[i].d_tag != DT_NULL; i++) {
		switch (d[i].d_tag) {
			case DT_NEEDED:
				needed_offset = d[i].d_un.d_ptr;
				// ToDo: ever heard of the LIBRARY_PATH env variable?
				sprintf(path, "/boot/beos/system/lib/%s", STRING(image, needed_offset));
				image->needed[j] = load_container(path, STRING(image, needed_offset), B_LIBRARY_IMAGE);
				j += 1;

				break;
			default:
				/*
				 * ignore any other tag
				 */
				continue;
		}
	}

	FATAL((j != image->num_needed), "Internal error at load_dependencies()");

	return;
}


static uint32
topological_sort(image_t *image, uint32 slot, image_t **initList)
{
	uint32 i;

	image->flags |= RFLAG_SORTED; /* make sure we don't visit this one */
	for (i = 0; i < image->num_needed; i++) {
		if (!(image->needed[i]->flags & RFLAG_SORTED))
			slot = topological_sort(image->needed[i], slot, initList);
	}

	initList[slot] = image;
	return slot + 1;
}


static void
init_dependencies(image_t *image, bool initHead)
{
	unsigned i;
	unsigned slot;
	image_t **initList;

	initList = rldalloc(gLoadedImageCount * sizeof(image_t *));
	FATAL((!initList), "memory shortage in init_dependencies()");
	memset(initList, 0, gLoadedImageCount * sizeof(image_t *));

	image->flags |= RFLAG_SORTED; /* make sure we don't visit this one */
	slot = 0;
	for (i = 0; i < image->num_needed; i++) {
		if (!(image->needed[i]->flags & RFLAG_SORTED))
			slot = topological_sort(image->needed[i], slot, initList);
	}

	if (initHead) {
		initList[slot] = image;
		slot += 1;
	}

	for (i = 0; i < slot; i++) {
		addr_t _initf = initList[i]->init_routine;
		libinit_f *initf = (libinit_f *)(_initf);

		if (initf)
			initf(initList[i]->id, gProgramArgs);
	}

	rldfree(initList);
}


static void
put_image(image_t *image)
{
	// If all references to the image are gone, add it to the disposable list
	// and remove all dependencies

	if (atomic_add(&image->ref_count, -1) == 1) {
		size_t i;

		dequeue_image(&gLoadedImages, image);
		enqueue_image(&gDisposableImages, image);

		for (i = 0; i < image->num_needed; i++) {
			put_image(image->needed[i]);
		}
	}
}


//	#pragma mark -

/*
 * exported functions:
 *
 *	+ load_program()
 *	+ load_library()
 *	+ load_addon()
 *	+ unload_program()
 *	+ unload_library()
 *	+ unload_addon()
 *	+ dynamic_symbol()
 */

image_id
load_program(char const *path, void **_entry)
{
	image_t *image;
	image_t *iter;

	rld_lock();
		// for now, just do stupid simple global locking

	image = load_container(path, MAGIC_APP_NAME, B_APP_IMAGE);

	for (iter = gLoadedImages.head; iter; iter = iter->next) {
		load_dependencies(iter);
	}

	for (iter = gLoadedImages.head; iter; iter = iter->next) {
		bool relocate_success;

		relocate_success = relocate_image(iter);
		FATAL(!relocate_success, "troubles relocating\n");
	}

	init_dependencies(gLoadedImages.head, false);

	*_entry = (void *)(image->entry_point);

	rld_unlock();
	return image->id;
}


image_id
load_library(char const *path, uint32 flags)
{
	image_t *image;
	image_t *iter;

	// ToDo: implement flags
	(void)flags;

	rld_lock();
		// for now, just do stupid simple global locking

	// have we already loaded this library?
	// Checking it at this stage saves loading its dependencies again
	// ToDo: don't we have to increment the reference counter of the dependencies??
	image = find_image(path);
	if (image) {
		atomic_add(&image->ref_count, 1);
		rld_unlock();
		return image->id;
	}

	image = load_container(path, path, B_LIBRARY_IMAGE);

	for (iter = gLoadedImages.head; iter; iter = iter->next) {
		load_dependencies(iter);
	}

	for (iter = gLoadedImages.head; iter; iter = iter->next) {
		bool relocateSuccess;

		relocateSuccess = relocate_image(iter);
		FATAL(!relocateSuccess, "troubles relocating\n");
	}

	init_dependencies(image, true);

	rld_unlock();
	return image->id;
}


status_t
unload_library(image_id imageID)
{
	status_t status;
	image_t *image;

	rld_lock();
		// for now, just do stupid simple global locking

	/*
	 * we only check images that have been already initialized
	 */

	for (image = gLoadedImages.head; image; image = image->next) {
		if (image->id == imageID) {
			/*
			 * do the unloading
			 */
			put_image(image);
			break;
		}
	}

	status = image ? B_OK : B_BAD_IMAGE_ID;

	while ((image = gDisposableImages.head) != NULL) {
		// call image fini here...
		if (image->term_routine)
			((libinit_f *)image->term_routine)(image->id, gProgramArgs);

		dequeue_image(&gDisposableImages, image);
		unmap_image(image);

		delete_image(image);
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

/*
 * init routine, just get hold of the user-space program args
 */

void
rldelf_init(struct uspace_program_args const *_args)
{
	gProgramArgs = _args;

	rld_sem = create_sem(1, "rld_lock\n");
	rld_sem_owner = -1;
	rld_sem_count = 0;
}
