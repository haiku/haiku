#include "pageManager.h"

#include "vmHeaderBlock.h"

extern vmHeaderBlock *vmBlock;
/* This is the template
 * replace TYPE with the type you need a pool for
 *
class poolTYPE
{
	private: 
		list unused;
		sem_id inUse;
	public:
		poolTYPE(void)
			{
			inUse = create_sem(1,"TYPEpool");
			}
		TYPE *get(void)
			{
			TYPE *ret=NULL;
			if (unused.count())
				{
				printf ("poolTYPE::get: Getting an unused one!\n");
				acquire_sem(inUse);
				ret=(TYPE *)unused.next();
				release_sem(inUse);
				}
			if (ret)
				{
				printf ("poolTYPE::get: Returning address:%x \n",ret);
				return ret;
				}
			else
				{
				printf ("poolTYPE::get: Getting a new page!\n");
				page *newPage=vmBlock->pageMan->getPage();
				if (!newPage)
					throw ("Out of pages to allocate a pool!");
				int newCount=PAGE_SIZE/sizeof(TYPE);
				acquire_sem(inUse);
				printf ("poolTYPE::get: Adding %d new elements to the pool!\n",newCount);
				for (int i=0;i<newCount;i++)
					unused.add(((void *)(newPage->getAddress()+(i*sizeof(TYPE)))));	
				release_sem(inUse);
				return (get()); // A little cheat - call self again to get the first one from stack...
				}
			}
		void put(TYPE *in)
			{
			acquire_sem(inUse);
			unused.add(in);
			release_sem(inUse);
			}

};
*/
