#include "areaManager.h"

areaManager::areaManager(void)
{
	team=0; // should be proc_get_current_proc_id()
	myLock=create_sem(1,"Area Manager Semaphore"); // Should have team name in it.
}

unsigned long areaManager::getNextAddress(int pages, unsigned long start)
{
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

area *areaManager::findArea(char *address)
{
	for (struct node *cur=areas.rock;cur;cur=cur->next)
		{
		area *myArea=(area *)cur;
		if (myArea->nameMatch(address))
			return myArea;
		}
	return NULL;
}

area *areaManager::findArea(void *address)
{
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

area *areaManager::findArea(area_id id)
{
	for (struct node *cur=areas.rock;cur;cur=cur->next)
		{
		area *myArea=(area *)cur;
		if (myArea->getAreaID()==id)
				return myArea;
		}
	return NULL;
}

bool areaManager::fault(void *fault_address, bool writeError) // true = OK, false = panic.
{
	area *myArea;
	if (myArea=findArea(fault_address))
		return myArea->fault(fault_address,writeError);	
	else
		return false;
}

char areaManager::getByte(unsigned long address)
{
	area *myArea;
	if (myArea=findArea((void *)address))
		return myArea->getByte(address);	
	else
		return 0;
}

int areaManager::getInt(unsigned long address)
{
	area *myArea;
	if (myArea=findArea((void *)address))
		return myArea->getInt(address);	
	else
		return 0;
}

void areaManager::setByte(unsigned long offset,char value)
{
	area *myArea;
	if (myArea=findArea((void *)offset))
		myArea->setByte(offset,value);	
}

void areaManager::setInt(unsigned long offset,int value)
{
	area *myArea;
	if (myArea=findArea((void *)offset))
		myArea->setInt(offset,value);	
}

void areaManager::pager(int desperation)
{
	for (struct node *cur=areas.rock;cur;cur=cur->next)
		{
		area *myArea=(area *)cur;
		myArea->pager(desperation);
		}
}

void areaManager::saver(void)
{
	for (struct node *cur=areas.rock;cur;cur=cur->next)
		{
		area *myArea=(area *)cur;
		myArea->saver();
		}
}

