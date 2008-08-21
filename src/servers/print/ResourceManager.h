/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "ObjectList.h"

#include <Locker.h>
#include <String.h>

#include "BeUtils.h"

class Resource : public Object {
private:
	BString	 fTransport;
	BString  fTransportAddress;
	BString  fConnection;
	sem_id   fResourceAvailable;

public:
	Resource(const char* transport, const char* address, const char* connection);
	~Resource();

	bool NeedsLocking();

	bool Equals(const char* transport, const char* address, const char* connection);

	const BString& Transport() const { return fTransport; }

	bool Lock();
	void Unlock();
};

class ResourceManager {
private:
	BObjectList<Resource> fResources;	
		
	Resource* Find(const char* transport, const char* address, const char* connection);

public:
	~ResourceManager();

	Resource* Allocate(const char* transport, const char* address, const char* connection);
	void Free(Resource* r);
};

#endif
