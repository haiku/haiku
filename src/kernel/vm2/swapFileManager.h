#include <unistd.h>
#include <fcntl.h>
#include "vm.h"

class swapFileManager {
	public:
	swapFileManager (void);
	vnode findNode(void); // Get an unused node
	void freeVNode(vnode); // Free a node
	void write_block(vnode node,void *loc,unsigned long size);
	void read_block(vnode node,void *loc,unsigned long size);
	private:
	int swapFile;
	unsigned long maxNode;
};
