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

	bool NeedsLocking();


public:
	Resource(const char* transport, const char* address, const char* connection);
	~Resource();

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
