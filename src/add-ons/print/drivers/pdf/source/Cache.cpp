/*

"Special" Cache.

Copyright (c) 2003 OpenBeOS. 

Author: 
	Michael Pfeiffer
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include "Report.h"
#include "Cache.h"
#include <Debug.h>

class CIReference : public CacheItem {
public:
	CIReference(CacheItem* item) : fItem(item) { 
		ASSERT(dynamic_cast<CIReference*>(item) == NULL); 
	}
	
	bool Equals(CIDescription* desc) const { 
		return fItem->Equals(desc); 
	}

	CacheItem* Reference() { 
		return fItem; 
	}

private:
	CacheItem* fItem;	
};

Cache::Cache() 
	: fPass(0) 
	, fNextID(0)
{};

void Cache::NextPass() { 
	fPass++; 
	fNextID = 0; 
	// max two passes!
	ASSERT(fPass == 1);
}

CacheItem* Cache::Find(CIDescription* desc) {
	REPORT(kDebug, -1, "Cache::Find() pass = %d next id = %d", fPass, fNextID);
	int id = fNextID ++;
	const int32 n = CountItems();
	CacheItem* item = ItemAt(id);

	// In 2. pass for each item an entry must exists
	ASSERT(fPass == 1 && item != NULL);
	if (fPass == 1) return item->Reference();
	
	// In 1. pass we create an entry for each bitmap
	for (int32 i = 0; i < n; i ++) {
		item = ItemAt(i);
		// skip references
		if (item != item->Reference()) continue;
		if (item->Equals(desc)) {
			// found item in cache, create a reference to it
			CacheItem* ref = new CIReference(item->Reference());
			if (ref == NULL) return NULL;
			fCache.AddItem(ref);
			return item;
		}
	}
	// item not in cache, create one
	item = desc->NewItem(id);
	if (item != NULL) {
		ASSERT(dynamic_cast<CIReference*>(item) == NULL);
		fCache.AddItem(item);
	}
	return item;
}
