#include "vpage.h"
#include "vnodePool.h"
#include "vmHeaderBlock.h"
#include "areaManager.h"
#include "vnodeManager.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

extern vmHeaderBlock *vmBlock;

// Write this vpage out if necessary
void vpage::flush(void) {
	if (physPage && protection==writable && dirty) {
		error ("vpage::write_block: writing, backingNode->fd = %d, backingNode->offset = %d, address = %x\n",backingNode->fd, backingNode->offset,physPage->getAddress());
		if (-1==lseek(backingNode->fd,backingNode->offset,SEEK_SET))
			error ("vpage::flush:seek failed, fd = %d, errno = %d, %s\n",backingNode->fd,errno,strerror(errno));
		if (-1==write(backingNode->fd,(void *)(physPage->getAddress()),PAGE_SIZE))
			error ("vpage::flush: failed address =%x, fd = %d, offset = %d, errno = %d, %s\n",
							start_address,backingNode->fd, backingNode->offset, errno,strerror(errno));
		backingNode->valid=true;
		//error ("vpage::write_block: done, backingNode->fd = %d, backingNode->offset = %d, address = %x\n",backingNode->fd, backingNode->offset,loc);
		}
	}

// Load this vpage in if necessary
void vpage::refresh(void) {
	error ("vpage::refresh: reading into %x\n",physPage->getAddress());
	backingNode->dump();
	if (backingNode->valid==false)
		return; // Do nothing. This prevents "garbage" data on disk from being read in...	
	if (-1==lseek(backingNode->fd,backingNode->offset,SEEK_SET))
		error ("vpage::refresh: seek failed, fd = %d, errno = %d, %s\n",backingNode->fd,errno,strerror(errno));
	if (-1==read(backingNode->fd,(void *)(physPage->getAddress()),PAGE_SIZE))
		error ("vpage::refresh: failed, fd = %d, errno = %d, %s\n",backingNode->fd,errno,strerror(errno));
	}

// Simple, empty constructor
vpage::vpage(void) : physPage(NULL),backingNode(NULL),protection(none),dirty(false),swappable(false),start_address(0),end_address(0), locked(false) 
{
}

// Does the real setup work for making a vpage.
// backing and/or physMem can be NULL/0.
void vpage::setup(unsigned long start,vnode *backing, page *physMem,protectType prot,pageState state, mmapSharing share) { 
	// Basic setup from parameters
	vpage *clonedPage; // This is the page that this page is to be the clone of...
	error ("vpage::setup: start = %x, vnode.fd=%d, vnode.offset=%d, physMem = %x\n",start,((backing)?backing->fd:0),((backing)?backing->offset:0), ((physMem)?(physMem->getAddress()):0));
	physPage=physMem;
	backingNode=backing; 
	protection=prot;
	dirty=false;
	swappable=(state==NO_LOCK);
	start_address=start;
	end_address=start+PAGE_SIZE-1;

// Set up the backing store. If one is specified, use it; if not, get a swap file page.
	if (backingNode) { // This is an mmapped file (or a cloned area)
		switch (share) {
			case CLONE: // This is a cloned area
			case SHARED: // This is a shared mmap
				clonedPage=vmBlock->vnodeMan->addVnode(*backingNode,*this,&backingNode); // Use the reference version which will make a new one if this one is not found
				if (clonedPage) physPage=clonedPage->physPage;	
				break;
			case PRIVATE: // This is a one way share - we get others changes (until we make a change) but no one gets our changes
				clonedPage=vmBlock->vnodeMan->addVnode(*backingNode,*this,&backingNode); // Use the reference version which will make a new one if this one is not found
				if (clonedPage) physPage=clonedPage->physPage;	
				protection=(protection<=readable)?protection: copyOnWrite;
				break;
			case COPY: // This is not shared - get a fresh page and fresh swap file space and copy the original page
				physPage=vmBlock->pageMan->getPage();
				clonedPage=vmBlock->vnodeMan->findVnode(*backing); // Find out if the page we are copying is in memory already...
				if (clonedPage && clonedPage->physPage) // If it is in memory, copy its values
					memcpy((void *)(physPage->getAddress()),(void *)(clonedPage->physPage->getAddress()),PAGE_SIZE);
				else 
					refresh(); // otherwise, get a copy from disk...
				backingNode=&(vmBlock->swapMan->findNode()); // Now get swap space (since we don't want to be backed by the file...
				clonedPage=vmBlock->vnodeMan->addVnode(backingNode,*this); // Add this vnode to the vnode keeper
				break;
			}
		}
	else { // Going to swap file.
		backingNode=&(vmBlock->swapMan->findNode());
		clonedPage=vmBlock->vnodeMan->addVnode(backingNode,*this); // Use the pointer version which will use this one. Should always return NULL
	}
// If there is no physical page already and we can't wait to get one, then get one now	
	if (!physPage && (state!=LAZY) && (state!=NO_LOCK)) {
		physPage=vmBlock->pageMan->getPage();
		//error ("vpage::setup, state = %d, allocated page %x\n",state,physPage);
		}
	else { // We either don't need it or we already have it. 
		if (physPage)
			atomic_add(&(physPage->count),1);
		}

	error ("vpage::setup: ended : start = %x, vnode.fd=%d, vnode.offset=%d, physMem = %x\n",start,((backing)?backing->fd:0),((backing)?backing->offset:0), ((physMem)?(physMem->getAddress()):0));
	}

// Destruction. 
void vpage::cleanup(void) {
	if (physPage) { //  Note that free means release one reference 
		//error ("vpage::cleanup, freeing physcal page %x\n",physPage);
		vmBlock->pageMan->freePage(physPage); // This does nothing if someone else is using the physical page
		}
	if (backingNode) { // If no one else is using this vnode, wipe it out
		if (vmBlock->vnodeMan->remove(*backingNode,*this))			
			if (backingNode->fd  && (backingNode->fd==vmBlock->swapMan->getFD()))
				vmBlock->swapMan->freeVNode(*backingNode);
			else
				vmBlock->vnodePool->put(backingNode);
		}
	}

// Change this pages protection
void vpage::setProtection(protectType prot) {
	protection=prot;
	// Change the hardware
	}

// This is dispatched by the real interrupt handler, who locates us
// true = OK, false = panic.  
bool vpage::fault(void *fault_address, bool writeError, int &in_count) { 
	error ("vpage::fault: virtual address = %lx, write = %s\n",(unsigned long) fault_address,((writeError)?"true":"false"));
	if (writeError && protection != copyOnWrite && protection != writable)
		return false;
	if (writeError && physPage) { // If we already have a page and this is a write, it is either a copy on write or a "dirty" notice
		dirty=true;
		if (protection==copyOnWrite) { // Else, this was just a "let me know when I am dirty"...  
			page *newPhysPage=vmBlock->pageMan->getPage();
			error ("vpage::fault - copy on write allocated page %x\n",newPhysPage);
			memcpy((void *)(newPhysPage->getAddress()),(void *)(physPage->getAddress()),PAGE_SIZE);
			physPage=newPhysPage;
			protection=writable;
			vmBlock->vnodeMan->remove(*backingNode,*this);
			backingNode=&(vmBlock->swapMan->findNode()); // Need new backing store for this node, since it was copied, the original is no good...
			vmBlock->vnodeMan->addVnode(backingNode,*this);
			// Update the architecture specific stuff here...
			}
		return true; 
		}
	// Guess this is the real deal. Get a physical page.
	physPage=vmBlock->pageMan->getPage();
	error ("vpage::fault - regular - allocated page %x\n",physPage);
	if (!physPage) // No room at the inn
		return false;
	error ("vpage::fault: New page allocated! new physical address = %x vnode.fd=%d, vnode.offset=%d, \n",physPage->getAddress(),((backingNode)?backingNode->fd:0),((backingNode)?backingNode->offset:0));
	// Update the architecture specific stuff here...
	// This refresh is unneeded if the data was never written out... 
	dump();
	refresh(); // I wonder if these vnode calls are safe during an interrupt...
	dirty=writeError; // If the client is writing, we are now dirty (or will be when we get back to user land)
	in_count++;
	//error ("vpage::fault: Refreshed\n");
	dump();
	//error ("vpage::fault: exiting\n");
	return true;
	}

bool vpage::lock(long flags) {
	locked=true;
	if (!physPage) {
		physPage=vmBlock->pageMan->getPage();
		if (!physPage)
			return false;
		refresh();
	}
	return true;
}

void vpage::unlock(long flags) {
	if ((flags & B_DMA_IO) || (!(flags & B_READ_DEVICE)))
		dirty=true;
	locked=false;
}

char vpage::getByte(unsigned long address,areaManager *manager) {
//	error ("vpage::getByte: address = %ld\n",address );
	if (!physPage)
		if (!manager->fault((void *)(address),false))
			throw ("vpage::getByte");
//	error ("vpage::getByte: About to return %d\n", *((char *)(address-start_address+physPage->getAddress())));
	return *((char *)(address-start_address+physPage->getAddress()));
	}

void vpage::setByte(unsigned long address,char value,areaManager *manager) {
//	error ("vpage::setByte: address = %d, value = %d\n",address, value);
	if (!physPage)
		if (!manager->fault((void *)(address),true))
			throw ("vpage::setByte");
	*((char *)(address-start_address+physPage->getAddress()))=value;
//	error ("vpage::setByte: physical address = %d, value = %d\n",physPage->getAddress(), *((char *)(physPage->getAddress())));
	}

int  vpage::getInt(unsigned long address,areaManager *manager) {
	error ("vpage::getInt: address = %ld\n",address );
	if (!physPage)
		if (!manager->fault((void *)(address),false))
			throw ("vpage::getInt");
	//error ("vpage::getInt: About to return %d\n", *((char *)(address-start_address+physPage->getAddress())));
	dump();
	return *((int *)(address-start_address+physPage->getAddress()));
	}

void  vpage::setInt(unsigned long address,int value,areaManager *manager) {
	if (!physPage)
		if (!manager->fault((void *)(address),true))
			throw ("vpage::setInt");
	*((int *)(address-start_address+physPage->getAddress()))=value;
	}

// Swaps pages out where necessary.
bool vpage::pager(int desperation) {
	//error ("vpage::pager start desperation = %d\n",desperation);
	if (!swappable)
			return false;
	//error ("vpage::pager swappable\n");
	switch (desperation) {
		case 1: return false; break;
		case 2: if (!physPage || protection!=readable || locked)  return false;break;
		case 3: if (!physPage || dirty || locked)  return false;break;
		case 4: if (!physPage || locked)  return false;break;
		case 5: if (!physPage || locked)  return false;break;
		default: return false;break;
		}
	//error ("vpage::pager flushing\n");
	flush();
	//error ("vpage::pager freeing\n");
	vmBlock->pageMan->freePage(physPage);
	//error ("vpage::pager going to NULL\n");
	physPage=NULL;
	return true;
	}

// Saves dirty pages
void vpage::saver(void) {
	if (dirty) {
		flush();
		dirty=false;
		}
	}
