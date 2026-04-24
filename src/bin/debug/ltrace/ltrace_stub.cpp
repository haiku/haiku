/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>

#include <new>

#include <OS.h>

#include <runtime_loader.h>
#include <util/OpenHashTable.h>

#include <syscalls.h>

#include "arch/ltrace_stub.h"


//#define TRACE_STUB
#ifdef TRACE_STUB
#   define TRACE(x...) ktrace_printf(x)
#else
#   define TRACE(x...) ;
#endif


static void* function_call_callback(const void* stub, const void* args);


// TODO: Per-image_t patch memory (so it can be freed,
// or marked RX-only after symbol resolution completes?)
static const size_t kPatchMemoryChunkSize = B_PAGE_SIZE * 16;
static const size_t kPatchMemoryReserveSize = 1 * 1024 * 1024;
static uint8* sPatchMemoryChunk = NULL;
static size_t sPatchMemoryChunkRemaining = 0;
static area_id sPatchMemoryChunkArea = 0;
static size_t sPatchMemoryChunkAreaSize = 0;


static uint8*
patch_malloc(size_t size)
{
	if (sPatchMemoryChunkRemaining < size) {
		status_t status = B_NO_MEMORY;
		if (sPatchMemoryChunkArea != 0) {
			// Try to resize the previous area instead of creating a new one.
			status = _kern_resize_area(sPatchMemoryChunkArea,
				sPatchMemoryChunkAreaSize + kPatchMemoryChunkSize);
			if (status == B_OK) {
				sPatchMemoryChunkAreaSize += kPatchMemoryChunkSize;
				sPatchMemoryChunkRemaining += kPatchMemoryChunkSize;
			}
		}
		if (status != B_OK) {
			void* reservedBase;
			status = _kern_reserve_address_range((addr_t*)&reservedBase,
				B_RANDOMIZED_ANY_ADDRESS, kPatchMemoryReserveSize);
			if (status != B_OK)
				return NULL;

			void* base = reservedBase;
			area_id area = _kern_create_area("ltrace patches", &base,
				B_EXACT_ADDRESS, size, B_NO_LOCK,
				B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA);
			if (area < 0) {
				_kern_unreserve_address_range((addr_t)reservedBase,
					kPatchMemoryReserveSize);
				return NULL;
			}

			sPatchMemoryChunk = (uint8*)base;
			sPatchMemoryChunkArea = area;
			sPatchMemoryChunkRemaining = sPatchMemoryChunkAreaSize = size;
		}
	}

	uint8* allocation = sPatchMemoryChunk;
	sPatchMemoryChunk += size;
	sPatchMemoryChunkRemaining -= size;
	return allocation;
}


struct PatchEntry {
	PatchEntry*	original_table_link;

	void*		original_function;
	const char*	function_name;

	bool		trace;

	static PatchEntry* Create(const char* name, void* function)
	{
		void* memory = patch_malloc(_ALIGN(sizeof(PatchEntry))
			+ arch_call_stub_size());
		if (memory == NULL)
			return NULL;

		PatchEntry* entry = new(memory) PatchEntry;

		void* stub = (uint8*)memory + OffsetToStub();
		arch_init_call_stub(stub, &function_call_callback, function);

		entry->original_function = function;
		entry->function_name = name;
		entry->trace = true;

		return entry;
	}

	static size_t OffsetToStub()
	{
		return _ALIGN(sizeof(PatchEntry));
	}

	void* Stub()
	{
		return (uint8*)this + OffsetToStub();
	}
};


struct OriginalTableDefinition {
	typedef const void*	KeyType;
	typedef PatchEntry	ValueType;

	size_t HashKey(const void* key) const
	{
		return (addr_t)key >> 2;
	}

	size_t Hash(PatchEntry* value) const
	{
		return HashKey(value->original_function);
	}

	bool Compare(const void* key, PatchEntry* value) const
	{
		return value->original_function == key;
	}

	PatchEntry*& GetLink(PatchEntry* value) const
	{
		return value->original_table_link;
	}
};


static rld_export* sRuntimeLoaderInterface;
static runtime_loader_add_on_export* sRuntimeLoaderAddOnInterface;

static BOpenHashTable<OriginalTableDefinition> sOriginalTable;


static void*
function_call_callback(const void* stub, const void* _args)
{
	PatchEntry* entry = (PatchEntry*)((uint8*)stub - PatchEntry::OffsetToStub());

	if (!entry->trace)
		return entry->original_function;

	char buffer[1024];
	size_t bufferSize = sizeof(buffer);
	size_t written = 0;

	const ulong* args = (const ulong*)_args;
	written += snprintf(buffer, bufferSize, "ltrace: %s(",
		entry->function_name);
	for (int32 i = 0; i < 5; i++) {
		written += snprintf(buffer + written, bufferSize - written, "%s%#lx",
			i == 0 ? "" : ", ", args[i]);
	}
	written += snprintf(buffer + written, bufferSize - written, ")\n");
	write(0, buffer, written);

	return entry->original_function;
}


static void
symbol_patcher(void* cookie, image_t* rootImage, image_t* image,
	const char* name, image_t** foundInImage, void** symbol, int32* type)
{
	TRACE("symbol_patcher(%p, %p, %p, \"%s\", %p, %p, %" B_PRId32 ")\n",
		cookie, rootImage, image, name, *foundInImage, *symbol, *type);

	// patch functions only
	if (*type != B_SYMBOL_TYPE_TEXT)
		return;

	// already patched?
	PatchEntry* entry = sOriginalTable.Lookup(*symbol);
	if (entry != NULL) {
		*foundInImage = NULL;
		*symbol = entry->Stub();
		return;
	}

	entry = PatchEntry::Create(name, *symbol);
	if (entry == NULL)
		return;

	sOriginalTable.Insert(entry);

	TRACE("  -> patching to %p\n", entry->Stub());

	*foundInImage = NULL;
	*symbol = entry->Stub();
}


static void
ltrace_stub_init(rld_export* standardInterface,
	runtime_loader_add_on_export* addOnInterface)
{
	TRACE("ltrace_stub_init(%p, %p)\n", standardInterface, addOnInterface);
	sRuntimeLoaderInterface = standardInterface;
	sRuntimeLoaderAddOnInterface = addOnInterface;

	sOriginalTable.Init();
}


static void
ltrace_stub_image_loaded(image_t* image)
{
	TRACE("ltrace_stub_image_loaded(%p): \"%s\" (%" B_PRId32 ")\n",
		image, image->path, image->id);

	if (sRuntimeLoaderAddOnInterface->register_undefined_symbol_patcher(image,
			symbol_patcher, (void*)(addr_t)0xc0011eaf) != B_OK) {
		TRACE("  failed to install symbol patcher\n");
	}
}


static void
ltrace_stub_image_relocated(image_t* image)
{
	TRACE("ltrace_stub_image_relocated(%p): \"%s\" (%" B_PRId32 ")\n",
		image, image->path, image->id);
}


static void
ltrace_stub_image_initialized(image_t* image)
{
	TRACE("ltrace_stub_image_initialized(%p): \"%s\" (%" B_PRId32 ")\n",
		image, image->path, image->id);
}


static void
ltrace_stub_image_uninitializing(image_t* image)
{
	TRACE("ltrace_stub_image_uninitializing(%p): \"%s\" (%" B_PRId32
		")\n",image, image->path, image->id);
}


static void
ltrace_stub_image_unloading(image_t* image)
{
	TRACE("ltrace_stub_image_unloading(%p): \"%s\" (%" B_PRId32 ")\n",
		image, image->path, image->id);
}


// interface for the runtime loader
runtime_loader_add_on __gRuntimeLoaderAddOn = {
	RUNTIME_LOADER_ADD_ON_VERSION,	// version
	0,								// flags

	ltrace_stub_init,

	ltrace_stub_image_loaded,
	ltrace_stub_image_relocated,
	ltrace_stub_image_initialized,
	ltrace_stub_image_uninitializing,
	ltrace_stub_image_unloading
};
