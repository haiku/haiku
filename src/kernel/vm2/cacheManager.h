#include <lockedList.h>
#include <vm.h>
#include <vpage.h>
#include <area.h>
#include <hashTable.h>

struct cacheMember : public node
{
	vnode *vn;
	vpage *vp;
};

class cacheManager : public area
{
	private:
		hashTable cacheMembers; 
		void *findBlock (vnode *target,bool readOnly); 
		void *createBlock (vnode *target,bool readOnly, cacheMember *candidate=NULL); 
		sem_id myLock;
	public:
		// For these two, the VFS passes in the target vnode
		// Return value is the address. Note that the paging daemon does the actual loading
		
		// Constructors and Destructors and related
		cacheManager(void);

		// Mutators
		void *readBlock (vnode *target); 
		void *writeBlock (vnode *target);
		void lock() {acquire_sem(myLock);}
		void unlock() {release_sem(myLock);}

		// External methods for "server" type calls
		void pager(int desperation); // override, as we should blow away useless nodes, not just free blocks.
		void saver(void); // Override - not sure why
};
