/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ADD_ONS_H
#define ADD_ONS_H

#include <OS.h>

#include <util/DoublyLinkedList.h>

#include <runtime_loader.h>


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


void		init_add_ons();
status_t	add_add_on(image_t* image, runtime_loader_add_on* addOnStruct);
void		image_event(image_t* image, uint32 event);


#endif	// ADD_ONS_H
