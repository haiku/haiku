#include <new.h>
#include "mman.h"
#include "areaManager.h"
#include "vmHeaderBlock.h"
#include "areaPool.h"
extern vmHeaderBlock *vmBlock; 

bool areaIsLessThan(void *a,void *b)
{
	return (((reinterpret_cast<area *>(a))->getStartAddress()) < (reinterpret_cast<area *>(b))->getStartAddress());
}

areaManager::areaManager(void)
{  
	team=0; // should be proc_get_current_proc_id()
	myLock=0;
	myLock=create_sem(1,"Area Manager Semaphore"); // Should have team name in it.
	areas.setIsLessThan(areaIsLessThan);
}

unsigned long areaManager::getNextAddress(int pages, unsigned long start)
{
		// This function needs to deal with the possibility that we run out of address space...
//	areas.dump();
	unsigned long end=start+(pages*PAGE_SIZE)-1;
	for (struct node *cur=areas.rock;cur;cur=cur->next)
		{
		if (cur)
			{
			area *myArea=(area *)cur;
//			error ("Looking for %x, %d pages; current = %x\n",start,pages,myArea->getEndAddress());
			if (!myArea->couldAdd(start,end))
				{ // if we don't work, there must be an overlap, so go to the end of this area.
				start=myArea->getEndAddress();
				end=start+(pages*PAGE_SIZE)-1;
				}
			}
		}
	return start;
}

void areaManager::freeArea(area_id areaID)
{
	error ("areaManager::freeArea: begin\n");
	lock();
	area *oldArea=findArea(areaID);	
	//error ("areaManager::freeArea: found area %x\n",oldArea);
	if (oldArea)
		{
//		error ("areaManager::freeArea: removing area %x from linked list\n",oldArea);
//		error ("areaManager::freeArea: areaManager =  %x \n",manager);
		removeArea(oldArea);
//		error ("areaManager::freeArea: deleting area %x \n",oldArea);
		oldArea->freeArea();
//		error ("areaManager::freeArea: freeArea complete \n");
		vmBlock->areaPool->put(oldArea);
		}
	else
		error ("areaManager::freeArea: unable to find requested area\n");
	unlock();
}

area *areaManager::findAreaLock(void *address)
{
	lock();
	area *retVal=findArea(address);
	unlock();
	return retVal;
}

area *areaManager::findArea(char *address)
{
	error ("Finding area by string\n");
	area *retVal=NULL;
	lock();
	for (struct node *cur=areas.rock;cur && !retVal;cur=cur->next)
		{
		area *myArea=(area *)cur;
		if (myArea->nameMatch(address))
			retVal= myArea;
		}
	unlock();
	return retVal;
}

area *areaManager::findArea(void *address)
{
	// THIS DOES NOT HAVE LOCKING - all callers must lock.
//	error ("Finding area by void * address\n");
	for (struct node *cur=areas.rock;cur;cur=cur->next)
		{
		area *myArea=(area *)cur;
		//error ("areaManager::findArea: Looking for %x between %x and %x\n",address,myArea->getStartAddress(),myArea->getEndAddress());
		if (myArea->contains(address))
				return myArea;
		}
	error ("areaManager::findArea is giving up\n");
	return NULL;
}

area *areaManager::findAreaLock(area_id id)
{
	error ("Finding area by areaID \n");
	lock();
	area *retVal=findArea(id);
	unlock();
	return retVal;
}

area *areaManager::findArea(area_id id)
{
	//error ("Finding area by area_id\n");
	area *retVal=NULL;
	for (struct node *cur=areas.rock;cur && !retVal;cur=cur->next)
		{
		area *myArea=(area *)cur;
		if (myArea->getAreaID()==id)
				retVal= myArea;
		}
	return retVal;
}

bool areaManager::fault(void *fault_address, bool writeError) // true = OK, false = panic.
{
	area *myArea;
	bool retVal;
	error ("Faulting \n");
// 	lock(); // Normally this should occur, but since we will be locked when we read/write anyway...
	myArea=findArea(fault_address);
	if (myArea)
		retVal= myArea->fault(fault_address,writeError);	
	else
		retVal= false;
//	unlock();
	return retVal;
}

long areaManager::nextAreaID=0;

int areaManager::createArea(char *AreaName,int pageCount,void **address, addressSpec addType,pageState state,protectType protect) 
{
	error ("areaManager::createArea - Creating an area\n");
	lock();
    area *newArea = new (vmBlock->areaPool->get()) area;
    error ("areaManager::createArea - got a new area (%p) from the areaPool\n",newArea);
    newArea->setup(this);
    error ("areaManager::createArea - setup complete\n");
    newArea->createArea(AreaName,pageCount,address,addType,state,protect);
    error ("areaManager::createArea - new area's createArea called\n");
 	atomic_add(&nextAreaID,1);
    newArea->setAreaID(nextAreaID);
    error ("areaManager::createArea - new area's setAreaID called\n");
    addArea(newArea);
    error ("areaManager::createArea - new area added to list\n");
	int retVal=newArea->getAreaID();  
    error ("areaManager::createArea - new area id found\n");
	unlock();
	error ("areaManager::createArea - Done Creating an area\n");
    return  retVal;
}

int areaManager::cloneArea(int newAreaID,char *AreaName,void **address, addressSpec addType,pageState state,protectType protect) 
    {
	int retVal;
	error ("Cloning an area\n");
	lock();
    area *oldArea=findArea(newAreaID);
	if (oldArea)
		{
    	area *newArea = new (vmBlock->areaPool->get()) area;
    	newArea->setup(this);
    	newArea->cloneArea(oldArea,AreaName,address,addType,state,protect);
		atomic_add(&nextAreaID,1);
    	newArea->setAreaID(nextAreaID); 
    	addArea(newArea);
    	retVal=newArea->getAreaID();
		}
	else
		retVal=B_ERROR;
	unlock();
	return retVal;
    }   

char areaManager::getByte(unsigned long address)
{
	area *myArea;
//	error ("areaManager::getByte : starting\n");
	int retVal;
	lock();
	myArea=findArea((void *)address);
	if (myArea)
		retVal=myArea->getByte(address);	
	else
		{
		char temp[1000];
		sprintf (temp,"Unable to find an area for address %d\n",address);
		throw (temp);
		}
	unlock();
	return retVal;
}

int areaManager::getInt(unsigned long address)
{
	area *myArea;
	int retVal;
	lock();
	myArea=findArea((void *)address);
	if (myArea)
		retVal=myArea->getInt(address);	
	else
		{
		char temp[1000];
		sprintf (temp,"Unable to find an area for address %d\n",address);
		throw (temp);
		}
	unlock();
	return retVal;
}

void areaManager::setByte(unsigned long address,char value)
{
//	error ("areaManager::setByte : starting\n");
	area *myArea;
	lock();
	myArea=findArea((void *)address);
	if (myArea)
		myArea->setByte(address,value);	
	else
		{
		char temp[1000];
		sprintf (temp,"Unable to find an area for address %d\n",address);
		throw (temp);
		}
	unlock();
}

void areaManager::setInt(unsigned long address,int value)
{
	area *myArea;
	lock();
	myArea=findArea((void *)address);
	if (myArea)
		myArea->setInt(address,value);	
	else
		{
		char temp[1000];
		sprintf (temp,"Unable to find an area for address %d\n",address);
		throw (temp);
		}
	unlock();
}

void areaManager::pager(int desperation)
{
	lock();
	for (struct node *cur=areas.rock;cur;cur=cur->next)
		{
		area *myArea=(area *)cur;
		//error ("areaManager::pager; area = \n");
		//myArea->dump();
		myArea->pager(desperation);
		}
	unlock();
}

void areaManager::saver(void)
{
	lock();
	for (struct node *cur=areas.rock;cur;cur=cur->next)
		{
		area *myArea=(area *)cur;
		myArea->saver();
		}
	unlock();
}

void *areaManager::mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	char name[MAXPATHLEN];
	// Get the filename from fd...
	strcpy(	name,"mmap - need to include fileName");
		
	addressSpec addType=((flags&MAP_FIXED)?EXACT:ANY);
	protectType protType=(prot&PROT_WRITE)?writable:(prot&PROT_READ)?readable:none;
	// Not doing anything with MAP_SHARED and MAP_COPY - needs to be done
	//error ("flags = %x, anon = %x\n",flags,MAP_ANON);
	lock();
	if (flags & MAP_ANON) 
		{
		createArea(name,(int)((len+PAGE_SIZE-1)/PAGE_SIZE),&addr, addType ,LAZY,protType);
		return addr;
		}

	area *newArea = new (vmBlock->areaPool->get()) area;
	newArea->setup(this);
	//error ("area = %x, start = %x\n",newArea, newArea->getStartAddress());
	newArea->createAreaMappingFile(name,(int)((len+PAGE_SIZE-1)/PAGE_SIZE),&addr,addType,LAZY,protType,fd,offset);
 	atomic_add(&nextAreaID,1);
	newArea->setAreaID(nextAreaID);
	addArea(newArea);
	newArea->getAreaID();
	//pageMan.dump();
	//newArea->dump();
	unlock();
	return addr;
	}

status_t areaManager::munmap(void *addr,size_t len)
		{
		// Note that this is broken for any and all munmaps that are not full area in size. This is an all or nothing game...
		status_t retVal=B_OK;
		lock();
		area *myArea=findArea(addr);
		if (myArea)
			{
			removeArea(myArea);
			myArea->freeArea();
			vmBlock->areaPool->put(myArea);
			}
		else
			{
			error ("areaManager::munmap: unable to find requested area\n");
			retVal=B_ERROR;
			}
		unlock();
		return retVal;
		}
