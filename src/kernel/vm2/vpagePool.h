#include "pageManager.h"
#include "vpage.h"

extern pageManager pageMan;
class poolvpage
{
	private: 
		list unused;
		sem_id inUse;
	public:
		poolvpage(void)
			{
			inUse = create_sem(1,"vpagepool");
			}
		vpage *get(void)
			{
			vpage *ret=NULL;
			if (unused.count())
				{
				//printf ("poolvpage::get: Getting an unused one!\n");
				acquire_sem(inUse);
				ret=(vpage *)unused.next();
				release_sem(inUse);
				}
			if (ret)
				{
				//printf ("poolvpage::get: Returning address:%x \n",ret);
				return ret;
				}
			else
				{
				//printf ("poolvpage::get: Getting a new page!\n");
				page *newPage=pageMan.getPage();
				if (!newPage)
					throw ("Out of pages to allocate a pool!");
				int newCount=PAGE_SIZE/sizeof(vpage);
				acquire_sem(inUse);
				//printf ("poolvpage::get: Adding %d new elements to the pool!\n",newCount);
				for (int i=0;i<newCount;i++)
					unused.add(((void *)(newPage->getAddress()+(i*sizeof(vpage)))));	
				release_sem(inUse);
				return (get()); // A little cheat - call self again to get the first one from stack...
				}
			}
		void put(vpage *in)
			{
			acquire_sem(inUse);
			unused.add(in);
			release_sem(inUse);
			}

};
