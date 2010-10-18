/*
 * Copyright 2009-2010, Haiku Inc. All rights reserved.
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
#	define TRACE(x...)			dprintf("attribute_overlay: " x)
#	define TRACE_VOLUME(x...)	dprintf("attribute_overlay: " x)
#	define TRACE_ALWAYS(x...)	dprintf("attribute_overlay: " x)
#else
#	define TRACE(x...)			/* nothing */
#	define TRACE_VOLUME(x...)	/* nothing */
#	define TRACE_ALWAYS(x...)	dprintf("attribute_overlay: " x)
#endif


#define ATTRIBUTE_OVERLAY_FILE_MAGIC			'attr'
#define ATTRIBUTE_OVERLAY_ATTRIBUTE_DIR_NAME	"_HAIKU"


#define OVERLAY_CALL(op, params...) \
	TRACE("relaying op: " #op "\n"); \
	OverlayInode *node = (OverlayInode *)vnode->private_node; \
	fs_vnode *superVnode = node->SuperVnode(); \
	if (superVnode->ops->op != NULL) \
		return superVnode->ops->op(volume->super_volume, superVnode, params); \
	return B_UNSUPPORTED;


#define OVERLAY_VOLUME_CALL(op, params...) \
	TRACE_VOLUME("relaying volume op: " #op "\n"); \
	if (volume->super_volume->ops->op != NULL) \
		return volume->super_volume->ops->op(volume->super_volume, params);


namespace attribute_overlay {

class AttributeFile;
class AttributeEntry;


struct attribute_dir_cookie {
	AttributeFile *	file;
	uint32			index;
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

		status_t			GetAttributeFile(AttributeFile **attributeFile);
		status_t			WriteAttributeFile();
		status_t			RemoveAttributeFile();

private:
		OverlayVolume *		fVolume;
		fs_vnode			fSuperVnode;
		ino_t				fInodeNumber;
		AttributeFile *		fAttributeFile;
};


class AttributeFile {
public:
								AttributeFile(fs_volume *overlay,
									fs_volume *volume, fs_vnode *vnode);
								~AttributeFile();

			status_t			InitCheck() { return fStatus; }

			dev_t				VolumeID() { return fVolumeID; }
			ino_t				FileInode() { return fFileInode; }

			status_t			CreateEmpty();
			status_t			WriteAttributeFile(fs_volume *overlay,
									fs_volume *volume, fs_vnode *vnode);
			status_t			RemoveAttributeFile(fs_volume *overlay,
									fs_volume *volume, fs_vnode *vnode);

			status_t			ReadAttributeDir(struct dirent *dirent,
									size_t bufferSize, uint32 *numEntries,
									uint32 *index);

			uint32				CountAttributes();
			AttributeEntry *	FindAttribute(const char *name,
									uint32 *index = NULL);

			status_t			CreateAttribute(const char *name, type_code type,
									int openMode, AttributeEntry **entry);
			status_t			OpenAttribute(const char *name, int openMode,
									AttributeEntry **entry);
			status_t			RemoveAttribute(const char *name,
									AttributeEntry **entry);
			status_t			AddAttribute(AttributeEntry *entry);

private:
			struct attribute_file {
				uint32			magic;
					// ATTRIBUTE_OVERLAY_FILE_MAGIC
				uint32			entry_count;
				uint8			entries[1];
			} _PACKED;

			status_t			fStatus;
			dev_t				fVolumeID;
			ino_t				fFileInode;
			ino_t				fDirectoryInode;
			ino_t				fAttributeDirInode;
			ino_t				fAttributeFileInode;
			attribute_file *	fFile;
			uint32				fAttributeDirIndex;
			AttributeEntry **	fEntries;
};


class AttributeEntry {
public:
								AttributeEntry(AttributeFile *parent,
									uint8 *buffer);
								AttributeEntry(AttributeFile *parent,
									const char *name, type_code type);
								~AttributeEntry();

			status_t			InitCheck() { return fStatus; }

			uint8 *				Entry() { return (uint8 *)fEntry; }
			size_t				EntrySize();
			uint8 *				Data() { return fData; }
			size_t				DataSize() { return fEntry->size; }

			status_t			SetType(type_code type);
			type_code			Type() { return fEntry->type; }

			status_t			SetSize(size_t size);
			uint32				Size() { return fEntry->size; }

			status_t			SetName(const char *name);
			const char *		Name() { return fEntry->name; }
			uint8				NameLength() { return fEntry->name_length; }

			status_t			FillDirent(struct dirent *dirent,
									size_t bufferSize, uint32 *numEntries);

			status_t			Read(off_t position, void *buffer,
									size_t *length);
			status_t			Write(off_t position, const void *buffer,
									size_t *length);

			status_t			ReadStat(struct stat *stat);
			status_t			WriteStat(const struct stat *stat,
									uint32 statMask);

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
			status_t			fStatus;
			bool				fAllocatedEntry;
			bool				fAllocatedData;
};


//	#pragma mark OverlayVolume


OverlayVolume::OverlayVolume(fs_volume *volume)
	:
	fVolume(volume)
{
}


OverlayVolume::~OverlayVolume()
{
}


//	#pragma mark OverlayInode


OverlayInode::OverlayInode(OverlayVolume *volume, fs_vnode *superVnode,
	ino_t inodeNumber)
	:
	fVolume(volume),
	fSuperVnode(*superVnode),
	fInodeNumber(inodeNumber),
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


status_t
OverlayInode::GetAttributeFile(AttributeFile **attributeFile)
{
	if (fAttributeFile == NULL) {
		fAttributeFile = new(std::nothrow) AttributeFile(Volume(),
			SuperVolume(), &fSuperVnode);
		if (fAttributeFile == NULL) {
			TRACE_ALWAYS("no memory to allocate attribute file\n");
			return B_NO_MEMORY;
		}
	}

	status_t result = fAttributeFile->InitCheck();
	if (result != B_OK) {
		if (result == B_ENTRY_NOT_FOUND) {
			// TODO: need to check if we're able to create the file
			// but at least allow virtual attributes for now
		}

		result = fAttributeFile->CreateEmpty();
		if (result != B_OK)
			return result;
	}

	*attributeFile = fAttributeFile;
	return B_OK;
}


status_t
OverlayInode::WriteAttributeFile()
{
	if (fAttributeFile == NULL)
		return B_NO_INIT;

	status_t result = fAttributeFile->InitCheck();
	if (result != B_OK)
		return result;

	return fAttributeFile->WriteAttributeFile(Volume(), SuperVolume(),
		&fSuperVnode);
}


status_t
OverlayInode::RemoveAttributeFile()
{
	if (fAttributeFile == NULL)
		return B_NO_INIT;

	status_t result = fAttributeFile->InitCheck();
	if (result != B_OK)
		return result;

	return fAttributeFile->RemoveAttributeFile(Volume(), SuperVolume(),
		&fSuperVnode);
}


//	#pragma mark AttributeFile


AttributeFile::AttributeFile(fs_volume *overlay, fs_volume *volume,
	fs_vnode *vnode)
	:
	fStatus(B_NO_INIT),
	fVolumeID(volume->id),
	fFileInode(0),
	fDirectoryInode(0),
	fAttributeDirInode(0),
	fAttributeFileInode(0),
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
		fFileInode = stat.st_ino;
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

		if (i == 0)
			fDirectoryInode = inodeNumber;
		else if (i == 1)
			fAttributeDirInode = inodeNumber;
		else if (i == 2)
			fAttributeFileInode = inodeNumber;

		OverlayInode *overlayInode = NULL;
		fStatus = get_vnode(overlay, inodeNumber, (void **)&overlayInode);
		if (fStatus != B_OK) {
			TRACE_ALWAYS("getting vnode failed: %s\n", strerror(fStatus));
			return;
		}

		currentVnode = *overlayInode->SuperVnode();
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

	fEntries = (AttributeEntry **)malloc(fFile->entry_count
		* sizeof(AttributeEntry *));
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

		totalSize += fEntries[i]->EntrySize() + fEntries[i]->DataSize();
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

		free(fEntries);
	}

	free(fFile);
}


status_t
AttributeFile::CreateEmpty()
{
	if (fFile == NULL) {
		fFile = (attribute_file *)malloc(sizeof(attribute_file) - 1);
		if (fFile == NULL) {
			TRACE_ALWAYS("failed to allocate file buffer\n");
			fStatus = B_NO_MEMORY;
			return fStatus;
		}

		fFile->entry_count = 0;
		fFile->magic = ATTRIBUTE_OVERLAY_FILE_MAGIC;
	}

	fStatus = B_OK;
	return B_OK;
}


status_t
AttributeFile::WriteAttributeFile(fs_volume *overlay, fs_volume *volume,
	fs_vnode *vnode)
{
	if (fFile == NULL)
		return B_NO_INIT;

	char nameBuffer[B_FILE_NAME_LENGTH];
	nameBuffer[sizeof(nameBuffer) - 1] = 0;
	status_t result = vnode->ops->get_vnode_name(volume, vnode, nameBuffer,
		sizeof(nameBuffer) - 1);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to get vnode name: %s\n", strerror(result));
		return result;
	}

	fs_vnode currentVnode = *vnode;
	if (fDirectoryInode == 0) {
		if (currentVnode.ops->lookup == NULL) {
			TRACE_ALWAYS("lookup not possible, lookup hook missing\n");
			return B_UNSUPPORTED;
		}

		// see TODO above
		result = currentVnode.ops->lookup(volume, &currentVnode, "..",
			&fDirectoryInode);
		if (result != B_OK) {
			TRACE_ALWAYS("lookup of parent directory failed: %s\n",
				strerror(result));
			return B_UNSUPPORTED;
		}

		put_vnode(volume, fDirectoryInode);
	}

	OverlayInode *overlayInode = NULL;
	if (fAttributeDirInode == 0) {
		result = get_vnode(overlay, fDirectoryInode, (void **)&overlayInode);
		if (result != B_OK) {
			TRACE_ALWAYS("failed to get directory vnode: %s\n",
				strerror(result));
			return result;
		}

		currentVnode = *overlayInode->SuperVnode();

		// create the attribute directory
		result = currentVnode.ops->create_dir(volume, &currentVnode,
			ATTRIBUTE_OVERLAY_ATTRIBUTE_DIR_NAME, S_IRWXU | S_IRWXG | S_IRWXO);

		if (result == B_OK) {
			result = currentVnode.ops->lookup(volume, &currentVnode,
				ATTRIBUTE_OVERLAY_ATTRIBUTE_DIR_NAME, &fAttributeDirInode);

			// lookup() got us a reference we don't need -- put it
			if (result == B_OK)
				put_vnode(volume, fAttributeDirInode);
		}

		put_vnode(volume, fDirectoryInode);

		if (result != B_OK) {
			TRACE_ALWAYS("failed to create attribute directory: %s\n",
				strerror(result));
			fAttributeDirInode = 0;
			return result;
		}
	}

	void *attrFileCookie = NULL;
	if (fAttributeFileInode == 0) {
		result = get_vnode(overlay, fAttributeDirInode, (void **)&overlayInode);
		if (result != B_OK) {
			TRACE_ALWAYS("failed to get attribute directory vnode: %s\n",
				strerror(result));
			return result;
		}

		currentVnode = *overlayInode->SuperVnode();

		// create the attribute file
		result = currentVnode.ops->create(volume, &currentVnode,
			nameBuffer, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP
			| S_IWGRP | S_IROTH | S_IWOTH, &attrFileCookie,
			&fAttributeFileInode);

		put_vnode(volume, fAttributeDirInode);

		if (result != B_OK) {
			TRACE_ALWAYS("failed to create attribute file: %s\n",
				strerror(result));
			return result;
		}

		result = get_vnode(overlay, fAttributeFileInode,
			(void **)&overlayInode);
		if (result != B_OK) {
			TRACE_ALWAYS("getting attribute file vnode after create failed: "
				"%s\n", strerror(result));
			return result;
		}

		currentVnode = *overlayInode->SuperVnode();
	} else {
		result = get_vnode(overlay, fAttributeFileInode,
			(void **)&overlayInode);
		if (result != B_OK) {
			TRACE_ALWAYS("getting attribute file vnode failed: %s\n",
				strerror(result));
			return result;
		}

		currentVnode = *overlayInode->SuperVnode();

		// open the attribute file
		result = currentVnode.ops->open(volume, &currentVnode, O_RDWR | O_TRUNC,
			&attrFileCookie);
		if (result != B_OK) {
			TRACE_ALWAYS("failed to open attribute file for writing: %s\n",
				strerror(result));
			put_vnode(volume, fAttributeFileInode);
			return result;
		}
	}

	off_t position = 0;
	size_t writeLength = sizeof(attribute_file) - 1;
	result = currentVnode.ops->write(volume, &currentVnode, attrFileCookie,
		position, fFile, &writeLength);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to write to attribute file: %s\n",
			strerror(result));
		goto close_and_put;
	}

	for (uint32 i = 0; i < fFile->entry_count; i++) {
		writeLength = fEntries[i]->EntrySize();
		result = currentVnode.ops->write(volume, &currentVnode, attrFileCookie,
			position, fEntries[i]->Entry(), &writeLength);
		if (result != B_OK) {
			TRACE_ALWAYS("failed to write to attribute file: %s\n",
				strerror(result));
			goto close_and_put;
		}

		writeLength = fEntries[i]->DataSize();
		result = currentVnode.ops->write(volume, &currentVnode, attrFileCookie,
			position, fEntries[i]->Data(), &writeLength);
		if (result != B_OK) {
			TRACE_ALWAYS("failed to write to attribute file: %s\n",
				strerror(result));
			goto close_and_put;
		}
	}

close_and_put:
	if (currentVnode.ops->close != NULL)
		currentVnode.ops->close(volume, &currentVnode, attrFileCookie);
	if (currentVnode.ops->free_cookie != NULL)
		currentVnode.ops->free_cookie(volume, &currentVnode, attrFileCookie);

	put_vnode(volume, fAttributeFileInode);
	return B_OK;
}


status_t
AttributeFile::RemoveAttributeFile(fs_volume *overlay, fs_volume *volume,
	fs_vnode *vnode)
{
	bool hasAttributeFile = fAttributeFileInode != 0;
	ino_t attributeDirInode = fAttributeDirInode;

	// invalidate all of our cached inode numbers
	fDirectoryInode = 0;
	fAttributeDirInode = 0;
	fAttributeFileInode = 0;

	if (!hasAttributeFile) {
		// there is no backing file at all yet
		return B_OK;
	}

	char nameBuffer[B_FILE_NAME_LENGTH];
	nameBuffer[sizeof(nameBuffer) - 1] = 0;
	status_t result = vnode->ops->get_vnode_name(volume, vnode, nameBuffer,
		sizeof(nameBuffer) - 1);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to get vnode name: %s\n", strerror(result));
		return result;
	}

	OverlayInode *overlayInode = NULL;
	result = get_vnode(overlay, attributeDirInode, (void **)&overlayInode);
	if (result != B_OK) {
		TRACE_ALWAYS("getting attribute directory vnode failed: %s\n",
			strerror(result));
		return result;
	}

	fs_vnode attributeDir = *overlayInode->SuperVnode();
	if (attributeDir.ops->unlink == NULL) {
		TRACE_ALWAYS("cannot remove attribute file, unlink hook missing\n");
		put_vnode(volume, attributeDirInode);
		return B_UNSUPPORTED;
	}

	result = attributeDir.ops->unlink(volume, &attributeDir, nameBuffer);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to unlink attribute file: %s\n", strerror(result));
		put_vnode(volume, attributeDirInode);
		return result;
	}

	put_vnode(volume, attributeDirInode);
	return B_OK;
}


uint32
AttributeFile::CountAttributes()
{
	if (fFile == NULL)
		return 0;

	return fFile->entry_count;
}


AttributeEntry *
AttributeFile::FindAttribute(const char *name, uint32 *index)
{
	for (uint32 i = 0; i < fFile->entry_count; i++) {
		if (strcmp(fEntries[i]->Name(), name) == 0) {
			if (index)
				*index = i;

			return fEntries[i];
		}
	}

	return NULL;
}


status_t
AttributeFile::CreateAttribute(const char *name, type_code type, int openMode,
	AttributeEntry **_entry)
{
	AttributeEntry *existing = FindAttribute(name);
	if (existing != NULL) {
		if ((openMode & O_TRUNC) != 0)
			existing->SetSize(0);

		// attribute already exists, only allow if the attribute type is
		// compatible or the attribute size is 0
		if (existing->Type() != type) {
			if (existing->Size() != 0)
				return B_FILE_EXISTS;
			existing->SetType(type);
		}

		if (existing->InitCheck() == B_OK) {
			*_entry = existing;
			return B_OK;
		}

		// we tried to change the existing item but failed, try to just
		// remove it instead and creating a new one
		RemoveAttribute(name, NULL);
	}

	AttributeEntry *entry = new(std::nothrow) AttributeEntry(this, name, type);
	if (entry == NULL)
		return B_NO_MEMORY;

	status_t result = AddAttribute(entry);
	if (result != B_OK) {
		delete entry;
		return result;
	}

	*_entry = entry;
	return B_OK;
}


status_t
AttributeFile::OpenAttribute(const char *name, int openMode,
	AttributeEntry **_entry)
{
	AttributeEntry *entry = FindAttribute(name);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	if (openMode & O_TRUNC)
		entry->SetSize(0);

	*_entry = entry;
	return B_OK;
}


status_t
AttributeFile::RemoveAttribute(const char *name, AttributeEntry **_entry)
{
	uint32 index = 0;
	AttributeEntry *entry = FindAttribute(name, &index);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	for (uint32 i = index + 1; i < fFile->entry_count; i++)
		fEntries[i - 1] = fEntries[i];
	fFile->entry_count--;

	if (_entry)
		*_entry = entry;
	else
		delete entry;

	notify_attribute_changed(fVolumeID, fFileInode, name, B_ATTR_REMOVED);
	return B_OK;
}


status_t
AttributeFile::AddAttribute(AttributeEntry *entry)
{
	status_t result = entry->InitCheck();
	if (result != B_OK)
		return result;

	if (FindAttribute(entry->Name()) != NULL)
		return B_FILE_EXISTS;

	AttributeEntry **newEntries = (AttributeEntry **)realloc(fEntries,
		(fFile->entry_count + 1) * sizeof(AttributeEntry *));
	if (newEntries == NULL)
		return B_NO_MEMORY;

	fEntries = newEntries;
	fEntries[fFile->entry_count++] = entry;

	notify_attribute_changed(fVolumeID, fFileInode, entry->Name(),
		B_ATTR_CREATED);

	return B_OK;
}


status_t
AttributeFile::ReadAttributeDir(struct dirent *dirent, size_t bufferSize,
	uint32 *numEntries, uint32 *index)
{
	if (fFile == NULL || *index >= fFile->entry_count) {
		*numEntries = 0;
		return B_OK;
	}

	return fEntries[(*index)++]->FillDirent(dirent, bufferSize, numEntries);
}


//	#pragma mark AttributeEntry


AttributeEntry::AttributeEntry(AttributeFile *parent, uint8 *buffer)
	:
	fParent(parent),
	fEntry(NULL),
	fData(NULL),
	fStatus(B_NO_INIT),
	fAllocatedEntry(false),
	fAllocatedData(false)
{
	if (buffer == NULL)
		return;

	fEntry = (attribute_entry *)buffer;
	fData = (uint8 *)fEntry->name + fEntry->name_length;
	fStatus = B_OK;
}


AttributeEntry::AttributeEntry(AttributeFile *parent, const char *name,
	type_code type)
	:
	fParent(parent),
	fEntry(NULL),
	fData(NULL),
	fStatus(B_NO_INIT),
	fAllocatedEntry(false),
	fAllocatedData(false)
{
	fStatus = SetName(name);
	if (fStatus != B_OK)
		return;

	fEntry->type = type;
	fEntry->size = 0;
}


AttributeEntry::~AttributeEntry()
{
	if (fAllocatedEntry)
		free(fEntry);
	if (fAllocatedData)
		free(fData);
}


size_t
AttributeEntry::EntrySize()
{
	return sizeof(attribute_entry) - 1 + fEntry->name_length;
}


status_t
AttributeEntry::SetType(type_code type)
{
	fEntry->type = type;
	return B_OK;
}


status_t
AttributeEntry::SetSize(size_t size)
{
	if (size <= fEntry->size) {
		fEntry->size = size;
		return B_OK;
	}

	if (fAllocatedData) {
		uint8 *newData = (uint8 *)realloc(fData, size);
		if (newData == NULL) {
			fStatus = B_NO_MEMORY;
			return fStatus;
		}

		fData = newData;
		fEntry->size = size;
		return B_OK;
	}

	uint8 *newData = (uint8 *)malloc(size);
	if (newData == NULL) {
		fStatus = B_NO_MEMORY;
		return fStatus;
	}

	memcpy(newData, fData, min_c(fEntry->size, size));
	fEntry->size = size;
	fAllocatedData = true;
	fData = newData;
	return B_OK;
}


status_t
AttributeEntry::SetName(const char *name)
{
	size_t nameLength = strlen(name) + 1;
	if (nameLength > 255) {
		fStatus = B_NAME_TOO_LONG;
		return fStatus;
	}

	if (!fAllocatedEntry || fEntry->name_length < nameLength) {
		attribute_entry *newEntry = (attribute_entry *)malloc(
			sizeof(attribute_entry) - 1 + nameLength);
		if (newEntry == NULL) {
			fStatus = B_NO_MEMORY;
			return fStatus;
		}

		if (fEntry != NULL)
			memcpy(newEntry, fEntry, sizeof(attribute_entry) - 1);
		if (fAllocatedEntry)
			free(fEntry);

		fAllocatedEntry = true;
		fEntry = newEntry;
	}

	fEntry->name_length = nameLength;
	strlcpy(fEntry->name, name, nameLength);
	return B_OK;
}


status_t
AttributeEntry::FillDirent(struct dirent *dirent, size_t bufferSize,
	uint32 *numEntries)
{
	dirent->d_dev = dirent->d_pdev = fParent->VolumeID();
	dirent->d_ino = (ino_t)this;
	dirent->d_pino = fParent->FileInode();
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
	*length = min_c(*length, fEntry->size - position);
	memcpy(buffer, fData + position, *length);
	return B_OK;
}


status_t
AttributeEntry::Write(off_t position, const void *buffer, size_t *length)
{
	size_t neededSize = position + *length;
	if (neededSize > fEntry->size) {
		status_t result = SetSize(neededSize);
		if (result != B_OK) {
			*length = 0;
			return result;
		}
	}

	memcpy(fData + position, buffer, *length);
	notify_attribute_changed(fParent->VolumeID(), fParent->FileInode(),
		fEntry->name, B_ATTR_CHANGED);
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


status_t
AttributeEntry::WriteStat(const struct stat *stat, uint32 statMask)
{
	return B_UNSUPPORTED;
}


//	#pragma mark - vnode ops


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
}


static status_t
overlay_get_vnode_name(fs_volume *volume, fs_vnode *vnode, char *buffer,
	size_t bufferSize)
{
	OVERLAY_CALL(get_vnode_name, buffer, bufferSize)
}


static bool
overlay_can_page(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	TRACE("relaying op: can_page\n");
	OverlayInode *node = (OverlayInode *)vnode->private_node;
	fs_vnode *superVnode = node->SuperVnode();

	if (superVnode->ops->can_page != NULL) {
		return superVnode->ops->can_page(volume->super_volume, superVnode,
			cookie);
	}

	return false;
}


static status_t
overlay_read_pages(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *numBytes)
{
	OVERLAY_CALL(read_pages, cookie, pos, vecs, count, numBytes)
}


static status_t
overlay_write_pages(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *numBytes)
{
	OVERLAY_CALL(write_pages, cookie, pos, vecs, count, numBytes)
}


static status_t
overlay_io(fs_volume *volume, fs_vnode *vnode, void *cookie,
	io_request *request)
{
	OVERLAY_CALL(io, cookie, request)
}


static status_t
overlay_cancel_io(fs_volume *volume, fs_vnode *vnode, void *cookie,
	io_request *request)
{
	OVERLAY_CALL(cancel_io, cookie, request)
}


static status_t
overlay_get_file_map(fs_volume *volume, fs_vnode *vnode, off_t offset,
	size_t size, struct file_io_vec *vecs, size_t *count)
{
	OVERLAY_CALL(get_file_map, offset, size, vecs, count)
}


static status_t
overlay_ioctl(fs_volume *volume, fs_vnode *vnode, void *cookie, uint32 op,
	void *buffer, size_t length)
{
	OVERLAY_CALL(ioctl, cookie, op, buffer, length)
}


static status_t
overlay_set_flags(fs_volume *volume, fs_vnode *vnode, void *cookie,
	int flags)
{
	OVERLAY_CALL(set_flags, cookie, flags)
}


static status_t
overlay_select(fs_volume *volume, fs_vnode *vnode, void *cookie, uint8 event,
	selectsync *sync)
{
	OVERLAY_CALL(select, cookie, event, sync)
}


static status_t
overlay_deselect(fs_volume *volume, fs_vnode *vnode, void *cookie, uint8 event,
	selectsync *sync)
{
	OVERLAY_CALL(deselect, cookie, event, sync)
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
}


static status_t
overlay_create_symlink(fs_volume *volume, fs_vnode *vnode, const char *name,
	const char *path, int mode)
{
	OVERLAY_CALL(create_symlink, name, path, mode)
}


static status_t
overlay_link(fs_volume *volume, fs_vnode *vnode, const char *name,
	fs_vnode *target)
{
	OverlayInode *targetNode = (OverlayInode *)target->private_node;
	OVERLAY_CALL(link, name, targetNode->SuperVnode())
}


static status_t
overlay_unlink(fs_volume *volume, fs_vnode *vnode, const char *name)
{
	OVERLAY_CALL(unlink, name)
}


static status_t
overlay_rename(fs_volume *volume, fs_vnode *vnode,
	const char *fromName, fs_vnode *toDir, const char *toName)
{
	OverlayInode *toDirNode = (OverlayInode *)toDir->private_node;
	OVERLAY_CALL(rename, fromName, toDirNode->SuperVnode(), toName)
}


static status_t
overlay_access(fs_volume *volume, fs_vnode *vnode, int mode)
{
	OVERLAY_CALL(access, mode)
}


static status_t
overlay_read_stat(fs_volume *volume, fs_vnode *vnode, struct stat *stat)
{
	OVERLAY_CALL(read_stat, stat)
}


static status_t
overlay_write_stat(fs_volume *volume, fs_vnode *vnode, const struct stat *stat,
	uint32 statMask)
{
	OVERLAY_CALL(write_stat, stat, statMask)
}


static status_t
overlay_create(fs_volume *volume, fs_vnode *vnode, const char *name,
	int openMode, int perms, void **cookie, ino_t *newVnodeID)
{
	OVERLAY_CALL(create, name, openMode, perms, cookie, newVnodeID)
}


static status_t
overlay_open(fs_volume *volume, fs_vnode *vnode, int openMode, void **cookie)
{
	OVERLAY_CALL(open, openMode, cookie)
}


static status_t
overlay_close(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(close, cookie)
}


static status_t
overlay_free_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(free_cookie, cookie)
}


static status_t
overlay_read(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	void *buffer, size_t *length)
{
	OVERLAY_CALL(read, cookie, pos, buffer, length)
}


static status_t
overlay_write(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const void *buffer, size_t *length)
{
	OVERLAY_CALL(write, cookie, pos, buffer, length)
}


static status_t
overlay_create_dir(fs_volume *volume, fs_vnode *vnode, const char *name,
	int perms)
{
	OVERLAY_CALL(create_dir, name, perms)
}


static status_t
overlay_remove_dir(fs_volume *volume, fs_vnode *vnode, const char *name)
{
	OVERLAY_CALL(remove_dir, name)
}


static status_t
overlay_open_dir(fs_volume *volume, fs_vnode *vnode, void **cookie)
{
	OVERLAY_CALL(open_dir, cookie)
}


static status_t
overlay_close_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(close_dir, cookie)
}


static status_t
overlay_free_dir_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	OVERLAY_CALL(free_dir_cookie, cookie)
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
}


static status_t
overlay_open_attr_dir(fs_volume *volume, fs_vnode *vnode, void **cookie)
{
	OverlayInode *node = (OverlayInode *)vnode->private_node;
	AttributeFile *attributeFile = NULL;
	status_t result = node->GetAttributeFile(&attributeFile);
	if (result != B_OK)
		return result;

	attribute_dir_cookie *dirCookie = (attribute_dir_cookie *)malloc(
		sizeof(attribute_dir_cookie));
	if (dirCookie == NULL)
		return B_NO_MEMORY;

	dirCookie->file = attributeFile;
	dirCookie->index = 0;
	*cookie = dirCookie;
	return B_OK;
}


static status_t
overlay_close_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	return B_OK;
}


static status_t
overlay_free_attr_dir_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	free(cookie);
	return B_OK;
}


static status_t
overlay_read_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *num)
{
	attribute_dir_cookie *dirCookie = (attribute_dir_cookie *)cookie;
	return dirCookie->file->ReadAttributeDir(buffer, bufferSize, num,
		&dirCookie->index);
}


static status_t
overlay_rewind_attr_dir(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	attribute_dir_cookie *dirCookie = (attribute_dir_cookie *)cookie;
	dirCookie->index = 0;
	return B_OK;
}


static status_t
overlay_create_attr(fs_volume *volume, fs_vnode *vnode, const char *name,
	uint32 type, int openMode, void **cookie)
{
	OverlayInode *node = (OverlayInode *)vnode->private_node;
	AttributeFile *attributeFile = NULL;
	status_t result = node->GetAttributeFile(&attributeFile);
	if (result != B_OK)
		return result;

	return attributeFile->CreateAttribute(name, type, openMode,
		(AttributeEntry **)cookie);
}


static status_t
overlay_open_attr(fs_volume *volume, fs_vnode *vnode, const char *name,
	int openMode, void **cookie)
{
	OverlayInode *node = (OverlayInode *)vnode->private_node;
	AttributeFile *attributeFile = NULL;
	status_t result = node->GetAttributeFile(&attributeFile);
	if (result != B_OK)
		return result;

	return attributeFile->OpenAttribute(name, openMode,
		(AttributeEntry **)cookie);
}


static status_t
overlay_close_attr(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	return B_OK;
}


static status_t
overlay_free_attr_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie)
{
	return B_OK;
}


static status_t
overlay_read_attr(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	void *buffer, size_t *length)
{
	return ((AttributeEntry *)cookie)->Read(pos, buffer, length);
}


static status_t
overlay_write_attr(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
	const void *buffer, size_t *length)
{
	return ((AttributeEntry *)cookie)->Write(pos, buffer, length);
}


static status_t
overlay_read_attr_stat(fs_volume *volume, fs_vnode *vnode, void *cookie,
	struct stat *stat)
{
	return ((AttributeEntry *)cookie)->ReadStat(stat);
}


static status_t
overlay_write_attr_stat(fs_volume *volume, fs_vnode *vnode, void *cookie,
	const struct stat *stat, int statMask)
{
	return ((AttributeEntry *)cookie)->WriteStat(stat, statMask);
}


static status_t
overlay_rename_attr(fs_volume *volume, fs_vnode *vnode,
	const char *fromName, fs_vnode *toVnode, const char *toName)
{
	OverlayInode *node = (OverlayInode *)vnode->private_node;
	AttributeFile *attributeFile = NULL;
	status_t result = node->GetAttributeFile(&attributeFile);
	if (result != B_OK)
		return B_OK;

	AttributeFile *toAttributeFile = attributeFile;
	if (vnode->private_node != toVnode->private_node) {
		OverlayInode *toNode = (OverlayInode *)toVnode->private_node;
		result = toNode->GetAttributeFile(&toAttributeFile);
		if (result != B_OK)
			return result;
	}

	AttributeEntry *entry = NULL;
	result = attributeFile->RemoveAttribute(fromName, &entry);
	if (result != B_OK)
		return result;

	result = entry->SetName(toName);
	if (result != B_OK) {
		if (attributeFile->AddAttribute(entry) != B_OK)
			delete entry;
		return result;
	}

	result = toAttributeFile->AddAttribute(entry);
	if (result != B_OK) {
		if (entry->SetName(fromName) != B_OK
			|| attributeFile->AddAttribute(entry) != B_OK)
			delete entry;
		return result;
	}

	return B_OK;
}


static status_t
overlay_remove_attr(fs_volume *volume, fs_vnode *vnode, const char *name)
{
	OverlayInode *node = (OverlayInode *)vnode->private_node;
	AttributeFile *attributeFile = NULL;
	status_t result = node->GetAttributeFile(&attributeFile);
	if (result != B_OK)
		return result;

	return attributeFile->RemoveAttribute(name, NULL);
}


static status_t
overlay_create_special_node(fs_volume *volume, fs_vnode *vnode,
	const char *name, fs_vnode *subVnode, mode_t mode, uint32 flags,
	fs_vnode *_superVnode, ino_t *nodeID)
{
	OVERLAY_CALL(create_special_node, name, subVnode, mode, flags, _superVnode,
		nodeID)
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

		info->flags |= B_FS_HAS_MIME | B_FS_HAS_ATTR /*| B_FS_HAS_QUERY*/;
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
			vnode->ops->put_vnode(volume->super_volume, vnode, reenter);
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
	delete (OverlayInode *)vnode->private_node;
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
	TRACE_VOLUME("mounting attribute overlay\n");
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
		"file_systems/attribute_overlay" B_CURRENT_FS_API_VERSION,
		0,
		overlay_std_ops,
	},

	"attribute_overlay",				// short_name
	"Attribute Overlay File System",	// pretty_name
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

}	// namespace attribute_overlay

using namespace attribute_overlay;

module_info *modules[] = {
	(module_info *)&sOverlayFileSystem,
	NULL,
};
