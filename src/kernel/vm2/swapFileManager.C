#include "swapFileManager.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

swapFileManager::swapFileManager(void)
{
	swapFile = open("/boot/var/tmp/OBOS_swap",O_RDWR|O_CREAT,0x777 );
	if (swapFile==-1)
	printf ("swapfileManager::swapFileManger: swapfile not opened, errno  = %ul, %s\n",errno,strerror(errno));
	lockFreeList=create_sem(1,"SwapFile Free List Semaphore"); // Should have team name in it.
}

void swapFileManager::write_block(vnode &node,void *loc,unsigned long size)
	{
	printf ("swapFileManager::write_block: writing, node.fd = %d, node.offset = %d\n",node.fd, node.offset);
	lseek(node.fd,SEEK_SET,node.offset);
	write(node.fd,loc,size);
	node.valid=true;
	}

void swapFileManager::read_block(vnode &node,void *loc,unsigned long size)
	{
	lseek(node.fd,SEEK_SET,node.offset);
	if (node.valid==false)
		return; // Do nothing. This prevents "garbage" data on disk from being read in...	
	printf ("swapFileManager::read_block: reading, node.fd = %d, node.offset = %d\n",node.fd, node.offset);
	read(node.fd,loc,size);
	}

vnode &swapFileManager::findNode(void)
	{
	//printf ("swapFileManager::findNode: Entering findNode \n");
	//swapFileFreeList.dump();
	//printf ("swapFileManager::findNode: Finding a new node for you, Master: ");
	vnode *newNode;
	Lock();
	if (newNode=reinterpret_cast<vnode *>(swapFileFreeList.next()))
		{
		//printf (" Reused: %d\n",newNode->offset);
		}
	else
		{
		newNode=new vnode;
		newNode->fd=swapFile;
		newNode->offset=maxNode+=PAGE_SIZE; 
		newNode->valid=false;
		//printf (" New One: %d\n",newNode->offset);
		}
	Unlock();
	//printf ("swapFileManager::findNode: swapFileFreeList is now: ");
	//swapFileFreeList.dump();
	return *newNode;
	}

void swapFileManager::freeVNode(vnode &v)
	{
	Lock();
	//printf ("swapFileManager::freeNode: Starting Freeing a new node for you, Master: offset:%d\n",v.offset);
	v.valid=false;
	swapFileFreeList.add(&v);
	Unlock();
	}
