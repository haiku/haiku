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
	
// The purpose of this interface is to validate options, translate where necessary and pass through to the areaManager.

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
	sprintf (temp,"vm_test_clone_%ld",getpid());
	if (clone_area(temp,(void **)(&vmBlock),B_ANY_ADDRESS,B_WRITE_AREA,find_area("vm_test"))<0)
		{
		// This is compatability for in BeOS usage only...
		if (0>=create_area("vm_test",(void **)(&vmBlock),B_ANY_ADDRESS,B_PAGE_SIZE*pages,B_NO_LOCK,B_READ_AREA|B_WRITE_AREA))
			{
			error ("pageManager::pageManager: No memory!\n");
			exit(1);
			}
		//error ("Allocated an area. Address = %x\n",vmBlock);
		// Figure out how many pages we need
		int pageCount = (sizeof(poolarea)+sizeof(poolvpage)+sizeof(poolvnode)+sizeof(pageManager)+sizeof(swapFileManager)+sizeof(cacheManager)+sizeof(vmHeaderBlock)+PAGE_SIZE-1)/PAGE_SIZE;
		if (pageCount >=pages)
			{
			error ("Hey! Go buy some ram! Trying to create a VM with fewer pages than the setup will take!\n");
			exit(1);
			}
		//error ("Need %d pages, creation calls for %d\n",pageCount,pages);
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
		//error ("Need %d pages, creation calls for %d\n",pageCount,pages);
		//error ("vmBlock is at %x, end of structures is at %x, pageMan called with address %x, pages = %d\n",vmBlock,currentAddress,addToPointer(vmBlock,PAGE_SIZE*pageCount),pages-pageCount);
		vmBlock->pageMan->setup(addToPointer(vmBlock,PAGE_SIZE*pageCount),pages-pageCount);
	    }

	resume_thread(tid_cleaner=spawn_thread(cleanerThread,"cleanerThread",0,(vmBlock->pageMan)));
	resume_thread(tid_saver=spawn_thread(saverThread,"saverThread",0,getAM()));
	resume_thread(tid_pager=spawn_thread(pagerThread,"pagerThread",0,getAM()));
	}

int vmInterface::getAreaByAddress(void *address)
	{
	int retVal;
	area *myArea = getAM()->findAreaLock(address);
	if (myArea)
		retVal= myArea->getAreaID();
	else
		retVal= B_ERROR;
	return retVal;
	}

status_t vmInterface::setAreaProtection(int Area,protectType prot)
	{
	status_t retVal;
	retVal= getAM()->setProtection(Area,prot);
	return retVal;
	}

status_t vmInterface::resizeArea(int Area,size_t size)
	{
	status_t retVal;
	retVal = getAM()->resizeArea(Area,size);
	return retVal;
	}

int vmInterface::createArea(char *AreaName,int pageCount,void **address, addressSpec addType,pageState state,protectType protect)
	{
	int retVal;
	error ("vmInterface::createArea: Creating an area!\n");
	retVal = getAM()->createArea(AreaName,pageCount,address,addType,state,protect);
	error ("vmInterface::createArea: Done creating an area!\n");
	return retVal;
	}

void vmInterface::freeArea(int area)
	{
	getAM()->freeArea(area);
	}

status_t vmInterface::getAreaInfo(int Area,area_info *dest)
	{
	status_t retVal;
	error ("vmInterface::getAreaInfo: Getting info about an area!\n");
	retVal = getAM()->getAreaInfo(Area,dest);
	error ("vmInterface::getAreaInfo: Done getting info about an area!\n");
	return retVal;
	}

status_t vmInterface::getNextAreaInfo(int  process,int32 *cookie,area_info *dest)
	{
	status_t retVal; 
	// We *SHOULD* be getting the AM for this process. Something for HW integration time...
	retVal = getAM()->getInfoAfter(*cookie,dest);
	return retVal;
	}

int vmInterface::getAreaByName(char *name)
	{
	int retVal;
	retVal = getAM()->getAreaByName(name);
	return retVal;
	}

int vmInterface::cloneArea(int newAreaID,char *AreaName,void **address, addressSpec addType=ANY, pageState state=NO_LOCK, protectType prot=writable)
	{
	int retVal;
	retVal = getAM()->cloneArea(newAreaID,AreaName,address, addType, state, prot);
	return retVal;
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
	void *retVal;
	retVal = getAM()->mmap(addr,len,prot,flags,fd,offset);
	return retVal;
	}

status_t vmInterface::munmap(void *addr, size_t len)
{
	int retVal;
	retVal = getAM()->munmap(addr,len);
	return retVal;
}

void vmInterface::suspendAll(void)
{
	suspend_thread(tid_cleaner);
	suspend_thread(tid_saver);
	suspend_thread(tid_pager);
}
