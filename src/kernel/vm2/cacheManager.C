#include <cacheManager.h>

cacheManager::cacheManager(void) : area (NULL)
{
	myLock=create_sem(1,"Area Manager Semaphore"); // Should have team name in it.
}

void *cacheManager::findBlock(vnode *target,bool readOnly)
	{
	if (!cacheMembers.rock)
		return NULL;
	for (struct cacheMember *cur=((cacheMember *)cacheMembers.rock);cur;cur=((cacheMember *)cur->next))
		{
		if ((target==cur->vn) && (readOnly || (cur->vp->getProtection()>=writable)))
			return (cur->vp->getStartAddress());
		}
	return NULL;
	}

void *cacheManager::createBlock(vnode *target,bool readOnly)
{
	bool foundSpot=false;
	vpage *prev=NULL,*cur=NULL;
	unsigned long begin=CACHE_BEGIN;
	if (vpages.rock)
		for (cur=((vpage *)(vpages.rock));!foundSpot && cur;cur=(vpage *)(cur->next))
			if (cur->getStartAddress()!=(void *)begin)
				foundSpot=true;
			else // no joy
				{
				begin+=PAGE_SIZE;
				prev=cur;
				}
	lock();
	// Create a vnode here
	vpage *newPage = new vpage(begin,target,NULL,((readOnly)?readable:writable),NO_LOCK);
	vpages.add(newPage); 
	cacheMembers.add(newPage);
	unlock();
	// return address from this vnode
	return (void *)begin;

}

void *cacheManager::readBlock(vnode *target)
{
	void *destination=findBlock(target,true);
	if (destination) return destination;
	return createBlock(target,true);
}

void *cacheManager::writeBlock(vnode *target)
{
	void *destination=findBlock(target,false);
	if (destination) return destination;
	return createBlock(target,false);
}
