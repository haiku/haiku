/*****************************************************************************/
// ResourceManager
//
// Author
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

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

bool Resource::NeedsLocking() {
	return !(fTransport == "Print to File" || fTransport == "NONE"); 
}

bool Resource::Equals(const char* transport, const char* address, const char* connection) {
	return fTransport == transport &&
			fTransportAddress == address &&
			fConnection == connection;
}

bool Resource::Lock() {
	if (fResourceAvailable > 0) {
		return acquire_sem(fResourceAvailable) == B_NO_ERROR;
	}
	return true;
}

void Resource::Unlock() {
	if (fResourceAvailable > 0) {
		release_sem(fResourceAvailable);
	}
}

ResourceManager::~ResourceManager() {
	ASSERT(fResources.CountItems() == 0);
}

Resource* ResourceManager::Find(const char* transport, const char* address, const char* connection) {
	for (int i = 0; i < fResources.CountItems(); i ++) {
		Resource* r = fResources.ItemAt(i);
		if (r->Equals(transport, address, connection)) return r;
	}
	return NULL;
}

Resource* ResourceManager::Allocate(const char* transport, const char* address, const char* connection) {
	Resource* r = Find(transport, address, connection);
	if (r == NULL) {
		r = new Resource(transport, address, connection);
		fResources.AddItem(r);
	} else {
		r->Acquire();
	}
	return r;
}


void ResourceManager::Free(Resource* r) {
	if (r->Release()) {
		fResources.RemoveItem(r);
	}	
}
