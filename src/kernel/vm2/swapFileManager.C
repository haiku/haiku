#include "swapFileManager.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

swapFileManager::swapFileManager(void)
{
	swapFile = open("/boot/var/tmp/OBOS_swap",O_RDWR|O_CREAT,0x777 );
	if (swapFile==-1)
	printf ("swapfileManager::swapFileManger: swapfile not opened, errno  = %ul, %s\n",errno,strerror(errno));
}

void swapFileManager::write_block(vnode node,void *loc,unsigned long size)
	{
	printf ("swapFileManager::write_block: writing, node.fd = %d, node.offset = %d\n",node.fd, node.offset);
	lseek(node.fd,SEEK_SET,node.offset);
	write(node.fd,loc,size);
	node.valid=true;
	}

void swapFileManager::read_block(vnode node,void *loc,unsigned long size)
	{
	lseek(node.fd,SEEK_SET,node.offset);
	if (node.valid==false)
		return; // Do nothing. This prevents "garbage" data on disk from being read in...	
	printf ("swapFileManager::read_block: reading, node.fd = %d, node.offset = %d\n",node.fd, node.offset);
	read(node.fd,loc,size);
	}

vnode swapFileManager::findNode(void)
	{
	printf ("swapFileManager::findNode: Finding a new node for you, Master\n");
	vnode tmp;
	tmp.fd=swapFile;
	tmp.offset=maxNode+=PAGE_SIZE; // Can't ever free, swap file grows forever... :-(
	tmp.valid=false;
	return tmp;
	}

void swapFileManager::freeVNode(vnode v)
	{
	printf ("swapFileManager::freeNode: Freeing a new node for you, Master\n");
	// Should put this one on the free list, someday
	}
