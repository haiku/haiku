#include <vnodeManager.h>

vpage *vnodeManager::findVnode(vnode &target)
{
	managedVnode *found=vnodes.find(&target);
	if (found==NULL)
		return NULL;
	else
		return found->pages.peek();
}

void vnodeManager::addVnode(vnode &target,vpage &vp)
{
	// Allocate space for a managed vnode
	managedVnode *mv = NULL; // Fill this in later - maybe another pool?
	mv->node=&target;
	mv->pages.add(&vp);
}
