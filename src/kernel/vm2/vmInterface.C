#include "vmInterface.h"
//#include "areaManager.h"
#include "mman.h"
#include "area.h"
#include "areaPool.h"
#include "vpagePool.h"
#include "vnodePool.h"
		
areaManager am;
swapFileManager swapMan;
poolarea areaPool;
poolvpage vpagePool;
poolvnode vnodePool;
pageManager pageMan(30); // Obviously this hard coded number is a hack...
	
areaManager *vmInterface::getAM(void)
	{
	// Normally, we would go to the current user process to get this. Since there no such thing exists here...
	return &am;
	}

int32 cleanerThread(void *pageMan)
	{
	pageManager *pm=(pageManager *)pageMan;
	pm->cleaner();
	return 0;
	}

int32 saverThread(void *areaMan)
	{
	areaManager *am=(areaManager *)areaMan;
	// This should iterate over all processes...
	while (1)
		{
		snooze(250000);	
		am->saver();	
		}
	}

int32 pagerThread(void *areaMan)
	{
	areaManager *am=(areaManager *)areaMan;
	while (1)
		{
		snooze(1000000);	
		am->pager(pageMan.desperation());	
		}
	}

vmInterface::vmInterface(int pages) 
	{
	nextAreaID=0;
	resume_thread(spawn_thread(cleanerThread,"cleanerThread",0,&pageMan));
	resume_thread(spawn_thread(saverThread,"saverThread",0,getAM()));
	resume_thread(spawn_thread(pagerThread,"pagerThread",0,getAM()));
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
	area *newArea = areaPool.get();
	newArea->setup(getAM());
	newArea->createArea(AreaName,pageCount,address,addType,state,protect);
	newArea->setAreaID(nextAreaID++); // THIS IS NOT  THREAD SAFE
	getAM()->addArea(newArea);
	return newArea->getAreaID();
	}

void vmInterface::freeArea(int Area)
	{
	//printf ("vmInterface::freeArea: begin\n");
	area *oldArea=getAM()->findArea(Area);	
	//printf ("vmInterface::freeArea: found area %x\n",oldArea);
	if (oldArea)
		{
//		printf ("vmInterface::freeArea: removing area %x from linked list\n",oldArea);
		areaManager *manager=getAM();
//		printf ("vmInterface::freeArea: areaManager =  %x \n",manager);
		manager->removeArea(oldArea);
//		printf ("vmInterface::freeArea: deleting area %x \n",oldArea);
		oldArea->freeArea();
//		printf ("vmInterface::freeArea: freeArea complete \n");
		areaPool.put(oldArea);
		}
	else
		printf ("vmInterface::freeArea: unable to find requested area\n");
	}

status_t vmInterface::getAreaInfo(int Area,area_info *dest)
	{
	area *oldArea=getAM()->findArea(Area);	
	if (oldArea)
		return oldArea->getInfo(dest);
	else
		printf ("vmInterface::getAreaInfo: unable to find requested area\n");
	}

status_t vmInterface::getNextAreaInfo(int  process,int32 *cookie,area_info *dest)
	{
	area *oldArea=getAM()->findArea(*cookie);	
	area *newArea=(area *)(oldArea->next);
	if (newArea)
		return newArea->getInfo(dest);
	else
		return B_BAD_VALUE;
	}

int vmInterface::getAreaByName(char *name)
	{
	return getAM()->findArea(name)->getAreaID();
	}

int vmInterface::cloneArea(int newAreaID,char *AreaName,void **address, addressSpec addType=ANY, pageState state=NO_LOCK, protectType prot=writable)
	{
	area *newArea = areaPool.get();
	newArea->setup(getAM());
	area *oldArea=getAM()->findArea(newAreaID);
	newArea->cloneArea(oldArea,AreaName,address,addType,state,prot);
	newArea->setAreaID(nextAreaID++); // THIS IS NOT  THREAD SAFE
	getAM()->addArea(newArea);
	return newArea->getAreaID();
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
		
	addressSpec addType=((flags&MAP_FIXED)?EXACT:ANY);
	protectType protType=(prot&PROT_WRITE)?writable:(prot&PROT_READ)?readable:none;
	// Not doing anything with MAP_SHARED and MAP_COPY - needs to be done
	//printf ("flags = %x, anon = %x\n",flags,MAP_ANON);
	if (flags & MAP_ANON) 
		{
		createArea(name,(int)((len+PAGE_SIZE-1)/PAGE_SIZE),&addr, addType ,LAZY,protType);
		return addr;
		}

	area *newArea = areaPool.get();
	newArea->setup(getAM());
	//printf ("area = %x, start = %x\n",newArea, newArea->getStartAddress());
	newArea->createAreaMappingFile(name,(int)((len+PAGE_SIZE-1)/PAGE_SIZE),&addr,addType,LAZY,protType,fd,offset);
	newArea->setAreaID(nextAreaID++); // THIS IS NOT  THREAD SAFE
	getAM()->addArea(newArea);
	newArea->getAreaID();
	//pageMan.dump();
	//newArea->dump();
	return addr;
	}

status_t vmInterface::munmap(void *addr, size_t len)
{
	// Note that this is broken for any and all munmaps that are not full area in size. This is an all or nothing game...
	int area=getAreaByAddress(addr);
	freeArea(area);	
	//pageMan.dump();
}
