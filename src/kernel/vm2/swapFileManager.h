#ifndef _SWAPFILE_MANAGER
#define _SWAPFILE_MANAGER
#include <unistd.h>
#include <fcntl.h>
#include "vm.h"
#include "OS.h"
#include "lockedList.h"

class swapFileManager {
	private:
	int swapFile;
	unsigned long maxNode;
	lockedList swapFileFreeList;

	public:
		// Constructors and Destructors and related
	swapFileManager (void);
	void freeVNode(vnode &); // Free a node

		// Mutators
	vnode &findNode(void); // Get an unused node
	void write_block(vnode &node,void *loc,unsigned long size); // The general access points
	void read_block(vnode &node,void *loc,unsigned long size);

		// Accessors
	int getFD(void) {return swapFile;}
};
#endif
