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

#ifndef _CACHE_H
#define _CACHE_H

#include "Utils.h"

class CacheItem;
class CacheItemReference;

// CacheItemDescription
class CIDescription {
public:
	CIDescription() {};
	virtual ~CIDescription() {};
	
	virtual CacheItem* NewItem(int id) { return NULL; }
};

class CacheItem {
public:
	CacheItem() {}
	virtual ~CacheItem() {}
	
	virtual bool Equals(CIDescription* desc) const = 0;
	virtual CacheItem* Reference() { return this; }
};

class Cache {
public:
	Cache();
	virtual ~Cache() {};

	void NextPass(); 

	// returns the CacheItem at "NextID", returns NULL on error
	CacheItem* Find(CIDescription* desc);
	void MakeEmpty() { fCache.MakeEmpty(); }
	int32 CountItems() const { return fCache.CountItems(); }
	CacheItem* ItemAt(int32 i) const { return fCache.ItemAt(i); }
	
private:
	int8  fPass;
	int32 fNextID;
	TList<CacheItem> fCache;
};

#endif
