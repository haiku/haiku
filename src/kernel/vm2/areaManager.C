#include <new.h>
#include "mman.h"
#include "areaManager.h"
#include "vmHeaderBlock.h"
#include "areaPool.h"
extern vmHeaderBlock *vmBlock; 

bool areaIsLessThan(void *a,void *b) {
	return (((reinterpret_cast<area *>(a))->getStartAddress()) < (reinterpret_cast<area *>(b))->getStartAddress());
}

// This creates the one true lock for this area
areaManager::areaManager(void) {  
	team=0; // should be proc_get_current_proc_id()
	myLock=0;
	myLock=create_sem(1,"Area Manager Semaphore"); // Should have team name in it.
	areas.setIsLessThan(areaIsLessThan);
}

// Loops over every area looking for someplace where we can get the space we need.
unsigned long areaManager::getNextAddress(int pages, unsigned long start) {
		// This function needs to deal with the possibility that we run out of address space...
//	areas.dump();
	unsigned long end=start+(pages*PAGE_SIZE)-1;
	for (struct node *cur=areas.top();cur;cur=cur->next)
		{
		if (cur)
			{
			area *myArea=(area *)cur;
//			error ("Looking for %x, %d pages; current = %x\n",start,pages,myArea->getEndAddress());
			if (!myArea->couldAdd(start,end))
				{ // if we don't work, there must be an overlap, so go to the end of this area.
				start=myArea->getEndAddress()+1; // Since the end address == last byte in the area...
				end=start+(pages*PAGE_SIZE)-1; // See above...
				}
			}
		}
	return start;
}

// Remove the area from our list, put it on the area pool and move on
status_t areaManager::freeArea(area_id areaID) {
	//error ("areaManager::freeArea: begin\n");
	status_t retVal=B_OK;
	lock();
	area *oldArea=findArea(areaID);	
	//error ("areaManager::freeArea: found area %x\n",oldArea);
	if (oldArea)
		{
//		error ("areaManager::freeArea: removing area %x from linked list\n",oldArea);
//		error ("areaManager::freeArea: areaManager =  %x \n",manager);
		removeArea(oldArea);
//		error ("areaManager::freeArea: deleting area %x \n",oldArea);
		vmBlock->areas.remove(oldArea);
		oldArea->freeArea();
//		error ("areaManager::freeArea: freeArea complete \n");
		vmBlock->areaPool->put(oldArea);
		}
	else {
		retVal=B_ERROR;
		error ("areaManager::freeArea: unable to find requested area\n");
		}
	unlock();
//	error ("areaManager::freeArea: final unlock complete\n");
	return retVal;
}

area *areaManager::findAreaLock(void *address) {
	lock();
	area *retVal=findArea(address);
	unlock();
	return retVal;
}

// Loops over our areas looking for this one by name
area *areaManager::findArea(char *address) {
	//error ("Finding area by string\n");
	area *retVal=NULL;
	lock();
	for (struct node *cur=areas.top();cur && !retVal;cur=cur->next)
		{
		area *myArea=(area *)cur;
		if (myArea->nameMatch(address))
			retVal= myArea;
		}
	unlock();
	return retVal;
}

// Loops over our areas looking for the one whose virtual address matches the passed in address
area *areaManager::findArea(const void *address) {
	// THIS DOES NOT HAVE LOCKING - all callers must lock.
//	error ("Finding area by void * address\n");
	for (struct node *cur=areas.top();cur;cur=cur->next)
		{
		area *myArea=(area *)cur;
		//error ("areaManager::findArea: Looking for %x between %x and %x\n",address,myArea->getStartAddress(),myArea->getEndAddress());
		if (myArea->contains(address))
				return myArea;
		}
//	error ("areaManager::findArea is giving up\n");
	return NULL;
}

area *areaManager::findAreaLock(area_id id) {
	//error ("Finding area by areaID \n");
	lock();
	area *retVal=findArea(id);
	unlock();
	return retVal;
}

// Loops over our areas looking for the one whose ID was passed in
area *areaManager::findArea(area_id id) {
	//error ("Finding area by area_id\n");
	area *retVal=NULL;
	for (struct node *cur=areas.top();cur && !retVal;cur=cur->next)
		{
		area *myArea=(area *)cur;
		if (myArea->getAreaID()==id)
				retVal= myArea;
		}
	return retVal;
}

// Find the area whose address matches this page fault and dispatch the fault to it.
bool areaManager::fault(void *fault_address, bool writeError) { // true = OK, false = panic. 
	area *myArea;
	bool retVal;
	//error ("Faulting \n");
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

// Create an area; get a new structure, call setup, create the guts, set its ID, add it to our list 
int areaManager::createArea(char *AreaName,int pageCount,void **address, addressSpec addType,pageState state,protectType protect) {
//	error ("areaManager::createArea - Creating an area\n");
	lock();
    area *newArea = new (vmBlock->areaPool->get()) area;
//	error ("areaManager::createArea - got a new area (%p) from the areaPool\n",newArea);
    newArea->setup(this);
//	error ("areaManager::createArea - setup complete\n");
    int retVal = newArea->createArea(AreaName,pageCount,address,addType,state,protect);
//	error ("areaManager::createArea - new area's createArea called\n");
	if (retVal==B_OK) {
	 	atomic_add(&nextAreaID,1);
   	 	newArea->setAreaID(nextAreaID);
    	//error ("areaManager::createArea - new area's setAreaID called\n");
    	addArea(newArea);
    	//error ("areaManager::createArea - new area added to list\n");
		retVal=newArea->getAreaID();  
//		error ("areaManager::createArea - getAreaID=%d\n",retVal);
    //error ("areaManager::createArea - new area id found\n");
	}
	unlock();
	//error ("areaManager::createArea - Done Creating an area\n");
    return  retVal;
}

area *findAreaGlobal(int areaID) {
	for (struct node *cur=vmBlock->areas.top();cur;cur=cur->next) {
		area *myArea=(area *)cur;
		if (((area *)(cur))->getAreaID()==areaID)
			return myArea;
		}
	return NULL;
}

int areaManager::cloneArea(int newAreaID,char *AreaName,void **address, addressSpec addType,pageState state,protectType protect) {
	int retVal;
	//error ("Cloning an area\n");
	lock();
    area *oldArea=findArea(newAreaID);
	if (!oldArea)
		oldArea=findAreaGlobal(newAreaID);
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

char areaManager::getByte(unsigned long address) {
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

int areaManager::getInt(unsigned long address) {
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

void areaManager::setByte(unsigned long address,char value) {
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

void areaManager::setInt(unsigned long address,int value) {
//	error ("areaManager::setInt starting to set on address %lx, value = %d\n",address,value);
	area *myArea;
	lock();
//	error ("areaManager::setInt locked\n");
	myArea=findArea((void *)address);
//	error ("areaManager::setInt area %s found\n",((myArea)?"":" not "));
	try {
	if (myArea)
		myArea->setInt(address,value);	
	else {
		char temp[1000];
		sprintf (temp,"Unable to find an area for address %d\n",address);
		unlock();
		throw (temp);
		}
	}
	catch (const char *t) { unlock();throw t;}
	catch (char *t) { unlock();throw t;}
//	error ("areaManager::setInt unlocking\n");
	unlock();
}

// Call pager for each of our areas
void areaManager::pager(int desperation) {
	lock();
	for (struct node *cur=areas.top();cur;cur=cur->next) {
		area *myArea=(area *)cur;
		//error ("areaManager::pager; area = \n");
		//myArea->dump();
		myArea->pager(desperation);
		}
	unlock();
}

// Call saver for each of our areas
void areaManager::saver(void) {
	lock();
	for (struct node *cur=areas.top();cur;cur=cur->next) {
		area *myArea=(area *)cur;
		myArea->saver();
		}
	unlock();
}

// mmap is basically map POSIX values to ours and call createAreaMappingFile...
void *areaManager::mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
	char name[MAXPATHLEN];
	if (fd<0)
		return NULL;
	// Get the filename from fd...
	strcpy(	name,"mmap - need to include fileName");
		
	addressSpec addType=((flags&MAP_FIXED)?EXACT:ANY);

	protectType protType;
	protType=(prot&PROT_WRITE)?writable:(prot&(PROT_READ|PROT_EXEC))?readable:none;
	//error ("flags = %x, anon = %x\n",flags,MAP_ANON);
	lock();
	if (flags & MAP_ANON) 
		createArea(name,(int)((len+PAGE_SIZE-1)/PAGE_SIZE),&addr, addType ,LAZY,protType);
	else {
		int shareCount=0;
		mmapSharing share;
		if (flags & MAP_SHARED) { share=SHARED;shareCount++;}
		if (flags & MAP_PRIVATE) { share=PRIVATE;shareCount++;}
		if (flags & MAP_COPY){ share=COPY;shareCount++;}
		if (shareCount!=1) 
			addr=NULL;	
		else {
			area *newArea = new (vmBlock->areaPool->get()) area;
			newArea->setup(this);
			//error ("area = %x, start = %x\n",newArea, newArea->getStartAddress());
			newArea->createAreaMappingFile(name,(int)((len+PAGE_SIZE-1)/PAGE_SIZE),&addr,addType,LAZY,protType,fd,offset,share);
			atomic_add(&nextAreaID,1);
			newArea->setAreaID(nextAreaID);
			addArea(newArea);
			newArea->getAreaID();
			//pageMan.dump();
			//newArea->dump();
		}
	}
	unlock();
	return addr;
	}

// Custom area destruction for mapped files
status_t areaManager::munmap(void *addr,size_t len)
		{
		// Note that this is broken for any and all munmaps that are not full area in size. This is an all or nothing game...
		status_t retVal=B_OK;
		lock();
		area *myArea=findArea(addr);
		if (myArea) {
			removeArea(myArea);
			myArea->freeArea();
			vmBlock->areaPool->put(myArea);
			}
		else {
			error ("areaManager::munmap: unable to find requested area\n");
			retVal=B_ERROR;
			}
		unlock();
		return retVal;
		}


long areaManager::get_memory_map(const void *address, ulong numBytes, physical_entry *table, long numEntries) {
	long retVal = B_ERROR; // Be pessimistic
	lock();
	// First, figure out what area we should be talking about...
	area *myArea=findArea(address);
	if (myArea) 
		retVal =  myArea->get_memory_map(address, numBytes,table,numEntries);
	unlock();
	return retVal;
}

long areaManager::lock_memory(void *address, ulong numBytes, ulong flags) {
	long retVal = B_ERROR; // Be pessimistic
	lock();
	// First, figure out what area we should be talking about...
	area *myArea=findArea(address);
	if (myArea) 
		retVal =  myArea->lock_memory(address, numBytes,flags);
	unlock();
	return retVal;
}

long areaManager::unlock_memory(void *address, ulong numBytes, ulong flags) {
	long retVal = B_ERROR; // Be pessimistic
	lock();
	// First, figure out what area we should be talking about...
	area *myArea=findArea(address);
	if (myArea) 
		retVal =  myArea->unlock_memory(address, numBytes,flags);
	unlock();
	return retVal;
}

status_t areaManager::getAreaInfo(int areaID,area_info *dest) {
	status_t retVal;
	lock();
	area *oldArea=findArea(areaID);
	if (oldArea)
		retVal=oldArea->getInfo(dest);
	else
		retVal=B_ERROR;
	unlock();
	return retVal;
}

int areaManager::getAreaByName(char *name) {
	int retVal;
	lock();
	area *oldArea=findArea(name);
	if (oldArea)
		retVal= oldArea->getAreaID();
	else
		retVal= B_ERROR; 
	unlock();
	return retVal;
	}


status_t areaManager::setProtection(int areaID,protectType prot) {
	status_t retVal;
	error ("area::setProtection about to lock\n");
	lock();
	error ("area::setProtection locked\n");
	area *myArea=findArea(areaID);
	if (myArea)
		retVal= myArea->setProtection(prot);
	else
		retVal= B_ERROR;
	unlock();
	error ("area::setProtection unlocked\n");
	return retVal;
}
status_t areaManager::resizeArea(int Area,size_t size) {
	status_t retVal;				
	lock();
	area *oldArea=findArea(Area);
	if (oldArea)
		retVal= oldArea->resize(size);
	else
		retVal= B_ERROR; 
	unlock();
	return retVal;
	}
status_t areaManager::getInfoAfter(int32 & areaID,area_info *dest) {
	status_t retVal;
	lock();
	area *oldArea=findArea(areaID);
	if (oldArea->next)
		{
		area *newCurrent=(reinterpret_cast<area *>(oldArea->next));
		retVal=newCurrent->getInfo(dest);
		areaID=(int)newCurrent;
		}
	else
		retVal=B_ERROR;
	unlock();
	return retVal;
}
