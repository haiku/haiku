#include "areaPool.h"
#include "area.h"
#include "page.h"
#include "vmHeaderBlock.h"
#include "pageManager.h"

extern vmHeaderBlock *vmBlock;

area *poolarea::get(void)
	{
	area *ret=NULL;
	if (unused.count())
		{
		//error ("poolarea::get: Getting an unused one!\n");
		acquire_sem(inUse);
		ret=(area *)unused.next();
		release_sem(inUse);
		}
	if (ret)
		{
		//error ("poolarea::get: Returning address:%x \n",ret);
		return ret;
		}
	else
		{
		page *newPage=vmBlock->pageMan->getPage();
		error ("poolarea::get: Getting new page %lx!\n",newPage->getAddress());
		if (!newPage)
			throw ("Out of pages to allocate a pool!");
		int newCount=PAGE_SIZE/sizeof(area);
		acquire_sem(inUse);
		//error ("poolarea::get: Adding %d new elements to the pool!\n",newCount);
		for (int i=0;i<newCount;i++)
			unused.add(((node *)(newPage->getAddress()+(i*sizeof(area)))));	
		release_sem(inUse);
		return (get()); // A little cheat - call self again to get the first one from stack...
		}
	}
