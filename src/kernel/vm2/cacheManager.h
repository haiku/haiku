#include <list.h>
#include <vm.h>
#include <vpage.h>
#include <area.h>
#include <olist.h>

struct cacheMember : public node
{
	vnode *vn;
	vpage *vp;
};

class cacheManager : public area
{
	private:
		orderedList cacheMembers; // Yes, this is slow and should be a hash table. This should be done prior to 
		// moving into the kernel, so we can test it better.
		// While this very much mirrors the area's vpage list, it won't when it is a hash table...
		void *findBlock (vnode *target,bool readOnly); 
		void *createBlock (vnode *target,bool readOnly); 
		sem_id myLock;
	public:
		// For these two, the VFS passes in the target vnode
		// Return value is the address. Note that the paging daemon does the actual loading
		cacheManager(void);
		void *readBlock (vnode *target); 
		void *writeBlock (vnode *target);
		void pager(int desperation); // override, as we should blow away useless nodes, not just free blocks.
		void saver(void); // Override - not sure why
		void lock() {acquire_sem(myLock);}
		void unlock() {release_sem(myLock);}
};
