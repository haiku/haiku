/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <string.h>
#include <stdio.h>
#include <Errors.h>
#include <elf32.h>
#include <user_runtime.h>
#include <syscalls.h>
#include <arch/cpu.h>
#include <sem.h>

#include "rld_priv.h"


#define	PAGE_SIZE 4096
#define	PAGE_MASK ((PAGE_SIZE)-1)
#define	PAGE_OFFS(y) ((y)&(PAGE_MASK))
#define	PAGE_BASE(y) ((y)&~(PAGE_MASK))

/* david - added '_' to avoid macro's in kernel.h */
#define	_ROUNDOWN(x,y) ((x)&~((y)-1))
#define	_ROUNDUP(x,y) ROUNDOWN(x+y-1,y)


enum {
	RFLAG_RW             = 0x0001,
	RFLAG_ANON           = 0x0002,

	RFLAG_SORTED         = 0x0400,
	RFLAG_SYMBOLIC       = 0x0800,
	RFLAG_RELOCATED      = 0x1000,
	RFLAG_PROTECTED      = 0x2000,
	RFLAG_INITIALIZED    = 0x4000,
	RFLAG_NEEDAGIRLFRIEND= 0x8000
};


typedef
struct elf_region_t {
	region_id id;
	addr start;
	addr size;
	addr vmstart;
	addr vmsize;
	addr fdstart;
	addr fdsize;
	long delta;
	unsigned flags;
} elf_region_t;


typedef
struct image_t {
	/*
	 * image identification
	 */
	char     name[SYS_MAX_OS_NAME_LEN];
	dynmodule_id imageid;

	struct   image_t *next;
	struct   image_t *prev;
	int      refcount;
	unsigned flags;

	addr entry_point;
	addr dynamic_ptr; // pointer to the dynamic section


	// pointer to symbol participation data structures
	unsigned int      *symhash;
	struct Elf32_Sym  *syms;
	char              *strtab;
	struct Elf32_Rel  *rel;
	int                rel_len;
	struct Elf32_Rela *rela;
	int                rela_len;
	struct Elf32_Rel  *pltrel;
	int                pltrel_len;

	unsigned           num_needed;
	struct image_t   **needed;

	// describes the text and data regions
	unsigned     num_regions;
	elf_region_t regions[1];
} image_t;


typedef
struct image_queue_t {
	image_t *head;
	image_t *tail;
} image_queue_t;


static image_queue_t loaded_images= { 0, 0 };
static image_queue_t loading_images= { 0, 0 };
static image_queue_t disposable_images= { 0, 0 };
static unsigned      loaded_image_count= 0;
static unsigned      imageid_count= 0;

static sem_id rld_sem;
static struct uspace_prog_args_t const *uspa;


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
	if(x) { \
		printf("rld.so: " y); \
		sys_exit(0); \
	}



static
void
enqueue_image(image_queue_t *queue, image_t *img)
{
	img->next= 0;

	img->prev= queue->tail;
	if(queue->tail) {
		queue->tail->next= img;
	}
	queue->tail= img;
	if(!queue->head) {
		queue->head= img;
	}
}

static
void
dequeue_image(image_queue_t *queue, image_t *img)
{
	if(img->next) {
		img->next->prev= img->prev;
	} else {
		queue->tail= img->prev;
	}

	if(img->prev) {
		img->prev->next= img->next;
	} else {
		queue->head= img->next;
	}

	img->prev= 0;
	img->next= 0;
}

static
unsigned long
elf_hash(const unsigned char *name)
{
	unsigned long hash = 0;
	unsigned long temp;

	while(*name) {
		hash = (hash << 4) + *name++;
		if((temp = hash & 0xf0000000)) {
			hash ^= temp >> 24;
		}
		hash &= ~temp;
	}
	return hash;
}

static
image_t *
find_image(char const *name)
{
	image_t *iter;

	iter= loaded_images.head;
	while(iter) {
		if(strncmp(iter->name, name, sizeof(iter->name)) == 0) {
			return iter;
		}
		iter= iter->next;
	}

	iter= loading_images.head;
	while(iter) {
		if(strncmp(iter->name, name, sizeof(iter->name)) == 0) {
			return iter;
		}
		iter= iter->next;
	}

	return 0;
}

static
int
parse_eheader(struct Elf32_Ehdr *eheader)
{
	if(memcmp(eheader->e_ident, ELF_MAGIC, 4) != 0)
		return ERR_INVALID_BINARY;

	if(eheader->e_ident[4] != ELFCLASS32)
		return ERR_INVALID_BINARY;

	if(eheader->e_phoff == 0)
		return ERR_INVALID_BINARY;

	if(eheader->e_phentsize < sizeof(struct Elf32_Phdr))
		return ERR_INVALID_BINARY;

	return eheader->e_phentsize*eheader->e_phnum;
}

static
int
count_regions(char const *buff, int phnum, int phentsize)
{
	int i;
	int retval;
	struct Elf32_Phdr *pheaders;

	retval= 0;
	for(i= 0; i< phnum; i++) {
		pheaders= (struct Elf32_Phdr *)(buff+i*phentsize);

		switch(pheaders->p_type) {
			case PT_NULL:
				/* NOP header */
				break;
			case PT_LOAD:
				retval+= 1;
				if(pheaders->p_memsz!= pheaders->p_filesz) {
					unsigned A= pheaders->p_vaddr+pheaders->p_memsz;
					unsigned B= pheaders->p_vaddr+pheaders->p_filesz;

					A= PAGE_BASE(A);
					B= PAGE_BASE(B);

					if(A!= B) {
						retval+= 1;
					}
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
				FATAL(true, "unhandled pheader type 0x%x\n", pheaders[i].p_type);
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
static
image_t *
create_image(char const *name, int num_regions)
{
	image_t *retval;
	size_t   alloc_size;

	alloc_size= sizeof(image_t)+(num_regions-1)*sizeof(elf_region_t);

	retval= rldalloc(alloc_size);

	memset(retval, 0, alloc_size);

	strlcpy(retval->name, name, sizeof(retval->name));
	retval->imageid= imageid_count;
	retval->refcount= 1;
	retval->num_regions= num_regions;

	imageid_count+= 1;

	return retval;
}

static
void
destroy_image(image_t *image)
{
	size_t alloc_size;

	alloc_size= sizeof(image_t)+(image->num_regions-1)*sizeof(elf_region_t);

	memset(image->needed, 0xa5, sizeof(image->needed[0])*image->num_needed);
	rldfree(image->needed);

	memset(image, 0xa5, alloc_size);
	rldfree(image);
}



static
void
parse_program_headers(image_t *image, char *buff, int phnum, int phentsize)
{
	int i;
	int regcount;
	struct Elf32_Phdr *pheaders;

	regcount= 0;
	for(i= 0; i< phnum; i++) {
		pheaders= (struct Elf32_Phdr *)(buff+i*phentsize);

		switch(pheaders->p_type) {
			case PT_NULL:
				/* NOP header */
				break;
			case PT_LOAD:
				if(pheaders->p_memsz== pheaders->p_filesz) {
					/*
					 * everything in one area
					 */
					image->regions[regcount].start  = pheaders->p_vaddr;
					image->regions[regcount].size   = pheaders->p_memsz;
					image->regions[regcount].vmstart= _ROUNDOWN(pheaders->p_vaddr, PAGE_SIZE);
					image->regions[regcount].vmsize = _ROUNDUP (pheaders->p_memsz + (pheaders->p_vaddr % PAGE_SIZE), PAGE_SIZE);
					image->regions[regcount].fdstart= pheaders->p_offset;
					image->regions[regcount].fdsize = pheaders->p_filesz;
					image->regions[regcount].delta= 0;
					image->regions[regcount].flags= 0;
					if(pheaders->p_flags & PF_W) {
						// this is a writable segment
						image->regions[regcount].flags|= RFLAG_RW;
					}
				} else {
					/*
					 * may require splitting
					 */
					unsigned A= pheaders->p_vaddr+pheaders->p_memsz;
					unsigned B= pheaders->p_vaddr+pheaders->p_filesz;

					A= PAGE_BASE(A);
					B= PAGE_BASE(B);

					image->regions[regcount].start  = pheaders->p_vaddr;
					image->regions[regcount].size   = pheaders->p_filesz;
					image->regions[regcount].vmstart= _ROUNDOWN(pheaders->p_vaddr, PAGE_SIZE);
					image->regions[regcount].vmsize = _ROUNDUP (pheaders->p_filesz + (pheaders->p_vaddr % PAGE_SIZE), PAGE_SIZE);
					image->regions[regcount].fdstart= pheaders->p_offset;
					image->regions[regcount].fdsize = pheaders->p_filesz;
					image->regions[regcount].delta= 0;
					image->regions[regcount].flags= 0;
					if(pheaders->p_flags & PF_W) {
						// this is a writable segment
						image->regions[regcount].flags|= RFLAG_RW;
					}

					if(A!= B) {
						/*
						 * yeah, it requires splitting
						 */
						regcount+= 1;
						image->regions[regcount].start  = pheaders->p_vaddr;
						image->regions[regcount].size   = pheaders->p_memsz - pheaders->p_filesz;
						image->regions[regcount].vmstart= image->regions[regcount-1].vmstart + image->regions[regcount-1].vmsize;
						image->regions[regcount].vmsize = _ROUNDUP (pheaders->p_memsz + (pheaders->p_vaddr % PAGE_SIZE), PAGE_SIZE) - image->regions[regcount-1].vmsize;
						image->regions[regcount].fdstart= 0;
						image->regions[regcount].fdsize = 0;
						image->regions[regcount].delta= 0;
						image->regions[regcount].flags= RFLAG_ANON;
						if(pheaders->p_flags & PF_W) {
							// this is a writable segment
							image->regions[regcount].flags|= RFLAG_RW;
						}
					}
				}
				regcount+= 1;
				break;
			case PT_DYNAMIC:
				image->dynamic_ptr = pheaders->p_vaddr;
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
				FATAL(true, "unhandled pheader type 0x%x\n", pheaders[i].p_type);
				break;
		}
	}
}

static
bool
assert_dynamic_loadable(image_t *image)
{
	unsigned i;

	if(!image->dynamic_ptr) {
		return true;
	}

	for(i= 0; i< image->num_regions; i++) {
		if(image->dynamic_ptr>= image->regions[i].start) {
			if(image->dynamic_ptr< image->regions[i].start+image->regions[i].size) {
				return true;
			}
		}
	}

	return false;
}

static
bool
map_image(int fd, char const *path, image_t *image, bool fixed)
{
	unsigned i;

	(void)(fd);

	for(i= 0; i< image->num_regions; i++) {
		char     region_name[256];
		addr     load_address;
		unsigned addr_specifier;

		sprintf(
			region_name,
			"%s:seg_%d(%s)",
			path,
			i,
			(image->regions[i].flags&RFLAG_RW)?"RW":"RO"
		);

		if(image->dynamic_ptr && !fixed) {
			/*
			 * relocatable image... we can afford to place wherever
			 */
			if(i== 0) {
				/*
				 * but only the first segment gets a free ride
				 */
				load_address= 0;
				addr_specifier= REGION_ADDR_ANY_ADDRESS;
			} else {
				load_address= image->regions[i].vmstart + image->regions[i-1].delta;
				addr_specifier= REGION_ADDR_EXACT_ADDRESS;
			}
		} else {
			/*
			 * not relocatable, put it where it asks or die trying
			 */
			load_address= image->regions[i].vmstart;
			addr_specifier= REGION_ADDR_EXACT_ADDRESS;
		}

		if(image->regions[i].flags & RFLAG_ANON) {
			image->regions[i].id= sys_vm_create_anonymous_region(
				region_name,
				(void **)&load_address,
				addr_specifier,
				image->regions[i].vmsize,
				REGION_WIRING_LAZY,
				LOCK_RW
			);

			if(image->regions[i].id < 0) {
				goto error;
			}
			image->regions[i].delta  = load_address - image->regions[i].vmstart;
			image->regions[i].vmstart= load_address;
		} else {
			image->regions[i].id= sys_vm_map_file(
				region_name,
				(void **)&load_address,
				addr_specifier,
				image->regions[i].vmsize,
				LOCK_RW,
				REGION_PRIVATE_MAP,
				path,
				_ROUNDOWN(image->regions[i].fdstart, PAGE_SIZE)
			);
			if(image->regions[i].id < 0) {
				goto error;
			}
			image->regions[i].delta  = load_address - image->regions[i].vmstart;
			image->regions[i].vmstart= load_address;

			/*
			 * handle trailer bits in data segment
			 */
			if(image->regions[i].flags & RFLAG_RW) {
				unsigned start_clearing;
				unsigned to_clear;

				start_clearing=
					image->regions[i].vmstart
					+ PAGE_OFFS(image->regions[i].start)
					+ image->regions[i].size;
				to_clear=
					image->regions[i].vmsize
					- PAGE_OFFS(image->regions[i].start)
					- image->regions[i].size;
				memset((void*)start_clearing, 0, to_clear);
			}
		}
	}

	if(image->dynamic_ptr) {
		image->dynamic_ptr+= image->regions[0].delta;
	}

	return true;

error:
	return false;
}

static
void
unmap_image(image_t *image)
{
	unsigned i;

	for(i= 0; i< image->num_regions; i++) {
		sys_vm_delete_region(image->regions[i].id);

		image->regions[i].id= -1;
	}
}

static
bool
parse_dynamic_segment(image_t *image)
{
	struct Elf32_Dyn *d;
	int i;

	image->symhash = 0;
	image->syms = 0;
	image->strtab = 0;

	d = (struct Elf32_Dyn *)image->dynamic_ptr;
	if(!d) {
		return true;
	}

	for(i=0; d[i].d_tag != DT_NULL; i++) {
		switch(d[i].d_tag) {
			case DT_NEEDED:
				image->num_needed+= 1;
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
			// TK: procedure linkage table
			case DT_JMPREL:
				image->pltrel = (struct Elf32_Rel *)(d[i].d_un.d_ptr + image->regions[0].delta);
				break;
			case DT_PLTRELSZ:
				image->pltrel_len = d[i].d_un.d_val;
				break;
			default:
				continue;
		}
	}

	// lets make sure we found all the required sections
	if(!image->symhash || !image->syms || !image->strtab) {
		return false;
	}

	return true;
}

static
struct Elf32_Sym *
find_symbol_xxx(image_t *img, const char *name)
{
	unsigned int hash;
	unsigned int i;

	if(img->dynamic_ptr) {
		hash = elf_hash(name) % HASHTABSIZE(img);
		for(i = HASHBUCKETS(img)[hash]; i != STN_UNDEF; i = HASHCHAINS(img)[i]) {
			if(img->syms[i].st_shndx!= SHN_UNDEF) {
				if((ELF32_ST_BIND(img->syms[i].st_info)== STB_GLOBAL) || (ELF32_ST_BIND(img->syms[i].st_info)== STB_WEAK)) {
					if(!strcmp(SYMNAME(img, &img->syms[i]), name)) {
						return &img->syms[i];
					}
				}
			}
		}
	}

	return NULL;
}

static
struct Elf32_Sym *
find_symbol(image_t **shimg, const char *name)
{
	image_t *iter;
	unsigned int hash;
	unsigned int i;

	iter= loaded_images.head;
	while(iter) {
		if(iter->dynamic_ptr) {
			hash = elf_hash(name) % HASHTABSIZE(iter);
			for(i = HASHBUCKETS(iter)[hash]; i != STN_UNDEF; i = HASHCHAINS(iter)[i]) {
				if(iter->syms[i].st_shndx!= SHN_UNDEF) {
					if((ELF32_ST_BIND(iter->syms[i].st_info)== STB_GLOBAL) || (ELF32_ST_BIND(iter->syms[i].st_info)== STB_WEAK)) {
						if(!strcmp(SYMNAME(iter, &iter->syms[i]), name)) {
							*shimg= iter;
							return &iter->syms[i];
						}
					}
				}
			}
		}

		iter= iter->next;
	}

	return NULL;
}

static
int
resolve_symbol(image_t *image, struct Elf32_Sym *sym, addr *sym_addr)
{
	struct Elf32_Sym *sym2;
	char             *symname;
	image_t          *shimg;

	switch(sym->st_shndx) {
		case SHN_UNDEF:
			// patch the symbol name
			symname= SYMNAME(image, sym);

			// it's undefined, must be outside this image, try the other image
			sym2 = find_symbol(&shimg, symname);
			if(!sym2) {
				printf("elf_resolve_symbol: could not resolve symbol '%s'\n", symname);
				return ERR_ELF_RESOLVING_SYMBOL;
			}

			// make sure they're the same type
			if(ELF32_ST_TYPE(sym->st_info)!= STT_NOTYPE) {
				if(ELF32_ST_TYPE(sym->st_info) != ELF32_ST_TYPE(sym2->st_info)) {
					printf("elf_resolve_symbol: found symbol '%s' in shared image but wrong type\n", symname);
					return ERR_ELF_RESOLVING_SYMBOL;
				}
			}

			if(ELF32_ST_BIND(sym2->st_info) != STB_GLOBAL && ELF32_ST_BIND(sym2->st_info) != STB_WEAK) {
				printf("elf_resolve_symbol: found symbol '%s' but not exported\n", symname);
				return ERR_ELF_RESOLVING_SYMBOL;
			}

			*sym_addr = sym2->st_value + shimg->regions[0].delta;
			return B_NO_ERROR;
		case SHN_ABS:
			*sym_addr = sym->st_value + image->regions[0].delta;
			return B_NO_ERROR;
		case SHN_COMMON:
			// XXX finish this
			printf("elf_resolve_symbol: COMMON symbol, finish me!\n");
			return ERR_NOT_IMPLEMENTED_YET;
		default:
			// standard symbol
			*sym_addr = sym->st_value + image->regions[0].delta;
			return B_NO_ERROR;
	}
}


#include "arch/rldreloc.inc"


static
image_t *
load_container(char const *path, char const *name, bool fixed)
{
	int      fd;
	int      len;
	int      ph_len;
	char     ph_buff[4096];
	int      num_regions;
	bool     map_success;
	bool     dynamic_success;
	image_t *found;
	image_t *image;

	struct Elf32_Ehdr eheader;

	found= find_image(name);
	if(found) {
		found->refcount+= 1;
		return found;
	}

	fd= sys_open(path, STREAM_TYPE_FILE, 0);
	FATAL((fd< 0), "cannot open file %s\n", path);

	len= sys_read(fd, &eheader, 0, sizeof(eheader));
	FATAL((len!= sizeof(eheader)), "troubles reading ELF header\n");

	ph_len= parse_eheader(&eheader);
	FATAL((ph_len<= 0), "incorrect ELF header\n");
	FATAL((ph_len> (int)sizeof(ph_buff)), "cannot handle Program headers bigger than %lu\n", (long unsigned)sizeof(ph_buff));

	len= sys_read(fd, ph_buff, eheader.e_phoff, ph_len);
	FATAL((len!= ph_len), "troubles reading Program headers\n");

	num_regions= count_regions(ph_buff, eheader.e_phnum, eheader.e_phentsize);
	FATAL((num_regions<= 0), "troubles parsing Program headers, num_regions= %d\n", num_regions);

	image= create_image(name, num_regions);
	FATAL((!image), "failed to allocate image_t control block\n");

	parse_program_headers(image, ph_buff, eheader.e_phnum, eheader.e_phentsize);
	FATAL(!assert_dynamic_loadable(image), "dynamic segment must be loadable (implementation restriction)\n");

	map_success= map_image(fd, path, image, fixed);
	FATAL(!map_success, "troubles reading image\n");

	dynamic_success= parse_dynamic_segment(image);
	FATAL(!dynamic_success, "troubles handling dynamic section\n");

	image->entry_point= eheader.e_entry + image->regions[0].delta;

	sys_close(fd);

	enqueue_image(&loaded_images, image);

	return image;
}


static
void
load_dependencies(image_t *img)
{
	unsigned i;
	unsigned j;

	struct Elf32_Dyn *d;
	addr   needed_offset;
	char   path[256];

	d = (struct Elf32_Dyn *)img->dynamic_ptr;
	if(!d) {
		return;
	}

	img->needed= rldalloc(img->num_needed*sizeof(image_t*));
	FATAL((!img->needed), "failed to allocate needed struct\n");
	memset(img->needed, 0, img->num_needed*sizeof(image_t*));

	for(i=0, j= 0; d[i].d_tag != DT_NULL; i++) {
		switch(d[i].d_tag) {
			case DT_NEEDED:
				needed_offset = d[i].d_un.d_ptr;
				sprintf(path, "/boot/lib/%s", STRING(img, needed_offset));
				img->needed[j]= load_container(path, STRING(img, needed_offset), false);
				j+= 1;

				break;
			default:
				/*
				 * ignore any other tag
				 */
				continue;
		}
	}

	FATAL((j!= img->num_needed), "Internal error at load_dependencies()");

	return;
}

static
unsigned
topological_sort(image_t *img, unsigned slot, image_t **init_list)
{
	unsigned i;

	img->flags|= RFLAG_SORTED; /* make sure we don't visit this one */
	for(i= 0; i< img->num_needed; i++) {
		if(!(img->needed[i]->flags & RFLAG_SORTED)) {
			slot= topological_sort(img->needed[i], slot, init_list);
		}
	}

	init_list[slot]= img;
	return slot+1;
}

static
void
init_dependencies(image_t *img, bool init_head)
{
	unsigned i;
	unsigned slot;
	image_t **init_list;

	init_list= rldalloc(loaded_image_count*sizeof(image_t*));
	FATAL((!init_list), "memory shortage in init_dependencies()");
	memset(init_list, 0, loaded_image_count*sizeof(image_t*));

	img->flags|= RFLAG_SORTED; /* make sure we don't visit this one */
	slot= 0;
	for(i= 0; i< img->num_needed; i++) {
		if(!(img->needed[i]->flags & RFLAG_SORTED)) {
			slot= topological_sort(img->needed[i], slot, init_list);
		}
	}

	if(init_head) {
		init_list[slot]= img;
		slot+= 1;
	}

	for(i= 0; i< slot; i++) {
		addr _initf= init_list[i]->entry_point;
		libinit_f *initf= (libinit_f *)(_initf);

		if(initf) {
			initf(init_list[i]->imageid, uspa);
		}
	}

	rldfree(init_list);
}


static
void
put_image(image_t *img)
{
	img->refcount-= 1;
	if(img->refcount== 0) {
		size_t i;

		dequeue_image(&loaded_images, img);
		enqueue_image(&disposable_images, img);

		for(i= 0; i< img->num_needed; i++) {
			put_image(img->needed[i]);
		}
	}
}


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
dynmodule_id
load_program(char const *path, void **entry)
{
	image_t *image;
	image_t *iter;


	image = load_container(path, NEWOS_MAGIC_APPNAME, true);

	iter= loaded_images.head;
	while(iter) {
		load_dependencies(iter);

		iter= iter->next;
	};

	iter= loaded_images.head;
	while(iter) {
		bool relocate_success;

		relocate_success= relocate_image(iter);
		FATAL(!relocate_success, "troubles relocating\n");

		iter= iter->next;
	};

	init_dependencies(loaded_images.head, false);

	*entry= (void*)(image->entry_point);
	return image->imageid;
}

dynmodule_id
load_library(char const *path)
{
	image_t *image;
	image_t *iter;


	image = find_image(path);
	if(image) {
		image->refcount+= 1;
		return image->imageid;
	}

	image = load_container(path, path, false);

	iter= loaded_images.head;
	while(iter) {
		load_dependencies(iter);

		iter= iter->next;
	};

	iter= loaded_images.head;
	while(iter) {
		bool relocate_success;

		relocate_success= relocate_image(iter);
		FATAL(!relocate_success, "troubles relocating\n");

		iter= iter->next;
	};

	init_dependencies(image, true);

	return image->imageid;
}

dynmodule_id
unload_library(dynmodule_id imid)
{
	int retval;
	image_t *iter;

	/*
	 * we only check images that have been already initialized
	 */
	iter= loaded_images.head;
	while(iter) {
		if(iter->imageid== imid) {
			/*
			 * do the unloading
			 */
			put_image(iter);

			break;
		}

		iter= iter->next;
	}

	if(iter) {
		retval= 0;
	} else {
		retval= -1;
	}

	iter= disposable_images.head;
	while(iter) {
		// call image fini here...

		dequeue_image(&disposable_images, iter);
		unmap_image(iter);

		destroy_image(iter);
		iter= disposable_images.head;
	}


	return retval;
}

void *
dynamic_symbol(dynmodule_id imid, char const *symname)
{
	image_t *iter;

	/*
	 * we only check images that have been already initialized
	 */
	iter= loaded_images.head;
	while(iter) {
		if(iter->imageid== imid) {
			struct Elf32_Sym *sym= find_symbol_xxx(iter, symname);

			if(sym) {
				return (void*)(sym->st_value + iter->regions[0].delta);
			}
		}

		iter= iter->next;
	}

	return NULL;
}

/*
 * init routine, just get hold of the uspa args
 */
void
rldelf_init(struct uspace_prog_args_t const *_uspa)
{
	uspa= _uspa;

	rld_sem= create_sem(1, "rld_lock\n");
}
