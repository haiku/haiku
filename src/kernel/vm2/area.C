#include <new.h>
#include "area.h"
#include "areaManager.h"
#include "vpage.h"
#include "vnodePool.h"
#include "vpagePool.h"

#include "vmHeaderBlock.h"

extern vmHeaderBlock *vmBlock;

bool vpageIsLessThan(void *a,void *b)
{
	return (((reinterpret_cast<vpage *>(a))->getStartAddress()) < (reinterpret_cast<vpage *>(b))->getStartAddress());
}

area::area(void)
	{
	vpages.setIsLessThan(vpageIsLessThan);
	}

void area::setup (areaManager *myManager)
	{
	//error ("area::setup setting up new area\n");
	manager=myManager;
	//error ("area::setup done setting up new area\n");
	}

unsigned long area::mapAddressSpecToAddress(addressSpec type,void * req,int pageCount)
{
	// We will lock in the callers
	unsigned long base,requested=(unsigned long)req;
	switch (type)
		{
		case EXACT: 
			base=manager->getNextAddress(pageCount,requested); 
			if (base!=requested)
				return 0; 
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
		default: // should never happen
			throw ("Unknown type passed to mapAddressSpecToAddress");
		}
//	error ("area::mapAddressSpecToAddress, in type: %s, address = %x, size = %d\n", ((type==EXACT)?"Exact":(type==BASE)?"BASE":(type==ANY)?"ANY":(type==CLONE)?"CLONE":"ANY_KERNEL"), requested,pageCount);
	return base;
}

status_t area::createAreaGuts( char *inName, int pageCount, void **address, addressSpec type, pageState inState, protectType protect, bool inFinalWrite, int fd, size_t offset, area *originalArea=NULL /* For clone only*/)
{
	strcpy(name,inName);
	vpage *newPage;

	unsigned long base=mapAddressSpecToAddress(type,*address,pageCount);
	if (base==0)
		return B_ERROR;
	state=inState;
	start_address=base;
	end_address=base+(pageCount*PAGE_SIZE)-1;
	*address=(void *)base;
	finalWrite=inFinalWrite;
	if (!originalArea)
		for (int i=0;i<pageCount;i++)
			{
			newPage=new (vmBlock->vpagePool->get()) vpage;
			if (fd)
				{
				vnode *newVnode=new (vmBlock->vnodePool->get()) vnode;
				newVnode->fd=fd;
				newVnode->offset=offset+PAGE_SIZE*i;
				newVnode->valid=true;
				newPage->setup(base+PAGE_SIZE*i,newVnode,NULL,protect,inState);
				}
			else
				newPage->setup(base+PAGE_SIZE*i,NULL,NULL,protect,inState);
			vpages.add(newPage);
			}
	else
		for (struct node *cur=originalArea->vpages.rock;cur;cur=cur->next)
			{
			vpage *page=(vpage *)cur;
			newPage=new (vmBlock->vpagePool->get()) vpage;	
			newPage->setup(base,page->getBacking(),page->getPhysPage(),protect,inState);// Cloned area has the same physical page and backing store...
			vpages.add(newPage);
			base+=PAGE_SIZE;
			}
	return B_OK;
}

status_t area::createAreaMappingFile(char *inName, int pageCount,void **address, addressSpec type,pageState inState,protectType protect,int fd,size_t offset)
	{
	return createAreaGuts(inName,pageCount,address,type,inState,protect,true,fd,offset);
	}

status_t area::createArea(char *inName, int pageCount,void **address, addressSpec type,pageState inState,protectType protect)
	{
	return createAreaGuts(inName,pageCount,address,type,inState,protect,false,0,0);
	}

status_t area::cloneArea(area *origArea, char *inName, void **address, addressSpec type,pageState inState,protectType protect)
	{
	if (type==CLONE)			
		{
		*address=(void *)(origArea->getStartAddress());
		type=EXACT;
		}
	if (origArea->getAreaManager()!=manager)
		{
		origArea->getAreaManager()->lock(); // This is just begging for a deadlock...
		status_t retVal = createAreaGuts(inName,origArea->getPageCount(),address,type,inState,protect,false,0,0,origArea);
		origArea->getAreaManager()->unlock(); 
		return retVal;
		}
	else
		return createAreaGuts(inName,origArea->getPageCount(),address,type,inState,protect,false,0,0,origArea);
	}

void area::freeArea(void)
	{
//error ("area::freeArea: starting \n");

//	vpages.dump();
	node *cur;
	while ((cur=vpages.next())!=NULL)
		{
//error ("area::freeArea: wasting a page: %x\n",cur);
		vpage *page=reinterpret_cast<vpage *>(cur);
		if (finalWrite) 
			page->flush(); 
//error ("area::freeArea: flushed a page \n");
		page->cleanup();
		//page->next=NULL;
		vmBlock->vpagePool->put(page);
		}
//error ("area::freeArea: unlocking \n");
//error ("area::freeArea: ending \n");
	}

status_t area::getInfo(area_info *dest)
	{
	dest->area=areaID;
	strcpy(dest->name,name);
	dest->size=end_address-start_address;
	dest->lock=state;
	dest->protection=protection; 
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
//	error ("area::contains: looking for %d in %d -- %d, value = %d\n",base,start_address,end_address, ((start_address<=base) && (end_address>=base)));
					
	return ((start_address<=base) && (base<=end_address));
	}

status_t area::resize(size_t newSize)
	{
	size_t oldSize =end_address-start_address;
	if (newSize==oldSize)
		return B_OK;
	if (newSize>oldSize)
		{
		int pageCount = (newSize - oldSize + PAGE_SIZE - 1) / PAGE_SIZE;
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
		int pageCount = (oldSize - newSize + PAGE_SIZE - 1) / PAGE_SIZE;
		vpage *oldPage;
		struct node *cur;
		for (int i=0;i<pageCount;i++) // This is probably really slow. Adding an "end" to list would be faster.
			{
			for (cur=vpages.rock;cur->next;cur=cur->next); // INTENTIONAL - find the last one;
			oldPage=(vpage *)cur;
			vpages.remove(cur);
			if (finalWrite) 
				oldPage->flush(); 
			oldPage->cleanup();
			vmBlock->vpagePool->put(oldPage);
			}
		}
	return B_OK;
	}

status_t area::setProtection(protectType prot)
	{
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		page->setProtection(prot);
		}
	protection=prot;
	return B_OK;
	}

vpage *area::findVPage(unsigned long address)
	{
	for (struct node *cur=vpages.rock;cur;cur=cur->next)
		{
		vpage *page=(vpage *)cur;
		//error ("Examining following vpage, looking for address %lx\n",address);
		//page->dump();
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
		return page->getByte(address,manager);
	else
		return 0;
	}

void area::setByte(unsigned long address,char value) // This is for testing only
	{
	vpage *page=findVPage(address);
	if (page)
		page->setByte(address,value,manager);
	}

int area::getInt(unsigned long address) // This is for testing only
	{
	vpage *page=findVPage(address);
	if (page)
		return page->getInt(address,manager);
	else
		return 0;
	}

void area::setInt(unsigned long address,int value) // This is for testing only
	{
	vpage *page=findVPage(address);
	if (page)
		page->setInt(address,value,manager);
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

void area::dump(void) 
	{ 
	error ("area::dump: size = %ld, lock = %d, address = %lx\n",end_address-start_address,state,start_address); 
	for (struct node *cur=vpages.rock;cur;)
		{
		vpage *page=(vpage *)cur;
		page->dump();
		cur=cur->next;
		}
	}

