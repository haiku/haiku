#include "vpagePool.h"

#include "vmHeaderBlock.h"
#include "pageManager.h"
#include "vpage.h"

extern vmHeaderBlock *vmBlock;
vpage *poolvpage::get(void)
	{
	vpage *ret=NULL;
	if (unused.count())
		{
		//error ("poolvpage::get: Getting an unused one!\n");
		acquire_sem(inUse);
		ret=(vpage *)unused.next();
		release_sem(inUse);
		}
	if (ret)
		{
		//error ("poolvpage::get: Returning address:%x \n",ret);
		return ret;
		}
	else
		{
		page *newPage=vmBlock->pageMan->getPage();
		error ("poolvpage::get: Getting new page %lx!\n",newPage->getAddress());
		if (!newPage)
			throw ("Out of pages to allocate a pool!");
		int newCount=PAGE_SIZE/sizeof(vpage);
		acquire_sem(inUse);
		//error ("poolvpage::get: Adding %d new elements to the pool!\n",newCount);
		for (int i=0;i<newCount;i++)
			unused.add(((node *)(newPage->getAddress()+(i*sizeof(vpage)))));	
		release_sem(inUse);
		return (get()); // A little cheat - call self again to get the first one from stack...
		}
	}
