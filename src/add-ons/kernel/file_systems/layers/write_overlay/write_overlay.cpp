/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <new>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include <util/kernel_cpp.h>

#include <fs_info.h>
#include <fs_interface.h>

#include <debug.h>
#include <KernelExport.h>
#include <NodeMonitor.h>


//#define TRACE_OVERLAY
#ifdef TRACE_OVERLAY
#define TRACE(x...)			dprintf("write_overlay: " x)
#define TRACE_VOLUME(x...)	dprintf("write_overlay: " x)
#define TRACE_ALWAYS(x...)	dprintf("write_overlay: " x)
#else
#define TRACE(x...)			/* nothing */
#define TRACE_VOLUME(x...)	/* nothing */
#define TRACE_ALWAYS(x...)	dprintf("write_overlay: " x)
#endif


namespace write_overlay {

struct open_cookie {
	int				open_mode;
	void *			super_cookie;
};

struct write_buffer {
	write_buffer *	next;
	off_t			position;
	size_t			length;
	uint8			buffer[1];
};


class OverlayVolume {
public:
							OverlayVolume(fs_volume *volume);
							~OverlayVolume();

		fs_volume *			Volume() { return fVolume; }
		fs_volume *			SuperVolume() { return fVolume->super_volume; }

private:
		fs_volume *			fVolume;
};


class OverlayInode {
public:
							OverlayInode(OverlayVolume *volume,
								fs_vnode *superVnode, ino_t inodeNumber);
							~OverlayInode();

		status_t			InitCheck();

		fs_volume *			Volume() { return fVolume->Volume(); }
		fs_volume *			SuperVolume() { return fVolume->SuperVolume(); }
		fs_vnode *			SuperVnode() { return &fSuperVnode; }
		ino_t				InodeNumber() { return fInodeNumber; }

		status_t			ReadStat(struct stat *stat);

		status_t			Open(int openMode, void **cookie);
		status_t			Close(void *cookie);
		status_t			FreeCookie(void *cookie);
		status_t			Read(void *cookie, off_t position, void *buffer,
								size_t *length);
		status_t			Write(void *cookie, off_t position,
								const void *buffer, size_t *length);

private:
		OverlayVolume *		fVolume;
		fs_vnode			fSuperVnode;
		ino_t				fInodeNumber;
		write_buffer *		fWriteBuffers;
		off_t				fOriginalNodeLength;
		off_t				fCurrentNodeLength;
		time_t				fModificationTime;
};


//	#pragma mark OverlayVolume


OverlayVolume::OverlayVolume(fs_volume *volume)
	:	fVolume(volume)
{
}


OverlayVolume::~OverlayVolume()
{
}


//	#pragma mark OverlayInode


OverlayInode::OverlayInode(OverlayVolume *volume, fs_vnode *superVnode,
	ino_t inodeNumber)
	:	fVolume(volume),
		fSuperVnode(*superVnode),
		fInodeNumber(inodeNumber),
		fWriteBuffers(NULL),
		fOriginalNodeLength(-1),
		fCurrentNodeLength(-1)
{
	TRACE("inode created\n");
}


OverlayInode::~OverlayInode()
{
	TRACE("inode destroyed\n");
	write_buffer *element = fWriteBuffers;
	while (element) {
		write_buffer *next = element->next;
		free(element);
		element = next;
	}
}


status_t
OverlayInode::InitCheck()
{
	return B_OK;
}


status_t
OverlayInode::ReadStat(struct stat *stat)
{
	if (fSuperVnode.ops->read_stat == NULL)
		return B_UNSUPPORTED;

	status_t result = fSuperVnode.ops->read_stat(SuperVolume(), &fSuperVnode,
		stat);
	if (result != B_OK)
		return result;

	if (fCurrentNodeLength >= 0) {
		stat->st_size = fCurrentNodeLength;
		stat->st_blocks = (stat->st_size + stat->st_blksize - 1)
			/ stat->st_blksize;
		stat->st_mtime = fModificationTime;
	}

	return B_OK;
}


status_t
OverlayInode::Open(int openMode, void **_cookie)
{
	if (fSuperVnode.ops->open == NULL)
		return B_UNSUPPORTED;

	open_cookie *cookie = (open_cookie *)malloc(sizeof(open_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	if (fOriginalNodeLength < 0) {
		struct stat stat;
		status_t result = fSuperVnode.ops->read_stat(SuperVolume(),
			&fSuperVnode, &stat);
		if (result != B_OK) {
			free(cookie);
			return result;
		}

		fOriginalNodeLength = stat.st_size;
		fCurrentNodeLength = stat.st_size;
		fModificationTime = stat.st_mtime;
	}

	cookie->open_mode = openMode;
	*_cookie = cookie;

	if (openMode & O_TRUNC)
		fCurrentNodeLength = 0;

	openMode &= ~(O_RDWR | O_WRONLY | O_TRUNC | O_CREAT);
	status_t result = fSuperVnode.ops->open(SuperVolume(), &fSuperVnode,
		openMode, &cookie->super_cookie);
	if (result != B_OK) {
		free(cookie);
		return result;
	}

	return B_OK;
}


status_t
OverlayInode::Close(void *_cookie)
{
	open_cookie *cookie = (open_cookie *)_cookie;
	return fSuperVnode.ops->close(SuperVolume(), &fSuperVnode,
		cookie->super_cookie);
}


status_t
OverlayInode::FreeCookie(void *_cookie)
{
	open_cookie *cookie = (open_cookie *)_cookie;
	status_t result = fSuperVnode.ops->free_cookie(SuperVolume(),
		&fSuperVnode, cookie->super_cookie);
	free(cookie);
	return result;
}


status_t
OverlayInode::Read(void *_cookie, off_t position, void *buffer, size_t *length)
{
	if (position >= fCurrentNodeLength) {
		*length = 0;
		return B_OK;
	}

	if (position < fOriginalNodeLength) {
		open_cookie *cookie = (open_cookie *)_cookie;
		size_t readLength = MIN(fOriginalNodeLength - position, *length);
		status_t result = fSuperVnode.ops->read(SuperVolume(), &fSuperVnode,
			cookie->super_cookie, position, buffer, &readLength);
		if (result != B_OK)
			return result;
	}

	// overlay the read with whatever chunks we have written
	write_buffer *element = fWriteBuffers;
	*length = MIN(fCurrentNodeLength - position, *length);
	off_t end = position + *length;
	while (element) {
		if (element->position > end)
			break;

		off_t elementEnd = element->position + element->length;
		if (elementEnd > position) {
			off_t copyPosition = MAX(position, element->position);
			size_t copyLength = MIN(elementEnd - position, *length);
			memcpy((uint8 *)buffer + (copyPosition - position),
				element->buffer + (copyPosition - element->position),
				copyLength);
		}

		element = element->next;
	}

	return B_OK;
}


status_t
OverlayInode::Write(void *_cookie, off_t position, const void *buffer,
	size_t *length)
{
	// find insertion point
	write_buffer **link = &fWriteBuffers;
	write_buffer *other = fWriteBuffers;
	write_buffer *swallow = NULL;
	off_t newPosition = position;
	size_t newLength = *length;
	uint32 swallowCount = 0;

	while (other) {
		off_t newEnd = newPosition + newLength;
		off_t otherEnd = other->position + other->length;
		if (otherEnd < newPosition) {
			// other is completely before us
			link = &other->next;
			other = other->next;
			continue;
		}

		if (other->position > newEnd) {
			// other is completely past us
			break;
		}

		swallowCount++;
		if (swallow == NULL)
			swallow = other;

		if (other->position <= newPosition) {
			if (swallowCount == 1 && otherEnd >= newEnd) {
				// other chunk completely covers us, just copy
				memcpy(other->buffer + (newPosition - other->position),
					buffer, *length);

				fModificationTime = time(NULL);
				notify_stat_changed(SuperVolume()->id, fInodeNumber,
					B_STAT_MODIFICATION_TIME);
				return B_OK;
			}

			newLength += newPosition - other->position;
			newPosition = other->position;
		}

		if (otherEnd > newEnd)
			newLength += otherEnd - newEnd;

		other = other->next;
	}

	write_buffer *element = (write_buffer *)malloc(sizeof(write_buffer) - 1
		+ newLength);
	if (element == NULL)
		return B_NO_MEMORY;

	element->next = *link;
	element->position = newPosition;
	element->length = newLength;
	*link = element;

	bool sizeChanged = false;
	off_t newEnd = newPosition + newLength;
	if (newEnd > fCurrentNodeLength) {
		fCurrentNodeLength = newEnd;
		sizeChanged = true;
	}

	// populate the buffer with the existing chunks
	if (swallowCount > 0) {
		while (swallowCount-- > 0) {
			memcpy(element->buffer + (swallow->position - newPosition),
				swallow->buffer, swallow->length);

			element->next = swallow->next;
			free(swallow);
			swallow = element->next;
		}
	}

	memcpy(element->buffer + (position - newPosition), buffer, *length);

	fModificationTime = time(NULL);
	notify_stat_changed(SuperVolume()->id, fInodeNumber,
		B_STAT_MODIFICATION_TIME | (sizeChanged ? B_STAT_SIZE : 0));

	return B_OK;
}


//	#pragma mark - vnode ops


#define OVERLAY_CALL(op, params...) \
	TRACE("relaying op: " #op "\n"); \
	OverlayInode *node = (OverlayInode *)vnode->private_node; \
	fs_vnode *superVnode = node->SuperVnode(); \
	if (superVnode->ops->op != NULL) \
		return superVnode->ops->op(volume->super_volume, superVnode, params);


static status_t
overlay_put_vnode(fs_volume *volume, fs_vnode *vnode, bool reenter)
{
	OverlayInode *node = (OverlayInode *)vnode->private_node;
	fs_vnode *superVnode = node->SuperVnode();

	status_t result = B_OK;
	if (superVnode->ops->put_vnode != NULL) {
		result = superVnode->ops->put_vnode(volume->super_volume, superVnode,
			reenter);
	}

	delete node;
	return result;
}


static status_t
overlay_remove_vnode(fs_volume *volume, fs_vnode *vnode, bool reenter)
{
	OverlayInode *node = (OverlayInode *)vnode->private_node;
	fs_vnode *superVnode = node->SuperVnode();

	status_t result = B_OK;
	if (superVnode->ops->remove_vnode != NULL) {
		result = superVnode->ops->remove_vnode(volume->super_volume, superVnode,
			reenter);
	}

	delete node;
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

	OverlayInode *node = (OverlayInode *)vnode->private_node;
	fs_vnode *superVnode = node->SuperVnode();

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
	OVERLAY_CALL(lookup, name, id)
	return B_UNSUPPORTED;
}


static status_t
overlay_get_vnode_name(fs_volume *volume, fs_vnode *vnode, char *buffer,
	size_t bufferSize)
{
	OVERLAY_CALL(get_vnode_name, buffer, bufferSize)
	return B_UNSUPPORTED;
}


static bool
overlay_can_page(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(can_page, cookie)
	return false;
}


static status_t
overlay_read_pages(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *numBytes)
{
	OVERLAY_CALL(read_pages, cookie, pos, vecs, count, numBytes)
	return B_UNSUPPORTED;
}


static status_t
overlay_write_pages(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *numBytes)
{
	OVERLAY_CALL(write_pages, cookie, pos, vecs, count, numBytes)
	return B_UNSUPPORTED;
}


#if 0
static status_t
overlay_io(fs_volume *volume, fs_vnode *vnode, void *cookie,
	io_request *request)
{
	OVERLAY_CALL(io, cookie, request)
	return B_UNSUPPORTED;
}
#endif


static status_t
overlay_cancel_io(fs_volume *volume, fs_vnode *vnode, void *cookie,
	io_request *request)
{
	OVERLAY_CALL(cancel_io, cookie, request)
	return B_UNSUPPORTED;
}


static status_t
overlay_get_file_map(fs_volume *volume, fs_vnode *vnode, off_t offset,
	size_t size, struct file_io_vec *vecs, size_t *count)
{
	OVERLAY_CALL(get_file_map, offset, size, vecs, count)
	return B_UNSUPPORTED;
}


static status_t
overlay_ioctl(fs_volume *volume, fs_vnode *vnode, void *cookie, ulong op,
	void *buffer, size_t length)
{
	OVERLAY_CALL(ioctl, cookie, op, buffer, length)
	return B_UNSUPPORTED;
}


static status_t
overlay_set_flags(fs_volume *volume, fs_vnode *vnode, void *cookie,
	int flags)
{
	OVERLAY_CALL(set_flags, cookie, flags)
	return B_UNSUPPORTED;
}


static status_t
overlay_select(fs_volume *volume, fs_vnode *vnode, void *cookie, uint8 event,
	selectsync *sync)
{
	OVERLAY_CALL(select, cookie, event, sync)
	return B_UNSUPPORTED;
}


static status_t
overlay_deselect(fs_volume *volume, fs_vnode *vnode, void *cookie, uint8 event,
	selectsync *sync)
{
	OVERLAY_CALL(deselect, cookie, event, sync)
	return B_UNSUPPORTED;
}


static status_t
overlay_fsync(fs_volume *volume, fs_vnode *vnode)
{
	OverlayInode *node = (OverlayInode *)vnode->private_node;
	fs_vnode *superVnode = node->SuperVnode();

	if (superVnode->ops->fsync != NULL)
		return superVnode->ops->fsync(volume->super_volume, superVnode);

	return B_OK;
}


static status_t
overlay_read_symlink(fs_volume *volume, fs_vnode *vnode, char *buffer,
	size_t *bufferSize)
{
	OVERLAY_CALL(read_symlink, buffer, bufferSize)
	return B_UNSUPPORTED;
}


static status_t
overlay_create_symlink(fs_volume *volume, fs_vnode *vnode, const char *name,
	const char *path, int mode)
{
	OVERLAY_CALL(create_symlink, name, path, mode)
	return B_UNSUPPORTED;
}


static status_t
overlay_link(fs_volume *volume, fs_vnode *vnode, const char *name,
	fs_vnode *target)
{
	OVERLAY_CALL(link, name, target)
	return B_UNSUPPORTED;
}


static status_t
overlay_unlink(fs_volume *volume, fs_vnode *vnode, const char *name)
{
	OVERLAY_CALL(unlink, name)
	return B_UNSUPPORTED;
}


static status_t
overlay_rename(fs_volume *volume, fs_vnode *vnode,
	const char *fromName, fs_vnode *toDir, const char *toName)
{
	OVERLAY_CALL(rename, fromName, toDir, toName)
	return B_UNSUPPORTED;
}


static status_t
overlay_access(fs_volume *volume, fs_vnode *vnode, int mode)
{
	OVERLAY_CALL(access, mode)
	return B_UNSUPPORTED;
}


static status_t
overlay_read_stat(fs_volume *volume, fs_vnode *vnode, struct stat *stat)
{
	return ((OverlayInode *)vnode->private_node)->ReadStat(stat);
}


static status_t
overlay_write_stat(fs_volume *volume, fs_vnode *vnode, const struct stat *stat,
	uint32 statMask)
{
	OVERLAY_CALL(write_stat, stat, statMask)
	return B_UNSUPPORTED;
}


static status_t
overlay_create(fs_volume *volume, fs_vnode *vnode, const char *name,
	int openMode, int perms, void **cookie, ino_t *newVnodeID)
{
	OVERLAY_CALL(create, name, openMode, perms, cookie, newVnodeID)
	return B_UNSUPPORTED;
}


static status_t
overlay_open(fs_volume *volume, fs_vnode *vnode, int openMode, void **cookie)
{
	return ((OverlayInode *)vnode->private_node)->Open(openMode, cookie);
}


static status_t
overlay_close(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	return ((OverlayInode *)vnode->private_node)->Close(cookie);
}


static status_t
overlay_free_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	return ((OverlayInode *)vnode->private_node)->FreeCookie(cookie);
}


static status_t
overlay_read(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	void *buffer, size_t *length)
{
	return ((OverlayInode *)vnode->private_node)->Read(cookie, pos, buffer,
		length);
}


static status_t
overlay_write(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const void *buffer, size_t *length)
{
	return ((OverlayInode *)vnode->private_node)->Write(cookie, pos, buffer,
		length);
}


static status_t
overlay_create_dir(fs_volume *volume, fs_vnode *vnode, const char *name,
	int perms, ino_t *newVnodeID)
{
	OVERLAY_CALL(create_dir, name, perms, newVnodeID)
	return B_UNSUPPORTED;
}


static status_t
overlay_remove_dir(fs_volume *volume, fs_vnode *vnode, const char *name)
{
	OVERLAY_CALL(remove_dir, name)
	return B_UNSUPPORTED;
}


static status_t
overlay_open_dir(fs_volume *volume, fs_vnode *vnode, void **cookie)
{
	OVERLAY_CALL(open_dir, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_close_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(close_dir, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_free_dir_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(free_dir_cookie, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_read_dir(fs_volume *volume, fs_vnode *vnode, void *cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *num)
{
	OVERLAY_CALL(read_dir, cookie, buffer, bufferSize, num)
	return B_UNSUPPORTED;
}


static status_t
overlay_rewind_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(rewind_dir, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_open_attr_dir(fs_volume *volume, fs_vnode *vnode, void **cookie)
{
	OVERLAY_CALL(open_attr_dir, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_close_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(close_attr_dir, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_free_attr_dir_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(free_attr_dir_cookie, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_read_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *num)
{
	OVERLAY_CALL(read_attr_dir, cookie, buffer, bufferSize, num)
	return B_UNSUPPORTED;
}


static status_t
overlay_rewind_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(rewind_attr_dir, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_create_attr(fs_volume *volume, fs_vnode *vnode, const char *name,
	uint32 type, int openMode, void **cookie)
{
	OVERLAY_CALL(create_attr, name, type, openMode, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_open_attr(fs_volume *volume, fs_vnode *vnode, const char *name,
	int openMode, void **cookie)
{
	OVERLAY_CALL(open_attr, name, openMode, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_close_attr(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(close_attr, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_free_attr_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(free_attr_cookie, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_read_attr(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	void *buffer, size_t *length)
{
	OVERLAY_CALL(read_attr, cookie, pos, buffer, length)
	return B_UNSUPPORTED;
}


static status_t
overlay_write_attr(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const void *buffer, size_t *length)
{
	OVERLAY_CALL(write_attr, cookie, pos, buffer, length)
	return B_UNSUPPORTED;
}


static status_t
overlay_read_attr_stat(fs_volume *volume, fs_vnode *vnode, void *cookie,
	struct stat *stat)
{
	OVERLAY_CALL(read_attr_stat, cookie, stat)
	return B_UNSUPPORTED;
}


static status_t
overlay_write_attr_stat(fs_volume *volume, fs_vnode *vnode, void *cookie,
	const struct stat *stat, int statMask)
{
	OVERLAY_CALL(write_attr_stat, cookie, stat, statMask)
	return B_UNSUPPORTED;
}


static status_t
overlay_rename_attr(fs_volume *volume, fs_vnode *vnode,
	const char *fromName, fs_vnode *toVnode, const char *toName)
{
	OVERLAY_CALL(rename_attr, fromName, toVnode, toName)
	return B_UNSUPPORTED;
}


static status_t
overlay_remove_attr(fs_volume *volume, fs_vnode *vnode, const char *name)
{
	OVERLAY_CALL(remove_attr, name)
	return B_UNSUPPORTED;
}


static status_t
overlay_create_special_node(fs_volume *volume, fs_vnode *vnode,
	const char *name, fs_vnode *subVnode, mode_t mode, uint32 flags,
	fs_vnode *_superVnode, ino_t *nodeID)
{
	OVERLAY_CALL(create_special_node, name, subVnode, mode, flags, _superVnode, nodeID)
	return B_UNSUPPORTED;
}


static fs_vnode_ops sOverlayVnodeOps = {
	&overlay_lookup,
	&overlay_get_vnode_name,

	&overlay_put_vnode,
	&overlay_remove_vnode,

	&overlay_can_page,
	&overlay_read_pages,
	&overlay_write_pages,

	// TODO: the io scheduler uses it when available but we may simply
	// return B_UNSUPPORTED and I'm not sure it then falls back correctly
	NULL, //&overlay_io,
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


#define OVERLAY_VOLUME_CALL(op, params...) \
	TRACE_VOLUME("relaying volume op: " #op "\n"); \
	if (volume->super_volume->ops->op != NULL) \
		return volume->super_volume->ops->op(volume->super_volume, params);


static status_t
overlay_unmount(fs_volume *volume)
{
	TRACE_VOLUME("relaying volume op: unmount\n");
	if (volume->super_volume != NULL
		&& volume->super_volume->ops != NULL
		&& volume->super_volume->ops->unmount != NULL)
		volume->super_volume->ops->unmount(volume->super_volume);

	delete (OverlayVolume *)volume->private_volume;
	return B_OK;
}


static status_t
overlay_read_fs_info(fs_volume *volume, struct fs_info *info)
{
	TRACE_VOLUME("relaying volume op: read_fs_info\n");
	status_t result = B_UNSUPPORTED;
	if (volume->super_volume->ops->read_fs_info != NULL) {
		result = volume->super_volume->ops->read_fs_info(volume->super_volume,
			info);
		if (result != B_OK)
			return result;

		info->flags &= ~(B_FS_IS_READONLY | B_FS_IS_PERSISTENT);
		return B_OK;
	}

	return B_UNSUPPORTED;
}


static status_t
overlay_write_fs_info(fs_volume *volume, const struct fs_info *info,
	uint32 mask)
{
	OVERLAY_VOLUME_CALL(write_fs_info, info, mask)
	return B_UNSUPPORTED;
}


static status_t
overlay_sync(fs_volume *volume)
{
	TRACE_VOLUME("relaying volume op: sync\n");
	if (volume->super_volume->ops->sync != NULL)
		return volume->super_volume->ops->sync(volume->super_volume);
	return B_UNSUPPORTED;
}


static status_t
overlay_get_vnode(fs_volume *volume, ino_t id, fs_vnode *vnode, int *_type,
	uint32 *_flags, bool reenter)
{
	TRACE_VOLUME("relaying volume op: get_vnode\n");
	if (volume->super_volume->ops->get_vnode != NULL) {
		status_t status = volume->super_volume->ops->get_vnode(
			volume->super_volume, id, vnode, _type, _flags, reenter);
		if (status != B_OK)
			return status;

		OverlayInode *node = new(std::nothrow) OverlayInode(
			(OverlayVolume *)volume->private_volume, vnode, id);
		if (node == NULL) {
			vnode->ops->put_vnode(volume->super_volume, vnode, reenter);
			return B_NO_MEMORY;
		}

		status = node->InitCheck();
		if (status != B_OK) {
			delete node;
			return status;
		}

		vnode->private_node = node;
		vnode->ops = &sOverlayVnodeOps;
		return B_OK;
	}

	return B_UNSUPPORTED;
}


static status_t
overlay_open_index_dir(fs_volume *volume, void **cookie)
{
	OVERLAY_VOLUME_CALL(open_index_dir, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_close_index_dir(fs_volume *volume, void *cookie)
{
	OVERLAY_VOLUME_CALL(close_index_dir, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_free_index_dir_cookie(fs_volume *volume, void *cookie)
{
	OVERLAY_VOLUME_CALL(free_index_dir_cookie, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_read_index_dir(fs_volume *volume, void *cookie, struct dirent *buffer,
	size_t bufferSize, uint32 *_num)
{
	OVERLAY_VOLUME_CALL(read_index_dir, cookie, buffer, bufferSize, _num)
	return B_UNSUPPORTED;
}


static status_t
overlay_rewind_index_dir(fs_volume *volume, void *cookie)
{
	OVERLAY_VOLUME_CALL(rewind_index_dir, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_create_index(fs_volume *volume, const char *name, uint32 type,
	uint32 flags)
{
	OVERLAY_VOLUME_CALL(create_index, name, type, flags)
	return B_UNSUPPORTED;
}


static status_t
overlay_remove_index(fs_volume *volume, const char *name)
{
	OVERLAY_VOLUME_CALL(remove_index, name)
	return B_UNSUPPORTED;
}


static status_t
overlay_read_index_stat(fs_volume *volume, const char *name, struct stat *stat)
{
	OVERLAY_VOLUME_CALL(read_index_stat, name, stat)
	return B_UNSUPPORTED;
}


static status_t
overlay_open_query(fs_volume *volume, const char *query, uint32 flags,
	port_id port, uint32 token, void **_cookie)
{
	OVERLAY_VOLUME_CALL(open_query, query, flags, port, token, _cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_close_query(fs_volume *volume, void *cookie)
{
	OVERLAY_VOLUME_CALL(close_query, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_free_query_cookie(fs_volume *volume, void *cookie)
{
	OVERLAY_VOLUME_CALL(free_query_cookie, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_read_query(fs_volume *volume, void *cookie, struct dirent *buffer,
	size_t bufferSize, uint32 *_num)
{
	OVERLAY_VOLUME_CALL(read_query, cookie, buffer, bufferSize, _num)
	return B_UNSUPPORTED;
}


static status_t
overlay_rewind_query(fs_volume *volume, void *cookie)
{
	OVERLAY_VOLUME_CALL(rewind_query, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_all_layers_mounted(fs_volume *volume)
{
	return B_OK;
}


static status_t
overlay_create_sub_vnode(fs_volume *volume, ino_t id, fs_vnode *vnode)
{
	OverlayInode *node = new(std::nothrow) OverlayInode(
		(OverlayVolume *)volume->private_volume, vnode, id);
	if (node == NULL)
		return B_NO_MEMORY;

	status_t status = node->InitCheck();
	if (status != B_OK) {
		delete node;
		return status;
	}

	vnode->private_node = node;
	vnode->ops = &sOverlayVnodeOps;
	return B_OK;
}


static status_t
overlay_delete_sub_vnode(fs_volume *volume, fs_vnode *vnode)
{
	delete (OverlayInode *)vnode;
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
	TRACE_VOLUME("mounting write overlay\n");
	volume->private_volume = new(std::nothrow) OverlayVolume(volume);
	if (volume->private_volume == NULL)
		return B_NO_MEMORY;

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
		"file_systems/write_overlay"B_CURRENT_FS_API_VERSION,
		0,
		overlay_std_ops,
	},

	"write_overlay",				// short_name
	"Write Overlay File System",	// pretty_name
	0,								// DDM flags

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

}	// namespace write_overlay

using namespace write_overlay;

module_info *modules[] = {
	(module_info *)&sOverlayFileSystem,
	NULL,
};
