/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <disk_device_manager.h>
#include <fs_cache.h>
#include <fs_interface.h>
#include <io_requests.h>
#include <KernelExport.h>
#include <NodeMonitor.h>

#include "Debug.h"

#include "../FileSystem.h"
#include "../kernel_emu.h"

#include "HaikuKernelNode.h"
#include "HaikuKernelVolume.h"
#include "vfs.h"


// #pragma mark - Notifications


// notify_entry_created
status_t
notify_entry_created(dev_t device, ino_t directory, const char *name,
	ino_t node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ENTRY_CREATED, 0, device, 0,
		directory, node, NULL, name);
}

// notify_entry_removed
status_t
notify_entry_removed(dev_t device, ino_t directory, const char *name,
	ino_t node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ENTRY_REMOVED, 0, device, 0,
		directory, node, NULL, name);
}

// notify_entry_moved
status_t
notify_entry_moved(dev_t device, ino_t fromDirectory,
	const char *fromName, ino_t toDirectory, const char *toName,
	ino_t node)
{
	if (!fromName || !toName)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ENTRY_MOVED, 0, device,
		fromDirectory, toDirectory, node, fromName, toName);
}

// notify_stat_changed
status_t
notify_stat_changed(dev_t device, ino_t node, uint32 statFields)
{
	return UserlandFS::KernelEmu::notify_listener(B_STAT_CHANGED, statFields,
		device, 0, 0, node, NULL, NULL);
}

// notify_attribute_changed
status_t
notify_attribute_changed(dev_t device, ino_t node, const char *attribute,
	int32 cause)
{
	if (!attribute)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ATTR_CHANGED, cause,
		device, 0, 0, node, NULL, attribute);
}

// notify_select_event
status_t
notify_select_event(selectsync *sync, uint8 event)
{
	return UserlandFS::KernelEmu::notify_select_event(sync, event, false);
}

// notify_query_entry_created
status_t
notify_query_entry_created(port_id port, int32 token, dev_t device,
	ino_t directory, const char *name, ino_t node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_query(port, token, B_ENTRY_CREATED,
		device, directory, name, node);
}

// notify_query_entry_removed
status_t
notify_query_entry_removed(port_id port, int32 token, dev_t device,
	ino_t directory, const char *name, ino_t node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_query(port, token, B_ENTRY_REMOVED,
		device, directory, name, node);
}


// #pragma mark - VNodes


// new_vnode
status_t
new_vnode(fs_volume *_volume, ino_t vnodeID, void *privateNode,
	fs_vnode_ops *ops)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	// translate to a wrapper node
	HaikuKernelNode* node;
	status_t error = volume->NewVNode(vnodeID, privateNode, ops, &node);
	if (error != B_OK)
		return error;

	// announce the new node
	error = UserlandFS::KernelEmu::new_vnode(volume->GetID(), vnodeID, node);
	if (error != B_OK)
		volume->UndoNewVNode(node);

	return error;
}

// publish_vnode
status_t
publish_vnode(fs_volume *_volume, ino_t vnodeID, void *privateNode,
	fs_vnode_ops *ops, int type, uint32 flags)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	// translate to a wrapper node
	HaikuKernelNode* node;
	status_t error = volume->PublishVNode(vnodeID, privateNode, ops, type,
		flags, &node);
	if (error != B_OK)
		return error;

	// publish the new node
	error = UserlandFS::KernelEmu::publish_vnode(volume->GetID(), vnodeID, node,
		type, flags);
	if (error != B_OK)
		volume->UndoPublishVNode(node);

	return error;
}

// get_vnode
status_t
get_vnode(fs_volume *_volume, ino_t vnodeID, void **privateNode)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	// get the node
	void* foundNode;
	status_t error = UserlandFS::KernelEmu::get_vnode(volume->GetID(), vnodeID,
		&foundNode);
	if (error != B_OK)
		return error;

	*privateNode = ((HaikuKernelNode*)foundNode)->private_node;

	return B_OK;
}

// put_vnode
status_t
put_vnode(fs_volume *_volume, ino_t vnodeID)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	return UserlandFS::KernelEmu::put_vnode(volume->GetID(), vnodeID);
}

// acquire_vnode
status_t
acquire_vnode(fs_volume *_volume, ino_t vnodeID)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	return UserlandFS::KernelEmu::acquire_vnode(volume->GetID(), vnodeID);
}

// remove_vnode
status_t
remove_vnode(fs_volume *_volume, ino_t vnodeID)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	return UserlandFS::KernelEmu::remove_vnode(volume->GetID(), vnodeID);
}

// unremove_vnode
status_t
unremove_vnode(fs_volume *_volume, ino_t vnodeID)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	return UserlandFS::KernelEmu::unremove_vnode(volume->GetID(), vnodeID);
}

// get_vnode_removed
status_t
get_vnode_removed(fs_volume *_volume, ino_t vnodeID, bool* removed)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	return UserlandFS::KernelEmu::get_vnode_removed(volume->GetID(), vnodeID,
		removed);
}

// volume_for_vnode
fs_volume*
volume_for_vnode(fs_vnode *vnode)
{
	return HaikuKernelNode::GetNode(vnode)->GetVolume()->GetFSVolume();
}


// read_file_io_vec_pages
status_t
read_file_io_vec_pages(int fd, const struct file_io_vec *fileVecs,
	size_t fileVecCount, const struct iovec *vecs, size_t vecCount,
	uint32 *_vecIndex, size_t *_vecOffset, size_t *_bytes)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


// write_file_io_vec_pages
status_t
write_file_io_vec_pages(int fd, const struct file_io_vec *fileVecs,
	size_t fileVecCount, const struct iovec *vecs, size_t vecCount,
	uint32 *_vecIndex, size_t *_vecOffset, size_t *_bytes)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


// do_fd_io
status_t
do_fd_io(int fd, io_request *request)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


// do_iterative_fd_io
status_t
do_iterative_fd_io(int fd, io_request *request, iterative_io_get_vecs getVecs,
	iterative_io_finished finished, void *cookie)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


// #pragma mark - I/O requests


bool
io_request_is_write(const io_request* request)
{
	// TODO: Implement!
	return false;
}


// #pragma mark - Disk Device Manager


// get_default_partition_content_name
status_t
get_default_partition_content_name(partition_id partitionID,
	const char* fileSystemName, char* buffer, size_t bufferSize)
{
	// TODO: Improve!
	snprintf(buffer, bufferSize, "%s Volume", fileSystemName);
	return B_OK;
}


// scan_partition
status_t
scan_partition(partition_id partitionID)
{
	// Only needed when we decide to add disk system support.
	return B_OK;
}


// update_disk_device_job_progress
bool
update_disk_device_job_progress(disk_job_id jobID, float progress)
{
	// Only needed when we decide to add disk system support.
	return true;
}


// #pragma mark - VFS private


status_t
vfs_get_file_map(struct vnode *vnode, off_t offset, size_t size,
	struct file_io_vec *vecs, size_t *_count)
{
	HaikuKernelNode* node = (HaikuKernelNode*)vnode;

	return node->volume->GetFileMap(node, offset, size, vecs, _count);
}


status_t
vfs_lookup_vnode(dev_t mountID, ino_t vnodeID, struct vnode **_vnode)
{
	// get the volume
	HaikuKernelVolume* volume = dynamic_cast<HaikuKernelVolume*>(
		FileSystem::GetInstance()->VolumeWithID(mountID));
	if (volume == NULL)
		return B_BAD_VALUE;

	// get the node
	HaikuKernelNode* node = volume->NodeWithID(vnodeID);
	if (node == NULL)
		return B_BAD_VALUE;

	*_vnode = (struct vnode*)node;

	return B_OK;
}
