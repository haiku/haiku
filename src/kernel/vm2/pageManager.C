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

	totalPages=pages;
	//error ("pageManager::setup - %d pages ready to rock and roll\n",unused.count());
	}
	
// Try to get a clean page first. If that fails, get a dirty one and clean it. Loop on this.
page *pageManager::getPage(void) {
	page *ret=NULL;
	while (!ret)
		{
		if (clean.count()) {
			ret=(page *)clean.next();
			} // This could fail if someone swooped in and stole our page.
		if (!ret && unused.count()) {
			ret=(page *)unused.next();
			if (ret)
				ret->zero();
			} // This could fail if someone swooped in and stole our page.
		}
	//error ("pageManager::getPage - returning page %x, clean = %d, unused = %d, inuse = %x\n",ret,clean.count(),unused.count(),inUse.count());
	inUse.add(ret);
	ret->count++;
	if (!ret)
		throw ("Out of physical pages!");
	return ret;
	}

// Take page from in use list and put it on the unused list
void pageManager::freePage(page *toFree) {
	//error ("pageManager::freePage; count = %d, address = %p\n",toFree->count,toFree);
	if (atomic_add(&(toFree->count),-1)==1) { // atomic_add returns the *PREVIOUS* value. So we need to check to see if the one we are wasting was the last one.
		inUse.remove(toFree);
//		inUse.dump();

		unused.add(toFree);
//		unused.dump();
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
		page *first=(page *)unused.next();   
		if (first) {
			first->zero();
			clean.add(first);
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
	unused.lock();
	for (struct node *cur=unused.rock;cur;) {
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
	unused.unlock();
	error ("Dumping the clean list (%d entries)\n",getCleanCount());
	clean.lock();
	for (struct node *cur=clean.rock;cur;) {
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
	error ("Dumping the inuse list (%d entries)\n",getInUseCount());
	clean.unlock();
	inUse.lock();
	for (struct node *cur=inUse.rock;cur;) {
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
	inUse.unlock();
}
