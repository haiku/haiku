#include <new.h>
#include <cacheManager.h>
#include "vmHeaderBlock.h"

// functions for hash and isEqual. No surprises
ulong vnodeHash (node &vp) {vnode &vn=reinterpret_cast <vnode &>(vp); return vn.offset+vn.fd;}
bool vnodeisEqual (node &vp,node &vp2) {
	vnode &vn=reinterpret_cast <vnode &>(vp);
	vnode &vn2=reinterpret_cast <vnode &>(vp2);
	return vn.fd==vn2.fd && vn.offset==vn2.offset;
	}

extern vmHeaderBlock *vmBlock;

cacheManager::cacheManager(void) : area (),cacheMembers(30) {
	myLock=create_sem(1,"Cache Manager Semaphore"); 
	cacheMembers.setHash(vnodeHash);
	cacheMembers.setIsEqual(vnodeisEqual);
}

// Given a vnode and protection level, see if we have it in cache already
void *cacheManager::findBlock(vnode *target,bool readOnly) {
	cacheMember *candidate=reinterpret_cast<cacheMember *>(cacheMembers.find(target));

	if (!candidate || readOnly || candidate->vp->getProtection()>=writable)
		return candidate;
	
	// At this point, we have the first one in the hash bucket. Loop over the hash bucket from now on,
	// looking for an equality and writability match...
	for (struct cacheMember *cur=candidate;cur;cur=reinterpret_cast<cacheMember *>(cur->next)) {
		if ((target==cur->vn) && (readOnly || (cur->vp->getProtection()>=writable)))
			return (cur->vp->getStartAddress());
		}
	// We didn't find one, but to get here, there has to be one that is READ ONLY. So let's make a copy
	// of that one, but readable...
	return createBlock(target,false);
	}

// No cache hit found; have to make a new one. Find a virtual page, create a vnode, and map.
void *cacheManager::createBlock(vnode *target,bool readOnly, cacheMember *candidate) {
	bool foundSpot=false;
	vpage *cur=NULL;
	unsigned long begin=CACHE_BEGIN;

	lock();
	// Find a place in the cache's virtual space to put this vnode...
	// This *HAS* to succeed, because the address space should be larger than physical memory...
	for (int i=0;!foundSpot && i<pageCount;i++) {
		cur=getNthVpage(i);
		foundSpot=!cur->getPhysPage();
		}
		
	// Create a vnode here
	cur->setup((unsigned long)(cur->getStartAddress()),target,NULL,((readOnly)?readable:writable),NO_LOCK);
	cacheMembers.add(cur);
	// While this may not seem like a good idea (since this only happens on a write), 
	// it is because someone may only write to part of the file/page...
	if (candidate) 
		memcpy(cur->getStartAddress(),candidate->vp->getStartAddress(),PAGE_SIZE);
	unlock();
	// return address from this vnode
	return cur->getStartAddress();
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
