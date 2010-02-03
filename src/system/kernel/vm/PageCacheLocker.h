/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PAGE_CACHE_LOCKER_H
#define PAGE_CACHE_LOCKER_H


#include <null.h>

struct vm_page;


class PageCacheLocker {
public:
	inline						PageCacheLocker(vm_page* page,
									bool dontWait = true);
	inline						~PageCacheLocker();

			bool				IsLocked() { return fPage != NULL; }

			bool				Lock(vm_page* page, bool dontWait = true);
			void				Unlock();

private:
			bool				_IgnorePage(vm_page* page);

			vm_page*			fPage;
};


PageCacheLocker::PageCacheLocker(vm_page* page, bool dontWait)
	:
	fPage(NULL)
{
	Lock(page, dontWait);
}


PageCacheLocker::~PageCacheLocker()
{
	Unlock();
}


#endif	// PAGE_CACHE_LOCKER_H
