/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "PageCacheLocker.h"

#include <signal.h>

#include <algorithm>

#include <OS.h>

#include <vm.h>
#include <vm_priv.h>
#include <vm_cache.h>
#include <vm_page.h>
#include "VMAnonymousCache.h"


//#define TRACK_PAGE_USAGE_STATS	1


const static uint32 kMinScanPagesCount = 512;
const static uint32 kMaxScanPagesCount = 8192;
const static bigtime_t kMinScanWaitInterval = 50000LL;		// 50 ms
const static bigtime_t kMaxScanWaitInterval = 1000000LL;	// 1 sec

static uint32 sLowPagesCount;
static sem_id sPageDaemonSem;
static uint32 sNumPages;

#ifdef TRACK_PAGE_USAGE_STATS
static page_num_t sPageUsageArrays[512];
static page_num_t* sPageUsage = sPageUsageArrays;
static page_num_t sPageUsagePageCount;
static page_num_t* sNextPageUsage = sPageUsageArrays + 256;
static page_num_t sNextPageUsagePageCount;
#endif


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


bool
PageCacheLocker::_IgnorePage(vm_page* page)
{
	if (page->state == PAGE_STATE_WIRED || page->state == PAGE_STATE_BUSY
		|| page->state == PAGE_STATE_FREE || page->state == PAGE_STATE_CLEAR
		|| page->state == PAGE_STATE_UNUSED || page->wired_count > 0
		|| page->cache == NULL)
		return true;

	return false;
}


bool
PageCacheLocker::Lock(vm_page* page, bool dontWait)
{
	if (_IgnorePage(page))
		return false;

	// Grab a reference to this cache.
	vm_cache* cache = vm_cache_acquire_locked_page_cache(page, dontWait);
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

	fPage->cache->ReleaseRefAndUnlock();

	fPage = NULL;
}


//	#pragma mark -


#ifdef TRACK_PAGE_USAGE_STATS

static void
track_page_usage(vm_page* page)
{
	if (page->wired_count == 0) {
		sNextPageUsage[(int32)page->usage_count + 128]++;
		sNextPageUsagePageCount++;
	}
}


static void
update_page_usage_stats()
{
	std::swap(sPageUsage, sNextPageUsage);
	sPageUsagePageCount = sNextPageUsagePageCount;

	memset(sNextPageUsage, 0, sizeof(page_num_t) * 256);
	sNextPageUsagePageCount = 0;

	// compute average
	if (sPageUsagePageCount > 0) {
		int64 sum = 0;
		for (int32 i = 0; i < 256; i++)
			sum += (int64)sPageUsage[i] * (i - 128);

		dprintf("average page usage: %f (%lu pages)\n",
			(float)sum / sPageUsagePageCount, sPageUsagePageCount);
	}
}


static int
dump_page_usage_stats(int argc, char** argv)
{
	kprintf("distribution of page usage counts (%lu pages):",
		sPageUsagePageCount);

	int64 sum = 0;
	for (int32 i = 0; i < 256; i++) {
		if (i % 8 == 0)
			kprintf("\n%4ld:", i - 128);

		int64 count = sPageUsage[i];
		sum += count * (i - 128);

		kprintf("  %9llu", count);
	}

	kprintf("\n\n");

	kprintf("average usage count: %f\n",
		sPageUsagePageCount > 0 ? (float)sum / sPageUsagePageCount : 0);

	return 0;
}

#endif


static void
clear_page_activation(int32 index)
{
	vm_page *page = vm_page_at_index(index);
	PageCacheLocker locker(page);
	if (!locker.IsLocked())
		return;

	if (page->state == PAGE_STATE_ACTIVE)
		vm_clear_map_flags(page, PAGE_ACCESSED);
}


static bool
check_page_activation(int32 index)
{
	vm_page *page = vm_page_at_index(index);
	PageCacheLocker locker(page);
	if (!locker.IsLocked())
		return false;

	bool modified;
	int32 activation = vm_test_map_activation(page, &modified);
	if (modified && page->state != PAGE_STATE_MODIFIED) {
		//dprintf("page %p -> move to modified\n", page);
		vm_page_set_state(page, PAGE_STATE_MODIFIED);
	}

	if (activation > 0) {
		// page is still in active use
		if (page->usage_count < 0) {
			if (page->state != PAGE_STATE_MODIFIED)
				vm_page_set_state(page, PAGE_STATE_ACTIVE);
			page->usage_count = 1;
			//dprintf("page %p -> move to active\n", page);
		} else if (page->usage_count < 127)
			page->usage_count++;

#ifdef TRACK_PAGE_USAGE_STATS
		track_page_usage(page);
#endif

		return false;
	}

	if (page->usage_count > -128)
		page->usage_count--;

#ifdef TRACK_PAGE_USAGE_STATS
		track_page_usage(page);
#endif

	if (page->usage_count < 0) {
		uint32 flags;
		vm_remove_all_page_mappings(page, &flags);

		// recheck eventual last minute changes
		if ((flags & PAGE_MODIFIED) != 0 && page->state != PAGE_STATE_MODIFIED)
			vm_page_set_state(page, PAGE_STATE_MODIFIED);
		if ((flags & PAGE_ACCESSED) != 0 && ++page->usage_count >= 0)
			return false;

		if (page->state == PAGE_STATE_MODIFIED)
			vm_page_schedule_write_page(page);
		else
			vm_page_set_state(page, PAGE_STATE_INACTIVE);
		//dprintf("page %p -> move to inactive\n", page);
	}

	return true;
}


#if ENABLE_SWAP_SUPPORT
static bool
free_page_swap_space(int32 index)
{
	vm_page *page = vm_page_at_index(index);
	PageCacheLocker locker(page);
	if (!locker.IsLocked())
		return false;

	if (page->cache->temporary && page->wired_count == 0
			&& page->cache->HasPage(page->cache_offset << PAGE_SHIFT)
			&& page->usage_count > 0) {
		// TODO: how to judge a page is highly active?
		if (swap_free_page_swap_space(page)) {
			// We need to mark the page modified, since otherwise it could be
			// stolen and we'd lose its data.
			vm_page_set_state(page, PAGE_STATE_MODIFIED);
			return true;
		}
	}
	return false;
}
#endif


static status_t
page_daemon(void* /*unused*/)
{
	bigtime_t scanWaitInterval = kMaxScanWaitInterval;
	uint32 scanPagesCount = kMinScanPagesCount;
	uint32 clearPage = 0;
	uint32 checkPage = sNumPages / 2;

#if ENABLE_SWAP_SUPPORT
	uint32 swapPage = 0;
#endif

	while (true) {
		acquire_sem_etc(sPageDaemonSem, 1, B_RELATIVE_TIMEOUT,
			scanWaitInterval);

		// Compute next run time
		uint32 pagesLeft = vm_page_num_free_pages();
		if (pagesLeft > sLowPagesCount) {
			// don't do anything if we have enough free memory left
			continue;
		}

		scanWaitInterval = kMinScanWaitInterval
			+ (kMaxScanWaitInterval - kMinScanWaitInterval)
			* pagesLeft / sLowPagesCount;
		scanPagesCount = kMaxScanPagesCount
			- (kMaxScanPagesCount - kMinScanPagesCount)
			* pagesLeft / sLowPagesCount;
		uint32 leftToFree = sLowPagesCount - pagesLeft;
		/*dprintf("wait interval %Ld, scan pages %lu, free %lu, "
			"target %lu\n",	scanWaitInterval, scanPagesCount, 
			pagesLeft, leftToFree);*/

		for (uint32 i = 0; i < scanPagesCount && leftToFree > 0; i++) {
			if (clearPage == 0)
				dprintf("clear through\n");
			if (checkPage == 0) {
				dprintf("check through\n");
#ifdef TRACK_PAGE_USAGE_STATS
				update_page_usage_stats();
#endif
			}
			clear_page_activation(clearPage);

			if (check_page_activation(checkPage))
				leftToFree--;

			if (++clearPage == sNumPages)
				clearPage = 0;
			if (++checkPage == sNumPages)
				checkPage = 0;
		}

#if ENABLE_SWAP_SUPPORT
		uint32 availableSwapPages = swap_available_pages();
		if (swap_total_swap_pages() > 0 && availableSwapPages < sNumPages / 8) {
			uint32 swapPagesToFree = sNumPages / 8 - availableSwapPages;
			uint32 swapScanCount = kMaxScanPagesCount
				- (kMaxScanPagesCount - kMinScanPagesCount)
				* availableSwapPages / (sNumPages / 8);

			for (uint32 i = 0; i < swapScanCount && swapPagesToFree > 0; i++) {
				if (free_page_swap_space(swapPage))
					swapPagesToFree--;
				if (++swapPage == sNumPages)
					swapPage = 0;
			}
		}
#endif
	}

	return B_OK;
}


void
vm_schedule_page_scanner(uint32 target)
{
	release_sem(sPageDaemonSem);
}


status_t
vm_daemon_init()
{
#ifdef TRACK_PAGE_USAGE_STATS
	add_debugger_command_etc("page_usage", &dump_page_usage_stats,
		"Dumps statistics about page usage counts",
		"\n"
		"Dumps statistics about page usage counts.\n",
		B_KDEBUG_DONT_PARSE_ARGUMENTS);
#endif

	sPageDaemonSem = create_sem(0, "page daemon");

	sNumPages = vm_page_num_pages();

	sLowPagesCount = sNumPages / 16;
	if (sLowPagesCount < 1024)
		sLowPagesCount = 1024;

	// create a kernel thread to select pages for pageout
	thread_id thread = spawn_kernel_thread(&page_daemon, "page daemon",
		B_NORMAL_PRIORITY, NULL);
	send_signal_etc(thread, SIGCONT, B_DO_NOT_RESCHEDULE);

	return B_OK;
}

