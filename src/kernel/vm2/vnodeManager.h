#include <hashTable.h>

// vnode manager tracks which vnodes are already mapped to pages
// and facilitates sharing of memory containing the same disk space.

struct managedVnode
{
	vnode *node;
	list pages; // Hold the list of vpages that this vnode points to
};

ulong mnHash (node &vnodule) { return ((managedNode &)vnodule).node->fd; }
bool mnIsEqual (node &vnodule1,&vnodule2) {
		managedNode &v1= ((managedNode &)vnodule1); 
		managedNode &v2= ((managedNode &)vnodule2); 
		return v1.node->fd==v2.node->fd && v1.node->offset==v2.node->offset;
}

class vnodeManager
{
	public:
		vnodeManager(void) : vnodes(20)
		{
			vnodes.setHash(mnHash);
			vnodes.setIsEqual(mnIsEqual);
		}

		vpage *findVnode(vnode &target); // pass in a vnode, get back the "master" vpage
		void addVnode (vnode &target, vpage &vp);

	private:
		hashTable vnodes;
}
