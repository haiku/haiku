#include "swapFileManager.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <new.h>
#include <vnodePool.h>
#include "vmHeaderBlock.h"

extern vmHeaderBlock *vmBlock;

// Set up the swap file and make a semaphore to lock it
swapFileManager::swapFileManager(void)
{
	swapFile = open("/boot/var/tmp/OBOS_swap",O_RDWR|O_CREAT,0x777 );
	if (swapFile==-1)
	error ("swapfileManager::swapFileManger: swapfile not opened, errno  = %ul, %s\n",errno,strerror(errno));
	lockFreeList=create_sem(1,"SwapFile Free List Semaphore"); // Should have team name in it.
}

// Try to get a page from the free list. If not, make a new page
vnode &swapFileManager::findNode(void)
	{
	//error ("swapFileManager::findNode: Entering findNode \n");
	//swapFileFreeList.dump();
	//error ("swapFileManager::findNode: Finding a new node for you, Master: ");
	vnode *newNode;
	//error ("locking in sfm\n");
	lock();
	newNode=reinterpret_cast<vnode *>(swapFileFreeList.next());
	//error ("unlocking in sfm\n");
	unlock();
	if (!newNode)
		{
		newNode=new (vmBlock->vnodePool->get()) vnode;
		newNode->fd=swapFile;
		newNode->offset=maxNode+=PAGE_SIZE; 
		//error (" New One: %d\n",newNode->offset);
		}
	newNode->valid=false;
	//error ("swapFileManager::findNode: swapFileFreeList is now: ");
	//swapFileFreeList.dump();
	return *newNode;
	}

// Add this page to the free list.
void swapFileManager::freeVNode(vnode &v)
	{
	if (!v.vpages.count())
		{
	//error ("locking in sfm\n");
		lock();
		//error ("swapFileManager::freeNode: Starting Freeing a new node for you, Master: offset:%d\n",v.offset);
		v.valid=false;
		swapFileFreeList.add(&v);
	//error ("unlocking in sfm\n");
		unlock();
		}
	}
