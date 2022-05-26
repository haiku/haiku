/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include <fs_info.h>
#include <fs_interface.h>
#include <KernelExport.h>

#include <vfs.h>

#include <AutoDeleterDrivers.h>

#include "DebugSupport.h"
#include "kernel_interface.h"
#include "Node.h"
#include "Volume.h"


/*!	\brief Binds an arbitrary folder to a given path (which must be that of a
	folder, too). All requests to the mounted path will be passed to the
	corresponding node of the bound (source) filesystem.

	TODO: node monitoring!

	TODO: path filter, such that /dev can be bind-mounted with only a subset
		  of entries

	TODO: Since the source node IDs are used for our nodes, this doesn't work
		  for source trees with submounts.

	TODO: There's no file cache support (required for mmap()). We implement the
		  hooks, but they aren't used.
*/


// #pragma mark - helper macros


#define FETCH_SOURCE_VOLUME_AND_NODE(volume, nodeID)				\
	fs_volume* sourceVolume = volume->SourceFSVolume();				\
	if (sourceVolume == NULL)										\
		RETURN_ERROR(B_ERROR);										\
	vnode* sourceVnode;												\
	status_t error = vfs_get_vnode(volume->SourceFSVolume()->id,	\
		nodeID, true, &sourceVnode);								\
	if (error != B_OK)												\
		RETURN_ERROR(error);										\
	VnodePutter putter(sourceVnode);								\
	fs_vnode* sourceNode = vfs_fsnode_for_vnode(sourceVnode);		\
	if (sourceNode == NULL)											\
		RETURN_ERROR(B_ERROR);


// #pragma mark - Volume


static status_t
bindfs_mount(fs_volume* fsVolume, const char* device, uint32 flags,
	const char* parameters, ino_t* _rootID)
{
	FUNCTION("fsVolume: %p, device: \"%s\", flags: %#" B_PRIx32 ", "
			"parameters: \"%s\"\n",
		fsVolume, device, flags, parameters);

	// create a Volume object
	Volume* volume = new(std::nothrow) Volume(fsVolume);
	if (volume == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<Volume> volumeDeleter(volume);

	status_t error = volume->Mount(parameters);
	if (error != B_OK)
		return error;

	// set return values
	*_rootID = volume->RootNode()->ID();
	fsVolume->private_volume = volumeDeleter.Detach();
	fsVolume->ops = &gBindFSVolumeOps;

	return B_OK;
}


static status_t
bindfs_unmount(fs_volume* fsVolume)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	FUNCTION("volume: %p\n", volume);

	volume->Unmount();
	delete volume;

	return B_OK;
}


static status_t
bindfs_read_fs_info(fs_volume* fsVolume, struct fs_info* info)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	FUNCTION("volume: %p, info: %p\n", volume, info);

	fs_volume* sourceVolume = volume->SourceFSVolume();

	if (sourceVolume->ops->read_fs_info != NULL) {
		status_t error = sourceVolume->ops->read_fs_info(sourceVolume, info);
		if (error != B_OK)
			RETURN_ERROR(error);
	} else {
		info->block_size = 512;
		info->io_size = 64 * 1024;
	}

	info->dev = volume->ID();
	info->root = volume->RootNode()->ID();
	info->total_blocks = info->free_blocks = 0;
	info->total_nodes = info->free_nodes = 0;

	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));

	return B_OK;
}


// #pragma mark - VNodes


static status_t
bindfs_lookup(fs_volume* fsVolume, fs_vnode* fsDir, const char* entryName,
	ino_t* _vnid)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsDir->private_node;

	FUNCTION("volume: %p, dir: %p (%" B_PRIdINO "), entry: \"%s\"\n",
		volume, node, node->ID(), entryName);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	error = sourceNode->ops->lookup(sourceVolume, sourceNode, entryName, _vnid);
	if (error != B_OK)
		RETURN_ERROR(error);

	error = get_vnode(fsVolume, *_vnid, NULL);

	// lookup() on the source gave us a reference we don't need any longer
	vnode* sourceChildVnode;
	if (vfs_lookup_vnode(sourceVolume->id, *_vnid, &sourceChildVnode) == B_OK)
		vfs_put_vnode(sourceChildVnode);

	return error;
}


static status_t
bindfs_get_vnode(fs_volume* fsVolume, ino_t vnid, fs_vnode* fsNode,
	int* _type, uint32* _flags, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;

	FUNCTION("volume: %p, vnid: %" B_PRIdINO "\n", volume, vnid);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, vnid);

	struct stat st;
	error = sourceNode->ops->read_stat(sourceVolume, sourceNode, &st);

	Node* node = new(std::nothrow) Node(vnid, st.st_mode);
	if (node == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	fsNode->private_node = node;
	fsNode->ops = const_cast<fs_vnode_ops*>(volume->VnodeOps());
	*_type = node->Mode() & S_IFMT;
	*_flags = 0;

	return B_OK;
}


static status_t
bindfs_get_vnode_name(fs_volume* fsVolume, fs_vnode* fsNode, char* buffer,
	size_t bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p\n", volume, node);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->get_vnode_name(sourceVolume, sourceNode, buffer,
		bufferSize);
}

static status_t
bindfs_put_vnode(fs_volume* fsVolume, fs_vnode* fsNode, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p\n", volume, node);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	delete node;

	return B_OK;
}


static status_t
bindfs_remove_vnode(fs_volume* fsVolume, fs_vnode* fsNode, bool reenter)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p\n", volume, node);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	delete node;

	return sourceNode->ops->remove_vnode(sourceVolume, sourceNode, reenter);
}


// #pragma mark - VM access


// TODO: These hooks are obsolete. Since we don't create a file cache, they
// aren't needed anyway.


static bool
bindfs_can_page(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->can_page(sourceVolume, sourceNode, cookie);
}


static status_t
bindfs_read_pages(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p, "
			"pos: %" B_PRIdOFF ", vecs: %p, count: %ld\n",
		volume, node, node->ID(), cookie, pos, vecs, count);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->read_pages(sourceVolume, sourceNode, cookie, pos,
		vecs, count, _numBytes);
}


static status_t
bindfs_write_pages(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p, "
			"pos: %" B_PRIdOFF ", vecs: %p, count: %ld\n",
		volume, node, node->ID(), cookie, pos, vecs, count);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->write_pages(sourceVolume, sourceNode, cookie, pos,
		vecs, count, _numBytes);
}


// #pragma mark - Request I/O


// TODO: Since we don't create a file cache, these hooks aren't needed.


static status_t
bindfs_io(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	io_request* request)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p, request: %p\n",
		volume, node, node->ID(), cookie, request);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->io(sourceVolume, sourceNode, cookie, request);
}


static status_t
bindfs_cancel_io(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	io_request* request)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p, request: %p\n",
		volume, node, node->ID(), cookie, request);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->cancel_io(sourceVolume, sourceNode, cookie,
		request);
}


// #pragma mark - File Map


static status_t
bindfs_get_file_map(fs_volume* fsVolume, fs_vnode* fsNode, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), offset: %" B_PRIdOFF ", "
			"size: %ld, vecs: %p\n",
		volume, node, node->ID(), offset, size, vecs);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->get_file_map(sourceVolume, sourceNode, offset, size,
		vecs, _count);
}


// #pragma mark - Special


static status_t
bindfs_ioctl(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie, uint32 op,
	void* buffer, size_t length)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p, "
			"op: %" B_PRIx32 ", buffer: %p, length: %ld\n",
		volume, node, node->ID(), cookie, op, buffer, length);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->ioctl(sourceVolume, sourceNode, cookie, op, buffer,
		length);
}


static status_t
bindfs_set_flags(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie, int flags)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p, flags: %x\n",
		volume, node, node->ID(), cookie, flags);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->set_flags(sourceVolume, sourceNode, cookie, flags);
}


static status_t
bindfs_select(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie, uint8 event,
	selectsync* sync)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p, event: %x, "
			"sync: %p\n",
		volume, node, node->ID(), cookie, event, sync);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->select(sourceVolume, sourceNode, cookie, event,
		sync);
}


static status_t
bindfs_deselect(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	uint8 event, selectsync* sync)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p, event: %x, "
			"sync: %p\n",
		volume, node, node->ID(), cookie, event, sync);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->deselect(sourceVolume, sourceNode, cookie, event,
		sync);
}


static status_t
bindfs_fsync(fs_volume* fsVolume, fs_vnode* fsNode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO ")\n",
		volume, node, node->ID());

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->fsync(sourceVolume, sourceNode);
}


// #pragma mark - Nodes


static status_t
bindfs_read_symlink(fs_volume* fsVolume, fs_vnode* fsNode, char* buffer,
	size_t* _bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO ")\n",
		volume, node, node->ID());

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->read_symlink(sourceVolume, sourceNode, buffer,
		_bufferSize);
}


static status_t
bindfs_create_symlink(fs_volume* fsVolume, fs_vnode* fsNode, const char* name,
	const char* path, int mode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), "
			"name: %s, path: %s, mode: %x\n",
		volume, node, node->ID(), name, path, mode);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->create_symlink(sourceVolume, sourceNode, name, path,
		mode);
}


static status_t
bindfs_link(fs_volume* fsVolume, fs_vnode* fsNode, const char* name,
	fs_vnode* toNode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), "
			"name: %s, tonode: %p\n",
		volume, node, node->ID(), name, toNode);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->link(sourceVolume, sourceNode, name, toNode);
}


static status_t
bindfs_unlink(fs_volume* fsVolume, fs_vnode* fsNode, const char* name)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), name: %s\n",
		volume, node, node->ID(), name);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->unlink(sourceVolume, sourceNode, name);
}


static status_t
bindfs_rename(fs_volume* fsVolume, fs_vnode* fsNode, const char* fromName,
	fs_vnode* toDir, const char* toName)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), "
			"from: %s, toDir: %p, to: %s\n",
		volume, node, node->ID(), fromName, toDir, toName);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->rename(sourceVolume, sourceNode, fromName, toDir,
		toName);
}


static status_t
bindfs_access(fs_volume* fsVolume, fs_vnode* fsNode, int mode)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO" )\n",
		volume, node, node->ID());

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->access(sourceVolume, sourceNode, mode);
}


static status_t
bindfs_read_stat(fs_volume* fsVolume, fs_vnode* fsNode, struct stat* st)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO ")\n",
		volume, node, node->ID());

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	error = sourceNode->ops->read_stat(sourceVolume, sourceNode, st);
	if (error != B_OK)
		RETURN_ERROR(error);

	st->st_dev = volume->ID();

	return B_OK;
}


static status_t
bindfs_write_stat(fs_volume* fsVolume, fs_vnode* fsNode,
	const struct stat* _st, uint32 statMask)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO ")\n",
		volume, node, node->ID());

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	struct stat st;
	memcpy(&st, _st, sizeof(st));
	st.st_dev = sourceVolume->id;

	return sourceNode->ops->write_stat(sourceVolume, sourceNode, &st, statMask);
}


static status_t
bindfs_preallocate(fs_volume* fsVolume, fs_vnode* fsNode, off_t pos,
	off_t length)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), pos: %" B_PRIdOFF ", "
			"length: %" B_PRIdOFF "\n",
		volume, node, node->ID(), pos, length);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->preallocate(sourceVolume, sourceNode, pos, length);
}


// #pragma mark - Files


static status_t
bindfs_create(fs_volume* fsVolume, fs_vnode* fsNode, const char* name,
	int openMode, int perms, void** _cookie, ino_t* _newVnodeID)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), "
			"name: %s, openMode %#x, perms: %x\n",
		volume, node, node->ID(), name, openMode, perms);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	error = sourceNode->ops->create(sourceVolume, sourceNode, name, openMode,
		perms, _cookie, _newVnodeID);
	if (error != B_OK)
		return error;

	error = get_vnode(fsVolume, *_newVnodeID, NULL);

	// on error remove the newly created source entry
	if (error != B_OK)
		sourceNode->ops->unlink(sourceVolume, sourceNode, name);

	// create() on the source gave us a reference we don't need any longer
	vnode* newSourceVnode;
	if (vfs_lookup_vnode(sourceVolume->id, *_newVnodeID, &newSourceVnode)
			== B_OK) {
		vfs_put_vnode(newSourceVnode);
	}

	return error;

}


static status_t
bindfs_open(fs_volume* fsVolume, fs_vnode* fsNode, int openMode,
	void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), openMode %#x\n",
		volume, node, node->ID(), openMode);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->open(sourceVolume, sourceNode, openMode, _cookie);
}


static status_t
bindfs_close(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->close(sourceVolume, sourceNode, cookie);
}


static status_t
bindfs_free_cookie(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->free_cookie(sourceVolume, sourceNode, cookie);
}


static status_t
bindfs_read(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	off_t offset, void* buffer, size_t* bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p, "
			"offset: %" B_PRIdOFF ", buffer: %p, size: %lu\n",
		volume, node, node->ID(), cookie, offset, buffer, *bufferSize);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->read(sourceVolume, sourceNode, cookie, offset,
		buffer, bufferSize);
}


static status_t
bindfs_write(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	off_t offset, const void* buffer, size_t* bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p, "
			"offset: %" B_PRIdOFF ", buffer: %p, size: %lu\n",
		volume, node, node->ID(), cookie, offset, buffer, *bufferSize);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->write(sourceVolume, sourceNode, cookie, offset,
		buffer, bufferSize);
}


// #pragma mark - Directories


static status_t
bindfs_create_dir(fs_volume* fsVolume, fs_vnode* fsNode, const char* name,
	int perms)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), name: %s, perms: %x\n",
		volume, node, node->ID(), name, perms);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->create_dir(sourceVolume, sourceNode, name, perms);
}


static status_t
bindfs_remove_dir(fs_volume* fsVolume, fs_vnode* fsNode, const char* name)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), name: %s\n", volume, node,
		node->ID(), name);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->remove_dir(sourceVolume, sourceNode, name);
}


static status_t
bindfs_open_dir(fs_volume* fsVolume, fs_vnode* fsNode, void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO ")\n",
		volume, node, node->ID());

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->open_dir(sourceVolume, sourceNode, _cookie);
}


static status_t
bindfs_close_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->close_dir(sourceVolume, sourceNode, cookie);
}


static status_t
bindfs_free_dir_cookie(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->free_dir_cookie(sourceVolume, sourceNode, cookie);
}


static status_t
bindfs_read_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->read_dir(sourceVolume, sourceNode, cookie, buffer,
		bufferSize, _count);
}


static status_t
bindfs_rewind_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->rewind_dir(sourceVolume, sourceNode, cookie);
}


// #pragma mark - Attribute Directories


status_t
bindfs_open_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO ")\n",
		volume, node, node->ID());

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->open_attr_dir(sourceVolume, sourceNode, _cookie);
}


status_t
bindfs_close_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->close_attr_dir(sourceVolume, sourceNode, cookie);
}


status_t
bindfs_free_attr_dir_cookie(fs_volume* fsVolume, fs_vnode* fsNode,
	void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->free_attr_dir_cookie(sourceVolume, sourceNode,
		cookie);
}


status_t
bindfs_read_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->read_attr_dir(sourceVolume, sourceNode, cookie,
		buffer, bufferSize, _count);
}


status_t
bindfs_rewind_attr_dir(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->rewind_attr_dir(sourceVolume, sourceNode, cookie);
}


// #pragma mark - Attribute Operations


status_t
bindfs_create_attr(fs_volume* fsVolume, fs_vnode* fsNode, const char* name,
	uint32 type, int openMode, void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), name: \"%s\", "
			"type: %" B_PRIx32 ", openMode %#x\n",
		volume, node, node->ID(), name, type, openMode);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->create_attr(sourceVolume, sourceNode, name, type,
		openMode, _cookie);
}


status_t
bindfs_open_attr(fs_volume* fsVolume, fs_vnode* fsNode, const char* name,
	int openMode, void** _cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), name: \"%s\", "
			"openMode %#x\n",
		volume, node, node->ID(), name, openMode);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->open_attr(sourceVolume, sourceNode, name, openMode,
		_cookie);
}


status_t
bindfs_close_attr(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->close_attr(sourceVolume, sourceNode, cookie);
}


status_t
bindfs_free_attr_cookie(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->free_attr_cookie(sourceVolume, sourceNode, cookie);
}


status_t
bindfs_read_attr(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	off_t offset, void* buffer, size_t* bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->read_attr(sourceVolume, sourceNode, cookie, offset,
		buffer, bufferSize);
}


status_t
bindfs_write_attr(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	off_t offset, const void* buffer, size_t* bufferSize)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->write_attr(sourceVolume, sourceNode, cookie, offset,
		buffer, bufferSize);
}


status_t
bindfs_read_attr_stat(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	struct stat* st)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	error
		= sourceNode->ops->read_attr_stat(sourceVolume, sourceNode, cookie, st);
	if (error != B_OK)
		RETURN_ERROR(error);

	st->st_dev = volume->ID();

	return B_OK;
}


status_t
bindfs_write_attr_stat(fs_volume* fsVolume, fs_vnode* fsNode, void* cookie,
	const struct stat* _st, int statMask)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO"), cookie: %p\n",
		volume, node, node->ID(), cookie);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	struct stat st;
	memcpy(&st, _st, sizeof(st));
	st.st_dev = sourceVolume->id;

	return sourceNode->ops->write_attr_stat(sourceVolume, sourceNode, cookie,
		&st, statMask);
}


static status_t
bindfs_rename_attr(fs_volume* fsVolume, fs_vnode* fsNode, const char* fromName,
	fs_vnode* toDir, const char* toName)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), from: %s, toDir: %p, "
			"to: %s\n",
		volume, node, node->ID(), fromName, toDir, toName);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->rename_attr(sourceVolume, sourceNode, fromName,
		toDir, toName);
}


static status_t
bindfs_remove_attr(fs_volume* fsVolume, fs_vnode* fsNode, const char* name)
{
	Volume* volume = (Volume*)fsVolume->private_volume;
	Node* node = (Node*)fsNode->private_node;

	FUNCTION("volume: %p, node: %p (%" B_PRIdINO "), name: %s\n",
		volume, node, node->ID(), name);

	FETCH_SOURCE_VOLUME_AND_NODE(volume, node->ID());

	return sourceNode->ops->remove_attr(sourceVolume, sourceNode, name);
}


// #pragma mark - Module Interface


static status_t
bindfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			init_debugging();
			PRINT("bindfs_std_ops(): B_MODULE_INIT\n");

			return B_OK;
		}

		case B_MODULE_UNINIT:
		{
			PRINT("bind_std_ops(): B_MODULE_UNINIT\n");
			exit_debugging();
			return B_OK;
		}

		default:
			return B_ERROR;
	}
}


static file_system_module_info sBindFSModuleInfo = {
	{
		"file_systems/bindfs" B_CURRENT_FS_API_VERSION,
		0,
		bindfs_std_ops,
	},

	"bindfs",				// short_name
	"Bind File System",		// pretty_name
	0,						// DDM flags


	// scanning
	NULL,	// identify_partition,
	NULL,	// scan_partition,
	NULL,	// free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&bindfs_mount
};


fs_volume_ops gBindFSVolumeOps = {
	&bindfs_unmount,
	&bindfs_read_fs_info,
	NULL,	// write_fs_info,
	NULL,	// sync,

	&bindfs_get_vnode

	// TODO: index operations
	// TODO: query operations
	// TODO: FS layer operations
};


fs_vnode_ops gBindFSVnodeOps = {
	// vnode operations
	&bindfs_lookup,
	&bindfs_get_vnode_name,
	&bindfs_put_vnode,
	&bindfs_remove_vnode,

	// VM file access
	&bindfs_can_page,
	&bindfs_read_pages,
	&bindfs_write_pages,

	&bindfs_io,
	&bindfs_cancel_io,

	&bindfs_get_file_map,

	&bindfs_ioctl,
	&bindfs_set_flags,
	&bindfs_select,
	&bindfs_deselect,
	&bindfs_fsync,

	&bindfs_read_symlink,
	&bindfs_create_symlink,

	&bindfs_link,
	&bindfs_unlink,
	&bindfs_rename,

	&bindfs_access,
	&bindfs_read_stat,
	&bindfs_write_stat,
	&bindfs_preallocate,

	// file operations
	&bindfs_create,
	&bindfs_open,
	&bindfs_close,
	&bindfs_free_cookie,
	&bindfs_read,
	&bindfs_write,

	// directory operations
	&bindfs_create_dir,
	&bindfs_remove_dir,
	&bindfs_open_dir,
	&bindfs_close_dir,
	&bindfs_free_dir_cookie,
	&bindfs_read_dir,
	&bindfs_rewind_dir,

	// attribute directory operations
	&bindfs_open_attr_dir,
	&bindfs_close_attr_dir,
	&bindfs_free_attr_dir_cookie,
	&bindfs_read_attr_dir,
	&bindfs_rewind_attr_dir,

	// attribute operations
	&bindfs_create_attr,
	&bindfs_open_attr,
	&bindfs_close_attr,
	&bindfs_free_attr_cookie,
	&bindfs_read_attr,
	&bindfs_write_attr,

	&bindfs_read_attr_stat,
	&bindfs_write_attr_stat,
	&bindfs_rename_attr,
	&bindfs_remove_attr,

	// TODO: FS layer operations
};


module_info *modules[] = {
	(module_info *)&sBindFSModuleInfo,
	NULL,
};
