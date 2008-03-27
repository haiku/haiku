/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"
#include "vm.h"

#include <OS.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arch/cpu.h>
#include <elf32.h>
#include <runtime_loader.h>
#include <sem.h>
#include <syscalls.h>
#include <user_runtime.h>
#include <vm_types.h>

#include "tracing_config.h"


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
	RFLAG_REMAPPED				= 0x8000,

	RFLAG_VISITED				= 0x10000
		// temporarily set in the symbol resolution code
};


#define IMAGE_TYPE_TO_MASK(type)	(1 << ((type) - 1))
#define ALL_IMAGE_TYPES				(IMAGE_TYPE_TO_MASK(B_APP_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_LIBRARY_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_ADD_ON_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_SYSTEM_IMAGE))
#define APP_OR_LIBRARY_TYPE			(IMAGE_TYPE_TO_MASK(B_APP_IMAGE) \
									| IMAGE_TYPE_TO_MASK(B_LIBRARY_IMAGE))

typedef void (*init_term_function)(image_id);

static image_queue_t sLoadedImages = {0, 0};
static image_queue_t sDisposableImages = {0, 0};
static uint32 sLoadedImageCount = 0;
static image_t *sProgramImage;
static KMessage sErrorMessage;

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


/*!	Mini atoi(), so we don't have to include the libroot dependencies.
 */
int
atoi(const char* num)
{
	int result = 0;
	while (*num >= '0' && *num <= '9') {
		result = (result * 10) + (*num - '0');
		num++;
	}

	return result;
}


#ifdef RUNTIME_LOADER_TRACING

void
ktrace_printf(const char *format, ...)
{
	va_list list;
	va_start(list, format);

	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), format, list);
	_kern_ktrace_output(buffer);

	va_end(list);
}

#define KTRACE(x...)	ktrace_printf(x)

#else
#	define KTRACE(x...)
#endif	// RUNTIME_LOADER_TRACING


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
elf_hash(const uint8 *name)
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


static inline bool
report_errors()
{
	return gProgramArgs->error_port >= 0;
}


static image_t *
find_image_in_queue(image_queue_t *queue, const char *name, bool isPath,
	uint32 typeMask)
{
	image_t *image;

	for (image = queue->head; image; image = image->next) {
		const char *imageName = isPath ? image->path : image->name;
		int length = isPath ? sizeof(image->path) : sizeof(image->name);

		if (!strncmp(imageName, name, length)
			&& (typeMask & IMAGE_TYPE_TO_MASK(image->type)) != 0) {
			return image;
		}
	}

	return NULL;
}


static image_t *
find_image(char const *name, uint32 typeMask)
{
	bool isPath = (strchr(name, '/') != NULL);
	return find_image_in_queue(&sLoadedImages, name, isPath, typeMask);
}


static image_t *
find_loaded_image_by_id(image_id id)
{
	image_t *image;

	for (image = sLoadedImages.head; image; image = image->next) {
		if (image->id == id)
			return image;
	}

	// For the termination routine, we need to look into the list of
	// disposable images as well
	for (image = sDisposableImages.head; image; image = image->next) {
		if (image->id == id)
			return image;
	}

	return NULL;
}


static image_t*
get_program_image()
{
	for (image_t *image = sLoadedImages.head; image; image = image->next) {
		if (image->type == B_APP_IMAGE)
			return image;
	}

	return NULL;
}


static const char *
get_program_path()
{
	if (image_t* image = get_program_image())
		return image->path;

	return NULL;
}


static status_t
parse_elf_header(struct Elf32_Ehdr *eheader, int32 *_pheaderSize,
	int32 *_sheaderSize)
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

	if (*_pheaderSize <= 0 || *_sheaderSize <= 0)
		return B_NOT_AN_EXECUTABLE;

	return B_OK;
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
	size_t allocSize = sizeof(image_t) + (num_regions - 1) * sizeof(elf_region_t);
	const char *lastSlash;

	image_t *image = (image_t*)malloc(allocSize);
	if (image == NULL) {
		FATAL("no memory for image %s\n", path);
		return NULL;
	}

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
	free(image->needed);

#ifdef DEBUG
	memset(image, 0xa5, size);
#endif
	free(image);
}


static void
delete_image(image_t *image)
{
	if (image == NULL)
		return;

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
analyze_object_gcc_version(int fd, image_t* image, Elf32_Ehdr& eheader,
	int32 sheaderSize, char* buffer, size_t bufferSize)
{
	image->gcc_version.major = 0;
	image->gcc_version.middle = 0;
	image->gcc_version.minor = 0;

	if (sheaderSize > (int)bufferSize) {
		FATAL("Cannot handle section headers bigger than %lu\n", bufferSize);
		return false;
	}

	// read section headers
	ssize_t length = _kern_read(fd, eheader.e_shoff, buffer, sheaderSize);
	if (length != sheaderSize) {
		FATAL("Could not read section headers: %s\n", strerror(length));
		return false;
	}

	// load the string section
	Elf32_Shdr* sectionHeader
		= (Elf32_Shdr*)(buffer + eheader.e_shstrndx * eheader.e_shentsize);

	if (sheaderSize + sectionHeader->sh_size > bufferSize) {
		FATAL("Buffer not big enough for section string section\n");
		return false;
	}

	char* sectionStrings = buffer + bufferSize - sectionHeader->sh_size;
	length = _kern_read(fd, sectionHeader->sh_offset, sectionStrings,
		sectionHeader->sh_size);
	if (length != (int)sectionHeader->sh_size) {
		FATAL("Could not read section string section: %s\n", strerror(length));
		return false;
	}

	// find the .comment section
	off_t commentOffset = 0;
	size_t commentSize = 0;
	for (uint32 i = 0; i < eheader.e_shnum; i++) {
		sectionHeader = (Elf32_Shdr*)(buffer + i * eheader.e_shentsize);
		const char* sectionName = sectionStrings + sectionHeader->sh_name;
		if (sectionHeader->sh_name != 0
			&& strcmp(sectionName, ".comment") == 0) {
			commentOffset = sectionHeader->sh_offset;
			commentSize = sectionHeader->sh_size;
			break;
		}
	}

	if (commentSize == 0) {
		FATAL("Could not find .comment section\n");
		return false;
	}

	// read a part of the comment section
	if (commentSize > 512)
		commentSize = 512;

	length = _kern_read(fd, commentOffset, buffer, commentSize);
	if (length != (int)commentSize) {
		FATAL("Could not read .comment section: %s\n", strerror(length));
		return false;
	}

	// the common prefix of the strings in the .comment section
	static const char* kGCCVersionPrefix = "GCC: (GNU) ";
	size_t gccVersionPrefixLen = strlen(kGCCVersionPrefix);

	size_t index = 0;
	int gccMajor = 0;
	int gccMiddle = 0;
	int gccMinor = 0;

	// Read up to 10 comments. The first three or four are usually from the
	// glue code.
	for (int i = 0; i < 10; i++) {
		// skip '\0'
		while (index < commentSize && buffer[index] == '\0')
			index++;
		char* stringStart = buffer + index;

		// find string end
		while (index < commentSize && buffer[index] != '\0')
			index++;

		// ignore the entry at the end of the buffer
		if (index == commentSize)
			break;

		// We have to analyze string like these:
		// GCC: (GNU) 2.9-beos-991026
		// GCC: (GNU) 2.95.3-haiku-080322
		// GCC: (GNU) 4.1.2

		// skip the common prefix
		if (strncmp(stringStart, kGCCVersionPrefix, gccVersionPrefixLen) != 0)
			continue;

		// the rest is the GCC version
		char* gccVersion = stringStart + gccVersionPrefixLen;
		char* gccPlatform = strchr(gccVersion, '-');
		char* patchLevel = NULL;
		if (gccPlatform != NULL) {
			*gccPlatform = '\0';
			gccPlatform++;
			patchLevel = strchr(gccPlatform, '-');
			if (patchLevel != NULL) {
				*patchLevel = '\0';
				patchLevel++;
			}
		}

		// split the gcc version into major, middle, and minor
		int version[3] = { 0, 0, 0 };

		for (int k = 0; gccVersion != NULL && k < 3; k++) {
			char* dot = strchr(gccVersion, '.');
			if (dot) {
				*dot = '\0';
				dot++;
			}
			version[k] = atoi(gccVersion);
			gccVersion = dot;
		}

		// got any version?
		if (version[0] == 0)
			continue;

		// Select the gcc version with the smallest major, but the greatest
		// middle/minor. This should usually ignore the glue code version as
		// well as cases where e.g. in a gcc 2 program a single C file has
		// been compiled with gcc 4.
		if (gccMajor == 0 || gccMajor > version[0]
		 	|| gccMajor == version[0]
				&& (gccMiddle < version[1]
					|| gccMiddle == version[1] && gccMinor < version[2])) {
			gccMajor = version[0];
			gccMiddle = version[1];
			gccMinor = version[2];
		}
	}

	image->gcc_version.major = gccMajor;
	image->gcc_version.middle = gccMiddle;
	image->gcc_version.minor = gccMinor;

	return gccMajor != 0;
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

	if (image->dynamic_ptr == 0)
		return NULL;

	hash = elf_hash((uint8 *)name) % HASHTABSIZE(image);

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


static struct Elf32_Sym*
find_symbol_recursively_impl(image_t* image, const char* name,
	image_t** foundInImage)
{
	image->flags |= RFLAG_VISITED;

	struct Elf32_Sym *symbol;

	// look up the symbol in this image
	if (image->dynamic_ptr) {
		symbol = find_symbol(image, name, B_SYMBOL_TYPE_ANY);
		if (symbol) {
			*foundInImage = image;
			return symbol;
		}
	}

	// recursively search dependencies
	for (uint32 i = 0; i < image->num_needed; i++) {
		if (!(image->needed[i]->flags & RFLAG_VISITED)) {
			symbol = find_symbol_recursively_impl(image->needed[i], name,
				foundInImage);
			if (symbol)
				return symbol;
		}
	}

	return NULL;
}


static void
clear_image_flag_recursively(image_t* image, uint32 flag)
{
	image->flags &= ~flag;

	for (uint32 i = 0; i < image->num_needed; i++) {
		if (image->needed[i]->flags & flag)
			clear_image_flag_recursively(image->needed[i], flag);
	}
}


static struct Elf32_Sym*
find_symbol_recursively(image_t* image, const char* name,
	image_t** foundInImage)
{
	struct Elf32_Sym* symbol = find_symbol_recursively_impl(image, name,
		foundInImage);
	clear_image_flag_recursively(image, RFLAG_VISITED);
	return symbol;
}


static struct Elf32_Sym*
find_symbol_in_loaded_images(const char* name, image_t** foundInImage)
{
	return find_symbol_recursively(sLoadedImages.head, name, foundInImage);
}


static struct Elf32_Sym*
find_undefined_symbol(image_t* rootImage, image_t* image, const char* name,
	image_t** foundInImage)
{
	// If not simulating BeOS style symbol resolution, undefined symbols are
	// searched recursively starting from the root image.
	// TODO: Breadth first might be better than the depth first strategy used
	// here. We're also visiting images multiple times. Consider building a
	// breadth-first sorted array of images for each root image.
	if ((rootImage->flags & IMAGE_FLAG_R5_SYMBOL_RESOLUTION) == 0) {
		Elf32_Sym* symbol = find_symbol_recursively(rootImage, name,
			foundInImage);
		if (symbol != NULL)
			return symbol;

		// If the root image is not the program image (i.e. it is a dynamically
		// loaded add-on or library), we try the program image hierarchy too.
		image_t* programImage = get_program_image();
		if (rootImage != programImage)
			return find_symbol_recursively(programImage, name, foundInImage);

		return NULL;
	}

	// BeOS style symbol resolution: It is sufficient to check the direct
	// dependencies. The linker would have complained, if the symbol wasn't
	// there.
	for (uint32 i = 0; i < image->num_needed; i++) {
		if (image->needed[i]->dynamic_ptr) {
			struct Elf32_Sym *symbol = find_symbol(image->needed[i], name,
				B_SYMBOL_TYPE_ANY);
			if (symbol) {
				*foundInImage = image->needed[i];
				return symbol;
			}
		}
	}

	return NULL;
}


int
resolve_symbol(image_t *rootImage, image_t *image, struct Elf32_Sym *sym,
	addr_t *sym_addr)
{
	struct Elf32_Sym *sym2;
	char *symname;
	image_t *shimg;

	switch (sym->st_shndx) {
		case SHN_UNDEF:
			// patch the symbol name
			symname = SYMNAME(image, sym);

			// it's undefined, must be outside this image, try the other images
			sym2 = find_undefined_symbol(rootImage, image, symname, &shimg);
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
	info.init_routine = (void (*)())image->init_routine;
	info.term_routine = (void (*)())image->term_routine;
	
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
relocate_image(image_t *rootImage, image_t *image)
{
	status_t status = arch_relocate_image(rootImage, image);
	if (status < B_OK) {
		FATAL("troubles relocating: 0x%lx (image: %s)\n", status, image->name);
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
			KTRACE("rld: load_container(\"%s\", type: %d, rpath: \"%s\") "
				"already loaded", name, type, rpath);
			return B_OK;
		}
	}

	KTRACE("rld: load_container(\"%s\", type: %d, rpath: \"%s\")", name, type,
		rpath);

	strlcpy(path, name, sizeof(path));

	// Try to load explicit image path first
	fd = open_executable(path, type, rpath, get_program_path());
	if (fd < 0) {
		FATAL("cannot open file %s\n", path);
		KTRACE("rld: load_container(\"%s\"): failed to open file", name);
		return fd;
	}

	// normalize the image path
	status = _kern_normalize_path(path, true, path);
	if (status != B_OK)
		goto err1;

	// Test again if this image has been registered already - this time,
	// we can check the full path, not just its name as noted.
	// You could end up loading an image twice with symbolic links, else.
	if (type != B_ADD_ON_IMAGE) {
		found = find_image(path, APP_OR_LIBRARY_TYPE);
		if (found) {
			atomic_add(&found->ref_count, 1);
			*_image = found;
			KTRACE("rld: load_container(\"%s\"): already loaded after all",
				name);
			return B_OK;
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

	if (!analyze_object_gcc_version(fd, image, eheader, sheaderSize, ph_buff,
			sizeof(ph_buff))) {
		FATAL("Failed to get gcc version for %s\n", path);
		// not really fatal, actually
	}

	// init gcc version dependent image flags
	// symbol resolution strategy (fallback is R5-style, if version is
	// unavailable)
	if (image->gcc_version.major == 0
		|| image->gcc_version.major == 2 && image->gcc_version.middle < 95) {
		image->flags |= IMAGE_FLAG_R5_SYMBOL_RESOLUTION;
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

	if (eheader.e_entry != 0)
		image->entry_point = eheader.e_entry + image->regions[0].delta;

	image->type = type;
	register_image(image, fd, path);

	_kern_close(fd);

	enqueue_image(&sLoadedImages, image);
	sLoadedImageCount++;

	*_image = image;

	KTRACE("rld: load_container(\"%s\"): done: id: %ld (gcc: %d.%d.%d)", name,
		image->id, image->gcc_version.major, image->gcc_version.middle,
		image->gcc_version.minor);

	return B_OK;

err3:
	unmap_image(image);
err2:
	delete_image_struct(image);
err1:
	_kern_close(fd);

	KTRACE("rld: load_container(\"%s\"): failed: %s", name,
		strerror(status));

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
	bool reportErrors = report_errors();
	status_t status = B_OK;
	uint32 i, j;
	const char *rpath;

	if (!d || (image->flags & RFLAG_DEPENDENCIES_LOADED))
		return B_OK;

	image->flags |= RFLAG_DEPENDENCIES_LOADED;

	if (image->num_needed == 0)
		return B_OK;

	KTRACE("rld: load_dependencies(\"%s\", id: %ld)", image->name,
		image->id);

	image->needed = (image_t**)malloc(image->num_needed * sizeof(image_t *));
	if (image->needed == NULL) {
		FATAL("failed to allocate needed struct\n");
		KTRACE("rld: load_dependencies(\"%s\", id: %ld) failed: no memory",
			image->name, image->id);
		return B_NO_MEMORY;
	}

	memset(image->needed, 0, image->num_needed * sizeof(image_t *));
	rpath = find_dt_rpath(image);

	for (i = 0, j = 0; d[i].d_tag != DT_NULL; i++) {
		switch (d[i].d_tag) {
			case DT_NEEDED:
			{
				int32 neededOffset = d[i].d_un.d_val;
				const char *name = STRING(image, neededOffset);

				status_t loadStatus = load_container(name, B_LIBRARY_IMAGE,
					rpath, &image->needed[j]);
				if (loadStatus < B_OK) {
					status = loadStatus;
					// correct error code in case the file could not been found
					if (status == B_ENTRY_NOT_FOUND) {
						status = B_MISSING_LIBRARY;

						if (reportErrors)
							sErrorMessage.AddString("missing library", name);
					}

					// Collect all missing libraries in case we report back
					if (!reportErrors) {
						KTRACE("rld: load_dependencies(\"%s\", id: %ld) "
							"failed: %s", image->name, image->id,
							strerror(status));
						return status;
					}
				}

				j += 1;
				break;
			}

			default:
				// ignore any other tag
				continue;
		}
	}

	if (status < B_OK) {
		KTRACE("rld: load_dependencies(\"%s\", id: %ld) "
			"failed: %s", image->name, image->id,
			strerror(status));
		return status;
	}

	if (j != image->num_needed) {
		FATAL("Internal error at load_dependencies()");
		KTRACE("rld: load_dependencies(\"%s\", id: %ld) "
			"failed: internal error", image->name, image->id);
		return B_ERROR;
	}

	KTRACE("rld: load_dependencies(\"%s\", id: %ld) done", image->name,
		image->id);

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


static ssize_t
get_sorted_image_list(image_t *image, image_t ***_list, uint32 sortFlag)
{
	image_t **list;

	list = (image_t**)malloc(sLoadedImageCount * sizeof(image_t *));
	if (list == NULL) {
		FATAL("memory shortage in get_sorted_image_list()");
		*_list = NULL;
		return B_NO_MEMORY;
	}

	memset(list, 0, sLoadedImageCount * sizeof(image_t *));

	*_list = list;
	return topological_sort(image, 0, list, sortFlag);
}


static status_t
relocate_dependencies(image_t *image)
{
	ssize_t count, i;
	image_t **list;

	count = get_sorted_image_list(image, &list, RFLAG_RELOCATED);
	if (count < B_OK)
		return count;

	for (i = 0; i < count; i++) {
		status_t status = relocate_image(image, list[i]);
		if (status < B_OK)
			return status;
	}

	free(list);
	return B_OK;
}


static void
init_dependencies(image_t *image, bool initHead)
{
	image_t **initList;
	ssize_t count, i;

	count = get_sorted_image_list(image, &initList, RFLAG_INITIALIZED);
	if (count <= 0)
		return;

	if (!initHead) {
		// this removes the "calling" image
		image->flags &= ~RFLAG_INITIALIZED;
		initList[--count] = NULL;
	}

	TRACE(("%ld: init dependencies\n", find_thread(NULL)));
	for (i = 0; i < count; i++) {
		image = initList[i];

		TRACE(("%ld:  init: %s\n", find_thread(NULL), image->name));

		if (image->init_routine != 0)
			((init_term_function)image->init_routine)(image->id);
	}
	TRACE(("%ld:  init done.\n", find_thread(NULL)));

	free(initList);
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
		sLoadedImageCount--;

		for (i = 0; i < image->num_needed; i++) {
			put_image(image->needed[i]);
		}
	}
}


//	#pragma mark - libroot.so exported functions


image_id
load_program(char const *path, void **_entry)
{
	status_t status;
	image_t *image;

	KTRACE("rld: load_program(\"%s\")", path);

	rld_lock();
		// for now, just do stupid simple global locking

	TRACE(("rld: load %s\n", path));

	status = load_container(path, B_APP_IMAGE, NULL, &sProgramImage);
	if (status < B_OK)
		goto err;

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
		struct Elf32_Sym *symbol = find_symbol_in_loaded_images(
			"__gRuntimeLoader", &image);
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
		struct Elf32_Sym *symbol = find_symbol_in_loaded_images("getenv",
			&image);
		if (symbol != NULL)
			gGetEnv = (char* (*)(const char*))
				(symbol->st_value + image->regions[0].delta);
	}

	if (sProgramImage->entry_point == 0) {
		status = B_NOT_AN_EXECUTABLE;
		goto err;
	}

	*_entry = (void *)(sProgramImage->entry_point);

	rld_unlock();

	KTRACE("rld: load_program(\"%s\") done: entry: %p, id: %ld", path,
		*_entry, sProgramImage->id);

	return sProgramImage->id;

err:
	KTRACE("rld: load_program(\"%s\") failed: %s", path, strerror(status));

	delete_image(sProgramImage);

	if (report_errors()) {
		// send error message
		sErrorMessage.AddInt32("error", status);
		sErrorMessage.SetDeliveryInfo(gProgramArgs->error_token,
			-1, 0, find_thread(NULL));

		_kern_write_port_etc(gProgramArgs->error_port, 'KMSG',
			sErrorMessage.Buffer(), sErrorMessage.ContentSize(), 0, 0);
	}
	_kern_loading_app_failed(status);
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

	if (path == NULL)
		return B_BAD_VALUE;

	// ToDo: implement flags
	(void)flags;

	KTRACE("rld: load_library(\"%s\", 0x%lx, %d)", path, flags, addOn);

	rld_lock();
		// for now, just do stupid simple global locking

	// have we already loaded this library?
	// Checking it at this stage saves loading its dependencies again
	if (!addOn) {
		image = find_image(path, APP_OR_LIBRARY_TYPE);
		if (image) {
			atomic_add(&image->ref_count, 1);
			rld_unlock();
			KTRACE("rld: load_library(\"%s\"): already loaded: %ld", path,
				image->id);
			return image->id;
		}
	}

	status = load_container(path, type, NULL, &image);
	if (status < B_OK) {
		rld_unlock();
		KTRACE("rld: load_library(\"%s\") failed to load container: %s", path,
			strerror(status));
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

	KTRACE("rld: load_library(\"%s\") done: id: %ld", path, image->id);

	return image->id;

err:
	KTRACE("rld: load_library(\"%s\") failed: %s", path, strerror(status));

	dequeue_image(&sLoadedImages, image);
	sLoadedImageCount--;
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

	if (imageID < B_OK)
		return B_BAD_IMAGE_ID;

	rld_lock();
		// for now, just do stupid simple global locking

	// we only check images that have been already initialized

	for (image = sLoadedImages.head; image; image = image->next) {
		if (image->id == imageID) {
			// unload image
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
			if (gRuntimeLoader.call_atexit_hooks_for_range) {
				gRuntimeLoader.call_atexit_hooks_for_range(
					image->regions[0].vmstart, image->regions[0].vmsize);
			}

			if (image->term_routine)
				((init_term_function)image->term_routine)(image->id);

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

				if (_type != NULL) {
					// ToDo: check with the return types of that BeOS function
					if (ELF32_ST_TYPE(symbol->st_info) == STT_FUNC)
						*_type = B_SYMBOL_TYPE_TEXT;
					else if (ELF32_ST_TYPE(symbol->st_info) == STT_OBJECT)
						*_type = B_SYMBOL_TYPE_DATA;
					else
						*_type = B_SYMBOL_TYPE_ANY;
				}

				if (_location != NULL)
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

	if (imageID < B_OK)
		return B_BAD_IMAGE_ID;
	if (symbolName == NULL)
		return B_BAD_VALUE;

	rld_lock();
		// for now, just do stupid simple global locking

	// get the image from those who have been already initialized
	image = find_loaded_image_by_id(imageID);
	if (image != NULL) {
		struct Elf32_Sym *symbol;

		// get the symbol in the image
		symbol = find_symbol(image, symbolName, symbolType);
		if (symbol) {
			if (_location != NULL)
				*_location = (void *)(symbol->st_value + image->regions[0].delta);
		} else
			status = B_ENTRY_NOT_FOUND;
	} else
		status = B_BAD_IMAGE_ID;

	rld_unlock();
	return status;
}


status_t
get_next_image_dependency(image_id id, uint32 *cookie, const char **_name)
{
	uint32 i, j, searchIndex = *cookie;
	struct Elf32_Dyn *dynamicSection;
	image_t *image;

	if (_name == NULL)
		return B_BAD_VALUE;

	rld_lock();

	image = find_loaded_image_by_id(id);
	if (image == NULL) {
		rld_unlock();
		return B_BAD_IMAGE_ID;
	}

	dynamicSection = (struct Elf32_Dyn *)image->dynamic_ptr;
	if (dynamicSection == NULL || image->num_needed <= searchIndex) {
		rld_unlock();
		return B_ENTRY_NOT_FOUND;
	}

	for (i = 0, j = 0; dynamicSection[i].d_tag != DT_NULL; i++) {
		if (dynamicSection[i].d_tag != DT_NEEDED)
			continue;

		if (j++ == searchIndex) {
			int32 neededOffset = dynamicSection[i].d_un.d_val;

			*_name = STRING(image, neededOffset);
			*cookie = searchIndex + 1;
			rld_unlock();
			return B_OK;
		}
	}

	rld_unlock();
	return B_ENTRY_NOT_FOUND;
}


//	#pragma mark - runtime_loader private exports


/** Read and verify the ELF header */

status_t
elf_verify_header(void *header, int32 length)
{
	int32 programSize, sectionSize;

	if (length < (int32)sizeof(struct Elf32_Ehdr))
		return B_NOT_AN_EXECUTABLE;

	return parse_elf_header((struct Elf32_Ehdr *)header, &programSize, &sectionSize);
}


void
terminate_program(void)
{
	image_t **termList;
	ssize_t count, i;

	count = get_sorted_image_list(sProgramImage, &termList, RFLAG_TERMINATED);
	if (count < B_OK)
		return;

	TRACE(("%ld: terminate dependencies\n", find_thread(NULL)));
	for (i = count; i-- > 0;) {
		image_t *image = termList[i];

		TRACE(("%ld:  term: %s\n", find_thread(NULL), image->name));

		if (image->term_routine)
			((init_term_function)image->term_routine)(image->id);
	}
	TRACE(("%ld:  term done.\n", find_thread(NULL)));

	free(termList);
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

	// initialize error message if needed
	if (report_errors()) {
		void *buffer = malloc(1024);
		if (buffer == NULL)
			return;

		sErrorMessage.SetTo(buffer, 1024, 'Rler');
	}
}


status_t
elf_reinit_after_fork()
{
	rld_sem = create_sem(1, "rld_lock");
	if (rld_sem < 0)
		return rld_sem;

	return B_OK;
}
