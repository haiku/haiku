#include "vpage.h"

	extern swapFileManager  swapMan;
	extern pageManager pageMan;  

void vpage::flush(void)
	{
	if (protection==writable && dirty)
		swapMan.write_block(*backingNode,physPage, PAGE_SIZE);
	}

void vpage::refresh(void)
	{
		swapMan.read_block(*backingNode,physPage, PAGE_SIZE);
	}

vpage *vpage::clone(unsigned long address) // The calling method will have to create this...
	{
	return  new vpage(address,NULL,physPage,(protection==readable)?protection:copyOnWrite,LAZY); // Not sure if LAZY is right or not
	}

// backing and/or physMem can be NULL/0.
vpage::vpage(unsigned long start,vnode *backing, page *physMem,protectType prot,pageState state) 
	{ 
	start_address=start;
	end_address=start+PAGE_SIZE-1;
	protection=prot;
	swappable=(state==NO_LOCK);
	if (backing)
		backingNode=backing; 
	else
		backingNode=&(swapMan.findNode());
	if (!physPage && (state!=LAZY) && (state!=NO_LOCK))
		physPage=pageMan.getPage();
	else
		physPage=physMem;
	}

vpage::~vpage(void)
	{
	if (physPage) // I doubt that this is always true. Probably need to check for sharing...
		pageMan.freePage(physPage);
	if (backingNode->fd) 
		swapMan.freeVNode(*backingNode);
	}

void vpage::setProtection(protectType prot)
	{
	protection=prot;
	// Change the hardware
	}

bool vpage::fault(void *fault_address, bool writeError) // true = OK, false = panic.
	{ // This is dispatched by the real interrupt handler, who locates us
	//printf ("vpage::fault: address = %d, write = %s\n",(unsigned long) fault_address,((writeError)?"true":"false"));
	if (writeError)
		{
		dirty=true;
		if (physPage)
			{
			if (protection==copyOnWrite) // Else, this was just a "let me know when I am dirty"...
				{
				page *newPhysPage=pageMan.getPage();
				memcpy(newPhysPage,physPage,PAGE_SIZE);
				physPage=newPhysPage;
				protection=writable;
				backingNode=&(swapMan.findNode()); // Need new backing store for this node, since it was copied, the original is no good...
				// Update the architecture specific stuff here...
				}
			return true; 
			}
		}
	physPage=pageMan.getPage();
	printf ("vpage::fault: New page allocated! address = %x\n",physPage->getAddress());
	// Update the architecture specific stuff here...
	// This refresh is unneeded if the data was never written out... 
	refresh(); // I wonder if these vnode calls are safe during an interrupt...
	//printf ("vpage::fault: Refreshed\n");

	}

char vpage::getByte(unsigned long address)
	{
//	printf ("vpage::getByte: address = %d\n",address );
	if (!physPage)
		fault((void *)(address),false);
//	printf ("vpage::getByte: About to return %d\n", *((char *)(address-start_address+physPage->getAddress())));
	return *((char *)(address-start_address+physPage->getAddress()));
	}

void vpage::setByte(unsigned long address,char value)
	{
//	printf ("vpage::setByte: address = %d, value = %d\n",address, value);
	if (!physPage)
		fault((void *)(address),true);
	*((char *)(address-start_address+physPage->getAddress()))=value;
//	printf ("vpage::setByte: physical address = %d, value = %d\n",physPage->getAddress(), *((char *)(physPage->getAddress())));
	}

int  vpage::getInt(unsigned long address)
	{
	if (!physPage)
		fault((void *)(address),false);
	return *((int *)(address-start_address+physPage->getAddress()));
	}

void  vpage::setInt(unsigned long address,int value)
	{
	if (!physPage)
		fault((void *)(address),true);
	*((int *)(address-start_address+physPage->getAddress()))=value;
	}

void vpage::pager(int desperation)
	{
	if (!swappable)
			return;
	switch (desperation)
		{
		case 1: return; break;
		case 2: if (!physPage || protection!=readable)  return;break;
		case 3: if (!physPage || dirty)  return;break;
		case 4: if (!physPage)  return;break;
		case 5: if (!physPage)  return;break;
		default: return;break;
		}
	flush();
	pageMan.freePage(physPage);
	physPage=NULL;
	}

void vpage::saver(void)
	{
	if (dirty)
		flush();
	dirty=false;
	}
