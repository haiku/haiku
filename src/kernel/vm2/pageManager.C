#include "pageManager.h"
#include <stdio.h>
#include <stdlib.h>

void *addOffset(void *base,unsigned long offset)
{
	return (void *)(((unsigned long)base+offset));
}

pageManager::pageManager(int pages)
	{
// This is compatability for in BeOS usage only...
	void *area;
	if (0>=create_area("vm_test",&area,B_ANY_ADDRESS,B_PAGE_SIZE*pages,B_NO_LOCK,B_READ_AREA|B_WRITE_AREA))
		{
		printf ("pageManager::pageManager: No memory!\n");
		exit(1);
		}
	printf ("Allocated an area. Address = %x\n",area);
	// Calculate the number of pages that we will need to hold the page structures
	int pageOverhead=((pages*sizeof(page))+(PAGE_SIZE-1))/PAGE_SIZE;
	for (int i=0;i<pages-pageOverhead;i++)
		{
		page *newPage=(page *)(addOffset(area,i*sizeof(page)));
		newPage->setup(addOffset(area,(i+pageOverhead)*PAGE_SIZE));
		printf ("newPage = %x, setup = %x\n",newPage,addOffset(area,(i+pageOverhead)*PAGE_SIZE));
		printf ("i = %d, sizeof(page) = %x, pageOverhead = %d\n",i,sizeof(page),pageOverhead);
		newPage->dump();
		unused.add(addOffset(area,i*sizeof(page)));
		}

	cleanLock=create_sem (1,"clean_lock");
	unusedLock=create_sem (1,"unused_lock");
	inUseLock=create_sem (1,"inuse_lock");
	totalPages=pages;
	/*
	printf ("pageManager::pageManager: About to dump the clean pages (should be 0):\n\n");
	clean.dump();
	printf ("pageManager::pageManager: About to dump the unused pages (should not be 0):\n\n");
	unused.dump();
	*/
	}
	
page *pageManager::getPage(void)
	{
	page *ret=NULL;
//	printf ("pageManager::getPage: Checking clean\n");
	//printf ("pageManager::getPage:cleanCount = %d\n", clean.nodeCount);
	if (clean.count())
		{
		//printf ("pageManager::getPage:locking clean\n");
		acquire_sem(cleanLock);
		//printf ("pageManager::getPage:locked clean\n");
		ret=(page *)clean.next();
		//printf ("pageManager::getPage:got next clean\n");
		release_sem(cleanLock);
		//printf ("pageManager::getPage:unlocked clean\n");
		} // This could fail if someone swooped in and stole our page.
	else if (unused.count())
		{
		//printf ("pageManager::getPage:Checking unused\n");
		acquire_sem(unusedLock);
		ret=(page *)unused.next();
		//printf ("pageManager::getPage:got next unused\n");
		release_sem(unusedLock);
		//printf ("pageManager::getPage:next unused = %x\n",ret);
		if (ret)
			ret->zero();
		} // This could fail if someone swooped in and stole our page.
	if (ret)
		{
		acquire_sem(inUseLock);
		inUse.add(ret);
		release_sem(inUseLock);
		ret->count++;
		}
//	printf ("pageManager::getPage:leaving with page = %x\n", ret->getAddress());
	return ret;
	}

void pageManager::freePage(page *toFree)
	{
//	printf ("Inside freePage; old value = %d",toFree->count);
	toFree->count--;
//	printf (" new value = %d, page = %x\n",toFree->count,toFree->getAddress());
	if (toFree->count==0)
		{
		acquire_sem(inUseLock);
		inUse.remove(toFree);
		release_sem(inUseLock);
		acquire_sem(unusedLock);
		unused.add(toFree);
		release_sem(unusedLock);
		}
	}

void pageManager::cleaner(void)
	{
	while (1)
		{
		if (unused.count())
			{
			//printf ("pageManager::cleaner: About to vacuum a page\n");
			acquire_sem(unusedLock);
			page *first=(page *)unused.next();   
			first->zero();
			acquire_sem(cleanLock);
			clean.add(first);
			release_sem(cleanLock);
			release_sem(unusedLock);
			//printf ("pageManager::cleaner: All done with vacuum a page\n");
			snooze(125000);	
			}
		}
	}

int pageManager::desperation(void)
	{ // Formula to determine how desperate system is to get pages back...
	int percentClean=(unused.count()+clean.count())*100/totalPages;
	if (percentClean>30) return 1;
	return (35-percentClean)/7;
	}

void pageManager::dump(void)
{
	printf ("Dumping the unused list\n");
	for (struct node *cur=unused.rock;cur;)
		{
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
	printf ("Dumping the clean list\n");
	for (struct node *cur=clean.rock;cur;)
		{
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
	printf ("Dumping the inuse list\n");
	for (struct node *cur=inUse.rock;cur;)
		{
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
}
