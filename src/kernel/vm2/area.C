#include "area.h"
#include "areaManager.h"
#include "vpage.h"

area::area (areaManager *myManager)
	{
	manager=myManager;
	}

unsigned long area::mapAddressSpecToAddress(addressSpec type,unsigned long requested,int pageCount)
{
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
	return base;
}

status_t area::createAreaMappingFile(char *name, int pageCount,void **address, addressSpec type,pageState inState,protectType protect,int fd,size_t offset)
	{
	unsigned long requested=(unsigned long)(*address); // Hold onto this to make sure that EXACT works...
	unsigned long base=mapAddressSpecToAddress(type,requested,pageCount);
	vpage *newPage;
	vnode newVnode;
	for (int i=0;i<pageCount;i++)
		{
		newVnode.fd=fd;
		newVnode.offset=offset+PAGE_SIZE*i;
		newPage = new vpage(base+PAGE_SIZE*i,newVnode,NULL,protect,inState);
		vpages.add(newPage);
		}
	state=inState;
	}

status_t area::createArea(char *name, int pageCount,void **address, addressSpec type,pageState inState,protectType protect)
	{
	unsigned long requested=(unsigned long)(*address); // Hold onto this to make sure that EXACT works...
	unsigned long base=mapAddressSpecToAddress(type,requested,pageCount);
	vpage *newPage;
	vnode newVnode;
	newVnode.fd=0;
	newVnode.offset=0;
	for (int i=0;i<pageCount;i++)
		{
		newPage = new vpage(base+PAGE_SIZE*i,newVnode,NULL,protect,inState);
		vpages.add(newPage);
		}
	state=inState;
	}

void area::freeArea(void)
	{
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		page->flush();
		delete page; // Probably need to add a destructor
		}
	}

status_t area::getInfo(area_info *dest)
	{
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
	unsigned long base=(unsigned long)(address); 
	return ((start_address>=base) && (end_address<=base));
	}

status_t area::resize(size_t newSize)
	{
	size_t oldSize =end_address-start_address;
	if (newSize==oldSize)
		return B_OK;
	if (newSize>oldSize)
		{
		int pageCount = (newSize-oldSize) / PAGE_SIZE;
		vpage *newPage;
		vnode newVnode;
		newVnode.fd=0;
		newVnode.offset=0;
		for (int i=0;i<pageCount;i++)
			{
			newPage = new vpage(end_address+PAGE_SIZE*i-1,newVnode,NULL,protection,state);
			vpages.add(newPage);
			}
		end_address+=start_address+newSize;
		}
	else
		{
		int pageCount = (oldSize -newSize) / PAGE_SIZE;
		vpage *oldPage;
		struct node *cur;
		for (int i=0;i<pageCount;i++) // This is probably really slow. Adding an "end" to list would be faster.
			{
			for (cur=vpages.rock;cur->next;cur=cur->next); // INTENTIONAL - find the last one;
			vpage *oldPage=(vpage *)cur;
			delete oldPage;
			}
		}
	}

status_t area::setProtection(protectType prot)
	{
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		page->setProtection(prot);
		}
	protection=prot;
	}

vpage *area::findVPage(unsigned long address)
	{
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
	vpage *page=findVPage((unsigned long)fault_address);
	if (page)
		return page->fault(fault_address,writeError);
	else
		return false;
	}

char area::getByte(unsigned long address) // This is for testing only
	{
	vpage *page=findVPage(address);
	if (page)
		return page->getByte(address);
	else
		return 0;
	}

void area::setByte(unsigned long address,char value) // This is for testing only
	{
	vpage *page=findVPage(address);
	if (page)
		page->setByte(address,value);
	}

int area::getInt(unsigned long address) // This is for testing only
	{
	vpage *page=findVPage(address);
	if (page)
		page->getInt(address);
	}

void area::setInt(unsigned long address,int value) // This is for testing only
	{
	vpage *page=findVPage(address);
	if (page)
		page->setInt(address,value);
	}

void area::pager(int desperation)
	{
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		page->pager(desperation);
		}
	}

void area::saver(void)
	{
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		page->saver();
		}
	}

