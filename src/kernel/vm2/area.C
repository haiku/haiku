#include <new.h>
#include "area.h"
#include "areaManager.h"
#include "vpage.h"
#include "vnodePool.h"
#include "vpagePool.h"

#include "vmHeaderBlock.h"

extern vmHeaderBlock *vmBlock;

area::area(void)
	{
	}

void area::setup (areaManager *myManager)
	{
	printf ("area::setup setting up new area\n");
	manager=myManager;
	}

unsigned long area::mapAddressSpecToAddress(addressSpec type,unsigned long requested,int pageCount)
{
	// We will lock in the callers
	unsigned long base;
	switch (type)
		{
		case EXACT: 
			base=manager->getNextAddress(pageCount,requested); 
			if (base!=requested)
				return B_ERROR;
			break;
		case BASE: 
			base=manager->getNextAddress(pageCount,requested); 
			break;
		case ANY: 
			base=manager->getNextAddress(pageCount,USER_BASE); 
			break;
		case ANY_KERNEL: 
			base=manager->getNextAddress(pageCount,KERNEL_BASE); 
			break;
		case CLONE: base=0;break; // Not sure what to do...
		}
//	printf ("area::mapAddressSpecToAddress, in type: %s, address = %x, size = %d\n", ((type==EXACT)?"Exact":(type==BASE)?"BASE":(type==ANY)?"ANY":(type==CLONE)?"CLONE":"ANY_KERNEL"), requested,pageCount);
	return base;
}

status_t area::createAreaMappingFile(char *inName, int pageCount,void **address, addressSpec type,pageState inState,protectType protect,int fd,size_t offset)
	{
	strcpy(name,inName);
	unsigned long requested=(unsigned long)(*address); // Hold onto this to make sure that EXACT works...
	vpage *newPage;

	manager->lock();
	unsigned long base=mapAddressSpecToAddress(type,requested,pageCount);
	for (int i=0;i<pageCount;i++)
		{
		vnode *newVnode=new (vmBlock->vnodePool->get()) vnode;
		newVnode->fd=fd;
		newVnode->offset=offset+PAGE_SIZE*i;
		newVnode->valid=true;
		newPage=new (vmBlock->vpagePool->get()) vpage;
		newPage->setup(base+PAGE_SIZE*i,newVnode,NULL,protect,inState);
		vpages.add(newPage);
//		printf ("New vnode with fd %d, offset = %d\n",fd,newVnode->offset);
		}
	manager->unlock();

	state=inState;
	start_address=base;
	end_address=base+pageCount*PAGE_SIZE;
	*address=(void *)base;
	finalWrite=true;
	}

status_t area::createArea(char *inName, int pageCount,void **address, addressSpec type,pageState inState,protectType protect)
	{
	vpage *newPage;

	strcpy(name,inName);
	state=inState;
	finalWrite=false;
	unsigned long requested=(unsigned long)(*address); // Hold onto this to make sure that EXACT works...

	manager->lock();
	printf ("area::createArea: Locked in createArea\n");
	unsigned long base=mapAddressSpecToAddress(type,requested,pageCount);
	printf ("area::createArea: base address = %d\n",base);
	for (int i=0;i<pageCount;i++)
		{
		printf ("in area::createArea: creating page = %d\n",i);
		newPage=new (vmBlock->vpagePool->get()) vpage;
		newPage->setup(base+PAGE_SIZE*i,NULL,NULL,protect,inState);
		vpages.add(newPage);
		}
	manager->unlock();
	start_address=base;
	end_address=base+pageCount*PAGE_SIZE;
	*address=(void *)base;
	printf ("area::createArea: unlocked in createArea\n");
	}

status_t area::cloneArea(area *origArea, char *inName, void **address, addressSpec type,pageState inState,protectType protect)
	{
//  printf ("area::cloneArea: entered\n");
	strcpy(name,inName);
	int pageCount = origArea->getPageCount();
	manager->lock();
//	printf ("area::cloneArea: locked\n");
	unsigned long requested=(unsigned long)(*address); // Hold onto this to make sure that EXACT works...
	unsigned long base=mapAddressSpecToAddress(type,requested,pageCount);
	start_address=base;
//  printf ("area::cloneArea: base address = %x\n",base);
	
	if (origArea->getAreaManager()!=manager) // Else, already locked;
		{
		printf ("Holding dual locks! \n");
		origArea->getAreaManager()->lock();
		}
	for (struct node *cur=origArea->vpages.rock;cur;)
		{
		vpage *newPage,*page=(vpage *)cur;
		// Cloned area has the same physical page and backing store...
		newPage=new (vmBlock->vpagePool->get()) vpage;	
		newPage->setup(base,page->getBacking(),page->getPhysPage(),protect,inState);
		vpages.add(newPage);
		base+=PAGE_SIZE;
		cur=cur->next;
		}
	if (origArea->getAreaManager()!=manager) // Else, already locked;
		origArea->getAreaManager()->unlock();
	state=inState;
	end_address=base+pageCount*PAGE_SIZE;
	manager->unlock();
	*address=(void *)start_address;
	finalWrite=false;
//	printf ("area::cloneArea: unlocked\n");
	}

void area::freeArea(void)
	{
//printf ("area::freeArea: starting \n");

	manager->lock();
//	vpages.dump();
	node *cur;
	while (cur=vpages.next())
		{
//printf ("area::freeArea: wasting a page: %x\n",cur);
		vpage *page=reinterpret_cast<vpage *>(cur);
		if (finalWrite) 
			page->flush(); 
//printf ("area::freeArea: flushed a page \n");
		page->cleanup();
		//page->next=NULL;
		vmBlock->vpagePool->put(page);
		}
//printf ("area::freeArea: unlocking \n");
	manager->unlock();
//printf ("area::freeArea: ending \n");
	}

status_t area::getInfo(area_info *dest)
	{
	// no need to lock here...
	strcpy(dest->name,name);
	dest->size=end_address-start_address;
	dest->lock=state;
	dest->team=manager->getTeam();
	dest->ram_size=0;
	dest->in_count=0;
	dest->out_count=0;
	dest->copy_count=0;
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		if (page->isMapped())
			dest->ram_size+=PAGE_SIZE;
		dest->in_count+=PAGE_SIZE;
		dest->out_count+=PAGE_SIZE;
		dest->copy_count+=PAGE_SIZE;
		}
	dest->address=(void *)start_address;
	return B_OK;
	}

bool area::contains(void *address)
	{
	// no need to lock here...
	unsigned long base=(unsigned long)(address); 
//	printf ("area::contains: looking for %d in %d -- %d, value = %d\n",base,start_address,end_address, ((start_address<=base) && (end_address>=base)));
					
	return ((start_address<=base) && (end_address>=base));
	}

status_t area::resize(size_t newSize)
	{
	size_t oldSize =end_address-start_address;
	if (newSize==oldSize)
		return B_OK;
	if (newSize>oldSize)
		{
		manager->lock();
		int pageCount = (newSize-oldSize) / PAGE_SIZE;
		vpage *newPage;
		for (int i=0;i<pageCount;i++)
			{
			newPage=new (vmBlock->vpagePool->get()) vpage;
			newPage->setup(end_address+PAGE_SIZE*i-1,NULL,NULL,protection,state);
			vpages.add(newPage);
			}
		end_address+=start_address+newSize;
		}
	else
		{
		manager->lock();
		int pageCount = (oldSize -newSize) / PAGE_SIZE;
		vpage *oldPage;
		struct node *cur;
		for (int i=0;i<pageCount;i++) // This is probably really slow. Adding an "end" to list would be faster.
			{
			for (cur=vpages.rock;cur->next;cur=cur->next); // INTENTIONAL - find the last one;
			vpage *oldPage=(vpage *)cur;
			oldPage->cleanup();
			vmBlock->vpagePool->put(oldPage);
			}
		}
	manager->unlock();
	}

status_t area::setProtection(protectType prot)
	{
	manager->lock();
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		page->setProtection(prot);
		}
	protection=prot;
	manager->unlock();
	}

vpage *area::findVPage(unsigned long address)
	{
	// No need to lock here...
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		if (page->contains(address))
			return page;
		}
	return NULL;
	}

bool area::fault(void *fault_address, bool writeError) // true = OK, false = panic.
	{
	// No need to lock here...
	vpage *page=findVPage((unsigned long)fault_address);
	if (page)
		return page->fault(fault_address,writeError);
	else
		return false;
	}

char area::getByte(unsigned long address) // This is for testing only
	{
	// No need to lock here...
	vpage *page=findVPage(address);
	if (page)
		return page->getByte(address);
	else
		return 0;
	}

void area::setByte(unsigned long address,char value) // This is for testing only
	{
	// No need to lock here...
	vpage *page=findVPage(address);
	if (page)
		page->setByte(address,value);
	}

int area::getInt(unsigned long address) // This is for testing only
	{
	// No need to lock here...
	vpage *page=findVPage(address);
	if (page)
		page->getInt(address);
	}

void area::setInt(unsigned long address,int value) // This is for testing only
	{
	// No need to lock here...
	vpage *page=findVPage(address);
	if (page)
		page->setInt(address,value);
	}

void area::pager(int desperation)
	{
	// No need to lock here...
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		page->pager(desperation);
		}
	}

void area::saver(void)
	{
	// No need to lock here...
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		page->saver();
		}
	}

void area::dump(void) 
	{ 
	printf ("area::dump: size = %d, lock = %d, address = %x\n",end_address-start_address,state,start_address); 
	for (struct node *cur=vpages.rock;cur;)
		{
		vpage *page=(vpage *)cur;
		page->dump();
		cur=cur->next;
		}
	}

