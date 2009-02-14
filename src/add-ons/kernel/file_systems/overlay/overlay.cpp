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

#include <fs_info.h>

#include <debug.h>
#include <KernelExport.h>
#include <NodeMonitor.h>

#include "overlay.h"


//#define TRACE_OVERLAY
#ifdef TRACE_OVERLAY
#define TRACE(x...)			dprintf("overlay: " x)
#define TRACE_VOLUME(x...)	dprintf("overlay: " x)
#define TRACE_ALWAYS(x...)	dprintf("overlay: " x)
#else
#define TRACE(x...)			/* nothing */
#define TRACE_VOLUME(x...)	/* nothing */
#define TRACE_ALWAYS(x...)	dprintf("overlay: " x)
#endif


namespace overlay {

class AttributeFile;
class AttributeEntry;


class OverlayInode {
public:
							OverlayInode(fs_volume *superVolume,
								fs_vnode *superVnode);
							~OverlayInode();

		status_t			InitCheck();

		fs_volume *			SuperVolume() { return fSuperVolume; }
		fs_vnode *			SuperVnode() { return &fSuperVnode; }

		AttributeFile *		GetAttributeFile();

private:
		fs_volume *			fSuperVolume;
		fs_vnode			fSuperVnode;
		AttributeFile *		fAttributeFile;
};


class AttributeFile {
public:
							AttributeFile(fs_volume *volume, fs_vnode *vnode);
							~AttributeFile();

		status_t			InitCheck() { return fStatus; }

		dev_t				VolumeID() { return fVolumeID; }
		ino_t				ParentInode() { return fParentInode; }

		uint32				CountAttributes();
		AttributeEntry *	FindAttribute(const char *name);

		status_t			ReadAttributeDir(struct dirent *dirent,
								size_t bufferSize, uint32 *numEntries);
		status_t			RewindAttributeDir()
							{
								fAttributeDirIndex = 0;
								return B_OK;
							}

private:
		#define ATTRIBUTE_OVERLAY_FILE_MAGIC			'attr'
		#define ATTRIBUTE_OVERLAY_ATTRIBUTE_DIR_NAME	"_HAIKU"

		struct attribute_file {
			uint32			magic; // 'attr'
			uint32			entry_count;
			uint8			entries[1];
		} _PACKED;

		status_t			fStatus;
		dev_t				fVolumeID;
		ino_t				fParentInode;
		attribute_file *	fFile;
		uint32				fAttributeDirIndex;
		AttributeEntry **	fEntries;
};


class AttributeEntry {
public:
							AttributeEntry(AttributeFile *parent,
								uint8 *buffer);
							~AttributeEntry();

		size_t				EntrySize();

		uint8				NameLength() { return fEntry->name_length; }
		const char *		Name() { return fEntry->name; }

		status_t			FillDirent(struct dirent *dirent,
								size_t bufferSize, uint32 *numEntries);

		status_t			Read(off_t position, void *buffer, size_t *length);
		status_t			ReadStat(struct stat *stat);

private:
		struct attribute_entry {
			type_code		type;
			uint32			size;
			uint8			name_length; // including 0 byte
			char			name[1]; // 0 terminated, followed by data
		} _PACKED;

		AttributeFile *		fParent;
		attribute_entry *	fEntry;
		uint8 *				fData;
};


//	#pragma mark OverlayInode


OverlayInode::OverlayInode(fs_volume *superVolume, fs_vnode *superVnode)
	:	fSuperVolume(superVolume),
		fSuperVnode(*superVnode),
		fAttributeFile(NULL)
{
	TRACE("inode created\n");
}


OverlayInode::~OverlayInode()
{
	TRACE("inode destroyed\n");
	delete fAttributeFile;
}


status_t
OverlayInode::InitCheck()
{
	return B_OK;
}


AttributeFile *
OverlayInode::GetAttributeFile()
{
	if (fAttributeFile == NULL) {
		fAttributeFile = new(std::nothrow) AttributeFile(fSuperVolume,
			&fSuperVnode);
		if (fAttributeFile == NULL) {
			TRACE_ALWAYS("no memory to allocate attribute file\n");
			return NULL;
		}
	}

	if (fAttributeFile->InitCheck() != B_OK)
		return NULL;

	return fAttributeFile;
}


//	#pragma mark AttributeFile


AttributeFile::AttributeFile(fs_volume *volume, fs_vnode *vnode)
	:	fStatus(B_NO_INIT),
		fVolumeID(volume->id),
		fParentInode(0),
		fFile(NULL),
		fAttributeDirIndex(0),
		fEntries(NULL)
{
	if (vnode->ops->get_vnode_name == NULL) {
		TRACE_ALWAYS("cannot get vnode name, hook missing\n");
		fStatus = B_UNSUPPORTED;
		return;
	}

	char nameBuffer[B_FILE_NAME_LENGTH];
	nameBuffer[sizeof(nameBuffer) - 1] = 0;
	fStatus = vnode->ops->get_vnode_name(volume, vnode, nameBuffer,
		sizeof(nameBuffer) - 1);
	if (fStatus != B_OK) {
		TRACE_ALWAYS("failed to get vnode name: %s\n", strerror(fStatus));
		return;
	}

	if (strcmp(nameBuffer, ATTRIBUTE_OVERLAY_ATTRIBUTE_DIR_NAME) == 0) {
		// we don't want attribute overlays on the attribute dir itself
		fStatus = B_UNSUPPORTED;
		return;
	}

	struct stat stat;
	if (vnode->ops->read_stat != NULL
		&& vnode->ops->read_stat(volume, vnode, &stat) == B_OK) {
		fParentInode = stat.st_ino;
	}

	// TODO: the ".." lookup is not actually valid for non-directory vnodes.
	// we make use of the fact that a filesystem probably still provides the
	// lookup hook and has hardcoded ".." to resolve to the parent entry. if we
	// wanted to do this correctly we need some other way to relate this vnode
	// to its parent directory vnode.
	const char *lookup[]
		= { "..", ATTRIBUTE_OVERLAY_ATTRIBUTE_DIR_NAME, nameBuffer };
	int32 lookupCount = sizeof(lookup) / sizeof(lookup[0]);
	fs_vnode currentVnode = *vnode;
	ino_t lastInodeNumber = 0;

	for (int32 i = 0; i < lookupCount; i++) {
		if (currentVnode.ops->lookup == NULL) {
			TRACE_ALWAYS("lookup not possible, lookup hook missing\n");
			fStatus = B_UNSUPPORTED;
			if (i > 0)
				put_vnode(volume, lastInodeNumber);
			return;
		}

		ino_t inodeNumber;
		fStatus = currentVnode.ops->lookup(volume, &currentVnode, lookup[i],
			&inodeNumber);

		if (i > 0)
			put_vnode(volume, lastInodeNumber);

		if (fStatus != B_OK) {
			if (fStatus != B_ENTRY_NOT_FOUND) {
				TRACE_ALWAYS("lookup of \"%s\" failed: %s\n", lookup[i],
					strerror(fStatus));
			}
			return;
		}

		fStatus = get_vnode(volume, inodeNumber, &currentVnode.private_node,
			&currentVnode.ops);
		if (fStatus != B_OK) {
			TRACE_ALWAYS("getting vnode failed: %s\n", strerror(fStatus));
			return;
		}

		lastInodeNumber = inodeNumber;
	}

	if (currentVnode.ops->read_stat == NULL || currentVnode.ops->open == NULL
		|| currentVnode.ops->read == NULL) {
		TRACE_ALWAYS("can't use attribute file, hooks missing\n");
		put_vnode(volume, lastInodeNumber);
		fStatus = B_UNSUPPORTED;
		return;
	}

	fStatus = currentVnode.ops->read_stat(volume, &currentVnode, &stat);
	if (fStatus != B_OK) {
		TRACE_ALWAYS("failed to stat attribute file: %s\n", strerror(fStatus));
		put_vnode(volume, lastInodeNumber);
		return;
	}

	void *attrFileCookie = NULL;
	fStatus = currentVnode.ops->open(volume, &currentVnode, O_RDONLY,
		&attrFileCookie);
	if (fStatus != B_OK) {
		TRACE_ALWAYS("failed to open attribute file: %s\n", strerror(fStatus));
		put_vnode(volume, lastInodeNumber);
		return;
	}

	size_t readLength = stat.st_size;
	uint8 *buffer = (uint8 *)malloc(readLength);
	if (buffer == NULL) {
		TRACE_ALWAYS("cannot allocate memory for read buffer\n");
		put_vnode(volume, lastInodeNumber);
		fStatus = B_NO_MEMORY;
		return;
	}

	fStatus = currentVnode.ops->read(volume, &currentVnode, attrFileCookie, 0,
		buffer, &readLength);
	if (fStatus != B_OK) {
		TRACE_ALWAYS("failed to read from file: %s\n", strerror(fStatus));
		put_vnode(volume, lastInodeNumber);
		return;
	}

	if (currentVnode.ops->close != NULL)
		currentVnode.ops->close(volume, &currentVnode, attrFileCookie);
	if (currentVnode.ops->free_cookie != NULL)
		currentVnode.ops->free_cookie(volume, &currentVnode, attrFileCookie);

	put_vnode(volume, lastInodeNumber);

	fFile = (attribute_file *)buffer;
	if (fFile->magic != ATTRIBUTE_OVERLAY_FILE_MAGIC) {
		TRACE_ALWAYS("attribute file has bad magic\n");
		fStatus = B_BAD_VALUE;
		return;
	}

	fEntries = new(std::nothrow) AttributeEntry *[fFile->entry_count];
	if (fEntries == NULL) {
		TRACE_ALWAYS("no memory to allocate entry pointers\n");
		fStatus = B_NO_MEMORY;
		return;
	}

	for (uint32 i = 0; i < fFile->entry_count; i++)
		fEntries[i] = NULL;

	size_t totalSize = 0;
	readLength -= sizeof(attribute_file) - 1;
	for (uint32 i = 0; i < fFile->entry_count; i++) {
		fEntries[i] = new(std::nothrow) AttributeEntry(this,
			fFile->entries + totalSize);
		if (fEntries[i] == NULL) {
			TRACE_ALWAYS("no memory to allocate attribute entry\n");
			fStatus = B_NO_MEMORY;
			return;
		}

		totalSize += fEntries[i]->EntrySize();
		if (totalSize > readLength) {
			TRACE_ALWAYS("attribute entries are too large for buffer\n");
			fStatus = B_BAD_VALUE;
			return;
		}
	}
}


AttributeFile::~AttributeFile()
{
	if (fFile == NULL)
		return;

	if (fEntries != NULL) {
		for (uint32 i = 0; i < fFile->entry_count; i++)
			delete fEntries[i];

		delete [] fEntries;
	}

	free(fFile);
}


uint32
AttributeFile::CountAttributes()
{
	if (fFile == NULL)
		return 0;

	return fFile->entry_count;
}


AttributeEntry *
AttributeFile::FindAttribute(const char *name)
{
	if (fFile == NULL)
		return NULL;

	for (uint32 i = 0; i < fFile->entry_count; i++) {
		if (strncmp(fEntries[i]->Name(), name, fEntries[i]->NameLength()) == 0)
			return fEntries[i];
	}

	return NULL;
}


status_t
AttributeFile::ReadAttributeDir(struct dirent *dirent, size_t bufferSize,
	uint32 *numEntries)
{
	if (fFile == NULL || fAttributeDirIndex >= fFile->entry_count) {
		*numEntries = 0;
		return B_OK;
	}

	return fEntries[fAttributeDirIndex++]->FillDirent(dirent, bufferSize,
		numEntries);
}


//	#pragma mark AttributeEntry


AttributeEntry::AttributeEntry(AttributeFile *parent, uint8 *buffer)
	:	fParent(parent),
		fEntry(NULL),
		fData(NULL)
{
	fEntry = (attribute_entry *)buffer;
	fData = (uint8 *)fEntry->name + fEntry->name_length;
}


AttributeEntry::~AttributeEntry()
{
}


size_t
AttributeEntry::EntrySize()
{
	return sizeof(attribute_entry) - 1 + fEntry->name_length + fEntry->size;
}


status_t
AttributeEntry::FillDirent(struct dirent *dirent, size_t bufferSize,
	uint32 *numEntries)
{
	dirent->d_dev = dirent->d_pdev = fParent->VolumeID();
	dirent->d_ino = (ino_t)this;
	dirent->d_pino = fParent->ParentInode();
	dirent->d_reclen = sizeof(struct dirent) + fEntry->name_length;
	if (bufferSize < dirent->d_reclen) {
		*numEntries = 0;
		return B_BAD_VALUE;
	}

	strncpy(dirent->d_name, fEntry->name, fEntry->name_length);
	dirent->d_name[fEntry->name_length - 1] = 0;
	*numEntries = 1;
	return B_OK;
}


status_t
AttributeEntry::Read(off_t position, void *buffer, size_t *length)
{
	*length = min_c(*length, fEntry->size);
	if (*length <= position) {
		*length = 0;
		return B_OK;
	}

	*length -= position;
	memcpy(buffer, fData + position, *length);
	return B_OK;
}


status_t
AttributeEntry::ReadStat(struct stat *stat)
{
	stat->st_dev = fParent->VolumeID();
	stat->st_ino = (ino_t)this;
	stat->st_nlink = 1;
	stat->st_blksize = 512;
	stat->st_uid = 1;
	stat->st_gid = 1;
	stat->st_size = fEntry->size;
	stat->st_mode = S_ATTR | 0x0777;
	stat->st_type = fEntry->type;
	stat->st_atime = stat->st_mtime = stat->st_crtime = time(NULL);
	stat->st_blocks = (fEntry->size + stat->st_blksize - 1) / stat->st_blksize;
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
	OVERLAY_CALL(read_stat, stat)
	return B_UNSUPPORTED;
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
	OVERLAY_CALL(open, openMode, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_close(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(close, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_free_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(free_cookie, cookie)
	return B_UNSUPPORTED;
}


static status_t
overlay_read(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	void *buffer, size_t *length)
{
	OVERLAY_CALL(read, cookie, pos, buffer, length)
	return B_UNSUPPORTED;
}


static status_t
overlay_write(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const void *buffer, size_t *length)
{
	OVERLAY_CALL(write, cookie, pos, buffer, length)
	return B_UNSUPPORTED;
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
	TRACE("relaying op: read_dir\n");
	OverlayInode *node = (OverlayInode *)vnode->private_node;
	fs_vnode *superVnode = node->SuperVnode();
	if (superVnode->ops->read_dir != NULL) {
		status_t result = superVnode->ops->read_dir(volume->super_volume,
			superVnode, cookie, buffer, bufferSize, num);

		// TODO: handle multiple records
		if (result == B_OK && *num == 1 && strcmp(buffer->d_name,
			ATTRIBUTE_OVERLAY_ATTRIBUTE_DIR_NAME) == 0) {
			// skip over the attribute directory
			return superVnode->ops->read_dir(volume->super_volume, superVnode,
				cookie, buffer, bufferSize, num);
		}

		return result;
	}

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
	AttributeFile *attributeFile = node->GetAttributeFile();
	if (attributeFile == NULL)
		return B_UNSUPPORTED;

	*cookie = attributeFile;
	return B_OK;
}


static status_t
overlay_close_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(close_attr_dir, cookie)
	return B_OK;
}


static status_t
overlay_free_attr_dir_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(free_attr_dir_cookie, cookie)
	return B_OK;
}


static status_t
overlay_read_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *num)
{
	OVERLAY_CALL(read_attr_dir, cookie, buffer, bufferSize, num)
	AttributeFile *attributeFile = (AttributeFile *)cookie;
	return attributeFile->ReadAttributeDir(buffer, bufferSize, num);
}


static status_t
overlay_rewind_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(rewind_attr_dir, cookie)
	AttributeFile *attributeFile = (AttributeFile *)cookie;
	return attributeFile->RewindAttributeDir();
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

	AttributeFile *attributeFile = node->GetAttributeFile();
	if (attributeFile == NULL)
		return B_UNSUPPORTED;

	AttributeEntry *entry = attributeFile->FindAttribute(name);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	*cookie = entry;
	return B_OK;
}


static status_t
overlay_close_attr(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(close_attr, cookie)
	return B_OK;
}


static status_t
overlay_free_attr_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(free_attr_cookie, cookie)
	return B_OK;
}


static status_t
overlay_read_attr(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	void *buffer, size_t *length)
{
	OVERLAY_CALL(read_attr, cookie, pos, buffer, length)
	AttributeEntry *entry = (AttributeEntry *)cookie;
	return entry->Read(pos, buffer, length);
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
	AttributeEntry *entry = (AttributeEntry *)cookie;
	return entry->ReadStat(stat);
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
	if (volume->super_volume->ops->unmount != NULL)
		return volume->super_volume->ops->unmount(volume->super_volume);
	return B_UNSUPPORTED;
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

		info->flags |= B_FS_HAS_MIME | B_FS_HAS_ATTR | B_FS_HAS_QUERY;
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
			volume->super_volume, vnode);
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
overlay_create_sub_vnode(fs_volume *volume, ino_t id, fs_vnode *vnode)
{
	OverlayInode *node = new(std::nothrow) OverlayInode(volume->super_volume,
		vnode);
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

	&overlay_create_sub_vnode,
	&overlay_delete_sub_vnode
};


//	#pragma mark - filesystem module


static status_t
overlay_mount(fs_volume *volume, const char *device, uint32 flags,
	const char *args, ino_t *rootID)
{
	TRACE_VOLUME("mounting overlay\n");
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
		"file_systems/overlay"B_CURRENT_FS_API_VERSION,
		0,
		overlay_std_ops,
	},

	"overlay",					// short_name
	"Overlay File System",		// pretty_name
	0,							// DDM flags

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

}	// namespace _overlay

using namespace overlay;

module_info *modules[] = {
	(module_info *)&sOverlayFileSystem,
	NULL,
};
