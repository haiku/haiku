#include "pageManager.h"
#include <stdio.h>
#include <stdlib.h>

void *addOffset(void *base,unsigned long offset)
{
	return (void *)(((unsigned long)base+offset));
}

pageManager::pageManager(void)
{
}

void pageManager::setup(void *area,int pages)
	{
	// Calculate the number of pages that we will need to hold the page structures
	int pageOverhead=((pages*sizeof(page))+(PAGE_SIZE-1))/PAGE_SIZE;
	for (int i=0;i<pages-pageOverhead;i++)
		{
		page *newPage=(page *)(addOffset(area,i*sizeof(page)));
		newPage->setup(addOffset(area,(i+pageOverhead)*PAGE_SIZE));
		//error ("newPage = %x, setup = %x\n",newPage,addOffset(area,(i+pageOverhead)*PAGE_SIZE));
		//error ("i = %d, sizeof(page) = %x, pageOverhead = %d\n",i,sizeof(page),pageOverhead);
		//newPage->dump();
		unused.add(newPage);
		}

	cleanLock=create_sem (1,"clean_lock");
	unusedLock=create_sem (1,"unused_lock");
	inUseLock=create_sem (1,"inuse_lock");
	totalPages=pages;
	/*
	error ("pageManager::pageManager: About to dump the clean pages (should be 0):\n\n");
	clean.dump();
	error ("pageManager::pageManager: About to dump the unused pages (should not be 0):\n\n");
	unused.dump();
	*/
	}
	
page *pageManager::getPage(void)
	{
	page *ret=NULL;
//	error ("pageManager::getPage: Checking clean\n");
	//error ("pageManager::getPage:cleanCount = %d\n", clean.nodeCount);
	while (!ret)
		{
		if (clean.count())
			{
			//error ("pageManager::getPage:locking clean\n");
			acquire_sem(cleanLock);
			//error ("pageManager::getPage:locked clean\n");
			ret=(page *)clean.next();
			//error ("pageManager::getPage:got next clean\n");
			release_sem(cleanLock);
			//error ("pageManager::getPage:unlocked clean\n");
			} // This could fail if someone swooped in and stole our page.
		else if (unused.count())
			{
			//error ("pageManager::getPage:Checking unused\n");
			acquire_sem(unusedLock);
			ret=(page *)unused.next();
			//error ("pageManager::getPage:got next unused\n");
			release_sem(unusedLock);
			//error ("pageManager::getPage:next unused = %x\n",ret);
			if (ret)
				ret->zero();
			} // This could fail if someone swooped in and stole our page.
		}
	acquire_sem(inUseLock);
	inUse.add(ret);
	release_sem(inUseLock);
	ret->count++;
//	error ("pageManager::getPage:leaving with page = %x\n", ret->getAddress());
	return ret;
	}

void pageManager::freePage(page *toFree)
	{
	error ("Inside freePage; old value = %d",toFree->count);
	if (atomic_add(&(toFree->count),-1)==1) // atomic_add returns the *PREVIOUS* value. So we need to check to see if the one we are wasting was the last one.
		{
		acquire_sem(inUseLock);
		inUse.remove(toFree);
		release_sem(inUseLock);
		acquire_sem(unusedLock);
		unused.add(toFree);
		release_sem(unusedLock);
		}
	error (" new value = %d, page = %x\n",toFree->count,toFree->getAddress());
	}

void pageManager::cleaner(void)
	{
	while (1)
		{
		if (unused.count())
			{
			//error ("pageManager::cleaner: About to vacuum a page\n");
			acquire_sem(unusedLock);
			page *first=(page *)unused.next();   
			release_sem(unusedLock);
			first->zero();
			acquire_sem(cleanLock);
			clean.add(first);
			release_sem(cleanLock);
			//error ("pageManager::cleaner: All done with vacuum a page\n");
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
	error ("Dumping the unused list\n");
	for (struct node *cur=unused.rock;cur;)
		{
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
	error ("Dumping the clean list\n");
	for (struct node *cur=clean.rock;cur;)
		{
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
	error ("Dumping the inuse list\n");
	for (struct node *cur=inUse.rock;cur;)
		{
		page *thisPage=(page *)cur;
		thisPage->dump();
		cur=cur->next;
		}
}
