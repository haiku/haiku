/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "add_ons.h"

#include <util/kernel_cpp.h>

#include "runtime_loader_private.h"


typedef DoublyLinkedList<RuntimeLoaderAddOn> AddOnList;


static status_t register_defined_symbol_patcher(struct image_t* image,
	runtime_loader_symbol_patcher* _patcher, void* cookie);
static void unregister_defined_symbol_patcher(struct image_t* image,
	runtime_loader_symbol_patcher* _patcher, void* cookie);
static status_t register_undefined_symbol_patcher(struct image_t* image,
	runtime_loader_symbol_patcher* _patcher, void* cookie);
static void unregister_undefined_symbol_patcher(struct image_t* image,
	runtime_loader_symbol_patcher* _patcher, void* cookie);


static AddOnList sAddOns;

static runtime_loader_add_on_export sRuntimeLoaderAddOnExport = {
	register_defined_symbol_patcher,
	unregister_defined_symbol_patcher,
	register_undefined_symbol_patcher,
	unregister_undefined_symbol_patcher
};


// #pragma mark - add-on support functions


static status_t
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


static void
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


static status_t
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


static void
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


// #pragma mark -


void
init_add_ons()
{
	// invoke static constructors
	new(&sAddOns) AddOnList;
}


status_t
add_add_on(image_t* image, runtime_loader_add_on* addOnStruct)
{
	RuntimeLoaderAddOn* addOn = new(mynothrow) RuntimeLoaderAddOn(image,
		addOnStruct);
	if (addOn == NULL)
		return B_NO_MEMORY;

	sAddOns.Add(addOn);
	addOnStruct->init(&gRuntimeLoader, &sRuntimeLoaderAddOnExport);

	return B_OK;
}


void
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
