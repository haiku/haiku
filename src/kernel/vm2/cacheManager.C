#include <new.h>
#include <cacheManager.h>
#include <vpagePool.h>
#include "vmHeaderBlock.h"

ulong vnodeHash (node &vp) {vnode &vn=reinterpret_cast <vnode &>(vp); return vn.offset+vn.fd;}
bool vnodeisEqual (node &vp,node &vp2) {
	vnode &vn=reinterpret_cast <vnode &>(vp);
	vnode &vn2=reinterpret_cast <vnode &>(vp2);
	return vn.fd==vn2.fd && vn.offset==vn2.offset;
	}

extern vmHeaderBlock *vmBlock;
// TODO - we need to (somehow) make sure that the same vnodes here are shared with mmap.
// Maybe a vnode manager...
cacheManager::cacheManager(void) : area (),cacheMembers(30) {
	myLock=create_sem(1,"Cache Manager Semaphore"); 
	cacheMembers.setHash(vnodeHash);
	cacheMembers.setIsEqual(vnodeisEqual);
}

void *cacheManager::findBlock(vnode *target,bool readOnly) {
	cacheMember *candidate=reinterpret_cast<cacheMember *>(cacheMembers.find(target));

	if (!candidate || readOnly || candidate->vp->getProtection()>=writable)
		return candidate;
	
	// At this point, we have the first one in the hahs bucket. Loop over the hash bucket from now on,
	// looking for an equality and writability match...
	for (struct cacheMember *cur=candidate;cur;cur=reinterpret_cast<cacheMember *>(cur->next)) {
		if ((target==cur->vn) && (readOnly || (cur->vp->getProtection()>=writable)))
			return (cur->vp->getStartAddress());
		}
	// We didn't find one, but to get here, there has to be one that is READ ONLY. So let's make a copy
	// of that one, but readable...
	return createBlock(target,false);
	}

void *cacheManager::createBlock(vnode *target,bool readOnly, cacheMember *candidate) {
	bool foundSpot=false;
	vpage *prev=NULL,*cur=NULL;
	unsigned long begin=CACHE_BEGIN;
	// Find a place in the cache's virtual space to put this vnode...
	if (vpages.rock) 
		for (cur=(reinterpret_cast <vpage *>(vpages.rock));!foundSpot && cur;cur=reinterpret_cast <vpage *>(cur->next)) 
			if (cur->getStartAddress()!=(void *)begin) 
				foundSpot=true;
			else { // no joy 
				begin+=PAGE_SIZE;
				prev=cur;
				}
	lock();
	// Create a vnode here
	vpage *newPage = new (vmBlock->vpagePool->get()) vpage;
	newPage->setup(begin,target,NULL,((readOnly)?readable:writable),NO_LOCK);
	vpages.add(newPage); 
	cacheMembers.add(newPage);
	// While this may not seem like a good idea (since this only happens on a write), 
	// it is because someone may only write to part of the file/page...
	if (candidate) 
		memcpy(newPage->getStartAddress(),candidate->vp->getStartAddress(),PAGE_SIZE);
	unlock();
	// return address from this vnode
	return (void *)begin;

}

void *cacheManager::readBlock(vnode *target) {
	void *destination=findBlock(target,true);
	if (destination) return destination;
	return createBlock(target,true);
}

void *cacheManager::writeBlock(vnode *target) {
	void *destination=findBlock(target,false);
	if (destination) return destination;
	return createBlock(target,false);
}
