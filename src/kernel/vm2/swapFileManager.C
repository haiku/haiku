#include "swapFileManager.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <new.h>
#include <vnodePool.h>
#include "vmHeaderBlock.h"

extern vmHeaderBlock *vmBlock;

swapFileManager::swapFileManager(void)
{
	swapFile = open("/boot/var/tmp/OBOS_swap",O_RDWR|O_CREAT,0x777 );
	if (swapFile==-1)
	error ("swapfileManager::swapFileManger: swapfile not opened, errno  = %ul, %s\n",errno,strerror(errno));
	lockFreeList=create_sem(1,"SwapFile Free List Semaphore"); // Should have team name in it.
}

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
	newNode->count=1;
	//error ("swapFileManager::findNode: swapFileFreeList is now: ");
	//swapFileFreeList.dump();
	return *newNode;
	}

void swapFileManager::freeVNode(vnode &v)
	{
	if ( atomic_add(&v.count,-1)==1)
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
