#include "pageManager.h"

extern pageManager pageMan;
class poolarea
{
	private: 
		list unused;
		sem_id inUse;
	public:
		poolarea(void)
			{
			inUse = create_sem(1,"areapool");
			}
		area *get(void)
			{
			area *ret=NULL;
			if (unused.count())
				{
				//printf ("poolarea::get: Getting an unused one!\n");
				acquire_sem(inUse);
				ret=(area *)unused.next();
				release_sem(inUse);
				}
			if (ret)
				{
				//printf ("poolarea::get: Returning address:%x \n",ret);
				return ret;
				}
			else
				{
				//printf ("poolarea::get: Getting a new page!\n");
				page *newPage=pageMan.getPage();
				if (!newPage)
					throw ("Out of pages to allocate a pool!");
				int newCount=PAGE_SIZE/sizeof(area);
				acquire_sem(inUse);
				//printf ("poolarea::get: Adding %d new elements to the pool!\n",newCount);
				for (int i=0;i<newCount;i++)
					unused.add(((void *)(newPage->getAddress()+(i*sizeof(area)))));	
				release_sem(inUse);
				return (get()); // A little cheat - call self again to get the first one from stack...
				}
			}
		void put(area *in)
			{
			acquire_sem(inUse);
			unused.add(in);
			release_sem(inUse);
			}

};
