/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>

#include <new>

#include <OS.h>

#include <runtime_loader.h>
#include <util/OpenHashTable.h>

#include "arch/ltrace_stub.h"


//#define TRACE_PRINTF debug_printf
#define TRACE_PRINTF ktrace_printf


static void* function_call_callback(const void* stub, const void* args);


struct PatchEntry {
	PatchEntry*	originalTableLink;
	PatchEntry*	patchedTableLink;

	void*		originalFunction;
	void*		patchedFunction;
	const char*	functionName;

	static PatchEntry* Create(const char* name, void* function)
	{
		// TODO memory should be executable, use mmap with PROT_EXEC
		void* memory = malloc(_ALIGN(sizeof(PatchEntry))
			+ arch_call_stub_size());
		if (memory == NULL)
			return NULL;

		PatchEntry* entry = new(memory) PatchEntry;

		void* stub = (uint8*)memory + _ALIGN(sizeof(PatchEntry));
		arch_init_call_stub(stub, &function_call_callback, function);

		entry->originalFunction = function;
		entry->patchedFunction = stub;
		entry->functionName = name;

		return entry;
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
		return HashKey(value->originalFunction);
	}

	bool Compare(const void* key, PatchEntry* value) const
	{
		return value->originalFunction == key;
	}

	PatchEntry*& GetLink(PatchEntry* value) const
	{
		return value->originalTableLink;
	}
};


struct PatchedTableDefinition {
	typedef const void*	KeyType;
	typedef PatchEntry	ValueType;

	size_t HashKey(const void* key) const
	{
		return (addr_t)key >> 2;
	}

	size_t Hash(PatchEntry* value) const
	{
		return HashKey(value->patchedFunction);
	}

	bool Compare(const void* key, PatchEntry* value) const
	{
		return value->patchedFunction == key;
	}

	PatchEntry*& GetLink(PatchEntry* value) const
	{
		return value->patchedTableLink;
	}
};


static rld_export* sRuntimeLoaderInterface;
static runtime_loader_add_on_export* sRuntimeLoaderAddOnInterface;

static BOpenHashTable<OriginalTableDefinition> sOriginalTable;
static BOpenHashTable<PatchedTableDefinition> sPatchedTable;


static void*
function_call_callback(const void* stub, const void* _args)
{
	PatchEntry* entry = sPatchedTable.Lookup(stub);
	if (entry == NULL)
{
TRACE_PRINTF("function_call_callback(): CALLED FOR UNKNOWN FUNCTION!\n");
		return NULL;
}

	char buffer[1024];
	size_t bufferSize = sizeof(buffer);
	size_t written = 0;

	const ulong* args = (const ulong*)_args;
	written += snprintf(buffer, bufferSize, "ltrace: %s(",
		entry->functionName);
	for (int32 i = 0; i < 5; i++) {
		written += snprintf(buffer + written, bufferSize - written, "%s%#lx",
			i == 0 ? "" : ", ", args[i]);
	}
	written += snprintf(buffer + written, bufferSize - written, ")");
	TRACE_PRINTF("%s\n", buffer);

	return entry->originalFunction;
}


static void
symbol_patcher(void* cookie, image_t* rootImage, image_t* image,
	const char* name, image_t** foundInImage, void** symbol, int32* type)
{
	TRACE_PRINTF("symbol_patcher(%p, %p, %p, \"%s\", %p, %p, %" B_PRId32 ")\n",
		cookie, rootImage, image, name, *foundInImage, *symbol, *type);

	// patch functions only
	if (*type != B_SYMBOL_TYPE_TEXT)
		return;

	// already patched?
	PatchEntry* entry = sOriginalTable.Lookup(*symbol);
	if (entry != NULL) {
		*foundInImage = NULL;
		*symbol = entry->patchedFunction;
		return;
	}

	entry = PatchEntry::Create(name, *symbol);
	if (entry == NULL)
		return;

	sOriginalTable.Insert(entry);
	sPatchedTable.Insert(entry);

	TRACE_PRINTF("  -> patching to %p\n", entry->patchedFunction);

	*foundInImage = NULL;
	*symbol = entry->patchedFunction;
}


static void
ltrace_stub_init(rld_export* standardInterface,
	runtime_loader_add_on_export* addOnInterface)
{
	TRACE_PRINTF("ltrace_stub_init(%p, %p)\n", standardInterface, addOnInterface);
	sRuntimeLoaderInterface = standardInterface;
	sRuntimeLoaderAddOnInterface = addOnInterface;

	sOriginalTable.Init();
	sPatchedTable.Init();
}


static void
ltrace_stub_image_loaded(image_t* image)
{
	TRACE_PRINTF("ltrace_stub_image_loaded(%p): \"%s\" (%" B_PRId32 ")\n",
		image, image->path, image->id);

	if (sRuntimeLoaderAddOnInterface->register_undefined_symbol_patcher(image,
			symbol_patcher, (void*)(addr_t)0xc0011eaf) != B_OK) {
		TRACE_PRINTF("  failed to install symbol patcher\n");
	}
}


static void
ltrace_stub_image_relocated(image_t* image)
{
	TRACE_PRINTF("ltrace_stub_image_relocated(%p): \"%s\" (%" B_PRId32 ")\n",
		image, image->path, image->id);
}


static void
ltrace_stub_image_initialized(image_t* image)
{
	TRACE_PRINTF("ltrace_stub_image_initialized(%p): \"%s\" (%" B_PRId32 ")\n",
		image, image->path, image->id);
}


static void
ltrace_stub_image_uninitializing(image_t* image)
{
	TRACE_PRINTF("ltrace_stub_image_uninitializing(%p): \"%s\" (%" B_PRId32
		")\n",image, image->path, image->id);
}


static void
ltrace_stub_image_unloading(image_t* image)
{
	TRACE_PRINTF("ltrace_stub_image_unloading(%p): \"%s\" (%" B_PRId32 ")\n",
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
