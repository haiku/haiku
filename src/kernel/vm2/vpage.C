#include "vpage.h"
#include "vnodePool.h"
#include "vmHeaderBlock.h"
#include "areaManager.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

extern vmHeaderBlock *vmBlock;

void vpage::flush(void)
	{
	if (protection==writable && dirty)
		{
		//error (vpage::write_block: writing, backingNode->fd = %d, backingNode->offset = %d, address = %x\n",backingNode->fd, backingNode->offset,loc);
		if (-1==lseek(backingNode->fd,backingNode->offset,SEEK_SET))
			error ("vpage::flush:seek failed, fd = %d, errno = %d, %s\n",backingNode->fd,errno,strerror(errno));
		if (-1==write(backingNode->fd,(void *)start_address,PAGE_SIZE))
			error ("vpage::flush: failed, fd = %d, errno = %d, %s\n",backingNode->fd,errno,strerror(errno));
		backingNode->valid=true;
		//error ("vpage::write_block: done, backingNode->fd = %d, backingNode->offset = %d, address = %x\n",backingNode->fd, backingNode->offset,loc);
		}
	}

void vpage::refresh(void)
	{
	if (backingNode->valid==false)
		return; // Do nothing. This prevents "garbage" data on disk from being read in...	
	//error ("vpage::refresh: reading, backingNode->fd = %d, backingNode->offset = %d into %x\n",backingNode->fd, backingNode->offset,loc);
	if (-1==lseek(backingNode->fd,backingNode->offset,SEEK_SET))
		error ("vpage::refresh: seek failed, fd = %d, errno = %d, %s\n",backingNode->fd,errno,strerror(errno));
	if (-1==read(backingNode->fd,(void *)start_address,PAGE_SIZE))
		error ("vpage::refresh: failed, fd = %d, errno = %d, %s\n",backingNode->fd,errno,strerror(errno));
	}

vpage::vpage(void)
{
	physPage=NULL;
	backingNode=NULL;
	protection=none;
	dirty=false;
	swappable=false;
	start_address=end_address=0;
}

// backing and/or physMem can be NULL/0.
void vpage::setup(unsigned long start,vnode *backing, page *physMem,protectType prot,pageState state) 
	{ 
	//error ("vpage::vpage: start = %x, vnode.fd=%d, vnode.offset=%d, physMem = %x\n",start,((backing)?backing->fd:0),((backing)?backing->offset:0), ((physMem)?(physMem->getAddress()):0));
	start_address=start;
	end_address=start+PAGE_SIZE-1;
	protection=prot;
	swappable=(state==NO_LOCK);

	if (backing)
		{
		backingNode=backing; 
		atomic_add(&(backing->count),1);
		}
	else
		backingNode=&(vmBlock->swapMan->findNode());
	if (!physMem && (state!=LAZY) && (state!=NO_LOCK))
		physPage=vmBlock->pageMan->getPage();
	else
		{
		if (physMem)
			atomic_add(&(physMem->count),1);
		physPage=physMem;
		}
	dirty=false;
	//error ("vpage::vpage: ended : start = %x, vnode.fd=%d, vnode.offset=%d, physMem = %x\n",start,((backing)?backing->fd:0),((backing)?backing->offset:0), ((physMem)?(physMem->getAddress()):0));
	}

void vpage::cleanup(void)
	{
	if (physPage) //  Note that free means release one reference
		vmBlock->pageMan->freePage(physPage);
	if (backingNode)
		{
		if (backingNode->fd)
			if (backingNode->fd==vmBlock->swapMan->getFD())
				vmBlock->swapMan->freeVNode(*backingNode);
			else
				{
				if ( atomic_add(&(backingNode->count),-1)==1)
					vmBlock->vnodePool->put(backingNode);
				}
		}
	}

void vpage::setProtection(protectType prot)
	{
	protection=prot;
	// Change the hardware
	}

bool vpage::fault(void *fault_address, bool writeError) // true = OK, false = panic.
	{ // This is dispatched by the real interrupt handler, who locates us
	error ("vpage::fault: virtual address = %lx, write = %s\n",(unsigned long) fault_address,((writeError)?"true":"false"));
	if (writeError && physPage)
		{
		dirty=true;
		if (protection==copyOnWrite) // Else, this was just a "let me know when I am dirty"...
			{
			page *newPhysPage=vmBlock->pageMan->getPage();
			if (!newPhysPage) // No room at the inn
				return false;
			memcpy((void *)(newPhysPage->getAddress()),(void *)(physPage->getAddress()),PAGE_SIZE);
			physPage=newPhysPage;
			protection=writable;
			backingNode=&(vmBlock->swapMan->findNode()); // Need new backing store for this node, since it was copied, the original is no good...
			// Update the architecture specific stuff here...
			}
		return true; 
		}
	physPage=vmBlock->pageMan->getPage();
	if (!physPage) // No room at the inn
		return false;
	error ("vpage::fault: New page allocated! new physical address = %x vnode.fd=%d, vnode.offset=%d, \n",physPage->getAddress(),((backingNode)?backingNode->fd:0),((backingNode)?backingNode->offset:0));
	// Update the architecture specific stuff here...
	// This refresh is unneeded if the data was never written out... 
	dump();
	refresh(); // I wonder if these vnode calls are safe during an interrupt...
	dirty=false;
	error ("vpage::fault: Refreshed\n");
	dump();
	error ("vpage::fault: exiting\n");
	return true;
	}

char vpage::getByte(unsigned long address,areaManager *manager)
	{
	//error ("vpage::getByte: address = %ld\n",address );
	if (!physPage)
		if (!manager->fault((void *)(address),false))
			throw ("vpage::getByte");
	//error ("vpage::getByte: About to return %d\n", *((char *)(address-start_address+physPage->getAddress())));
	return *((char *)(address-start_address+physPage->getAddress()));
	}

void vpage::setByte(unsigned long address,char value,areaManager *manager)
	{
//	error ("vpage::setByte: address = %d, value = %d\n",address, value);
	if (!physPage)
		if (!manager->fault((void *)(address),true))
			throw ("vpage::setByte");
	*((char *)(address-start_address+physPage->getAddress()))=value;
//	error ("vpage::setByte: physical address = %d, value = %d\n",physPage->getAddress(), *((char *)(physPage->getAddress())));
	}

int  vpage::getInt(unsigned long address,areaManager *manager)
	{
	error ("vpage::getInt: address = %ld\n",address );
	if (!physPage)
		if (!manager->fault((void *)(address),false))
			throw ("vpage::getInt");
	error ("vpage::getInt: About to return %d\n", *((char *)(address-start_address+physPage->getAddress())));
	dump();
	return *((int *)(address-start_address+physPage->getAddress()));
	}

void  vpage::setInt(unsigned long address,int value,areaManager *manager)
	{
	if (!physPage)
		if (!manager->fault((void *)(address),true))
			throw ("vpage::setInt");
	*((int *)(address-start_address+physPage->getAddress()))=value;
	}

void vpage::pager(int desperation)
	{
	//error ("vpage::pager start desperation = %d\n",desperation);
	if (!swappable)
			return;
	error ("vpage::pager swappable\n");
	switch (desperation)
		{
		case 1: return; break;
		case 2: if (!physPage || protection!=readable)  return;break;
		case 3: if (!physPage || dirty)  return;break;
		case 4: if (!physPage)  return;break;
		case 5: if (!physPage)  return;break;
		default: return;break;
		}
	error ("vpage::pager flushing\n");
	flush();
	error ("vpage::pager freeing\n");
	vmBlock->pageMan->freePage(physPage);
	error ("vpage::pager going to NULL\n");
	physPage=NULL;
	}

void vpage::saver(void)
	{
	if (dirty)
		flush();
	dirty=false;
	}
