#include <vnodeManager.h>
#include <vnodePool.h>

// Functions for hash and isEqual
ulong mnHash (node &vnodule) { return ((vnode &)vnodule).fd; }

bool mnIsEqual (node &vnodule1,node &vnodule2) {
		vnode &v1= ((vnode &)vnodule1); 
		vnode &v2= ((vnode &)vnodule2); 
		return v1.fd==v2.fd && v1.offset==v2.offset;
}

// Set the hash and isEqual functions
vnodeManager::vnodeManager(void) : vnodes(20) {
		vnodes.setHash(mnHash);
		vnodes.setIsEqual(mnIsEqual);
		}

// Find the vnode and return the first page that points to it.
vpage *vnodeManager::findVnode(vnode &target) {
	vnode *found=reinterpret_cast<vnode *>(vnodes.find(&target));
	if (found==NULL)
		return NULL;
	else
		return reinterpret_cast<vpage *>(found->vpages.top());
}

// If this vnode is already in use, add this vpage to it and return one to clone. If not, add this vnode, with this vpage, and return null
// This method will make a new vnode object
vpage *vnodeManager::addVnode(vnode &target,vpage &vp) {
	vpage *retVal;
	error ("vnodeManager::addVnode : Adding by reference node %x, fd = %d, offset = %d\n",&target,target.fd,target.offset);
	vnode *found=reinterpret_cast<vnode *>(vnodes.find(&target));
	if (!found)	{
		found=new (vmBlock->vnodePool->get()) vnode;  
		found->fd=target.fd;
		found->offset=target.offset;
		vnodes.add(found);
		retVal=NULL;
		}
	else
		retVal=reinterpret_cast<vpage *>(found->vpages.top());
	found->vpages.add(&vp);
	return retVal;
}

// If this vnode is already in use, add this vpage to it and return one to clone. If not, add this vnode, with this vpage, and return null
// This method will NOT make a new vnode object
vpage *vnodeManager::addVnode(vnode *target,vpage &vp) {
	vpage *retVal;
	error ("vnodeManager::addVnode : Adding by pointer node %x, fd = %d, offset = %d\n",target,target->fd,target->offset);
	vnode *found=reinterpret_cast<vnode *>(vnodes.find(target));
	if (!found)	{
		found=target;
		vnodes.add(found);
		retVal=NULL;
	}
	else
		retVal=reinterpret_cast<vpage *>(found->vpages.top());
	found->vpages.add(&vp);
	found->vpages.dump();
	vnodes.dump();
}

// Remove a vpage from the manager; return "is this the last one"
bool vnodeManager::remove(vnode &target,vpage &vp) {
	error ("vnodeManager::remove : Removing by reference node %x, fd = %d, offset = %d\n",&target,target.fd,target.offset);
	vnode *found=reinterpret_cast<vnode *>(vnodes.find(&target));
	if (!found) {
		vnodes.dump();
		throw ("An attempt to remove from an unknown vnode occured!\n");
	}
	found->vpages.remove(&vp);
	return (found->vpages.count()==0);
}
