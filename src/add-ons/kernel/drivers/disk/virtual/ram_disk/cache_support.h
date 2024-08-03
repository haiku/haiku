/*
 * Copyright 2010-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef CACHE_SUPPORT_H
#define CACHE_SUPPORT_H


static void
cache_get_pages(VMCache* cache, off_t offset, off_t length, bool isWrite, vm_page** pages)
{
	// get the pages, we already have
	AutoLocker<VMCache> locker(cache);

	size_t pageCount = length / B_PAGE_SIZE;
	size_t index = 0;
	size_t missingPages = 0;

	while (length > 0) {
		vm_page* page = cache->LookupPage(offset);
		if (page != NULL) {
			if (page->busy) {
				cache->WaitForPageEvents(page, PAGE_EVENT_NOT_BUSY, true);
				continue;
			}

			DEBUG_PAGE_ACCESS_START(page);
			page->busy = true;
		} else
			missingPages++;

		pages[index++] = page;
		offset += B_PAGE_SIZE;
		length -= B_PAGE_SIZE;
	}

	locker.Unlock();

	// For a write we need to reserve the missing pages.
	if (isWrite && missingPages > 0) {
		vm_page_reservation reservation;
		vm_page_reserve_pages(&reservation, missingPages,
			VM_PRIORITY_SYSTEM);

		for (size_t i = 0; i < pageCount; i++) {
			if (pages[i] != NULL)
				continue;

			pages[i] = vm_page_allocate_page(&reservation, VM_PAGE_ALLOC_CLEAR
				| PAGE_STATE_WIRED | VM_PAGE_ALLOC_BUSY);

			if (--missingPages == 0)
				break;
		}

		vm_page_unreserve_pages(&reservation);
	}
}


static void
cache_put_pages(VMCache* cache, off_t offset, off_t length, vm_page** pages, bool success)
{
	AutoLocker<VMCache> locker(cache);

	// Mark all pages unbusy. On error free the newly allocated pages.
	size_t index = 0;

	while (length > 0) {
		vm_page* page = pages[index++];
		if (page != NULL) {
			if (page->CacheRef() == NULL) {
				if (success) {
					cache->InsertPage(page, offset);
					cache->MarkPageUnbusy(page);
					DEBUG_PAGE_ACCESS_END(page);
				} else
					vm_page_free(NULL, page);
			} else {
				cache->MarkPageUnbusy(page);
				DEBUG_PAGE_ACCESS_END(page);
			}
		}

		offset += B_PAGE_SIZE;
		length -= B_PAGE_SIZE;
	}
}


#endif	// CACHE_SUPPORT_H
