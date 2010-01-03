/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNUSED_VNODES_H
#define UNUSED_VNODES_H


#include <algorithm>

#include <util/AutoLock.h>
#include <util/list.h>

#include <low_resource_manager.h>

#include "Vnode.h"


const static uint32 kMaxUnusedVnodes = 8192;
	// This is the maximum number of unused vnodes that the system
	// will keep around (weak limit, if there is enough memory left,
	// they won't get flushed even when hitting that limit).
	// It may be chosen with respect to the available memory or enhanced
	// by some timestamp/frequency heurism.


/*!	\brief Guards sUnusedVnodeList and sUnusedVnodes.

	Innermost lock. Must not be held when acquiring any other lock.
*/
static mutex sUnusedVnodesLock = MUTEX_INITIALIZER("unused vnodes");
static list sUnusedVnodeList;
static vuint32 sUnusedVnodes = 0;

static const int32 kMaxHotVnodes = 1024;
static rw_lock sHotVnodesLock = RW_LOCK_INITIALIZER("hot vnodes");
static Vnode* sHotVnodes[kMaxHotVnodes];
static vint32 sNextHotVnodeIndex = 0;

static const int32 kUnusedVnodesCheckInterval = 64;
static vint32 sUnusedVnodesCheckCount = 0;


/*!	Must be called with sHotVnodesLock write-locked.
*/
static void
flush_hot_vnodes_locked()
{
	MutexLocker unusedLocker(sUnusedVnodesLock);

	int32 count = std::min((int32)sNextHotVnodeIndex, kMaxHotVnodes);
	for (int32 i = 0; i < count; i++) {
		Vnode* vnode = sHotVnodes[i];
		if (vnode == NULL)
			continue;

		if (vnode->IsHot()) {
			if (vnode->IsUnused()) {
				list_add_item(&sUnusedVnodeList, vnode);
				sUnusedVnodes++;
			}
			vnode->SetHot(false);
		}

		sHotVnodes[i] = NULL;
	}

	unusedLocker.Unlock();

	sNextHotVnodeIndex = 0;
}



/*!	To be called when the vnode's ref count drops to 0.
	Must be called with sVnodeLock at least read-locked and the vnode locked.
	\param vnode The vnode.
	\return \c true, if the caller should trigger unused vnode freeing.
*/
static bool
vnode_unused(Vnode* vnode)
{
	ReadLocker hotReadLocker(sHotVnodesLock);

	vnode->SetUnused(true);

	bool result = false;
	int32 checkCount = atomic_add(&sUnusedVnodesCheckCount, 1);
	if (checkCount == kUnusedVnodesCheckInterval) {
		uint32 unusedCount = sUnusedVnodes;
		if (unusedCount > kMaxUnusedVnodes
			&& low_resource_state(
				B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY)
					!= B_NO_LOW_RESOURCE) {
			// there are too many unused vnodes -- tell the caller to free the
			// oldest ones
			result = true;
		} else {
			// nothing urgent -- reset the counter and re-check then
			atomic_set(&sUnusedVnodesCheckCount, 0);
		}
	}

	// nothing to do, if the node is already hot
	if (vnode->IsHot())
		return result;

	// no -- enter it
	int32 index = atomic_add(&sNextHotVnodeIndex, 1);
	if (index < kMaxHotVnodes) {
		vnode->SetHot(true);
		sHotVnodes[index] = vnode;
		return result;
	}

	// the array is full -- it has to be emptied
	hotReadLocker.Unlock();
	WriteLocker hotWriteLocker(sHotVnodesLock);

	// unless someone was faster than we were, we have to flush the array
	if (sNextHotVnodeIndex >= kMaxHotVnodes)
		flush_hot_vnodes_locked();

	// enter the vnode
	index = sNextHotVnodeIndex++;
	vnode->SetHot(true);
	sHotVnodes[index] = vnode;

	return result;
}


/*!	To be called when the vnode's ref count is changed from 0 to 1.
	Must be called with sVnodeLock at least read-locked and the vnode locked.
	\param vnode The vnode.
*/
static void
vnode_used(Vnode* vnode)
{
	ReadLocker hotReadLocker(sHotVnodesLock);

	if (!vnode->IsUnused())
		return;

	vnode->SetUnused(false);

	if (!vnode->IsHot()) {
		MutexLocker unusedLocker(sUnusedVnodesLock);
		list_remove_item(&sUnusedVnodeList, vnode);
		sUnusedVnodes--;
	}
}


/*!	To be called when the vnode's is about to be freed.
	Must be called with sVnodeLock at least read-locked and the vnode locked.
	\param vnode The vnode.
*/
static void
vnode_to_be_freed(Vnode* vnode)
{
	ReadLocker hotReadLocker(sHotVnodesLock);

	if (vnode->IsHot()) {
		// node is hot -- remove it from the array
// TODO: Maybe better completely flush the array while at it?
		int32 count = sNextHotVnodeIndex;
		count = std::min(count, kMaxHotVnodes);
		for (int32 i = 0; i < count; i++) {
			if (sHotVnodes[i] == vnode) {
				sHotVnodes[i] = NULL;
				break;
			}
		}
	} else if (vnode->IsUnused()) {
		MutexLocker unusedLocker(sUnusedVnodesLock);
		list_remove_item(&sUnusedVnodeList, vnode);
		sUnusedVnodes--;
	}

	vnode->SetUnused(false);
}


static inline void
flush_hot_vnodes()
{
	WriteLocker hotWriteLocker(sHotVnodesLock);
	flush_hot_vnodes_locked();
}


static inline void
unused_vnodes_check_started()
{
	atomic_set(&sUnusedVnodesCheckCount, kUnusedVnodesCheckInterval + 1);
}


static inline void
unused_vnodes_check_done()
{
	atomic_set(&sUnusedVnodesCheckCount, 0);
}


#endif	// UNUSED_VNODES_H
