/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <thread.h>
#include <debug.h>
#include <khash.h>
#include <OS.h>
#include <arch/cpu.h>
#include <vm.h>
#include <vm_priv.h>
#include <vm_cache.h>
#include <vm_page.h>
#include <atomic.h>

bool trimming_cycle;
static addr free_memory_low_water;
static addr free_memory_high_water;

static void scan_pages(vm_address_space *aspace, addr free_target)
{
	vm_region *first_region;
	vm_region *region;
	vm_page *page;
	addr va;
	addr pa;
	unsigned int flags, flags2;
//	int err;
	int quantum = PAGE_SCAN_QUANTUM;

//	dprintf("scan_pages called on aspace 0x%x, id 0x%x, free_target %d\n", aspace, aspace->id, free_target);

	acquire_sem_etc(aspace->virtual_map.sem, READ_COUNT, 0, 0);

	first_region = aspace->virtual_map.region_list;
	while(first_region && (first_region->base + (first_region->size - 1)) < aspace->scan_va)
		first_region = first_region->aspace_next;

	if(!first_region)
		first_region = aspace->virtual_map.region_list;

	if(!first_region) {
		release_sem_etc(aspace->virtual_map.sem, READ_COUNT, 0);
		return;
	}

	region = first_region;
	for(;;) {
		// scan the pages in this region
		mutex_lock(&region->cache_ref->lock);
		if(!region->cache_ref->cache->scan_skip) {
			for(va = region->base; va < (region->base + region->size); va += PAGE_SIZE) {
				aspace->translation_map.ops->lock(&aspace->translation_map);
				aspace->translation_map.ops->query(&aspace->translation_map, va, &pa, &flags);
				if((flags & PAGE_PRESENT) == 0) {
					aspace->translation_map.ops->unlock(&aspace->translation_map);
					continue;
				}

				page = vm_lookup_page(pa);
				if(!page) {
					aspace->translation_map.ops->unlock(&aspace->translation_map);
					continue;
				}

				// see if this page is busy, if it is lets forget it and move on
				if(page->state == PAGE_STATE_BUSY || page->state == PAGE_STATE_WIRED) {
					aspace->translation_map.ops->unlock(&aspace->translation_map);
					continue;
				}

				flags2 = 0;
				if(free_target > 0) {
					// look for a page we can steal
					if(!(flags & PAGE_ACCESSED) && page->state == PAGE_STATE_ACTIVE) {
						// unmap the page
						aspace->translation_map.ops->unmap(&aspace->translation_map, va, va + PAGE_SIZE);

						// flush the tlbs of all cpus
						aspace->translation_map.ops->flush(&aspace->translation_map);

						// re-query the flags on the old pte, to make sure we have accurate modified bit data
						aspace->translation_map.ops->query(&aspace->translation_map, va, &pa, &flags2);

						// clear the modified and accessed bits on the entries
						aspace->translation_map.ops->clear_flags(&aspace->translation_map, va, PAGE_MODIFIED|PAGE_ACCESSED);

						// decrement the ref count on the page. If we just unmapped it for the last time,
						// put the page on the inactive list
						if(atomic_add(&page->ref_count, -1) == 1) {
							vm_page_set_state(page, PAGE_STATE_INACTIVE);
							free_target--;
						}
					}
				}

				// if the page is modified, but the state is active or inactive, put it on the modified list
				if(((flags & PAGE_MODIFIED) || (flags2 & PAGE_MODIFIED))
					&& (page->state == PAGE_STATE_ACTIVE || page->state == PAGE_STATE_INACTIVE)) {
					vm_page_set_state(page, PAGE_STATE_MODIFIED);
				}

				aspace->translation_map.ops->unlock(&aspace->translation_map);
				if(--quantum == 0)
					break;
			}
		}
		mutex_unlock(&region->cache_ref->lock);
		// move to the next region, wrapping around and stopping if we get back to the first region
		region = region->aspace_next ? region->aspace_next : aspace->virtual_map.region_list;
		if(region == first_region)
			break;

		if(quantum == 0)
			break;
	}

	aspace->scan_va = region ? (first_region->base + first_region->size) : aspace->virtual_map.base;
	release_sem_etc(aspace->virtual_map.sem, READ_COUNT, 0);

//	dprintf("exiting scan_pages\n");
}

static int page_daemon()
{
	struct hash_iterator i;
	vm_address_space *old_aspace;
	vm_address_space *aspace;
	addr mapped_size;
	addr free_memory_target;
	int faults_per_second;
	bigtime_t now;

	dprintf("page daemon starting\n");

	for(;;) {
		thread_snooze(PAGE_DAEMON_INTERVAL);

		// scan through all of the address spaces
		vm_aspace_walk_start(&i);
		aspace = vm_aspace_walk_next(&i);
		while(aspace) {
			mapped_size = aspace->translation_map.ops->get_mapped_size(&aspace->translation_map);

//			dprintf("page_daemon: looking at aspace 0x%x, id 0x%x, mapped size %d\n", aspace, aspace->id, mapped_size);

			now = system_time();
			if(now - aspace->last_working_set_adjust > WORKING_SET_ADJUST_INTERVAL) {
				faults_per_second = (aspace->fault_count * 1000000) / (now - aspace->last_working_set_adjust);
//				dprintf("  faults_per_second = %d\n", faults_per_second);
				aspace->last_working_set_adjust = now;
				aspace->fault_count = 0;

				if(faults_per_second > MAX_FAULTS_PER_SECOND
					&& mapped_size >= aspace->working_set_size
					&& aspace->working_set_size < aspace->max_working_set) {

					aspace->working_set_size += WORKING_SET_INCREMENT;
//					dprintf("  new working set size = %d\n", aspace->working_set_size);
				} else if(faults_per_second < MIN_FAULTS_PER_SECOND
					&& mapped_size <= aspace->working_set_size
					&& aspace->working_set_size > aspace->min_working_set) {

					aspace->working_set_size -= WORKING_SET_DECREMENT;
//					dprintf("  new working set size = %d\n", aspace->working_set_size);
				}
			}

			// decide if we need to enter or leave the trimming cycle
			if(!trimming_cycle && vm_page_num_free_pages() < free_memory_low_water)
				trimming_cycle = true;
			else if(trimming_cycle && vm_page_num_free_pages() > free_memory_high_water)
				trimming_cycle = false;

			// scan some pages, trying to free some if needed
			free_memory_target = 0;
			if(trimming_cycle && mapped_size > aspace->working_set_size)
				free_memory_target = mapped_size - aspace->working_set_size;

			scan_pages(aspace, free_memory_target);

			// must hold a ref to the old aspace while we grab the next one,
			// otherwise the iterator becomes out of date.
			old_aspace = aspace;
			aspace = vm_aspace_walk_next(&i);
			vm_put_aspace(old_aspace);
		}
	}
}

int vm_daemon_init()
{
	thread_id tid;

	trimming_cycle = false;

	// calculate the free memory low and high water at which point we enter/leave trimming phase
	free_memory_low_water = vm_page_num_pages() / 8;
	free_memory_high_water = vm_page_num_pages() / 4;

	// create a kernel thread to select pages for pageout
	tid = thread_create_kernel_thread("page daemon", &page_daemon, NULL);
	thread_set_priority(tid, THREAD_MIN_RT_PRIORITY);
	thread_resume_thread(tid);

	return 0;
}

