#include "pageManager.h"

extern pageManager pageMan;
class poolvnode
{
	private: 
		list unused;
		sem_id inUse;
	public:
		poolvnode(void)
			{
			inUse = create_sem(1,"vnodepool");
			}
		vnode *get(void)
			{
			vnode *ret=NULL;
			if (unused.count())
				{
				//printf ("poolvnode::get: Getting an unused one!\n");
				acquire_sem(inUse);
				ret=(vnode *)unused.next();
				release_sem(inUse);
				}
			if (ret)
				{
				//printf ("poolvnode::get: Returning address:%x \n",ret);
				return ret;
				}
			else
				{
				//printf ("poolvnode::get: Getting a new page!\n");
				page *newPage=pageMan.getPage();
				if (!newPage)
					throw ("Out of pages to allocate a pool!");
				int newCount=PAGE_SIZE/sizeof(vnode);
				acquire_sem(inUse);
				//printf ("poolvnode::get: Adding %d new elements to the pool!\n",newCount);
				for (int i=0;i<newCount;i++)
					unused.add(((void *)(newPage->getAddress()+(i*sizeof(vnode)))));	
				release_sem(inUse);
				return (get()); // A little cheat - call self again to get the first one from stack...
				}
			}
		void put(vnode *in)
			{
			acquire_sem(inUse);
			unused.add(in);
			release_sem(inUse);
			}

};
