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
	printf ("swapfileManager::swapFileManger: swapfile not opened, errno  = %ul, %s\n",errno,strerror(errno));
	lockFreeList=create_sem(1,"SwapFile Free List Semaphore"); // Should have team name in it.
}

void swapFileManager::write_block(vnode &node,void *loc,unsigned long size)
	{
	//printf ("swapFileManager::write_block: writing, node.fd = %d, node.offset = %d, address = %x\n",node.fd, node.offset,loc);
	if (-1==lseek(node.fd,node.offset,SEEK_SET))
		printf ("seek failed, fd = %d, errno = %d, %s\n",node.fd,errno,strerror(errno));
	if (-1==write(node.fd,loc,size))
		printf ("Write failed, fd = %d, errno = %d, %s\n",node.fd,errno,strerror(errno));
	node.valid=true;
	//printf ("swapFileManager::write_block: done, node.fd = %d, node.offset = %d, address = %x\n",node.fd, node.offset,loc);
	}

void swapFileManager::read_block(vnode &node,void *loc,unsigned long size)
	{
	if (node.valid==false)
		return; // Do nothing. This prevents "garbage" data on disk from being read in...	
	//printf ("swapFileManager::read_block: reading, node.fd = %d, node.offset = %d into %x\n",node.fd, node.offset,loc);
	lseek(node.fd,node.offset,SEEK_SET);
	read(node.fd,loc,size);
	}

vnode &swapFileManager::findNode(void)
	{
	//printf ("swapFileManager::findNode: Entering findNode \n");
	//swapFileFreeList.dump();
	//printf ("swapFileManager::findNode: Finding a new node for you, Master: ");
	vnode *newNode;
	//printf ("locking in sfm\n");
	Lock();
	newNode=reinterpret_cast<vnode *>(swapFileFreeList.next());
	//printf ("unlocking in sfm\n");
	Unlock();
	if (!newNode)
		{
		newNode=new (vmBlock->vnodePool->get()) vnode;
		newNode->fd=swapFile;
		newNode->offset=maxNode+=PAGE_SIZE; 
		//printf (" New One: %d\n",newNode->offset);
		}
	newNode->valid=false;
	newNode->count=0;
	//printf ("swapFileManager::findNode: swapFileFreeList is now: ");
	//swapFileFreeList.dump();
	newNode->count++;
	return *newNode;
	}

void swapFileManager::freeVNode(vnode &v)
	{
	v.count--;
	if (v.count==0)
		{
	//printf ("locking in sfm\n");
		Lock();
		//printf ("swapFileManager::freeNode: Starting Freeing a new node for you, Master: offset:%d\n",v.offset);
		v.valid=false;
		swapFileFreeList.add(&v);
	//printf ("unlocking in sfm\n");
		Unlock();
		}
	}
