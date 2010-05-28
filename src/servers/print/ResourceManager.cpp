/*
 * Copyright 2002-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#include "ResourceManager.h"

#include <Debug.h>
#include <Autolock.h>

Resource::Resource(const char* transport, const char* address, const char* connection) 
	: fTransport(transport)
	, fTransportAddress(address)
	, fConnection(connection)
	, fResourceAvailable(0)

{
	if (NeedsLocking()) {
		fResourceAvailable = create_sem(1, "resource");
	}
}


Resource::~Resource() {
	if (fResourceAvailable > 0) delete_sem(fResourceAvailable);
}


bool 
Resource::NeedsLocking() {
	// TODO R2: Provide API to query that information
	// ATM: Print jobs are not processed sequentially
	// if the transport add-on is either "Print To File"
	// or in case of "Preview" printer it
	// is set on R5 to "NONE" IIRC and the Haiku
	// preflet sets an empty string.
	return !(fTransport == "Print to file" 
		|| fTransport == "NONE"
		|| fTransport == ""); 
}


bool 
Resource::Equals(const char* transport, const char* address, const char* connection) {
	return fTransport == transport &&
			fTransportAddress == address &&
			fConnection == connection;
}


bool 
Resource::Lock() {
	if (fResourceAvailable > 0) {
		return acquire_sem(fResourceAvailable) == B_NO_ERROR;
	}
	return true;
}


void 
Resource::Unlock() {
	if (fResourceAvailable > 0) {
		release_sem(fResourceAvailable);
	}
}


ResourceManager::~ResourceManager() {
	ASSERT(fResources.CountItems() == 0);
}


Resource* 
ResourceManager::Find(const char* transport, const char* address, const char* connection) {
	for (int i = 0; i < fResources.CountItems(); i ++) {
		Resource* r = fResources.ItemAt(i);
		if (r->Equals(transport, address, connection)) return r;
	}
	return NULL;
}


Resource* 
ResourceManager::Allocate(const char* transport, const char* address, const char* connection) {
	Resource* r = Find(transport, address, connection);
	if (r == NULL) {
		r = new Resource(transport, address, connection);
		fResources.AddItem(r);
	} else {
		r->Acquire();
	}
	return r;
}


void 
ResourceManager::Free(Resource* r) {
	if (r->Release()) {
		fResources.RemoveItem(r);
	}	
}
