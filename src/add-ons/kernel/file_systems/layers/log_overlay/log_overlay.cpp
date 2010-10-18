/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <dirent.h>

#include <fs_info.h>
#include <fs_interface.h>

#include <debug.h>
#include <KernelExport.h>

#include <io_requests.h>

static const char *kLogFilePrefix = "/var/log/log_overlay";


#define DO_LOG(format, args...) \
	{ \
		char _printBuffer[256]; \
		int _printSize = snprintf(_printBuffer, sizeof(_printBuffer), \
			"%llu %ld %p: " format, system_time(), find_thread(NULL), \
			((fs_vnode *)vnode->private_node)->private_node, args); \
		if ((unsigned int)_printSize > sizeof(_printBuffer)) { \
			_printBuffer[sizeof(_printBuffer) - 1] = '\n'; \
			_printSize = sizeof(_printBuffer); \
		} \
		write((int)volume->private_volume, _printBuffer, _printSize); \
	}

#define OVERLAY_CALL(op, params...) \
	status_t result = B_UNSUPPORTED; \
	fs_vnode *superVnode = (fs_vnode *)vnode->private_node; \
	if (superVnode->ops->op != NULL) \
		result = superVnode->ops->op(volume->super_volume, superVnode, params); \


static status_t
overlay_put_vnode(fs_volume *volume, fs_vnode *vnode, bool reenter)
{
	DO_LOG("put_vnode reenter: %s\n", reenter ? "yes" : "no");

	status_t result = B_UNSUPPORTED;
	fs_vnode *superVnode = (fs_vnode *)vnode->private_node;
	if (superVnode->ops->put_vnode != NULL) {
		result = superVnode->ops->put_vnode(volume->super_volume, superVnode,
			reenter);
	}

	DO_LOG("put_vnode result: 0x%08lx\n", result);
	delete (fs_vnode *)vnode->private_node;
	return result;
}


static status_t
overlay_remove_vnode(fs_volume *volume, fs_vnode *vnode, bool reenter)
{
	DO_LOG("remove_vnode reenter: %s\n", reenter ? "yes" : "no");

	status_t result = B_UNSUPPORTED;
	fs_vnode *superVnode = (fs_vnode *)vnode->private_node;
	if (superVnode->ops->remove_vnode != NULL) {
		result = superVnode->ops->remove_vnode(volume->super_volume, superVnode,
			reenter);
	}

	DO_LOG("remove_vnode result: 0x%08lx\n", result);
	delete (fs_vnode *)vnode->private_node;
	return result;
}


static status_t
overlay_get_super_vnode(fs_volume *volume, fs_vnode *vnode,
	fs_volume *superVolume, fs_vnode *_superVnode)
{
	if (volume == superVolume) {
		*_superVnode = *vnode;
		return B_OK;
	}

	fs_vnode *superVnode = (fs_vnode *)vnode->private_node;
	if (superVnode->ops->get_super_vnode != NULL) {
		return superVnode->ops->get_super_vnode(volume->super_volume,
			superVnode, superVolume, _superVnode);
	}

	*_superVnode = *superVnode;
	return B_OK;
}


static status_t
overlay_lookup(fs_volume *volume, fs_vnode *vnode, const char *name, ino_t *id)
{
	DO_LOG("lookup name: \"%s\"\n", name);
	OVERLAY_CALL(lookup, name, id)
	DO_LOG("lookup result: 0x%08lx; id: %llu\n", result, *id);
	return result;
}


static status_t
overlay_get_vnode_name(fs_volume *volume, fs_vnode *vnode, char *buffer,
	size_t bufferSize)
{
	DO_LOG("get_vnode_name buffer: %p; buffer_size: %lu\n", buffer, bufferSize);
	OVERLAY_CALL(get_vnode_name, buffer, bufferSize)
	DO_LOG("get_vnode_name result: 0x%08lx; buffer: \"%s\"\n", result,
		result == B_OK ? buffer : "unsafe");
	return result;
}


static bool
overlay_can_page(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("can_page cookie: %p\n", cookie);
	bool result = false;
	fs_vnode *superVnode = (fs_vnode *)vnode->private_node;
	if (superVnode->ops->can_page != NULL) {
		result = superVnode->ops->can_page(volume->super_volume, superVnode,
			cookie);
	}

	DO_LOG("can_page result: %s\n", result ? "yes" : "no");
	return result;
}


static status_t
overlay_read_pages(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *numBytes)
{
	DO_LOG("read_pages cookie: %p; pos: %lld; vecs: %p; count: %lu;"
		" num_bytes: %lu\n", cookie, pos, vecs, count, *numBytes);
	OVERLAY_CALL(read_pages, cookie, pos, vecs, count, numBytes)
	DO_LOG("read_pages result: 0x%08lx; num_bytes: %lu\n", result, *numBytes);
	return result;
}


static status_t
overlay_write_pages(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *numBytes)
{
	DO_LOG("write_pages cookie: %p; pos: %lld; vecs: %p; count: %lu;"
		" num_bytes: %lu\n", cookie, pos, vecs, count, *numBytes);
	OVERLAY_CALL(write_pages, cookie, pos, vecs, count, numBytes)
	DO_LOG("write_pages result: 0x%08lx; num_bytes: %lu\n", result, *numBytes);
	return result;
}


static status_t
overlay_io(fs_volume *volume, fs_vnode *vnode, void *cookie,
	io_request *request)
{
	DO_LOG("io cookie: %p; request: %p (write: %s; offset: %lld; length: %llu)"
		"\n", cookie, request, io_request_is_write(request) ? "yes" : "no",
		io_request_offset(request), io_request_length(request));
	OVERLAY_CALL(io, cookie, request)
	DO_LOG("io result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_cancel_io(fs_volume *volume, fs_vnode *vnode, void *cookie,
	io_request *request)
{
	DO_LOG("cancel_io cookie: %p; request: %p\n", cookie, request);
	OVERLAY_CALL(cancel_io, cookie, request)
	DO_LOG("cancel_io result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_get_file_map(fs_volume *volume, fs_vnode *vnode, off_t offset,
	size_t size, struct file_io_vec *vecs, size_t *count)
{
	DO_LOG("get_file_map offset: %lld; size: %lu; vecs: %p; count: %lu\n",
		offset, size, vecs, *count);
	OVERLAY_CALL(get_file_map, offset, size, vecs, count)
	DO_LOG("get_file_map result: 0x%08lx; count: %lu\n", result, *count);
	return result;
}


static status_t
overlay_ioctl(fs_volume *volume, fs_vnode *vnode, void *cookie, uint32 op,
	void *buffer, size_t length)
{
	DO_LOG("ioctl cookie: %p; op: %lu; buffer: %p; size: %lu\n", cookie, op,
		buffer, length);
	OVERLAY_CALL(ioctl, cookie, op, buffer, length)
	DO_LOG("ioctl result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_set_flags(fs_volume *volume, fs_vnode *vnode, void *cookie,
	int flags)
{
	DO_LOG("set_flags cookie: %p; flags: %d\n", cookie, flags);
	OVERLAY_CALL(set_flags, cookie, flags)
	DO_LOG("set_flags result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_select(fs_volume *volume, fs_vnode *vnode, void *cookie, uint8 event,
	selectsync *sync)
{
	DO_LOG("select cookie: %p; event: %u; selectsync: %p\n", cookie, event,
		sync);
	OVERLAY_CALL(select, cookie, event, sync)
	DO_LOG("select result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_deselect(fs_volume *volume, fs_vnode *vnode, void *cookie, uint8 event,
	selectsync *sync)
{
	DO_LOG("deselect cookie: %p; event: %u; selectsync: %p\n", cookie, event,
		sync);
	OVERLAY_CALL(deselect, cookie, event, sync)
	DO_LOG("deselect result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_fsync(fs_volume *volume, fs_vnode *vnode)
{
	DO_LOG("%s\n", "fsync");

	status_t result = B_UNSUPPORTED;
	fs_vnode *superVnode = (fs_vnode *)vnode->private_node;
	if (superVnode->ops->fsync != NULL)
		result = superVnode->ops->fsync(volume->super_volume, superVnode);

	DO_LOG("fsync result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_read_symlink(fs_volume *volume, fs_vnode *vnode, char *buffer,
	size_t *bufferSize)
{
	DO_LOG("read_symlink buffer: %p; buffer_size: %lu\n", buffer, *bufferSize);
	OVERLAY_CALL(read_symlink, buffer, bufferSize)
	DO_LOG("read_symlink result: 0x%08lx; buffer_size: %lu; \"%.*s\"\n", result,
		*bufferSize, result == B_OK ? (int)*bufferSize : 6,
		result == B_OK ? buffer : "unsafe");
	return result;
}


static status_t
overlay_create_symlink(fs_volume *volume, fs_vnode *vnode, const char *name,
	const char *path, int mode)
{
	DO_LOG("create_symlink name: \"%s\"; path: \"%s\"; mode: %u\n", name, path,
		mode);
	OVERLAY_CALL(create_symlink, name, path, mode)
	DO_LOG("create_symlink result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_link(fs_volume *volume, fs_vnode *vnode, const char *name,
	fs_vnode *target)
{
	DO_LOG("link name: \"%s\"; target: %p\n", name,
		((fs_vnode *)target->private_node)->private_node);
	OVERLAY_CALL(link, name, (fs_vnode *)target->private_node)
	DO_LOG("link result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_unlink(fs_volume *volume, fs_vnode *vnode, const char *name)
{
	DO_LOG("unlink name: \"%s\"\n", name);
	OVERLAY_CALL(unlink, name)
	DO_LOG("unlink result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_rename(fs_volume *volume, fs_vnode *vnode,
	const char *fromName, fs_vnode *toDir, const char *toName)
{
	DO_LOG("rename from_name: \"%s\"; to_dir: %p; to_name: \"%s\"\n",
		fromName, ((fs_vnode *)toDir->private_node)->private_node, toName);
	OVERLAY_CALL(rename, fromName, (fs_vnode *)toDir->private_node, toName)
	DO_LOG("rename result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_access(fs_volume *volume, fs_vnode *vnode, int mode)
{
	DO_LOG("access mode: %u\n", mode);
	OVERLAY_CALL(access, mode)
	DO_LOG("access result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_read_stat(fs_volume *volume, fs_vnode *vnode, struct stat *stat)
{
	DO_LOG("read_stat stat: %p\n", stat);
	OVERLAY_CALL(read_stat, stat)
	if (result == B_OK) {
		DO_LOG("read_stat result: 0x%08lx; stat(dev: %lu; ino: %llu; mode: %u;"
			" uid: %u; gid %u; size: %llu)\n", result, stat->st_dev,
			stat->st_ino, stat->st_mode, stat->st_uid, stat->st_gid,
			stat->st_size);
	} else
		DO_LOG("read_stat result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_write_stat(fs_volume *volume, fs_vnode *vnode, const struct stat *stat,
	uint32 statMask)
{
	DO_LOG("write_stat stat: %p; mask: %lu\n", stat, statMask);
	OVERLAY_CALL(write_stat, stat, statMask)
	DO_LOG("write_stat result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_create(fs_volume *volume, fs_vnode *vnode, const char *name,
	int openMode, int perms, void **cookie, ino_t *newVnodeID)
{
	DO_LOG("create name: \"%s\"; open_mode: %u; perms: %u\n", name, openMode,
		perms);
	OVERLAY_CALL(create, name, openMode, perms, cookie, newVnodeID)
	DO_LOG("create result: 0x%08lx; cookie: %p; new_vnode_id: %llu\n", result,
		*cookie, *newVnodeID);
	return result;
}


static status_t
overlay_open(fs_volume *volume, fs_vnode *vnode, int openMode, void **cookie)
{
	DO_LOG("open open_mode: %u\n", openMode);
	OVERLAY_CALL(open, openMode, cookie)
	DO_LOG("open result: 0x%08lx; cookie: %p\n", result, *cookie);
	return result;
}


static status_t
overlay_close(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("close cookie %p\n", cookie);
	OVERLAY_CALL(close, cookie)
	DO_LOG("close result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_free_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("free_cookie cookie %p\n", cookie);
	OVERLAY_CALL(free_cookie, cookie)
	DO_LOG("free_cookie result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_read(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	void *buffer, size_t *length)
{
	DO_LOG("read cookie: %p; pos: %lld; buffer: %p; length: %lu\n", cookie, pos,
		buffer, *length);
	OVERLAY_CALL(read, cookie, pos, buffer, length)
	DO_LOG("read result: 0x%08lx; length: %lu\n", result, *length);
	return result;
}


static status_t
overlay_write(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const void *buffer, size_t *length)
{
	DO_LOG("write cookie: %p; pos: %lld; buffer: %p; length: %lu\n", cookie,
		pos, buffer, *length);
	OVERLAY_CALL(write, cookie, pos, buffer, length)
	DO_LOG("write result: 0x%08lx; length: %lu\n", result, *length);
	return result;
}


static status_t
overlay_create_dir(fs_volume *volume, fs_vnode *vnode, const char *name,
	int perms)
{
	DO_LOG("create_dir name: \"%s\"; perms: %u\n", name, perms);
	OVERLAY_CALL(create_dir, name, perms)
	DO_LOG("create_dir result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_remove_dir(fs_volume *volume, fs_vnode *vnode, const char *name)
{
	DO_LOG("remove_dir name: \"%s\"\n", name);
	OVERLAY_CALL(remove_dir, name)
	DO_LOG("remove_dir result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_open_dir(fs_volume *volume, fs_vnode *vnode, void **cookie)
{
	DO_LOG("%s\n", "open_dir");
	OVERLAY_CALL(open_dir, cookie)
	DO_LOG("open_dir result: 0x%08lx; cookie: %p\n", result, *cookie);
	return result;
}


static status_t
overlay_close_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("close_dir cookie: %p\n", cookie);
	OVERLAY_CALL(close_dir, cookie)
	DO_LOG("close_dir result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_free_dir_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("free_dir_cookie cookie: %p\n", cookie);
	OVERLAY_CALL(free_dir_cookie, cookie)
	DO_LOG("free_dir_cookie result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_read_dir(fs_volume *volume, fs_vnode *vnode, void *cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *num)
{
	DO_LOG("read_dir cookie: %p; buffer: %p; buffer_size: %lu\n", cookie,
		buffer, bufferSize);
	OVERLAY_CALL(read_dir, cookie, buffer, bufferSize, num);
	DO_LOG("read_dir result: 0x%08lx; num: %lu\n", result, *num);
	return result;
}


static status_t
overlay_rewind_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("rewind_dir cookie: %p\n", cookie);
	OVERLAY_CALL(rewind_dir, cookie)
	DO_LOG("rewind_dir result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_open_attr_dir(fs_volume *volume, fs_vnode *vnode, void **cookie)
{
	DO_LOG("%s\n", "open_attr_dir");
	OVERLAY_CALL(open_attr_dir, cookie)
	DO_LOG("open_attr_dir result: 0x%08lx; cookie: %p\n", result, *cookie);
	return result;
}


static status_t
overlay_close_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("close_attr_dir cookie: %p\n", cookie);
	OVERLAY_CALL(close_attr_dir, cookie)
	DO_LOG("close_attr_dir result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_free_attr_dir_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("free_attr_dir_cookie cookie: %p\n", cookie);
	OVERLAY_CALL(free_attr_dir_cookie, cookie)
	DO_LOG("free_attr_dir_cookie result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_read_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *num)
{
	DO_LOG("read_attr_dir cookie: %p; buffer: %p; buffer_size: %lu\n", cookie,
		buffer, bufferSize);
	OVERLAY_CALL(read_attr_dir, cookie, buffer, bufferSize, num);
	DO_LOG("read_attr_dir result: 0x%08lx; num: %lu\n", result, *num);
	return result;
}


static status_t
overlay_rewind_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("rewind_attr_dir cookie: %p\n", cookie);
	OVERLAY_CALL(rewind_attr_dir, cookie)
	DO_LOG("rewind_attr_dir result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_create_attr(fs_volume *volume, fs_vnode *vnode, const char *name,
	uint32 type, int openMode, void **cookie)
{
	DO_LOG("create_attr name: \"%s\"; type: 0x%08lx; open_mode: %u\n", name,
		type, openMode);
	OVERLAY_CALL(create_attr, name, type, openMode, cookie)
	DO_LOG("create_attr result: 0x%08lx; cookie: %p\n", result, *cookie);
	return result;
}


static status_t
overlay_open_attr(fs_volume *volume, fs_vnode *vnode, const char *name,
	int openMode, void **cookie)
{
	DO_LOG("open_attr name: \"%s\"; open_mode: %u\n", name, openMode);
	OVERLAY_CALL(open_attr, name, openMode, cookie)
	DO_LOG("open_attr result: 0x%08lx; cookie: %p\n", result, *cookie);
	return result;
}


static status_t
overlay_close_attr(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("close_attr cookie: %p\n", cookie);
	OVERLAY_CALL(close_attr, cookie)
	DO_LOG("close_attr result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_free_attr_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	DO_LOG("free_attr_cookie cookie: %p\n", cookie);
	OVERLAY_CALL(free_attr_cookie, cookie)
	DO_LOG("free_attr_cookie result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_read_attr(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	void *buffer, size_t *length)
{
	DO_LOG("read_attr cookie: %p; pos: %lld; buffer: %p; length: %lu\n", cookie,
		pos, buffer, *length);
	OVERLAY_CALL(read_attr, cookie, pos, buffer, length)
	DO_LOG("read_attr result: 0x%08lx; length: %lu\n", result, *length);
	return result;
}


static status_t
overlay_write_attr(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const void *buffer, size_t *length)
{
	DO_LOG("write_attr cookie: %p; pos: %lld; buffer: %p; length: %lu\n",
		cookie, pos, buffer, *length);
	OVERLAY_CALL(write_attr, cookie, pos, buffer, length)
	DO_LOG("write_attr result: 0x%08lx; length: %lu\n", result, *length);
	return result;
}


static status_t
overlay_read_attr_stat(fs_volume *volume, fs_vnode *vnode, void *cookie,
	struct stat *stat)
{
	DO_LOG("read_attr_stat cookie: %p; stat: %p\n", cookie, stat);
	OVERLAY_CALL(read_attr_stat, cookie, stat)
	if (result == B_OK) {
		DO_LOG("read_attr_stat result: 0x%08lx; stat(dev: %lu; ino: %llu;"
			" mode: %u; uid: %u; gid %u; size: %llu; type: 0x%08lx)\n", result,
			stat->st_dev, stat->st_ino, stat->st_mode, stat->st_uid,
			stat->st_gid, stat->st_size, stat->st_type);
	} else
		DO_LOG("read_attr_stat result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_write_attr_stat(fs_volume *volume, fs_vnode *vnode, void *cookie,
	const struct stat *stat, int statMask)
{
	DO_LOG("write_attr_stat cookie: %p; stat: %p; mask: %u\n", cookie, stat,
		statMask);
	OVERLAY_CALL(write_attr_stat, cookie, stat, statMask)
	DO_LOG("write_attr_stat result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_rename_attr(fs_volume *volume, fs_vnode *vnode,
	const char *fromName, fs_vnode *toVnode, const char *toName)
{
	DO_LOG("rename_attr from_name: \"%s\"; to_vnode: %p; to_name: \"%s\"\n",
		fromName, ((fs_vnode *)toVnode->private_node)->private_node, toName);
	OVERLAY_CALL(rename_attr, fromName, (fs_vnode *)toVnode->private_node,
		toName)
	DO_LOG("rename_attr result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_remove_attr(fs_volume *volume, fs_vnode *vnode, const char *name)
{
	DO_LOG("remove_attr name: \"%s\"\n", name);
	OVERLAY_CALL(remove_attr, name)
	DO_LOG("remove_attr result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_create_special_node(fs_volume *volume, fs_vnode *vnode,
	const char *name, fs_vnode *subVnode, mode_t mode, uint32 flags,
	fs_vnode *_superVnode, ino_t *nodeID)
{
	DO_LOG("create_special_node name: \"%s\"; sub_vnode: %p; mode: %u;"
		" flags: %lu\n", name, subVnode->private_node, mode, flags);
	OVERLAY_CALL(create_special_node, name, (fs_vnode *)subVnode->private_node,
		mode, flags, _superVnode, nodeID)
	DO_LOG("create_special_node result: 0x%08lx; super_vnode: %p;"
		" node_id: %llu\n", result, _superVnode->private_node, *nodeID);
	return result;
}


static fs_vnode_ops sOverlayVnodeOps = {
	&overlay_lookup,
	&overlay_get_vnode_name,

	&overlay_put_vnode,
	&overlay_remove_vnode,

	&overlay_can_page,
	&overlay_read_pages,
	&overlay_write_pages,

	&overlay_io,
	&overlay_cancel_io,

	&overlay_get_file_map,

	/* common */
	&overlay_ioctl,
	&overlay_set_flags,
	&overlay_select,
	&overlay_deselect,
	&overlay_fsync,

	&overlay_read_symlink,
	&overlay_create_symlink,
	&overlay_link,
	&overlay_unlink,
	&overlay_rename,

	&overlay_access,
	&overlay_read_stat,
	&overlay_write_stat,
	NULL,	// fs_preallocate

	/* file */
	&overlay_create,
	&overlay_open,
	&overlay_close,
	&overlay_free_cookie,
	&overlay_read,
	&overlay_write,

	/* directory */
	&overlay_create_dir,
	&overlay_remove_dir,
	&overlay_open_dir,
	&overlay_close_dir,
	&overlay_free_dir_cookie,
	&overlay_read_dir,
	&overlay_rewind_dir,

	/* attribute directory operations */
	&overlay_open_attr_dir,
	&overlay_close_attr_dir,
	&overlay_free_attr_dir_cookie,
	&overlay_read_attr_dir,
	&overlay_rewind_attr_dir,

	/* attribute operations */
	&overlay_create_attr,
	&overlay_open_attr,
	&overlay_close_attr,
	&overlay_free_attr_cookie,
	&overlay_read_attr,
	&overlay_write_attr,

	&overlay_read_attr_stat,
	&overlay_write_attr_stat,
	&overlay_rename_attr,
	&overlay_remove_attr,

	/* support for node and FS layers */
	&overlay_create_special_node,
	&overlay_get_super_vnode
};


//	#pragma mark - volume ops


#define DO_VOLUME_LOG(format, args...) \
	{ \
		char _printBuffer[256]; \
		int _printSize = snprintf(_printBuffer, sizeof(_printBuffer), \
			"%llu %ld %p: " format, system_time(), find_thread(NULL), \
			volume->super_volume, args); \
		if ((unsigned int)_printSize > sizeof(_printBuffer)) { \
			_printBuffer[sizeof(_printBuffer) - 1] = '\n'; \
			_printSize = sizeof(_printBuffer); \
		} \
		write((int)volume->private_volume, _printBuffer, _printSize); \
	}

#define OVERLAY_VOLUME_CALL(op, params...) \
	status_t result = B_UNSUPPORTED; \
	if (volume->super_volume->ops->op != NULL) \
		result = volume->super_volume->ops->op(volume->super_volume, params); \


static status_t
overlay_unmount(fs_volume *volume)
{
	if (volume->super_volume != NULL
		&& volume->super_volume->ops != NULL
		&& volume->super_volume->ops->unmount != NULL)
		volume->super_volume->ops->unmount(volume->super_volume);

	close((int)volume->private_volume);
	return B_OK;
}


static status_t
overlay_read_fs_info(fs_volume *volume, struct fs_info *info)
{
	DO_VOLUME_LOG("%s\n", "read_fs_info");
	OVERLAY_VOLUME_CALL(read_fs_info, info)
	DO_VOLUME_LOG("read_fs_info result: 0x%08lx; info(dev: %lu; root: %llu;"
		" flags: %lu; block_size: %lld; io_size: %lld; total_blocks: %lld;"
		" free_blocks: %lld; total_nodes: %lld; free_nodes: %lld;"
		" device_name: \"%s\"; volume_name: \"%s\"; fsh_name: \"%s\")\n",
		result, info->dev, info->root, info->flags, info->block_size,
		info->io_size, info->total_blocks, info->free_blocks,
		info->total_nodes, info->free_nodes, info->device_name,
		info->volume_name, info->fsh_name);
	return result;
}


static status_t
overlay_write_fs_info(fs_volume *volume, const struct fs_info *info,
	uint32 mask)
{
	DO_VOLUME_LOG("write_fs_info info: %p; mask: %lu\n", info, mask);
	OVERLAY_VOLUME_CALL(write_fs_info, info, mask)
	DO_VOLUME_LOG("write_fs_info result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_sync(fs_volume *volume)
{
	DO_VOLUME_LOG("%s\n", "sync");
	status_t result = B_UNSUPPORTED;
	if (volume->super_volume->ops->sync != NULL)
		result = volume->super_volume->ops->sync(volume->super_volume);
	DO_VOLUME_LOG("sync result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_get_vnode(fs_volume *volume, ino_t id, fs_vnode *vnode, int *type,
	uint32 *flags, bool reenter)
{
	DO_VOLUME_LOG("get_vnode id: %llu; vnode: %p; type*: %p; flags*: %p;"
		" reenter: %d\n", id, vnode, type, flags, reenter);

	if (volume->super_volume->ops->get_vnode == NULL) {
		DO_VOLUME_LOG("get_vnode %s\n", "not supported");
		return B_UNSUPPORTED;
	}

	fs_vnode *superVnode = new(std::nothrow) fs_vnode;
	if (superVnode == NULL) {
		DO_VOLUME_LOG("get_vnode %s\n", "no memory");
		return B_NO_MEMORY;
	}

	status_t result = volume->super_volume->ops->get_vnode(volume->super_volume,
		id, superVnode, type, flags, reenter);
	if (result != B_OK) {
		DO_VOLUME_LOG("get_vnode result: 0x%08lx\n", result);
		delete superVnode;
		return result;
	}

	vnode->private_node = superVnode;
	vnode->ops = &sOverlayVnodeOps;

	DO_VOLUME_LOG("get_vnode result: 0x%08lx; super_vnode: %p\n", result,
		superVnode->private_node);
	return B_OK;
}


static status_t
overlay_open_index_dir(fs_volume *volume, void **cookie)
{
	DO_VOLUME_LOG("%s\n", "open_index_dir");
	OVERLAY_VOLUME_CALL(open_index_dir, cookie)
	DO_VOLUME_LOG("open_index_dir result: 0x%08lx; cookie: %p\n", result,
		*cookie);
	return result;
}


static status_t
overlay_close_index_dir(fs_volume *volume, void *cookie)
{
	DO_VOLUME_LOG("close_index_dir cookie: %p\n", cookie);
	OVERLAY_VOLUME_CALL(close_index_dir, cookie)
	DO_VOLUME_LOG("close_index_dir result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_free_index_dir_cookie(fs_volume *volume, void *cookie)
{
	DO_VOLUME_LOG("free_index_dir_cookie cookie: %p\n", cookie);
	OVERLAY_VOLUME_CALL(free_index_dir_cookie, cookie)
	DO_VOLUME_LOG("free_index_dir_cookie result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_read_index_dir(fs_volume *volume, void *cookie, struct dirent *buffer,
	size_t bufferSize, uint32 *num)
{
	DO_VOLUME_LOG("read_index_dir cookie: %p; buffer: %p; buffer_size: %lu\n",
		cookie, buffer, bufferSize);
	OVERLAY_VOLUME_CALL(read_index_dir, cookie, buffer, bufferSize, num)
	DO_VOLUME_LOG("read_index_dir result: 0x%08lx; num: %lu\n", result, *num);
	return result;
}


static status_t
overlay_rewind_index_dir(fs_volume *volume, void *cookie)
{
	DO_VOLUME_LOG("rewind_index_dir cookie: %p\n", cookie);
	OVERLAY_VOLUME_CALL(rewind_index_dir, cookie)
	DO_VOLUME_LOG("rewind_index_dir result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_create_index(fs_volume *volume, const char *name, uint32 type,
	uint32 flags)
{
	DO_VOLUME_LOG("create_index name: \"%s\"; type: %lu; flags: %lu\n", name,
		type, flags);
	OVERLAY_VOLUME_CALL(create_index, name, type, flags)
	DO_VOLUME_LOG("create_index result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_remove_index(fs_volume *volume, const char *name)
{
	DO_VOLUME_LOG("remove_index name: \"%s\"\n", name);
	OVERLAY_VOLUME_CALL(remove_index, name)
	DO_VOLUME_LOG("remove_index result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_read_index_stat(fs_volume *volume, const char *name, struct stat *stat)
{
	DO_VOLUME_LOG("read_index_stat name: \"%s\"; stat: %p\n", name, stat);
	OVERLAY_VOLUME_CALL(read_index_stat, name, stat)
	if (result == B_OK) {
		DO_VOLUME_LOG("read_index_stat result: 0x%08lx; stat(dev: %lu;"
			" ino: %llu; mode: %u; uid: %u; gid %u; size: %llu;"
			" type: 0x%08lx)\n", result, stat->st_dev, stat->st_ino,
			stat->st_mode, stat->st_uid, stat->st_gid, stat->st_size,
			stat->st_type);
	} else
		DO_VOLUME_LOG("read_index_stat result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_open_query(fs_volume *volume, const char *query, uint32 flags,
	port_id port, uint32 token, void **cookie)
{
	DO_VOLUME_LOG("open_query query: \"%s\"; flags: %lu; port: %ld;"
		" token: %lu\n", query, flags, port, token);
	OVERLAY_VOLUME_CALL(open_query, query, flags, port, token, cookie)
	DO_VOLUME_LOG("open_query result: 0x%08lx; cookie: %p\n", result, *cookie);
	return result;
}


static status_t
overlay_close_query(fs_volume *volume, void *cookie)
{
	DO_VOLUME_LOG("close_query cookie: %p\n", cookie);
	OVERLAY_VOLUME_CALL(close_query, cookie)
	DO_VOLUME_LOG("close_query result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_free_query_cookie(fs_volume *volume, void *cookie)
{
	DO_VOLUME_LOG("free_query_cookie cookie: %p\n", cookie);
	OVERLAY_VOLUME_CALL(free_query_cookie, cookie)
	DO_VOLUME_LOG("free_query_cookie result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_read_query(fs_volume *volume, void *cookie, struct dirent *buffer,
	size_t bufferSize, uint32 *num)
{
	DO_VOLUME_LOG("read_query cookie: %p; buffer: %p; buffer_size: %lu\n",
		cookie, buffer, bufferSize);
	OVERLAY_VOLUME_CALL(read_query, cookie, buffer, bufferSize, num)
	DO_VOLUME_LOG("read_query result: 0x%08lx; num: %lu\n", result, *num);
	return result;
}


static status_t
overlay_rewind_query(fs_volume *volume, void *cookie)
{
	DO_VOLUME_LOG("rewind_query cookie: %p\n", cookie);
	OVERLAY_VOLUME_CALL(rewind_query, cookie)
	DO_VOLUME_LOG("rewind_query result: 0x%08lx\n", result);
	return result;
}


static status_t
overlay_all_layers_mounted(fs_volume *volume)
{
	return B_OK;
}


static status_t
overlay_create_sub_vnode(fs_volume *volume, ino_t id, fs_vnode *vnode)
{
	fs_vnode *superVnode = new(std::nothrow) fs_vnode;
	if (superVnode == NULL)
		return B_NO_MEMORY;

	*superVnode = *vnode;
	vnode->private_node = superVnode;
	vnode->ops = &sOverlayVnodeOps;
	return B_OK;
}


static status_t
overlay_delete_sub_vnode(fs_volume *volume, fs_vnode *vnode)
{
	delete (fs_vnode *)vnode->private_node;
	return B_OK;
}


static fs_volume_ops sOverlayVolumeOps = {
	&overlay_unmount,

	&overlay_read_fs_info,
	&overlay_write_fs_info,
	&overlay_sync,

	&overlay_get_vnode,
	&overlay_open_index_dir,
	&overlay_close_index_dir,
	&overlay_free_index_dir_cookie,
	&overlay_read_index_dir,
	&overlay_rewind_index_dir,

	&overlay_create_index,
	&overlay_remove_index,
	&overlay_read_index_stat,

	&overlay_open_query,
	&overlay_close_query,
	&overlay_free_query_cookie,
	&overlay_read_query,
	&overlay_rewind_query,

	&overlay_all_layers_mounted,
	&overlay_create_sub_vnode,
	&overlay_delete_sub_vnode
};


//	#pragma mark - filesystem module


static status_t
overlay_mount(fs_volume *volume, const char *device, uint32 flags,
	const char *args, ino_t *rootID)
{
	char filename[256];
	snprintf(filename, sizeof(filename), "%s%s", kLogFilePrefix, device);
	filename[sizeof(filename) - 1] = 0;

	int filenameLength = strlen(filename);
	for (int i = strlen(kLogFilePrefix); i < filenameLength; i++) {
		if (filename[i] == '/')
			filename[i] = '_';
	}

	int fd = creat(filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0)
		return errno;

	volume->private_volume = (void *)fd;
	volume->ops = &sOverlayVolumeOps;
	return B_OK;
}


static status_t
overlay_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
		default:
			return B_ERROR;
	}
}


static file_system_module_info sOverlayFileSystem = {
	{
		"file_systems/log_overlay"B_CURRENT_FS_API_VERSION,
		0,
		overlay_std_ops,
	},

	"log_overlay",						// short_name
	"Logging Overlay File System",		// pretty_name
	0,									// DDM flags

	// scanning
	NULL, // identify_partition
	NULL, // scan_partition
	NULL, // free_identify_partition_cookie
	NULL, // free_partition_content_cookie

	// general operations
	&overlay_mount,

	// capability querying
	NULL, // get_supported_operations

	NULL, // validate_resize
	NULL, // validate_move
	NULL, // validate_set_content_name
	NULL, // validate_set_content_parameters
	NULL, // validate_initialize

	// shadow partition modification
	NULL, // shadow_changed

	// writing
	NULL, // defragment
	NULL, // repair
	NULL, // resize
	NULL, // move
	NULL, // set_content_name
	NULL, // set_content_parameters
	NULL // initialize
};

module_info *modules[] = {
	(module_info *)&sOverlayFileSystem,
	NULL,
};
