/*
 * Copyright 2003-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef RUNTIME_LOADER_PRIVATE_H
#define RUNTIME_LOADER_PRIVATE_H

#include <user_runtime.h>
#include <runtime_loader.h>

#include "tracing_config.h"

#include "utility.h"


//#define TRACE_RLD
#ifdef TRACE_RLD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#if RUNTIME_LOADER_TRACING
#	define KTRACE(x...)	ktrace_printf(x)
#else
#	define KTRACE(x...)
#endif	// RUNTIME_LOADER_TRACING

#if defined(_COMPAT_MODE) && !defined(__x86_64__)
	#if __GNUC__ == 2
		#define RLD_PREFIX "runtime_loader_x86_gcc2: "
	#else
		#define RLD_PREFIX "runtime_loader_x86: "
	#endif
#endif
#ifndef RLD_PREFIX
#define RLD_PREFIX "runtime_loader: "
#endif
#define FATAL(x...)							\
	do {									\
		dprintf(RLD_PREFIX x);		\
		if (!gProgramLoaded)				\
			printf(RLD_PREFIX x);	\
	} while (false)


struct SymbolLookupCache;


extern struct user_space_program_args* gProgramArgs;
extern void* __gCommPageAddress;
extern struct rld_export gRuntimeLoader;
extern char* (*gGetEnv)(const char* name);
extern bool gProgramLoaded;
extern image_t* gProgramImage;


extern "C" {

int runtime_loader(void* arg, void* commpage);
int open_executable(char* name, image_type type, const char* rpath,
	const char* programPath, const char* requestingObjectPath,
	const char* abiSpecificSubDir);
status_t test_executable(const char* path, char* interpreter);
status_t get_executable_architecture(const char* path,
	const char** _architecture);

void terminate_program(void);
image_id load_program(char const* path, void** entry);
image_id load_library(char const* path, uint32 flags, bool addOn,
	void** _handle);
status_t unload_library(void* handle, image_id imageID, bool addOn);
status_t get_nth_symbol(image_id imageID, int32 num, char* nameBuffer,
	int32* _nameLength, int32* _type, void** _location);
status_t get_nearest_symbol_at_address(void* address, image_id* _imageID,
	char** _imagePath, char** _imageName, char** _symbolName, int32* _type,
	void** _location, bool* _exactMatch);
status_t get_symbol(image_id imageID, char const* symbolName, int32 symbolType,
	bool recursive, image_id* _inImage, void** _location);
status_t get_library_symbol(void* handle, void* caller, const char* symbolName,
	void** _location);
status_t get_next_image_dependency(image_id id, uint32* cookie,
	const char** _name);
int resolve_symbol(image_t* rootImage, image_t* image, elf_sym* sym,
	SymbolLookupCache* cache, addr_t* sym_addr, image_t** symbolImage = NULL);


status_t elf_verify_header(void* header, size_t length);
#ifdef _COMPAT_MODE
#ifdef __x86_64__
status_t elf32_verify_header(void *header, size_t length);
#else
status_t elf64_verify_header(void *header, size_t length);
#endif	// __x86_64__
#endif	// _COMPAT_MODE
void rldelf_init(void);
void rldexport_init(void);
void set_abi_api_version(int abi_version, int api_version);
status_t elf_reinit_after_fork();

status_t heap_init();
status_t heap_reinit_after_fork();

// arch dependent prototypes
status_t arch_relocate_image(image_t* rootImage, image_t* image,
	SymbolLookupCache* cache);

}

#endif	/* RUNTIME_LOADER_PRIVATE_H */
