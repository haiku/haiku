#include "swapFileManager.h"
#include <stdio.h>

swapFileManager::swapFileManager(void)
{
	swapFile = open("/boot/var/tmp/OBOS_swap",O_RDWR );
}

void swapFileManager::write_block(vnode node,void *loc,unsigned long size)
	{
	printf ("writing, node.fd = %d, node.offset = %d\n",node.fd, node.offset);
	lseek(node.fd,SEEK_SET,node.offset);
	write(node.fd,loc,size);
	}

void swapFileManager::read_block(vnode node,void *loc,unsigned long size)
	{
	printf ("reading, node.fd = %d, node.offset = %d\n",node.fd, node.offset);
	lseek(node.fd,SEEK_SET,node.offset);
	read(node.fd,loc,size);
	}

vnode swapFileManager::findNode(void)
	{
	printf ("Finding a new node for you master\n");
	vnode tmp;
	tmp.fd=swapFile;
	tmp.offset=maxNode+=PAGE_SIZE; // Can't ever free, swap file grows forever... :-(
	return tmp;
	}
