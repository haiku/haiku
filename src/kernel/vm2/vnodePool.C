#include "vnodePool.h"

#include "vmHeaderBlock.h"
#include "pageManager.h"

extern vmHeaderBlock *vmBlock;
vnode *poolvnode::get(void) {
	vnode *ret=NULL;
	if (unused.count()) {
		//error ("poolvnode::get: Getting an unused one!\n");
		ret=(vnode *)unused.next();
		}
	if (ret) {
		//error ("poolvnode::get: Returning address:%x \n",ret);
		return ret;
		}
	else {
		page *newPage=vmBlock->pageMan->getPage();
		//error ("poolvnode::get: Getting new page %lx!\n",newPage->getAddress());
		if (!newPage)
			throw ("Out of pages to allocate a pool!");
		int newCount=PAGE_SIZE/sizeof(vnode);
		//error ("poolvnode::get: Adding %d new elements to the pool!\n",newCount);
		for (int i=0;i<newCount;i++)
			unused.add(((node *)(newPage->getAddress()+(i*sizeof(vnode)))));	
		return (get()); // A little cheat - call self again to get the first one from stack...
		}
	}
