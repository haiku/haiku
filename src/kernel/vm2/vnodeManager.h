#ifndef VNODE_MANAGER
#define VNODE_MANAGER
#include <vpage.h>
#include <hashTable.h>

// vnode manager tracks which vnodes are already mapped to pages
// and facilitates sharing of memory containing the same disk space.

class vnodeManager
{
	public:
		vnodeManager(void);
		vpage *findVnode(vnode &target); // pass in a vnode, get back the "master" vpage
		vpage *addVnode (vnode &target, vpage &vp,vnode **retOne);
		vpage *addVnode(vnode *target,vpage &vp);
		bool remove(vnode &target,vpage &vp);
		void dump(void);
	private:
		hashTable vnodes;
};
#endif
