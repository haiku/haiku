#include "pageManager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Handy function (actually handy for the casting) to add a long to a void *
void *addOffset(void *base,unsigned long offset) {
	return (void *)(((unsigned long)base+offset));
}

pageManager::pageManager(void) {
}

// Since all of the physical pages will need page structures, allocate memory off of the top for them. Set up the lists and semaphores.
void pageManager::setup(void *area,int pages) {
	// Calculate the number of pages that we will need to hold the page structures
	int pageOverhead=((pages*sizeof(page))+(PAGE_SIZE-1))/PAGE_SIZE;
	for (int i=0;i<pages-pageOverhead;i++) {
		page *newPage=(page *)(addOffset(area,i*sizeof(page)));
		newPage->setup(addOffset(area,(i+pageOverhead)*PAGE_SIZE));
		unused.add(newPage);
		}

	cleanLock=create_sem (1,"clean_lock");
	unusedLock=create_sem (1,"unused_lock");
	inUseLock=create_sem (1,"inuse_lock");
	totalPages=pages;
	//error ("pageManager::setup - %d pages ready to rock and roll\n",unused.count());
	}
	
// Try to get a clean page first. If that fails, get a dirty one and clean it. Loop on this.
page *pageManager::getPage(void) {
	page *ret=NULL;
	while (!ret)
		{
		if (clean.count()) {
			acquire_sem(cleanLock);
			ret=(page *)clean.next();
			release_sem(cleanLock);
			} // This could fail if someone swooped in and stole our page.
		if (!ret && unused.count()) {
			acquire_sem(unusedLock);
			ret=(page *)unused.next();
			release_sem(unusedLock);
			if (ret)
				ret->zero();
			} // This could fail if someone swooped in and stole our page.
		}
	//error ("pageManager::getPage - returning page %x, clean = %d, unused = %d, inuse = %x\n",ret,clean.count(),unused.count(),inUse.count());
	acquire_sem(inUseLock);
	inUse.add(ret);
	release_sem(inUseLock);
	ret->count++;
	if (!ret)
		throw ("Out of physical pages!");
	return ret;
	}

// Take page from in use list and put it on the unused list
void pageManager::freePage(page *toFree) {
	//error ("pageManager::freePage; count = %d, address = %p\n",toFree->count,toFree);
	if (atomic_add(&(toFree->count),-1)==1) { // atomic_add returns the *PREVIOUS* value. So we need to check to see if the one we are wasting was the last one.
		acquire_sem(inUseLock);
		inUse.remove(toFree);
//		inUse.dump();
		release_sem(inUseLock);

		acquire_sem(unusedLock);
		unused.add(toFree);
//		unused.dump();
		release_sem(unusedLock);
		}
	}

// Loop forever cleaning any necessary pages
void pageManager::cleaner(void) {
	while (1) {
		snooze(250000);	
		cleanOnePage();
		}
	}

// Find a page that needs cleaning. Take it from the "unused" list, clean it and put it on the clean list.
void pageManager::cleanOnePage(void) {
	if (unused.count()) {
		acquire_sem(unusedLock);
		page *first=(page *)unused.next();   
		release_sem(unusedLock);
		if (first) {
			first->zero();
			acquire_sem(cleanLock);
			clean.add(first);
			release_sem(cleanLock);
			}
		}
	}

// Calculate how desperate we are for physical pages; 1 is not desperate at all, 5 is critical.
int pageManager::desperation(void) { // Formula to determine how desperate system is to get pages back...
	int percentClean=(unused.count()+clean.count())*100/totalPages;
	if (percentClean>30) return 1;
	return (35-percentClean)/7;
	}

void pageManager::dump(void) {
	error ("Dumping the unused list (%d entries)\n",getUnusedCount());
	acquire_sem(unusedLock);
	for (struct node *cur=unused.rock;cur;) {
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
	release_sem(unusedLock);
	error ("Dumping the clean list (%d entries)\n",getCleanCount());
	acquire_sem(cleanLock);
	for (struct node *cur=clean.rock;cur;) {
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
	error ("Dumping the inuse list (%d entries)\n",getInUseCount());
	release_sem(cleanLock);
	acquire_sem(inUseLock);
	for (struct node *cur=inUse.rock;cur;) {
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
	release_sem(inUseLock);
}
