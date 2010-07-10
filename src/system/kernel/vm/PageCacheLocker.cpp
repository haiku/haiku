/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "PageCacheLocker.h"

#include <vm/VMCache.h>


bool
PageCacheLocker::_IgnorePage(vm_page* page)
{
	if (page->busy || page->State() == PAGE_STATE_WIRED
		|| page->State() == PAGE_STATE_FREE || page->State() == PAGE_STATE_CLEAR
		|| page->State() == PAGE_STATE_UNUSED || page->WiredCount() > 0)
		return true;

	return false;
}


bool
PageCacheLocker::Lock(vm_page* page, bool dontWait)
{
	if (_IgnorePage(page))
		return false;

	// Grab a reference to this cache.
	VMCache* cache = vm_cache_acquire_locked_page_cache(page, dontWait);
	if (cache == NULL)
		return false;

	if (_IgnorePage(page)) {
		cache->ReleaseRefAndUnlock();
		return false;
	}

	fPage = page;
	return true;
}


void
PageCacheLocker::Unlock()
{
	if (fPage == NULL)
		return;

	fPage->Cache()->ReleaseRefAndUnlock();

	fPage = NULL;
}
