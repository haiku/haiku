#include <new.h>
#include "mman.h"
#include "area.h"
#include "areaPool.h"
#include "vpagePool.h"
#include "vnodePool.h"
#include "pageManager.h"
#include "swapFileManager.h"
#include "cacheManager.h"
#define I_AM_VM_INTERFACE
#include "vmHeaderBlock.h"
#include "vmInterface.h"
		
vmHeaderBlock *vmBlock;
static areaManager am;
	
areaManager *getAM(void)
{
	return &am;
}

void *addToPointer(void *ptr,uint32 offset)
{
	return ((void *)(((unsigned long)ptr)+offset));
}

areaManager *vmInterface::getAM(void)
	{
	// Normally, we would go to the current user process to get this. Since there no such thing exists here...
	
	return &am;
	}

int32 cleanerThread(void *pageMan)
	{
	pageManager *pm=(vmBlock->pageMan);
	pm->cleaner();
	return 0;
	}

int32 saverThread(void *areaMan)
	{
	areaManager *am=getAM();
	// This should iterate over all processes...
	while (1)
		{
		snooze(250000);	
		am->saver();	
		}
	}

int32 pagerThread(void *areaMan)
	{
	areaManager *am=getAM();
	while (1)
		{
		snooze(1000000);	
		am->pager(vmBlock->pageMan->desperation());	
		}
	}

vmInterface::vmInterface(int pages) 
	{
	char temp[1000];
	sprintf (temp,"vm_test_clone_%d",getpid());
	if (clone_area(temp,(void **)(&vmBlock),B_ANY_ADDRESS,B_WRITE_AREA,find_area("vm_test"))<0)
		{
		// This is compatability for in BeOS usage only...
		if (0>=create_area("vm_test",(void **)(&vmBlock),B_ANY_ADDRESS,B_PAGE_SIZE*pages,B_NO_LOCK,B_READ_AREA|B_WRITE_AREA))
			{
			printf ("pageManager::pageManager: No memory!\n");
			exit(1);
			}
		//printf ("Allocated an area. Address = %x\n",vmBlock);
		// Figure out how many pages we need
		int pageCount = (sizeof(poolarea)+sizeof(poolvpage)+sizeof(poolvnode)+sizeof(pageManager)+sizeof(swapFileManager)+sizeof(cacheManager)+sizeof(vmHeaderBlock)+PAGE_SIZE-1)/PAGE_SIZE;
		if (pageCount >=pages)
			{
			printf ("Hey! Go buy some ram! Trying to create a VM with fewer pages than the setup will take!\n");
			exit(1);
			}
		//printf ("Need %d pages, creation calls for %d\n",pageCount,pages);
		void *currentAddress = addToPointer(vmBlock,sizeof(struct vmHeaderBlock));
		vmBlock->areaPool = new (currentAddress) poolarea;
		currentAddress=addToPointer(currentAddress,sizeof(poolarea));
		vmBlock->vpagePool = new (currentAddress) poolvpage;
		currentAddress=addToPointer(currentAddress,sizeof(poolvpage));
		vmBlock->vnodePool = new (currentAddress) poolvnode;
		currentAddress=addToPointer(currentAddress,sizeof(poolvnode));
		vmBlock->pageMan = new (currentAddress) pageManager;
		currentAddress=addToPointer(currentAddress,sizeof(pageManager));
		vmBlock->swapMan = new (currentAddress) swapFileManager;
		currentAddress=addToPointer(currentAddress,sizeof(swapFileManager));
		vmBlock->cacheMan = new (currentAddress) cacheManager;
		currentAddress=addToPointer(currentAddress,sizeof(cacheManager));
		//printf ("Need %d pages, creation calls for %d\n",pageCount,pages);
		//printf ("vmBlock is at %x, end of structures is at %x, pageMan called with address %x, pages = %d\n",vmBlock,currentAddress,addToPointer(vmBlock,PAGE_SIZE*pageCount),pages-pageCount);
		vmBlock->pageMan->setup(addToPointer(vmBlock,PAGE_SIZE*pageCount),pages-pageCount);
	    }
	nextAreaID=0;

	resume_thread(spawn_thread(cleanerThread,"cleanerThread",0,(vmBlock->pageMan)));
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
	area *newArea = new (vmBlock->areaPool->get()) area;
	areaManager *foo;
	printf ("vmInterface::createArea - got a new area (%x) from the areaPool\n",newArea);
	foo=getAM();
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
		vmBlock->areaPool->put(oldArea);
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
	area *newArea = new (vmBlock->areaPool->get()) area;
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
		getAM()->pager(vmBlock->pageMan->desperation());	
		}
	}

void vmInterface::saver(void)
	{
	// This should iterate over all processes...
	while (1)
		{
		snooze(250000);	
		getAM()->saver();	
		}
	}

void vmInterface::cleaner(void)
	{
	// This loops on its own
	vmBlock->pageMan->cleaner();
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

	area *newArea = new (vmBlock->areaPool->get()) area;
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
