#include "vpage.h"
#include "vnodePool.h"

	extern swapFileManager  swapMan;
	extern pageManager pageMan;  
	extern poolvnode vnodePool;

void vpage::flush(void)
	{
	if (protection==writable && dirty)
		swapMan.write_block(*backingNode,((void *)(physPage->getAddress())), PAGE_SIZE);
	}

void vpage::refresh(void)
	{
		swapMan.read_block(*backingNode,((void *)(physPage->getAddress())), PAGE_SIZE);
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
	//printf ("vpage::vpage: start = %x, vnode.fd=%d, vnode.offset=%d, physMem = %x\n",start,((backing)?backing->fd:0),((backing)?backing->offset:0), ((physMem)?(physMem->getAddress()):0));
	start_address=start;
	end_address=start+PAGE_SIZE-1;
	protection=prot;
	swappable=(state==NO_LOCK);
	dirty=true;

	if (backing)
		{
		backingNode=backing; 
		backing->count++;
		}
	else
		backingNode=&(swapMan.findNode());
	if (!physMem && (state!=LAZY) && (state!=NO_LOCK))
		physPage=pageMan.getPage();
	else
		{
		if (physMem)
			physMem->count++;
		physPage=physMem;
		}
	//printf ("vpage::vpage: ended : start = %x, vnode.fd=%d, vnode.offset=%d, physMem = %x\n",start,((backing)?backing->fd:0),((backing)?backing->offset:0), ((physMem)?(physMem->getAddress()):0));
	}

void vpage::cleanup(void)
	{
	if (physPage) //  Note that free means release one reference
		pageMan.freePage(physPage);
	if (backingNode)
		{
		if (backingNode->fd)
			swapMan.freeVNode(*backingNode);
		vnodePool.put(backingNode);
		}
	}

void vpage::setProtection(protectType prot)
	{
	protection=prot;
	// Change the hardware
	}

bool vpage::fault(void *fault_address, bool writeError) // true = OK, false = panic.
	{ // This is dispatched by the real interrupt handler, who locates us
//	printf ("vpage::fault: virtual address = %x, write = %s\n",(unsigned long) fault_address,((writeError)?"true":"false"));
	if (writeError)
		{
		dirty=true;
		if (physPage)
			{
			if (protection==copyOnWrite) // Else, this was just a "let me know when I am dirty"...
				{
				page *newPhysPage=pageMan.getPage();
				if (!newPhysPage) // No room at the inn
					return false;
				memcpy((void *)(newPhysPage->getAddress()),(void *)(physPage->getAddress()),PAGE_SIZE);
				physPage=newPhysPage;
				protection=writable;
				backingNode=&(swapMan.findNode()); // Need new backing store for this node, since it was copied, the original is no good...
				// Update the architecture specific stuff here...
				}
			return true; 
			}
		}
	physPage=pageMan.getPage();
	if (!physPage) // No room at the inn
		return false;
//	printf ("vpage::fault: New page allocated! new physical address = %x vnode.fd=%d, vnode.offset=%d, \n",physPage->getAddress(),((backingNode)?backingNode->fd:0),((backingNode)?backingNode->offset:0));
	// Update the architecture specific stuff here...
	// This refresh is unneeded if the data was never written out... 
//	dump();
	refresh(); // I wonder if these vnode calls are safe during an interrupt...
//	printf ("vpage::fault: Refreshed\n");
//	dump();
//	printf ("vpage::fault: exiting\n");
	return true;
	}

char vpage::getByte(unsigned long address)
	{
//	printf ("vpage::getByte: address = %d\n",address );
	if (!physPage)
		if (!fault((void *)(address),false))
			throw ("vpage::getByte",address,0);
//	printf ("vpage::getByte: About to return %d\n", *((char *)(address-start_address+physPage->getAddress())));
	return *((char *)(address-start_address+physPage->getAddress()));
	}

void vpage::setByte(unsigned long address,char value)
	{
//	printf ("vpage::setByte: address = %d, value = %d\n",address, value);
	if (!physPage)
		if (!fault((void *)(address),true))
			throw ("vpage::setByte",address,(int)value);
	*((char *)(address-start_address+physPage->getAddress()))=value;
//	printf ("vpage::setByte: physical address = %d, value = %d\n",physPage->getAddress(), *((char *)(physPage->getAddress())));
	}

int  vpage::getInt(unsigned long address)
	{
	if (!physPage)
		if (!fault((void *)(address),false))
			throw ("vpage::getInt",address,0);
	return *((int *)(address-start_address+physPage->getAddress()));
	}

void  vpage::setInt(unsigned long address,int value)
	{
	if (!physPage)
		if (!fault((void *)(address),true))
			throw ("vpage::setInt",address,value);
	*((int *)(address-start_address+physPage->getAddress()))=value;
	}

void vpage::pager(int desperation)
	{
	//printf ("vpage::pager start desperation = %d\n",desperation);
	if (!swappable)
			return;
	printf ("vpage::pager swappable\n");
	switch (desperation)
		{
		case 1: return; break;
		case 2: if (!physPage || protection!=readable)  return;break;
		case 3: if (!physPage || dirty)  return;break;
		case 4: if (!physPage)  return;break;
		case 5: if (!physPage)  return;break;
		default: return;break;
		}
	printf ("vpage::pager flushing\n");
	flush();
	printf ("vpage::pager freeing\n");
	pageMan.freePage(physPage);
	printf ("vpage::pager going to NULL\n");
	physPage=NULL;
	}

void vpage::saver(void)
	{
	if (dirty)
		flush();
	dirty=false;
	}
