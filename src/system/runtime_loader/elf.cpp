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

#include <ctype.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <elf32.h>
#include <runtime_loader.h>
#include <syscalls.h>
#include <user_runtime.h>
#include <util/DoublyLinkedList.h>
#include <util/kernel_cpp.h>
#include <util/KMessage.h>
#include <vm_defs.h>

#include "tracing_config.h"


//#define TRACE_RLD
#ifdef TRACE_RLD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// TODO: implement better locking strategy
// TODO: implement lazy binding

#define	PAGE_MASK (B_PAGE_SIZE - 1)

#define	PAGE_OFFSET(x) ((x) & (PAGE_MASK))
#define	PAGE_BASE(x) ((x) & ~(PAGE_MASK))
#define TO_PAGE_SIZE(x) ((x + (PAGE_MASK)) & ~(PAGE_MASK))

#define RLD_PROGRAM_BASE 0x00200000
	/* keep in sync with app ldscript */

// a handle returned by load_library() (dlopen())
#define RLD_GLOBAL_SCOPE	((void*)-2l)

enum {
	// the lower two bits are reserved for RTLD_NOW and RTLD_GLOBAL

	RFLAG_RW					= 0x0010,
	RFLAG_ANON					= 0x0020,

	RFLAG_TERMINATED			= 0x0200,
	RFLAG_INITIALIZED			= 0x0400,
	RFLAG_SYMBOLIC				= 0x0800,
	RFLAG_RELOCATED				= 0x1000,
	RFLAG_PROTECTED				= 0x2000,
	RFLAG_DEPENDENCIES_LOADED	= 0x4000,
	RFLAG_REMAPPED				= 0x8000,

	RFLAG_VISITED				= 0x10000,
	RFLAG_USE_FOR_RESOLVING		= 0x20000
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


// image events
enum {
	IMAGE_EVENT_LOADED,
	IMAGE_EVENT_RELOCATED,
	IMAGE_EVENT_INITIALIZED,
	IMAGE_EVENT_UNINITIALIZING,
	IMAGE_EVENT_UNLOADING
};


struct RuntimeLoaderAddOn
		: public DoublyLinkedListLinkImpl<RuntimeLoaderAddOn> {
	image_t*				image;
	runtime_loader_add_on*	addOn;

	RuntimeLoaderAddOn(image_t* image, runtime_loader_add_on* addOn)
		:
		image(image),
		addOn(addOn)
	{
	}
};

typedef DoublyLinkedList<RuntimeLoaderAddOn> AddOnList;

struct RuntimeLoaderSymbolPatcher {
	RuntimeLoaderSymbolPatcher*		next;
	runtime_loader_symbol_patcher*	patcher;
	void*							cookie;

	RuntimeLoaderSymbolPatcher(runtime_loader_symbol_patcher* patcher,
			void* cookie)
		:
		patcher(patcher),
		cookie(cookie)
	{
	}
};


static image_queue_t sLoadedImages = {0, 0};
static image_queue_t sDisposableImages = {0, 0};
static uint32 sLoadedImageCount = 0;
static image_t *sProgramImage;
static KMessage sErrorMessage;
static bool sProgramLoaded = false;
static const char *sSearchPathSubDir = NULL;
static bool sInvalidImageIDs;
static image_t **sPreloadedImages = NULL;
static uint32 sPreloadedImageCount = 0;
static AddOnList sAddOns;

// a recursive lock
static sem_id sSem;
static thread_id sSemOwner;
static int32 sSemCount;

extern runtime_loader_add_on_export gRuntimeLoaderAddOnExport;


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

#define FATAL(x...)							\
	do {									\
		dprintf("runtime_loader: " x);		\
		if (!sProgramLoaded)				\
			printf("runtime_loader: " x);	\
	} while (false)


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


#if RUNTIME_LOADER_TRACING

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
	if (sSemCount-- == 1) {
		sSemOwner = -1;
		release_sem(sSem);
	}
}


static void
rld_lock()
{
	thread_id self = find_thread(NULL);
	if (self != sSemOwner) {
		acquire_sem(sSem);
		sSemOwner = self;
	}
	sSemCount++;
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


//! Remaps the image ID of \a image after fork.
static status_t
update_image_id(image_t *image)
{
	int32 cookie = 0;
	image_info info;
	while (_kern_get_next_image_info(B_CURRENT_TEAM, &cookie, &info,
			sizeof(image_info)) == B_OK) {
		for (uint32 i = 0; i < image->num_regions; i++) {
			if (image->regions[i].vmstart == (addr_t)info.text) {
				image->id = info.id;
				return B_OK;
			}
		}
	}

	FATAL("Could not update image ID %ld after fork()!\n", image->id);
	return B_ENTRY_NOT_FOUND;
}


//! After fork, we lazily rebuild the image IDs of all loaded images.
static status_t
update_image_ids(void)
{
	for (image_t *image = sLoadedImages.head; image; image = image->next) {
		status_t status = update_image_id(image);
		if (status != B_OK)
			return status;
	}
	for (image_t *image = sDisposableImages.head; image; image = image->next) {
		status_t status = update_image_id(image);
		if (status != B_OK)
			return status;
	}

	sInvalidImageIDs = false;
	return B_OK;
}


static image_t *
find_image_in_queue(image_queue_t *queue, const char *name, bool isPath,
	uint32 typeMask)
{
	for (image_t *image = queue->head; image; image = image->next) {
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
	bool isPath = strchr(name, '/') != NULL;
	return find_image_in_queue(&sLoadedImages, name, isPath, typeMask);
}


static image_t *
find_loaded_image_by_id(image_id id)
{
	if (sInvalidImageIDs) {
		// After fork, we lazily rebuild the image IDs of all loaded images
		update_image_ids();
	}

	for (image_t *image = sLoadedImages.head; image; image = image->next) {
		if (image->id == id)
			return image;
	}

	// For the termination routine, we need to look into the list of
	// disposable images as well
	for (image_t *image = sDisposableImages.head; image; image = image->next) {
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

	while (RuntimeLoaderSymbolPatcher* patcher
			= image->defined_symbol_patchers) {
		image->defined_symbol_patchers = patcher->next;
		delete patcher;
	}
	while (RuntimeLoaderSymbolPatcher* patcher
			= image->undefined_symbol_patchers) {
		image->undefined_symbol_patchers = patcher->next;
		delete patcher;
	}

#ifdef DEBUG
	// overwrite images to make sure they aren't accidently reused anywhere
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


static void
update_image_flags_recursively(image_t* image, uint32 flagsToSet,
	uint32 flagsToClear)
{
	image_t* queue[sLoadedImageCount];
	uint32 count = 0;
	uint32 index = 0;
	queue[count++] = image;
	image->flags |= RFLAG_VISITED;

	while (index < count) {
		// pop next image
		image = queue[index++];

		// push dependencies
		for (uint32 i = 0; i < image->num_needed; i++) {
			image_t* needed = image->needed[i];
			if ((needed->flags & RFLAG_VISITED) == 0) {
				queue[count++] = needed;
				needed->flags |= RFLAG_VISITED;
			}
		}
	}

	// update flags
	for (uint32 i = 0; i < count; i++) {
		queue[i]->flags = (queue[i]->flags | flagsToSet)
			& ~(flagsToClear | RFLAG_VISITED);
	}
}


static void
set_image_flags_recursively(image_t* image, uint32 flags)
{
	update_image_flags_recursively(image, flags, 0);
}


static void
clear_image_flags_recursively(image_t* image, uint32 flags)
{
	update_image_flags_recursively(image, 0, flags);
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
	bool isHaiku = true;

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

		if (gccMajor == 2 && gccPlatform != NULL && strcmp(gccPlatform, "haiku"))
			isHaiku = false;
	}

	image->gcc_version.major = gccMajor;
	image->gcc_version.middle = gccMiddle;
	image->gcc_version.minor = gccMinor;
	image->gcc_version.haiku = isHaiku;

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
			image->regions[i].id = _kern_map_file(regionName,
				(void **)&loadAddress, addressSpecifier,
				image->regions[i].vmsize, B_READ_AREA | B_WRITE_AREA,
				REGION_PRIVATE_MAP, fd, PAGE_BASE(image->regions[i].fdstart));

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


static void
patch_defined_symbol(image_t* image, const char* name, void** symbol,
	int32* type)
{
	RuntimeLoaderSymbolPatcher* patcher = image->defined_symbol_patchers;
	while (patcher != NULL && *symbol != 0) {
		image_t* inImage = image;
		patcher->patcher(patcher->cookie, NULL, image, name, &inImage,
			symbol, type);
		patcher = patcher->next;
	}
}


static void
patch_undefined_symbol(image_t* rootImage, image_t* image, const char* name,
	image_t** foundInImage, void** symbol, int32* type)
{
	if (*foundInImage != NULL)
		patch_defined_symbol(*foundInImage, name, symbol, type);

	RuntimeLoaderSymbolPatcher* patcher = image->undefined_symbol_patchers;
	while (patcher != NULL) {
		patcher->patcher(patcher->cookie, rootImage, image, name, foundInImage,
			symbol, type);
		patcher = patcher->next;
	}
}


status_t
register_defined_symbol_patcher(struct image_t* image,
	runtime_loader_symbol_patcher* _patcher, void* cookie)
{
	RuntimeLoaderSymbolPatcher* patcher
		= new(mynothrow) RuntimeLoaderSymbolPatcher(_patcher, cookie);
	if (patcher == NULL)
		return B_NO_MEMORY;

	patcher->next = image->defined_symbol_patchers;
	image->defined_symbol_patchers = patcher;

	return B_OK;
}


void
unregister_defined_symbol_patcher(struct image_t* image,
	runtime_loader_symbol_patcher* _patcher, void* cookie)
{
	RuntimeLoaderSymbolPatcher** patcher = &image->defined_symbol_patchers;
	while (*patcher != NULL) {
		if ((*patcher)->patcher == _patcher && (*patcher)->cookie == cookie) {
			RuntimeLoaderSymbolPatcher* toDelete = *patcher;
			*patcher = (*patcher)->next;
			delete toDelete;
			return;
		}
		patcher = &(*patcher)->next;
	}
}


status_t
register_undefined_symbol_patcher(struct image_t* image,
	runtime_loader_symbol_patcher* _patcher, void* cookie)
{
	RuntimeLoaderSymbolPatcher* patcher
		= new(mynothrow) RuntimeLoaderSymbolPatcher(_patcher, cookie);
	if (patcher == NULL)
		return B_NO_MEMORY;

	patcher->next = image->undefined_symbol_patchers;
	image->undefined_symbol_patchers = patcher;

	return B_OK;
}


void
unregister_undefined_symbol_patcher(struct image_t* image,
	runtime_loader_symbol_patcher* _patcher, void* cookie)
{
	RuntimeLoaderSymbolPatcher** patcher = &image->undefined_symbol_patchers;
	while (*patcher != NULL) {
		if ((*patcher)->patcher == _patcher && (*patcher)->cookie == cookie) {
			RuntimeLoaderSymbolPatcher* toDelete = *patcher;
			*patcher = (*patcher)->next;
			delete toDelete;
			return;
		}
		patcher = &(*patcher)->next;
	}
}


runtime_loader_add_on_export gRuntimeLoaderAddOnExport = {
	register_defined_symbol_patcher,
	unregister_defined_symbol_patcher,
	register_undefined_symbol_patcher,
	unregister_undefined_symbol_patcher
};


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


static status_t
find_symbol(image_t* image, char const* symbolName, int32 symbolType,
	void **_location)
{
	// get the symbol in the image
	struct Elf32_Sym* symbol = find_symbol(image, symbolName, symbolType);
	if (symbol == NULL)
		return B_ENTRY_NOT_FOUND;

	void* location = (void*)(symbol->st_value + image->regions[0].delta);
	patch_defined_symbol(image, symbolName, &location, &symbolType);

	if (_location != NULL)
		*_location = location;

	return B_OK;
}


static status_t
find_symbol_breadth_first(image_t* image, const char* name, int32 type,
	image_t** _foundInImage, void** _location)
{
	image_t* queue[sLoadedImageCount];
	uint32 count = 0;
	uint32 index = 0;
	queue[count++] = image;
	image->flags |= RFLAG_VISITED;

	bool found = false;
	while (index < count) {
		// pop next image
		image = queue[index++];

		if (find_symbol(image, name, type, _location) == B_OK) {
			found = true;
			break;
		}

		// push needed images
		for (uint32 i = 0; i < image->num_needed; i++) {
			image_t* needed = image->needed[i];
			if ((needed->flags & RFLAG_VISITED) == 0) {
				queue[count++] = needed;
				needed->flags |= RFLAG_VISITED;
			}
		}
	}

	// clear visited flags
	for (uint32 i = 0; i < count; i++)
		queue[i]->flags &= ~RFLAG_VISITED;

	return found ? B_OK : B_ENTRY_NOT_FOUND;
}


static struct Elf32_Sym*
find_undefined_symbol_beos(image_t* rootImage, image_t* image, const char* name,
	image_t** foundInImage)
{
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


static struct Elf32_Sym*
find_undefined_symbol_global(image_t* rootImage, image_t* image,
	const char* name, image_t** foundInImage)
{
	// Global load order symbol resolution: All loaded images are searched for
	// the symbol in the order they have been loaded. We skip add-on images and
	// RTLD_LOCAL images though.
	image_t* otherImage = sLoadedImages.head;
	while (otherImage != NULL) {
		if (otherImage == rootImage
			|| otherImage->type != B_ADD_ON_IMAGE
				&& (otherImage->flags
					& (RTLD_GLOBAL | RFLAG_USE_FOR_RESOLVING)) != 0) {
			struct Elf32_Sym *symbol = find_symbol(otherImage, name,
				B_SYMBOL_TYPE_ANY);
			if (symbol) {
				*foundInImage = otherImage;
				return symbol;
			}
		}
		otherImage = otherImage->next;
	}

	return NULL;
}


static struct Elf32_Sym*
find_undefined_symbol_add_on(image_t* rootImage, image_t* image,
	const char* name, image_t** foundInImage)
{
// TODO: How do we want to implement this one? Using global scope resolution
// might be undesired as it is now, since libraries could refer to symbols in
// the add-on, which would result in add-on symbols implicitely becoming used
// outside of the add-on. So the options would be to use the global scope but
// skip the add-on, or to do breadth-first resolution in the add-on dependency
// scope, also skipping the add-on itself. BeOS style resolution is safe, too,
// but we miss out features like undefined symbols and preloading.
	return find_undefined_symbol_beos(rootImage, image, name, foundInImage);
}


/*!	This function is called when we run BeOS images on Haiku.
	It allows us to redirect functions to ensure compatibility.
*/
static const char*
beos_compatibility_map_symbol(const char* symbolName)
{
	struct symbol_mapping {
		const char* from;
		const char* to;
	};
	static const struct symbol_mapping kMappings[] = {
		// TODO: Improve this, and also use it for libnet.so compatibility!
		// Allow an image to provide a function that will be invoked for every
		// (transitively) depending image. The function can return a table to
		// remap symbols (probably better address to address). All the tables
		// for a single image would be combined into a hash table and an
		// undefined symbol patcher using this hash table would be added.
		{"fstat", "__be_fstat"},
		{"lstat", "__be_lstat"},
		{"stat", "__be_stat"},
	};
	const uint32 kMappingCount = sizeof(kMappings) / sizeof(kMappings[0]);

	for (uint32 i = 0; i < kMappingCount; i++) {
		if (!strcmp(symbolName, kMappings[i].from))
			return kMappings[i].to;
	}

	return symbolName;
}


int
resolve_symbol(image_t *rootImage, image_t *image, struct Elf32_Sym *sym,
	addr_t *symAddress)
{
	switch (sym->st_shndx) {
		case SHN_UNDEF:
		{
			struct Elf32_Sym *sharedSym;
			image_t *sharedImage;
			const char *symName;

			// patch the symbol name
			symName = SYMNAME(image, sym);
			if (!image->gcc_version.haiku) {
				// The image has been compiled with a BeOS compiler. This means
				// we'll have to redirect some functions for compatibility.
				symName = beos_compatibility_map_symbol(symName);
			}

			int32 type = B_SYMBOL_TYPE_ANY;
			if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC)
				type = B_SYMBOL_TYPE_TEXT;
			else if (ELF32_ST_TYPE(sym->st_info) == STT_OBJECT)
				type = B_SYMBOL_TYPE_DATA;

			// it's undefined, must be outside this image, try the other images
			sharedSym = rootImage->find_undefined_symbol(rootImage, image,
				symName, &sharedImage);
			void* location = NULL;

			enum {
				ERROR_NO_SYMBOL,
				ERROR_WRONG_TYPE,
				ERROR_NOT_EXPORTED,
				ERROR_UNPATCHED
			};
			uint32 lookupError = ERROR_UNPATCHED;

			if (sharedSym == NULL) {
				// symbol not found at all
				lookupError = ERROR_NO_SYMBOL;
				sharedImage = NULL;
			} else if (ELF32_ST_TYPE(sym->st_info) != STT_NOTYPE
				&& ELF32_ST_TYPE(sym->st_info)
					!= ELF32_ST_TYPE(sharedSym->st_info)) {
				// symbol not of the requested type
				lookupError = ERROR_WRONG_TYPE;
				sharedImage = NULL;
			} else if (ELF32_ST_BIND(sharedSym->st_info) != STB_GLOBAL
				&& ELF32_ST_BIND(sharedSym->st_info) != STB_WEAK) {
				// symbol not exported
				lookupError = ERROR_NOT_EXPORTED;
				sharedImage = NULL;
			} else {
				// symbol is fine, get its location
				location = (void*)(sharedSym->st_value
					+ sharedImage->regions[0].delta);
			}

			patch_undefined_symbol(rootImage, image, symName, &sharedImage,
				&location, &type);

			if (location == NULL) {
				switch (lookupError) {
					case ERROR_NO_SYMBOL:
						FATAL("elf_resolve_symbol: could not resolve symbol "
							"'%s'\n", symName);
						break;
					case ERROR_WRONG_TYPE:
						FATAL("elf_resolve_symbol: found symbol '%s' in shared "
							"image but wrong type\n", symName);
						break;
					case ERROR_NOT_EXPORTED:
						FATAL("elf_resolve_symbol: found symbol '%s', but not "
							"exported\n", symName);
						break;
					case ERROR_UNPATCHED:
						FATAL("elf_resolve_symbol: found symbol '%s', but was "
							"hidden by symbol patchers\n", symName);
						break;
				}
				return B_MISSING_SYMBOL;
			}

			*symAddress = (addr_t)location;
			return B_OK;
		}

		case SHN_ABS:
			*symAddress = sym->st_value + image->regions[0].delta;
			return B_NO_ERROR;

		case SHN_COMMON:
			// ToDo: finish this
			FATAL("elf_resolve_symbol: COMMON symbol, finish me!\n");
			return B_ERROR; //ERR_NOT_IMPLEMENTED_YET;

		default:
			// standard symbol
			*symAddress = sym->st_value + image->regions[0].delta;
			return B_NO_ERROR;
	}
}


static void
image_event(image_t* image, uint32 event)
{
	AddOnList::Iterator it = sAddOns.GetIterator();
	while (RuntimeLoaderAddOn* addOn = it.Next()) {
		void (*function)(image_t* image) = NULL;

		switch (event) {
			case IMAGE_EVENT_LOADED:
				function = addOn->addOn->image_loaded;
				break;
			case IMAGE_EVENT_RELOCATED:
				function = addOn->addOn->image_relocated;
				break;
			case IMAGE_EVENT_INITIALIZED:
				function = addOn->addOn->image_initialized;
				break;
			case IMAGE_EVENT_UNINITIALIZING:
				function = addOn->addOn->image_uninitializing;
				break;
			case IMAGE_EVENT_UNLOADING:
				function = addOn->addOn->image_unloading;
				break;
		}

		if (function != NULL)
			function(image);
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
		FATAL("troubles relocating: 0x%lx (image: %s, %s)\n", status,
			image->path, image->name);
		return status;
	}

	_kern_image_relocated(image->id);
	image_event(image, IMAGE_EVENT_RELOCATED);
	return B_OK;
}


static status_t
load_container(char const *name, image_type type, const char *rpath,
	image_t **_image)
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

		if (found == NULL && type != B_APP_IMAGE) {
			// Special case for add-ons that link against the application
			// executable, with the executable not having a soname set.
			if (const char* lastSlash = strrchr(name, '/')) {
				image_t* programImage = get_program_image();
				if (strcmp(programImage->name, lastSlash + 1) == 0)
					found = programImage;
			}
		}

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

	// find and open the file
	fd = open_executable(path, type, rpath, get_program_path(),
		sSearchPathSubDir);
	if (fd < 0) {
		FATAL("cannot open file %s\n", name);
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
			_kern_close(fd);
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

	if (analyze_object_gcc_version(fd, image, eheader, sheaderSize, ph_buff,
			sizeof(ph_buff))) {
		// If this is the executable image, we init the search path
		// subdir, if the compiler version doesn't match ours.
		if (type == B_APP_IMAGE) {
			#if __GNUC__ == 2
				if (image->gcc_version.major > 2)
					sSearchPathSubDir = "gcc4";
			#elif __GNUC__ == 4
				if (image->gcc_version.major == 2)
					sSearchPathSubDir = "gcc2";
			#endif
		}
	} else {
		FATAL("Failed to get gcc version for %s\n", path);
		// not really fatal, actually
	}

	// init gcc version dependent image flags
	// symbol resolution strategy (fallback is R5-style, if version is
	// unavailable)
	if (image->gcc_version.major == 0
		|| image->gcc_version.major == 2 && image->gcc_version.middle < 95) {
		image->find_undefined_symbol = find_undefined_symbol_beos;
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
	image_event(image, IMAGE_EVENT_LOADED);

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
load_immediate_dependencies(image_t *image)
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


static status_t
load_dependencies(image_t* image)
{
	for (image_t* otherImage = image; otherImage != NULL;
			otherImage = otherImage->next) {
		status_t status = load_immediate_dependencies(otherImage);
		if (status != B_OK)
			return status;
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
	// get the images that still have to be relocated
	image_t **list;
	ssize_t count = get_sorted_image_list(image, &list, RFLAG_RELOCATED);
	if (count < B_OK)
		return count;

	// relocate
	for (ssize_t i = 0; i < count; i++) {
		status_t status = relocate_image(image, list[i]);
		if (status < B_OK) {
			free(list);
			return status;
		}
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

		image_event(image, IMAGE_EVENT_INITIALIZED);
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


void
inject_runtime_loader_api(image_t* rootImage)
{
	// We patch any exported __gRuntimeLoader symbols to point to our private
	// API.
	image_t* image;
	void* _export;
	if (find_symbol_breadth_first(rootImage, "__gRuntimeLoader",
			B_SYMBOL_TYPE_DATA, &image, &_export) == B_OK) {
		*(void**)_export = &gRuntimeLoader;
	}
}


static status_t
add_preloaded_image(image_t* image)
{
	// We realloc() everytime -- not particularly efficient, but good enough for
	// small number of preloaded images.
	image_t** newArray = (image_t**)realloc(sPreloadedImages,
		sizeof(image_t*) * (sPreloadedImageCount + 1));
	if (newArray == NULL)
		return B_NO_MEMORY;

	sPreloadedImages = newArray;
	newArray[sPreloadedImageCount++] = image;

	return B_OK;
}


image_id
preload_image(char const* path)
{
	if (path == NULL)
		return B_BAD_VALUE;

	KTRACE("rld: preload_image(\"%s\")", path);

	image_t *image = NULL;
	status_t status = load_container(path, B_ADD_ON_IMAGE, NULL, &image);
	if (status < B_OK) {
		rld_unlock();
		KTRACE("rld: preload_image(\"%s\") failed to load container: %s", path,
			strerror(status));
		return status;
	}

	if (image->find_undefined_symbol == NULL)
		image->find_undefined_symbol = find_undefined_symbol_global;

	status = load_dependencies(image);
	if (status < B_OK)
		goto err;

	set_image_flags_recursively(image, RTLD_GLOBAL);

	status = relocate_dependencies(image);
	if (status < B_OK)
		goto err;

	status = add_preloaded_image(image);
	if (status < B_OK)
		goto err;

	inject_runtime_loader_api(image);

	remap_images();
	init_dependencies(image, true);

	// if the image contains an add-on, register it
	runtime_loader_add_on* addOnStruct;
	if (find_symbol(image, "__gRuntimeLoaderAddOn", B_SYMBOL_TYPE_DATA,
			(void**)&addOnStruct) == B_OK) {
		RuntimeLoaderAddOn* addOn = new(mynothrow) RuntimeLoaderAddOn(image,
			addOnStruct);
		if (addOn != NULL) {
			sAddOns.Add(addOn);
			addOnStruct->init(&gRuntimeLoader, &gRuntimeLoaderAddOnExport);
		}
	}

	KTRACE("rld: preload_image(\"%s\") done: id: %ld", path, image->id);

	return image->id;

err:
	KTRACE("rld: preload_image(\"%s\") failed: %s", path, strerror(status));

	dequeue_image(&sLoadedImages, image);
	sLoadedImageCount--;
	delete_image(image);
	return status;
}


static void
preload_images()
{
	const char* imagePaths = getenv("LD_PRELOAD");
	if (imagePaths == NULL)
		return;

	while (*imagePaths != '\0') {
		// find begin of image path
		while (*imagePaths != '\0' && isspace(*imagePaths))
			imagePaths++;

		if (*imagePaths == '\0')
			break;

		// find end of image path
		const char* imagePath = imagePaths;
		while (*imagePaths != '\0' && !isspace(*imagePaths))
			imagePaths++;

		// extract the path
		char path[B_PATH_NAME_LENGTH];
		size_t pathLen = imagePaths - imagePath;
		if (pathLen > sizeof(path) - 1)
			continue;
		memcpy(path, imagePath, pathLen);
		path[pathLen] = '\0';

		// load the image
		preload_image(path);
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

	preload_images();

	TRACE(("rld: load %s\n", path));

	status = load_container(path, B_APP_IMAGE, NULL, &sProgramImage);
	if (status < B_OK)
		goto err;

	if (sProgramImage->find_undefined_symbol == NULL)
		sProgramImage->find_undefined_symbol = find_undefined_symbol_global;

	status = load_dependencies(sProgramImage);
	if (status < B_OK)
		goto err;

	// Set RTLD_GLOBAL on all libraries, but clear it on the program image.
	// This results in the desired symbol resolution for dlopen()ed libraries.
	set_image_flags_recursively(sProgramImage, RTLD_GLOBAL);
	sProgramImage->flags &= ~RTLD_GLOBAL;

	status = relocate_dependencies(sProgramImage);
	if (status < B_OK)
		goto err;

	inject_runtime_loader_api(sProgramImage);

	remap_images();
	init_dependencies(sProgramImage, true);

	// Since the images are initialized now, we no longer should use our
	// getenv(), but use the one from libroot.so
	find_symbol_breadth_first(sProgramImage, "getenv", B_SYMBOL_TYPE_TEXT,
		&image, (void**)&gGetEnv);

	if (sProgramImage->entry_point == 0) {
		status = B_NOT_AN_EXECUTABLE;
		goto err;
	}

	*_entry = (void *)(sProgramImage->entry_point);

	rld_unlock();

	sProgramLoaded = true;

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
load_library(char const *path, uint32 flags, bool addOn, void** _handle)
{
	image_t *image = NULL;
	image_type type = (addOn ? B_ADD_ON_IMAGE : B_LIBRARY_IMAGE);
	status_t status;

	if (path == NULL && addOn)
		return B_BAD_VALUE;

	KTRACE("rld: load_library(\"%s\", 0x%lx, %d)", path, flags, addOn);

	rld_lock();
		// for now, just do stupid simple global locking

	// have we already loaded this library?
	// Checking it at this stage saves loading its dependencies again
	if (!addOn) {
		// a NULL path is fine -- it means the global scope shall be opened
		if (path == NULL) {
			*_handle = RLD_GLOBAL_SCOPE;
			rld_unlock();
			return 0;
		}

		image = find_image(path, APP_OR_LIBRARY_TYPE);
		if (image != NULL && (flags & RTLD_GLOBAL) != 0)
			set_image_flags_recursively(image, RTLD_GLOBAL);

		if (image) {
			atomic_add(&image->ref_count, 1);
			rld_unlock();
			KTRACE("rld: load_library(\"%s\"): already loaded: %ld", path,
				image->id);
			*_handle = image;
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

	if (image->find_undefined_symbol == NULL) {
		if (addOn)
			image->find_undefined_symbol = find_undefined_symbol_add_on;
		else
			image->find_undefined_symbol = find_undefined_symbol_global;
	}

	status = load_dependencies(image);
	if (status < B_OK)
		goto err;

	// If specified, set the RTLD_GLOBAL flag recursively on this image and all
	// dependencies. If not specified, we temporarily set
	// RFLAG_USE_FOR_RESOLVING so that the dependencies will correctly be used
	// for undefined symbol resolution.
	if ((flags & RTLD_GLOBAL) != 0)
		set_image_flags_recursively(image, RTLD_GLOBAL);
	else
		set_image_flags_recursively(image, RFLAG_USE_FOR_RESOLVING);

	status = relocate_dependencies(image);
	if (status < B_OK)
		goto err;

	if ((flags & RTLD_GLOBAL) == 0)
		clear_image_flags_recursively(image, RFLAG_USE_FOR_RESOLVING);

	remap_images();
	init_dependencies(image, true);

	rld_unlock();

	KTRACE("rld: load_library(\"%s\") done: id: %ld", path, image->id);

	*_handle = image;
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
unload_library(void* handle, image_id imageID, bool addOn)
{
	status_t status = B_BAD_IMAGE_ID;
	image_t *image;
	image_type type = addOn ? B_ADD_ON_IMAGE : B_LIBRARY_IMAGE;

	if (handle == NULL && imageID < 0)
		return B_BAD_IMAGE_ID;

	if (handle == RLD_GLOBAL_SCOPE)
		return B_OK;

	rld_lock();
		// for now, just do stupid simple global locking

	if (sInvalidImageIDs) {
		// After fork, we lazily rebuild the image IDs of all loaded images
		update_image_ids();
	}

	// we only check images that have been already initialized

	if (handle != NULL) {
		image = (image_t*)handle;
		put_image(image);
	} else {
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
	}

	if (status == B_OK) {
		while ((image = sDisposableImages.head) != NULL) {
			// call image fini here...
			if (gRuntimeLoader.call_atexit_hooks_for_range) {
				gRuntimeLoader.call_atexit_hooks_for_range(
					image->regions[0].vmstart, image->regions[0].vmsize);
			}

			image_event(image, IMAGE_EVENT_UNINITIALIZING);

			if (image->term_routine)
				((init_term_function)image->term_routine)(image->id);

			dequeue_image(&sDisposableImages, image);
			unmap_image(image);

			image_event(image, IMAGE_EVENT_UNLOADING);

			delete_image(image);
		}
	}

	rld_unlock();
	return status;
}


status_t
get_nth_symbol(image_id imageID, int32 num, char *nameBuffer,
	int32 *_nameLength, int32 *_type, void **_location)
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
			struct Elf32_Sym *symbol = &image->syms[j];

			if (count == num) {
				const char* symbolName = SYMNAME(image, symbol);
				strlcpy(nameBuffer, symbolName, *_nameLength);
				*_nameLength = strlen(symbolName);

				void* location = (void*)(symbol->st_value
					+ image->regions[0].delta);
				int32 type;
				if (ELF32_ST_TYPE(symbol->st_info) == STT_FUNC)
					type = B_SYMBOL_TYPE_TEXT;
				else if (ELF32_ST_TYPE(symbol->st_info) == STT_OBJECT)
					type = B_SYMBOL_TYPE_DATA;
				else
					type = B_SYMBOL_TYPE_ANY;
					// TODO: check with the return types of that BeOS function

				patch_defined_symbol(image, symbolName, &location, &type);

				if (_type != NULL)
					*_type = type;
				if (_location != NULL)
					*_location = location;
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
get_symbol(image_id imageID, char const *symbolName, int32 symbolType,
	void **_location)
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
	if (image != NULL)
		status = find_symbol(image, symbolName, symbolType, _location);
	else
		status = B_BAD_IMAGE_ID;

	rld_unlock();
	return status;
}


status_t
get_library_symbol(void* handle, void* caller, const char* symbolName,
	void **_location)
{
	status_t status = B_ENTRY_NOT_FOUND;

	if (symbolName == NULL)
		return B_BAD_VALUE;

	rld_lock();
		// for now, just do stupid simple global locking

	if (handle == RTLD_DEFAULT || handle == RLD_GLOBAL_SCOPE) {
		// look in the default scope
		image_t* image;
		Elf32_Sym* symbol = find_undefined_symbol_global(sProgramImage,
			sProgramImage, symbolName, &image);
		if (symbol != NULL) {
			*_location = (void*)(symbol->st_value + image->regions[0].delta);
			int32 symbolType = ELF32_ST_TYPE(symbol->st_info) == STT_FUNC
				? B_SYMBOL_TYPE_TEXT : B_SYMBOL_TYPE_DATA;
			patch_defined_symbol(image, symbolName, _location, &symbolType);
			status = B_OK;
		}
	} else if (handle == RTLD_NEXT) {
		// Look in the default scope, but also in the dependencies of the
		// calling image. Return the next after the caller symbol.

		// First of all, find the caller symbol and its image.
		Elf32_Sym* callerSymbol = NULL;
		image_t* callerImage = sLoadedImages.head;
		for (; callerImage != NULL; callerImage = callerImage->next) {
			elf_region_t& text = callerImage->regions[0];
			if ((addr_t)caller < text.vmstart
				|| (addr_t)caller >= text.vmstart + text.vmsize) {
				continue;
			}

			// found the image -- now find the symbol
			for (uint32 i = 0; i < callerImage->symhash[1]; i++) {
				Elf32_Sym& symbol = callerImage->syms[i];
				if ((ELF32_ST_TYPE(symbol.st_info) != STT_FUNC)
					|| symbol.st_value == 0) {
					continue;
				}

				addr_t address = symbol.st_value
					+ callerImage->regions[0].delta;
				if ((addr_t)caller >= address
					&& (addr_t)caller < address + symbol.st_size) {
					callerSymbol = &symbol;
					break;
				}
			}

			break;
		}

		if (callerSymbol != NULL) {
			// found the caller -- now search the global scope until we find
			// the next symbol
			set_image_flags_recursively(callerImage, RFLAG_USE_FOR_RESOLVING);

			image_t* image = sLoadedImages.head;
			for (; image != NULL; image = image->next) {
				if (image != callerImage
					&& (image->type == B_ADD_ON_IMAGE
						|| (image->flags
							& (RTLD_GLOBAL | RFLAG_USE_FOR_RESOLVING)) == 0)) {
					continue;
				}

				struct Elf32_Sym *symbol = find_symbol(image, symbolName,
					B_SYMBOL_TYPE_TEXT);
				if (symbol == NULL)
					continue;

				if (callerSymbol == NULL) {
					// already skipped the caller symbol -- so this is
					// the one we're looking for
					*_location = (void*)(symbol->st_value
						+ image->regions[0].delta);
					int32 symbolType = B_SYMBOL_TYPE_TEXT;
					patch_defined_symbol(image, symbolName, _location,
						&symbolType);
					status = B_OK;
					break;
				}
				if (symbol == callerSymbol) {
					// found the caller symbol
					callerSymbol = NULL;
				}
			}

			clear_image_flags_recursively(callerImage, RFLAG_USE_FOR_RESOLVING);
		}

	} else {
		// breadth-first search in the given image and its dependencies
		image_t* inImage;
		status = find_symbol_breadth_first((image_t*)handle, symbolName,
			B_SYMBOL_TYPE_ANY, &inImage, _location);
	}

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


/*! Read and verify the ELF header */
status_t
elf_verify_header(void *header, int32 length)
{
	int32 programSize, sectionSize;

	if (length < (int32)sizeof(struct Elf32_Ehdr))
		return B_NOT_AN_EXECUTABLE;

	return parse_elf_header((struct Elf32_Ehdr *)header, &programSize,
		&sectionSize);
}


void
terminate_program(void)
{
	image_t **termList;
	ssize_t count, i;

	count = get_sorted_image_list(sProgramImage, &termList, RFLAG_TERMINATED);
	if (count < B_OK)
		return;

	if (sInvalidImageIDs) {
		// After fork, we lazily rebuild the image IDs of all loaded images
		update_image_ids();
	}

	TRACE(("%ld: terminate dependencies\n", find_thread(NULL)));
	for (i = count; i-- > 0;) {
		image_t *image = termList[i];

		TRACE(("%ld:  term: %s\n", find_thread(NULL), image->name));

		image_event(image, IMAGE_EVENT_UNINITIALIZING);

		if (image->term_routine)
			((init_term_function)image->term_routine)(image->id);

		image_event(image, IMAGE_EVENT_UNLOADING);
	}
	TRACE(("%ld:  term done.\n", find_thread(NULL)));

	free(termList);
}


void
rldelf_init(void)
{
	// invoke static constructors
	new(&sAddOns) AddOnList;

	sSem = create_sem(1, "runtime loader");
	sSemOwner = -1;
	sSemCount = 0;

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
elf_reinit_after_fork(void)
{
	sSem = create_sem(1, "runtime loader");
	if (sSem < 0)
		return sSem;

	// We also need to update the IDs of our images. We are the child and
	// and have cloned images with different IDs. Since in most cases (fork()
	// + exec*()) this would just increase the fork() overhead with no one
	// caring, we do that lazily, when first doing something different.
	sInvalidImageIDs = true;

	return B_OK;
}
