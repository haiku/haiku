#ifndef _SWAPFILE_MANAGER
#define _SWAPFILE_MANAGER
#include <unistd.h>
#include <fcntl.h>
#include "vm.h"
#include "OS.h"

class swapFileManager {
	private:
	int swapFile;
	unsigned long maxNode;
	list swapFileFreeList;
	sem_id lockFreeList;

	public:
	swapFileManager (void);
	vnode &findNode(void); // Get an unused node
	void freeVNode(vnode &); // Free a node
	void write_block(vnode &node,void *loc,unsigned long size);
	void read_block(vnode &node,void *loc,unsigned long size);
	void Lock() {acquire_sem(lockFreeList);}
	void Unlock() {release_sem(lockFreeList);}
};
#endif
