#include "swapFileManager.h"
//#include <stdio.h>

swapFileManager::swapFileManager(void)
{
	swapFile = open("/tmp/OBOS_swap",O_RDWR );
}

void swapFileManager::write_block(vnode node,void *loc,unsigned long size)
	{
	lseek(node.fd,SEEK_SET,node.offset);
	write(node.fd,loc,size);
	}

void swapFileManager::read_block(vnode node,void *loc,unsigned long size)
	{
	lseek(node.fd,SEEK_SET,node.offset);
	read(node.fd,loc,size);
	}

vnode swapFileManager::findNode(void)
	{
	vnode tmp;
	tmp.fd=swapFile;
	tmp.offset=maxNode+=PAGE_SIZE; // Can't ever free, swap file grows forever... :-(
	return tmp;
	}
