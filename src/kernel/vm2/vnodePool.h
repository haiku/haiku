#include <OS.h>
#include "list.h"
class area;
class vnode;
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
		vnode *get(void);
		void put(vnode *in)
			{
			acquire_sem(inUse);
			unused.add(in);
			release_sem(inUse);
			}

};
