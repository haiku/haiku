#include "vmInterface.h"
//#include "areaManager.h"
#include "mman.h"
		
areaManager am;
swapFileManager swapMan;
pageManager pageMan(10); // Obviously this hard coded number is a hack...
	
int32 cleanerThread(void *pageMan)
	{
	pageManager *pm=(pageManager *)pageMan;
	pm->cleaner();
	return 0;
	}

vmInterface::vmInterface(int pages) 
	{
	nextAreaID=0;
	resume_thread(spawn_thread(cleanerThread,"cleanerThread",0,&pageMan));
	}

areaManager *vmInterface::getAM(void)
	{
	// Normally, we would go to the current user process to get this. Since there no such thing exists here...
	return &am;
	}

int vmInterface::getAreaByAddress(void *address)
	{
	area *myArea = getAM()->findArea(address);
	if (myArea)
		return myArea->getAreaID();
	else
		return B_ERROR;
	}

status_t vmInterface::setAreaProtection(int Area,protectType prot)
	{
	area *myArea = getAM()->findArea(Area);
	if (myArea)
		return myArea->setProtection(prot);
	else
		return B_ERROR;					
	}

status_t vmInterface::resizeArea(int Area,size_t size)
	{
	area *oldArea;
	oldArea=getAM()->findArea(Area);	
	if (oldArea)
		return oldArea->resize(size);
	else
		return B_ERROR;					
	}

int vmInterface::createArea(char *AreaName,int pageCount,void **address, addressSpec addType,pageState state,protectType protect)
	{
	area *newArea = new area(getAM());
	newArea->createArea(AreaName,pageCount,address,addType,state,protect);
	newArea->setAreaID(nextAreaID++); // THIS IS NOT  THREAD SAFE
	getAM()->addArea(newArea);
	return newArea->getAreaID();
	}

void vmInterface::freeArea(int Area)
	{
	area *oldArea=getAM()->findArea(Area);	
	getAM()->removeArea(oldArea);
	delete oldArea;
	}

status_t vmInterface::getAreaInfo(int Area,area_info *dest)
	{
	area *oldArea=getAM()->findArea(Area);	
	return oldArea->getInfo(dest);
	}

status_t vmInterface::getNextAreaInfo(int  process,int32 *cookie,area_info *dest)
	// Left for later..
	{
	;
	}

int vmInterface::getAreaByName(char *name)
	{
	return getAM()->findArea(name)->getAreaID();
	}

int vmInterface::cloneArea(int area,char *AreaName,void **address, addressSpec addType=ANY, pageState state=NO_LOCK, protectType prot=writable)
	{
	;
	}

void vmInterface::pager(void)
	{
	// This should iterate over all processes...
	while (1)
		{
		snooze(250000);	
		am.pager(pageMan.desperation());	
		}
	}

void vmInterface::saver(void)
	{
	// This should iterate over all processes...
	while (1)
		{
		snooze(250000);	
		am.saver();	
		}
	}

void vmInterface::cleaner(void)
	{
	// This loops on its own
	pageMan.cleaner();
	}

void *vmInterface::mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
	{
	char name[MAXPATHLEN];
	// Get the filename from fd...
	strcpy(	name,"mmap - need to include fileName");
		
	addressSpec addType=((flags|MAP_FIXED)?EXACT:ANY);
	protectType protType=(prot|PROT_WRITE)?writable:(prot|PROT_READ)?readable:none;
	// Not doing anything with MAP_SHARED and MAP_COPY - needs to be done
	if (flags | MAP_ANON) 
		{
		createArea(name,(int)((len+PAGE_SIZE-1)/PAGE_SIZE),&addr, addType ,LAZY,protType);
		return addr;
		}

	area *newArea = new area(getAM());
	newArea->createAreaMappingFile(name,(int)((len+PAGE_SIZE-1)/PAGE_SIZE),&addr,addType,LAZY,protType,fd,offset);
	newArea->setAreaID(nextAreaID++); // THIS IS NOT  THREAD SAFE
	getAM()->addArea(newArea);
	newArea->getAreaID();
	return addr;
	
	}
