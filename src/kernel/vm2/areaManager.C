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
//			printf ("Looking for %x, %d pages; current = %x\n",start,pages,myArea->getEndAddress());
			if (!myArea->couldAdd(start,end))
				{ // if we don't work, there must be an overlap, so go to the end of this area.
				start=myArea->getEndAddress();
				end=start+(pages*PAGE_SIZE)-1;
				}
			}
		}
	return start;
}

void areaManager::freeArea(int areaID)
{
	printf ("areaManager::freeArea: begin\n");
	lock();
	area *oldArea=findArea(areaID);	
	//printf ("areaManager::freeArea: found area %x\n",oldArea);
	if (oldArea)
		{
//		printf ("areaManager::freeArea: removing area %x from linked list\n",oldArea);
//		printf ("areaManager::freeArea: areaManager =  %x \n",manager);
		removeArea(oldArea);
//		printf ("areaManager::freeArea: deleting area %x \n",oldArea);
		oldArea->freeArea();
//		printf ("areaManager::freeArea: freeArea complete \n");
		vmBlock->areaPool->put(oldArea);
		}
	else
		printf ("areaManager::freeArea: unable to find requested area\n");
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
	printf ("Finding area by string\n");
	lock();
	area *retVal=NULL;
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
	//printf ("Finding area by void * address\n");
	for (struct node *cur=areas.rock;cur;cur=cur->next)
		{
		area *myArea=(area *)cur;
//		printf ("areaManager::findArea: Looking for %x between %x and %x\n",address,myArea->getStartAddress(),myArea->getEndAddress());
		fflush(NULL);
		if (myArea->contains(address))
				return myArea;
		}
	printf ("areaManager::findArea is giving up\n");
	return NULL;
}

area *areaManager::findAreaLock(area_id id)
{
	printf ("Finding area by areaID \n");
	lock();
	area *retVal=findArea(id);
	unlock();
	return retVal;
}

area *areaManager::findArea(area_id id)
{
	//printf ("Finding area by area_id\n");
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
	printf ("Faulting \n");
	lock();
	myArea=findArea(fault_address);
	if (myArea)
		retVal= myArea->fault(fault_address,writeError);	
	else
		retVal= false;
	unlock();
	return retVal;
}

int areaManager::nextAreaID=0;

int areaManager::createArea(char *AreaName,int pageCount,void **address, addressSpec addType,pageState state,protectType protect) 
{
	printf ("Creating an area\n");
	lock();
    area *newArea = new (vmBlock->areaPool->get()) area;
 //   printf ("areaManager::createArea - got a new area (%p) from the areaPool\n",newArea);
    newArea->setup(this);
 //   printf ("areaManager::createArea - setup complete\n");
    newArea->createArea(AreaName,pageCount,address,addType,state,protect);
 //   printf ("areaManager::createArea - new area's createArea called\n");
    newArea->setAreaID(nextAreaID++); // THIS IS NOT  THREAD SAFE
 //   printf ("areaManager::createArea - new area's setAreaID called\n");
    addArea(newArea);
 //   printf ("areaManager::createArea - new area added to list\n");
	int retVal=newArea->getAreaID();  
 //   printf ("areaManager::createArea - new area id found\n");
	unlock();
	//printf ("Done Creating an area\n");
    return  retVal;
}

int areaManager::cloneArea(int newAreaID,char *AreaName,void **address, addressSpec addType,pageState state,protectType protect) 
    {
	printf ("Cloning an area\n");
	lock();
    area *newArea = new (vmBlock->areaPool->get()) area;
    newArea->setup(this);
    area *oldArea=findArea(newAreaID);
    newArea->cloneArea(oldArea,AreaName,address,addType,state,protect);
    newArea->setAreaID(nextAreaID++); // THIS IS NOT  THREAD SAFE
    addArea(newArea);
    int retVal=newArea->getAreaID();
	unlock();
	return retVal;
    }   

char areaManager::getByte(unsigned long address)
{
	area *myArea;
	int retVal;
	myArea=findArea((void *)address);
	if (myArea)
		retVal=myArea->getByte(address);	
	else
		retVal= 0;
	return retVal;
}

int areaManager::getInt(unsigned long address)
{
	area *myArea;
	int retVal;
	myArea=findArea((void *)address);
	if (myArea)
		retVal=myArea->getInt(address);	
	else
		retVal= 0;
	return retVal;
}

void areaManager::setByte(unsigned long address,char value)
{
	area *myArea;
	myArea=findArea((void *)address);
	if (myArea)
		myArea->setByte(address,value);	
}

void areaManager::setInt(unsigned long address,int value)
{
	area *myArea;
	myArea=findArea((void *)address);
	if (myArea)
		myArea->setInt(address,value);	
}

void areaManager::pager(int desperation)
{
	lock();
	for (struct node *cur=areas.rock;cur;cur=cur->next)
		{
		area *myArea=(area *)cur;
		//printf ("areaManager::pager; area = \n");
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
	//printf ("flags = %x, anon = %x\n",flags,MAP_ANON);
	lock();
	if (flags & MAP_ANON) 
		{
		createArea(name,(int)((len+PAGE_SIZE-1)/PAGE_SIZE),&addr, addType ,LAZY,protType);
		return addr;
		}

	area *newArea = new (vmBlock->areaPool->get()) area;
	newArea->setup(this);
	//printf ("area = %x, start = %x\n",newArea, newArea->getStartAddress());
	newArea->createAreaMappingFile(name,(int)((len+PAGE_SIZE-1)/PAGE_SIZE),&addr,addType,LAZY,protType,fd,offset);
	newArea->setAreaID(nextAreaID++); // THIS IS NOT  THREAD SAFE
	addArea(newArea);
	newArea->getAreaID();
	//pageMan.dump();
	//newArea->dump();
	unlock();
	return addr;
	}
