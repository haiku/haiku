/* Contains the ELF loader */

/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <Errors.h>
#include <kerrors.h>
#include <elf.h>
#include <vfs.h>
#include <vm.h>
#include <thread.h>
#include <debug.h>
#include <malloc.h>
#include <stdlib.h>

#include <arch/cpu.h>
#include <arch/elf.h>
#include <elf_priv.h>

#include <string.h>
#include <stdio.h>

#define ELF_TRACE 0
#if ELF_TRACE
#	define PRINT(x) dprintf x
#	define TRACE(x) dprintf x
#else
#	define PRINT(x) ;
#	define TRACE(x) ;
#endif


// XXX TK this shall contain a list of linked images
//        (don't know enough about ELF how to get this list)
typedef struct elf_linked_image {
	struct elf_linked_image *next;
	struct elf_image_info *image;
} elf_linked_image;

static struct elf_image_info *kernel_images = NULL;
static struct elf_image_info *kernel_image = NULL;
static mutex image_lock;
static mutex image_load_lock;
static image_id next_image_id = 0;


int
elf_lookup_symbol_address(addr address, addr *baseAddress, char *text, size_t length)
{
	struct elf_image_info **ptr;
	struct elf_image_info *image;
	struct Elf32_Sym *sym;
	struct elf_image_info *found_image;
	struct Elf32_Sym *found_sym;
	long found_delta;
	uint32 i;
	int j,rv;

	PRINT(("looking up %p\n",(void *)address));

	mutex_lock(&image_lock);

	found_sym = 0;
	found_image = 0;
	found_delta = 0x7fffffff;

	for (ptr = &kernel_images; *ptr; ptr = &(*ptr)->next) {
		image = *ptr;

		PRINT((" image %p, base = %p, size = %p\n", image, (void *)image->regions[0].start, (void *)image->regions[0].size));
		if ((address < image->regions[0].start) || (address >= (image->regions[0].start + image->regions[0].size)))
			continue;

		PRINT((" searching...\n"));
		found_image = image;
		for (i = 0; i < HASHTABSIZE(image); i++) {
			for (j = HASHBUCKETS(image)[i]; j != STN_UNDEF; j = HASHCHAINS(image)[j]) {
				long d;
				sym = &image->syms[j];

				PRINT(("  %p looking at %s, type = %d, bind = %d, addr = %p\n",sym,SYMNAME(image, sym),ELF32_ST_TYPE(sym->st_info),ELF32_ST_BIND(sym->st_info),(void *)sym->st_value));
				PRINT(("  symbol: %lx (%x + %lx)\n", sym->st_value + image->regions[0].delta, sym->st_value, image->regions[0].delta));

				if ((ELF32_ST_TYPE(sym->st_info) != STT_FUNC) || (ELF32_ST_BIND(sym->st_info) != STB_GLOBAL))
					continue;

				d = (long)address - (long)(sym->st_value + image->regions[0].delta);
				if ((d >= 0) && (d < found_delta)) {
					found_delta = d;
					found_sym = sym;
				}
			}
		}
		break;
	}

	if (found_sym != 0) {
		PRINT(("symbol at %p, in image %p, name = %s\n", found_sym, found_image, found_image->name));
		PRINT(("name index %d, '%s'\n", found_sym->st_name, SYMNAME(found_image, found_sym)));
		PRINT(("addr = %#lx, offset = %#lx\n",(found_sym->st_value + found_image->regions[0].delta),found_delta));

		strlcpy(text, SYMNAME(found_image, found_sym), length);
		
		if (baseAddress)
			*baseAddress = found_sym->st_value + found_image->regions[0].delta;

		rv = 0;
	} else {
		PRINT(("symbol not found!\n"));
		strlcpy(text, "symbol not found", length);
		rv = -1;
	}

	mutex_unlock(&image_lock);
	return rv;
}


static int 
print_address_info(int argc, char **argv)
{
	char text[128];
	long address;

	if (argc < 2) {
		dprintf("not enough arguments\n");
		return 0;
	}

	address = atoul(argv[1]);

	elf_lookup_symbol_address(address, NULL, text, sizeof(text));
	dprintf("%p = %s\n",(void *)address,text);
	return 0;
}


static void
insert_image_in_list(struct elf_image_info *image)
{
	mutex_lock(&image_lock);

	image->next = kernel_images;
	kernel_images = image;

	mutex_unlock(&image_lock);
}


static void
remove_image_from_list(struct elf_image_info *image)
{
	struct elf_image_info **ptr;

	mutex_lock(&image_lock);

	for(ptr = &kernel_images; *ptr; ptr = &(*ptr)->next) {
		if(*ptr == image) {
			*ptr = image->next;
			image->next = 0;
			break;
		}
	}

	mutex_unlock(&image_lock);
}


static struct elf_image_info *
find_image(image_id id)
{
	struct elf_image_info *image;

	mutex_lock(&image_lock);

	for(image = kernel_images; image; image = image->next) {
		if(image->id == id)
			break;
	}
	mutex_unlock(&image_lock);

	return image;
}


static struct elf_image_info *
find_image_by_vnode(void *vnode)
{
	struct elf_image_info *image;

	mutex_lock(&image_lock);

	for (image = kernel_images; image; image = image->next) {
		if (image->vnode == vnode)
			break;
	}
	mutex_unlock(&image_lock);

	return image;
}


static struct elf_image_info *
create_image_struct()
{
	struct elf_image_info *image;

	image = (struct elf_image_info *)malloc(sizeof(struct elf_image_info));
	if(!image)
		return NULL;
	memset(image, 0, sizeof(struct elf_image_info));
	image->regions[0].id = -1;
	image->regions[1].id = -1;
	image->id = atomic_add(&next_image_id, 1);
	image->ref_count = 1;
	image->linked_images = NULL;
	return image;
}


static unsigned long
elf_hash(const unsigned char *name)
{
	unsigned long hash = 0;
	unsigned long temp;

	while(*name) {
		hash = (hash << 4) + *name++;
		if((temp = hash & 0xf0000000))
			hash ^= temp >> 24;
		hash &= ~temp;
	}
	return hash;
}


static void
dump_image_info(struct elf_image_info *image)
{
	int i;

	dprintf("elf_image_info at %p:\n", image);
	dprintf(" next %p\n", image->next);
	dprintf(" id 0x%lx\n", image->id);
	for (i = 0; i < 2; i++) {
		dprintf(" regions[%d].id 0x%lx\n", i, image->regions[i].id);
		dprintf(" regions[%d].start 0x%lx\n", i, image->regions[i].start);
		dprintf(" regions[%d].size 0x%lx\n", i, image->regions[i].size);
		dprintf(" regions[%d].delta %ld\n", i, image->regions[i].delta);
	}
	dprintf(" dynamic_ptr 0x%lx\n", image->dynamic_ptr);
	dprintf(" needed %p\n", image->needed);
	dprintf(" symhash %p\n", image->symhash);
	dprintf(" syms %p\n", image->syms);
	dprintf(" strtab %p\n", image->strtab);
	dprintf(" rel %p\n", image->rel);
	dprintf(" rel_len 0x%x\n", image->rel_len);
	dprintf(" rela %p\n", image->rela);
	dprintf(" rela_len 0x%x\n", image->rela_len);
	dprintf(" pltrel %p\n", image->pltrel);
	dprintf(" pltrel_len 0x%x\n", image->pltrel_len);
}

/* XXX - Currently unused
static void dump_symbol(struct elf_image_info *image, struct Elf32_Sym *sym)
{

	dprintf("symbol at %p, in image %p\n", sym, image);

	dprintf(" name index %d, '%s'\n", sym->st_name, SYMNAME(image, sym));
	dprintf(" st_value 0x%x\n", sym->st_value);
	dprintf(" st_size %d\n", sym->st_size);
	dprintf(" st_info 0x%x\n", sym->st_info);
	dprintf(" st_other 0x%x\n", sym->st_other);
	dprintf(" st_shndx %d\n", sym->st_shndx);
}
*/

static struct Elf32_Sym *
elf_find_symbol(struct elf_image_info *image, const char *name)
{
	uint32 hash;
	uint32 i;

	if (!image->dynamic_ptr)
		return NULL;

	hash = elf_hash(name) % HASHTABSIZE(image);
	for (i = HASHBUCKETS(image)[hash]; i != STN_UNDEF; i = HASHCHAINS(image)[i]) {
		if (!strcmp(SYMNAME(image, &image->syms[i]), name))
			return &image->syms[i];
	}

	return NULL;
}


addr
elf_lookup_symbol(image_id id, const char *symbol)
{
	struct elf_image_info *image;
	struct Elf32_Sym *sym;

	PRINT(("elf_lookup_symbol: %s\n", symbol));

	image = find_image(id);
	if (!image)
		return 0;

	sym = elf_find_symbol(image, symbol);
	if (!sym)
		return 0;

	if (sym->st_shndx == SHN_UNDEF)
		return 0;

	PRINT(("found: %lx (%x + %lx)\n", sym->st_value + image->regions[0].delta, 
		sym->st_value, image->regions[0].delta));

	return sym->st_value + image->regions[0].delta;
}


static int
elf_parse_dynamic_section(struct elf_image_info *image)
{
	struct Elf32_Dyn *d;
	int i;
	int needed_offset = -1;

	PRINT(("top of elf_parse_dynamic_section\n"));

	image->symhash = 0;
	image->syms = 0;
	image->strtab = 0;

	d = (struct Elf32_Dyn *)image->dynamic_ptr;
	if (!d)
		return ERR_GENERAL;

	for (i = 0; d[i].d_tag != DT_NULL; i++) {
		switch (d[i].d_tag) {
			case DT_NEEDED:
				needed_offset = d[i].d_un.d_ptr + image->regions[0].delta;
				break;
			case DT_HASH:
				image->symhash = (unsigned int *)(d[i].d_un.d_ptr + image->regions[0].delta);
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
			case DT_JMPREL:
				image->pltrel = (struct Elf32_Rel *)(d[i].d_un.d_ptr + image->regions[0].delta);
				break;
			case DT_PLTRELSZ:
				image->pltrel_len = d[i].d_un.d_val;
				break;
			case DT_PLTREL:
				image->pltrel_type = d[i].d_un.d_val;
				break;

			default:
				continue;
		}
	}

	// lets make sure we found all the required sections
	if (!image->symhash || !image->syms || !image->strtab)
		return ERR_GENERAL;

	PRINT(("needed_offset = %d\n", needed_offset));

	if (needed_offset >= 0)
		image->needed = STRING(image, needed_offset);

	return B_NO_ERROR;
}


/**	this function first tries to see if the first image and it's already resolved symbol is okay, otherwise
 *	it tries to link against the shared_image
 *	XXX gross hack and needs to be done better
 */

int
elf_resolve_symbol(struct elf_image_info *image, struct Elf32_Sym *sym, struct elf_image_info *shared_image, const char *sym_prepend,addr *sym_addr)
{
	struct Elf32_Sym *sym2;
	char new_symname[512];

	switch (sym->st_shndx) {
		case SHN_UNDEF:
			// patch the symbol name
			strlcpy(new_symname, sym_prepend, sizeof(new_symname));
			strlcat(new_symname, SYMNAME(image, sym), sizeof(new_symname));

			// it's undefined, must be outside this image, try the other image
			sym2 = elf_find_symbol(shared_image, new_symname);
			if (!sym2) {
				dprintf("!sym2: elf_resolve_symbol: could not resolve symbol '%s'\n", new_symname);
				return ERR_ELF_RESOLVING_SYMBOL;
			}

			// make sure they're the same type
			if (ELF32_ST_TYPE(sym->st_info) != ELF32_ST_TYPE(sym2->st_info)) {
				dprintf("elf_resolve_symbol: found symbol '%s' in shared image but wrong type\n", new_symname);
				return ERR_ELF_RESOLVING_SYMBOL;
			}

			if (ELF32_ST_BIND(sym2->st_info) != STB_GLOBAL && ELF32_ST_BIND(sym2->st_info) != STB_WEAK) {
				PRINT(("elf_resolve_symbol: found symbol '%s' but not exported\n", new_symname));
				return ERR_ELF_RESOLVING_SYMBOL;
			}

			*sym_addr = sym2->st_value + shared_image->regions[0].delta;
			return B_NO_ERROR;
		case SHN_ABS:
			*sym_addr = sym->st_value;
			return B_NO_ERROR;
		case SHN_COMMON:
			// XXX finish this
			PRINT(("elf_resolve_symbol: COMMON symbol, finish me!\n"));
			return ERR_NOT_IMPLEMENTED_YET;
		default:
			// standard symbol
			*sym_addr = sym->st_value + image->regions[0].delta;
			return B_NO_ERROR;
	}
}


// XXX for now just link against the kernel

static int
elf_relocate(struct elf_image_info *image, const char *sym_prepend)
{
	int status = B_NO_ERROR;

	PRINT(("top of elf_relocate\n"));

	// deal with the rels first
	if (image->rel) {
		TRACE(("total %i relocs\n", image->rel_len / (int)sizeof(struct Elf32_Rel)));

		status = arch_elf_relocate_rel(image, sym_prepend, kernel_image, image->rel, image->rel_len);
		if (status < B_OK)
			return status;
	}

	if (image->pltrel) {
		TRACE(("total %i plt-relocs\n", image->pltrel_len / (int)sizeof(struct Elf32_Rel)));

		if (image->pltrel_type == DT_REL)
			status = arch_elf_relocate_rel(image, sym_prepend, kernel_image, image->pltrel, image->pltrel_len);
		else
			status = arch_elf_relocate_rela(image, sym_prepend, kernel_image, (struct Elf32_Rela *)image->pltrel, image->pltrel_len);
		if (status < B_OK)
			return status;
	}

	if (image->rela) {
		status = arch_elf_relocate_rela(image, sym_prepend, kernel_image, image->rela, image->rela_len);
		if (status < B_OK)
			return status;
	}

	return status;
}


static int
verify_eheader(struct Elf32_Ehdr *eheader)
{
	if (memcmp(eheader->e_ident, ELF_MAGIC, 4) != 0)
		return ERR_INVALID_BINARY;

	if (eheader->e_ident[4] != ELFCLASS32)
		return ERR_INVALID_BINARY;

	if (eheader->e_phoff == 0)
		return ERR_INVALID_BINARY;

	if (eheader->e_phentsize < sizeof(struct Elf32_Phdr))
		return ERR_INVALID_BINARY;

	return 0;
}


int
elf_load_uspace(const char *path, struct team *p, int flags, addr *entry)
{
	struct Elf32_Ehdr eheader;
	struct Elf32_Phdr *pheaders = NULL;
	char baseName[64];
	int fd;
	int err;
	int i;
	ssize_t len;

	dprintf("elf_load: entry path '%s', team %p\n", path, p);

	fd = sys_open(path, 0);
	if (fd < 0)
		return fd;

	// read and verify the ELF header

	len = sys_read(fd, 0, &eheader, sizeof(eheader));
	if (len < 0) {
		err = len;
		goto error;
	}

	if (len != sizeof(eheader)) {
		// short read
		err = ERR_INVALID_BINARY;
		goto error;
	}
	err = verify_eheader(&eheader);
	if (err < 0)
		goto error;

	// read program header

	pheaders = (struct Elf32_Phdr *)malloc(eheader.e_phnum * eheader.e_phentsize);
	if (pheaders == NULL) {
		dprintf("error allocating space for program headers\n");
		err = ENOMEM;
		goto error;
	}

	dprintf("reading in program headers at 0x%lx, len 0x%x\n", eheader.e_phoff, eheader.e_phnum * eheader.e_phentsize);
	len = sys_read(fd, eheader.e_phoff, pheaders, eheader.e_phnum * eheader.e_phentsize);
	if (len < 0) {
		err = len;
		dprintf("error reading in program headers\n");
		goto error;
	}
	if (len != eheader.e_phnum * eheader.e_phentsize) {
		dprintf("short read while reading in program headers\n");
		err = -1;
		goto error;
	}

	// construct a nice name for the region we have to create below
	{
		int32 length = strlen(path);
		if (length > 52)
			sprintf(baseName, "...%s", path + length - 52);
		else
			strcpy(baseName, path);
	}

	// map the program's segments into memory

	for (i = 0; i < eheader.e_phnum; i++) {
		char regionName[64];
		char *regionAddress;
		region_id id;

		sprintf(regionName, "%s_seg%d", baseName, i);

		regionAddress = (char *)ROUNDOWN(pheaders[i].p_vaddr, PAGE_SIZE);
		if (pheaders[i].p_flags & PF_WRITE) {
			/*
			 * rw segment
			 */
			uint32 memUpperBound = (pheaders[i].p_vaddr % PAGE_SIZE) + pheaders[i].p_memsz;
			uint32 fileUpperBound = (pheaders[i].p_vaddr % PAGE_SIZE) + pheaders[i].p_filesz;

			memUpperBound = ROUNDUP(memUpperBound, PAGE_SIZE);
			fileUpperBound = ROUNDUP(fileUpperBound, PAGE_SIZE);

			id = vm_map_file(p->_aspace_id, regionName,
				(void **)&regionAddress,
				REGION_ADDR_EXACT_ADDRESS,
				fileUpperBound,
				LOCK_RW, REGION_PRIVATE_MAP,
				path, ROUNDOWN(pheaders[i].p_offset, PAGE_SIZE));
			if (id < 0) {
				dprintf("error allocating region!\n");
				err = ERR_INVALID_BINARY;
				goto error;
			}

			// clean garbage brought by mmap (the region behind the file,
			// at least parts of it are the bss and have to be zeroed)
			{
				uint32 start = (uint32)regionAddress
					+ (pheaders[i].p_vaddr % PAGE_SIZE)
					+ pheaders[i].p_filesz;
				uint32 amount = fileUpperBound
					- (pheaders[i].p_vaddr % PAGE_SIZE)
					- (pheaders[i].p_filesz);
				memset((void *)start, 0, amount);
			}

			// Check if we need extra storage for the bss - we have to do this if
			// the above region doesn't already comprise the memory size, too.

			if (memUpperBound != fileUpperBound) {
				size_t bss_size = memUpperBound - fileUpperBound;

				sprintf(regionName, "%s_bss%d", baseName, 'X');

				regionAddress += fileUpperBound;
				id = create_area_etc(p, regionName, (void **)&regionAddress,
					B_EXACT_ADDRESS, bss_size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
				if (id < 0) {
					dprintf("error allocating bss region: %s!\n", strerror(id));
					err = ERR_INVALID_BINARY;
					goto error;
				}
			}
		} else {
			/*
			 * assume rx segment
			 */
			id = vm_map_file(p->_aspace_id, regionName,
				(void **)&regionAddress,
				REGION_ADDR_EXACT_ADDRESS,
				ROUNDUP(pheaders[i].p_memsz + (pheaders[i].p_vaddr % PAGE_SIZE), PAGE_SIZE),
				LOCK_RO, REGION_PRIVATE_MAP,
				path, ROUNDOWN(pheaders[i].p_offset, PAGE_SIZE));
			if (id < 0) {
				dprintf("error mapping text!\n");
				err = ERR_INVALID_BINARY;
				goto error;
			}
		}
	}

	dprintf("elf_load: done!\n");

	*entry = eheader.e_entry;

	err = 0;

error:
	if (pheaders)
		free(pheaders);
	sys_close(fd);

	return err;
}


image_id
elf_load_kspace(const char *path, const char *sym_prepend)
{
	struct Elf32_Ehdr *eheader;
	struct Elf32_Phdr *pheaders;
	struct elf_image_info *image;
	void *vnode = NULL;
	int fd;
	int err;
	int i;
	ssize_t len;

	PRINT(("elf_load_kspace: entry path '%s'\n", path));

	fd = sys_open(path, 0);
	if (fd < 0)
		return fd;

	err = vfs_get_vnode_from_fd(fd, true, &vnode);
	if (err < 0)
		goto error0;

	// XXX awful hack to keep someone else from trying to load this image
	// probably not a bad thing, shouldn't be too many races
	mutex_lock(&image_load_lock);

	// make sure it's not loaded already. Search by vnode
	image = find_image_by_vnode(vnode);
	if (image) {
		atomic_add(&image->ref_count, 1);
		//err = ERR_NOT_ALLOWED;
		goto done;
	}

	eheader = (struct Elf32_Ehdr *)malloc(sizeof(*eheader));
	if (!eheader) {
		err = ENOMEM;
		goto error;
	}

	len = sys_read(fd, 0, eheader, sizeof(*eheader));
	if (len < 0) {
		err = len;
		goto error1;
	}
	if (len != sizeof(*eheader)) {
		// short read
		err = ERR_INVALID_BINARY;
		goto error1;
	}
	err = verify_eheader(eheader);
	if (err < 0)
		goto error1;

	image = create_image_struct();
	if (!image) {
		err = ENOMEM;
		goto error1;
	}
	image->vnode = vnode;
	image->eheader = eheader;
	image->name = strdup(path);

	pheaders = (struct Elf32_Phdr *)malloc(eheader->e_phnum * eheader->e_phentsize);
	if (pheaders == NULL) {
		dprintf("error allocating space for program headers\n");
		err = ENOMEM;
		goto error2;
	}

	PRINT(("reading in program headers at 0x%x, len 0x%x\n", eheader->e_phoff, eheader->e_phnum * eheader->e_phentsize));
	len = sys_read(fd, eheader->e_phoff, pheaders, eheader->e_phnum * eheader->e_phentsize);
	if (len < 0) {
		err = len;
		PRINT(("error reading in program headers\n"));
		goto error3;
	}
	if (len != eheader->e_phnum * eheader->e_phentsize) {
		PRINT(("short read while reading in program headers\n"));
		err = -1;
		goto error3;
	}

	for (i = 0; i < eheader->e_phnum; i++) {
		char region_name[64];
		bool ro_segment_handled = false;
		bool rw_segment_handled = false;
		int image_region;
		int protection;

		PRINT(("looking at program header %d\n", i));

		switch (pheaders[i].p_type) {
			case PT_LOAD:
				break;
			case PT_DYNAMIC:
				image->dynamic_ptr = pheaders[i].p_vaddr;
				continue;
			default:
				dprintf("unhandled pheader type 0x%lx\n", pheaders[i].p_type);
				continue;
		}

		// we're here, so it must be a PT_LOAD segment
		if ((pheaders[i].p_flags & (PF_PROTECTION_MASK)) == (PF_READ | PF_WRITE)) {
			// this is the writable segment
			if (rw_segment_handled) {
				// we've already created this segment
				continue;
			}
			rw_segment_handled = true;
			image_region = 1;
			protection = B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA;
			sprintf(region_name, "%s_rw", path);
		} else if ((pheaders[i].p_flags & (PF_PROTECTION_MASK)) == (PF_READ | PF_EXECUTE)) {
			// this is the non-writable segment
			if (ro_segment_handled) {
				// we've already created this segment
				continue;
			}
			ro_segment_handled = true;
			image_region = 0;
			protection = B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA;
				// ToDo: why is this? Right now, the kernel crashes without
				//	it, so go, investigate!

			sprintf(region_name, "%s_ro", path);
		} else {
			dprintf("weird program header flags 0x%lx\n", pheaders[i].p_flags);
			continue;
		}

		image->regions[image_region].size = ROUNDUP(pheaders[i].p_memsz + (pheaders[i].p_vaddr % PAGE_SIZE), PAGE_SIZE);
		image->regions[image_region].id = create_area(region_name,
			(void **)&image->regions[image_region].start, B_ANY_KERNEL_ADDRESS,
			image->regions[image_region].size, B_FULL_LOCK, protection);
		if (image->regions[image_region].id < 0) {
			dprintf("error allocating region!\n");
			err = ERR_INVALID_BINARY;
			goto error3;
		}
		image->regions[image_region].delta = image->regions[image_region].start - ROUNDOWN(pheaders[i].p_vaddr, PAGE_SIZE);

		PRINT(("elf_load_kspace: created a region at %p\n", (void *)image->regions[image_region].start));

		len = sys_read(fd, pheaders[i].p_offset,
			(void *)(image->regions[image_region].start + (pheaders[i].p_vaddr % PAGE_SIZE)),
			pheaders[i].p_filesz);
		if (len < 0) {
			err = len;
			dprintf("error reading in seg %d\n", i);
			goto error4;
		}
	}

	if (image->regions[1].start != 0
		&& image->regions[0].delta != image->regions[1].delta) {
		dprintf("could not load binary, fix the region problem!\n");
		dump_image_info(image);
		err = ENOMEM;
		goto error4;
	}

	// modify the dynamic ptr by the delta of the regions
	image->dynamic_ptr += image->regions[0].delta;

	err = elf_parse_dynamic_section(image);
	if (err < 0)
		goto error4;

	err = elf_relocate(image, sym_prepend);
	if (err < 0)
		goto error4;

	err = 0;

	free(pheaders);
	sys_close(fd);

	insert_image_in_list(image);

done:
	mutex_unlock(&image_load_lock);
	
	return image->id;

error4:
	if (image->regions[1].id >= 0)
		delete_area(image->regions[1].id);
	if (image->regions[0].id >= 0)
		delete_area(image->regions[0].id);
error3:
	free(image);
error2:
	free(pheaders);
error1:
	free(eheader);
error:
	mutex_unlock(&image_load_lock);
error0:
	if (vnode)
		vfs_put_vnode_ptr(vnode);
	sys_close(fd);

	return err;
}


static int elf_unload_image(struct elf_image_info *image);
static int
elf_unlink_relocs(struct elf_image_info *image)
{
	elf_linked_image *link, *next_link;
	
	for (link = image->linked_images; link; link = next_link) {
		next_link = link->next;
		elf_unload_image(link->image);
		free(link);
	}
	
	return B_NO_ERROR;
}


static void
elf_unload_image_final(struct elf_image_info *image)
{
	int i;
	
	for (i = 0; i < 2; ++i) {
		delete_area(image->regions[i].id);
	}

	if (image->vnode)
		vfs_put_vnode_ptr(image->vnode);

	remove_image_from_list(image);
	free(image->eheader);
	free(image->name);	
	free(image);
}


static int
elf_unload_image(struct elf_image_info *image)
{
	if (atomic_add(&image->ref_count, -1) > 0)
		return B_NO_ERROR;

	elf_unlink_relocs(image);
	elf_unload_image_final(image);

	return B_NO_ERROR;
}


int
elf_unload_kspace(const char *path)
{
	int fd;
	int err;
	void *vnode;
	struct elf_image_info *image;
	
	fd = sys_open(path, 0);
	if (fd < 0)
		return fd;

	err = vfs_get_vnode_from_fd(fd, true, &vnode);
	if (err < 0)
		goto error0;

	mutex_lock(&image_load_lock);

	image = find_image_by_vnode(vnode);
	if (!image) {
		dprintf("Tried to unload image that wasn't loaded (%s)\n", path);
		err = ERR_NOT_FOUND;
		goto error;
	}
	
	err = elf_unload_image(image);

error:
	mutex_unlock(&image_load_lock);
error0:
	if(vnode)
		vfs_put_vnode_ptr(vnode);
	sys_close(fd);

	return err;
}


int
elf_init(kernel_args *ka)
{
	area_info areaInfo;

	mutex_init(&image_lock, "kimages_lock");
	mutex_init(&image_load_lock, "kimages_load_lock");

	// build a image structure for the kernel, which has already been loaded

	kernel_image = create_image_struct();
	kernel_image->name = strdup("kernel");

	// text segment
	kernel_image->regions[0].id = find_area("kernel_ro");
	if (kernel_image->regions[0].id < 0)
		panic("elf_init: could not look up kernel text segment region\n");

	get_area_info(kernel_image->regions[0].id, &areaInfo);
	kernel_image->regions[0].start = (addr)areaInfo.address;
	kernel_image->regions[0].size = areaInfo.size;

	// data segment
	kernel_image->regions[1].id = find_area("kernel_rw");
	if (kernel_image->regions[1].id < 0)
		panic("elf_init: could not look up kernel data segment region\n");

	get_area_info(kernel_image->regions[1].id, &areaInfo);
	kernel_image->regions[1].start = (addr)areaInfo.address;
	kernel_image->regions[1].size = areaInfo.size;

	// we know where the dynamic section is
	kernel_image->dynamic_ptr = (addr)ka->kernel_dynamic_section_addr.start;

	// parse the dynamic section
	if (elf_parse_dynamic_section(kernel_image) < 0)
		dprintf("elf_init: WARNING elf_parse_dynamic_section couldn't find dynamic section.\n");

	// insert it first in the list of kernel images loaded
	kernel_images = NULL;
	insert_image_in_list(kernel_image);

	add_debugger_command("ls", &print_address_info, "lookup symbol for a particular address");

	return 0;
}

