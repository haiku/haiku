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
	for (int i=0;i<pages;i++)
		unused.add(new page(addOffset(area,i*PAGE_SIZE)));
//		unused.add(new page((void *)(i*PAGE_SIZE)));
	cleanLock=create_sem (1,"clean_lock");
	unusedLock=create_sem (1,"unused_lock");
	inUseLock=create_sem (1,"inuse_lock");
	totalPages=pages;
	printf ("pageManager::pageManager: About to dump the clean pages (should be 0):\n\n");
	clean.dump();
	printf ("pageManager::pageManager: About to dump the unused pages (should not be 0):\n\n");
	unused.dump();
	}
	
page *pageManager::getPage(void)
	{
	page *ret=NULL;
	printf ("pageManager::getPage: Checking clean\n");
	printf ("pageManager::getPage:cleanCount = %d\n", clean.nodeCount);
	if (clean.count())
		{
		printf ("pageManager::getPage:locking clean\n");
		acquire_sem(cleanLock);
		printf ("pageManager::getPage:locked clean\n");
		ret=(page *)clean.next();
		printf ("pageManager::getPage:got next clean\n");
		release_sem(cleanLock);
		printf ("pageManager::getPage:unlocked clean\n");
		} // This could fail if someone swoops in and steal our page.
	if (!ret && unused.count())
		{
		printf ("pageManager::getPage:Checking unused\n");
		acquire_sem(unusedLock);
		ret=(page *)unused.next();
		printf ("pageManager::getPage:got next unused\n");
		release_sem(unusedLock);
		printf ("pageManager::getPage:next unused = %x\n",ret);
		if (ret)
			ret->zero();
		} // This could fail if someone swoops in and steal our page.
	if (ret)
		{
		acquire_sem(inUseLock);
		inUse.add(ret);
		release_sem(inUseLock);
		}
	return ret;
	}

void pageManager::freePage(page *toFree)
	{
	acquire_sem(inUseLock);
	inUse.remove(toFree);
	release_sem(inUseLock);
	acquire_sem(unusedLock);
	unused.add(toFree);
	release_sem(unusedLock);
	}

void pageManager::cleaner(void)
	{
	if (unused.count())
		{
		acquire_sem(unusedLock);
		page *first=(page *)unused.next();   
		first->zero();
		acquire_sem(cleanLock);
		clean.add(first);
		release_sem(cleanLock);
		release_sem(unusedLock);
		snooze(250000);	
		}
	}

int pageManager::desperation(void)
	{ // Formula to determine how desperate system is to get pages back...
	int percentClean=(unused.count()+clean.count())/totalPages;
	if (percentClean>30) return 1;
	return (35-percentClean)/5;
	}
