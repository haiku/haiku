#include "vnodePool.h"

#include "vmHeaderBlock.h"
#include "pageManager.h"

extern vmHeaderBlock *vmBlock;
vnode *poolvnode::get(void)
	{
	vnode *ret=NULL;
	if (unused.count())
		{
		//error ("poolvnode::get: Getting an unused one!\n");
		acquire_sem(inUse);
		ret=(vnode *)unused.next();
		release_sem(inUse);
		}
	if (ret)
		{
		//error ("poolvnode::get: Returning address:%x \n",ret);
		return ret;
		}
	else
		{
		page *newPage=vmBlock->pageMan->getPage();
		error ("poolvnode::get: Getting new page %lx!\n",newPage->getAddress());
		if (!newPage)
			throw ("Out of pages to allocate a pool!");
		int newCount=PAGE_SIZE/sizeof(vnode);
		acquire_sem(inUse);
		//error ("poolvnode::get: Adding %d new elements to the pool!\n",newCount);
		for (int i=0;i<newCount;i++)
			unused.add(((node *)(newPage->getAddress()+(i*sizeof(vnode)))));	
		release_sem(inUse);
		return (get()); // A little cheat - call self again to get the first one from stack...
		}
	}
