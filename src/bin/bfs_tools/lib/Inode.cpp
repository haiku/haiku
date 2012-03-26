/*
 * Copyright 2001-2008 pinc Software. All Rights Reserved.
 */

//!	BFS Inode classes


#include "Inode.h"
#include "BPlusTree.h"

#include <Directory.h>
#include <SymLink.h>
#include <Entry.h>
#include <Path.h>
#include <String.h>

#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


class NodeGetter {
	public:
		NodeGetter(Inode* inode)
			:
			fInode(inode)
		{
			fInode->AcquireBuffer();
		}

		~NodeGetter()
		{
			fInode->ReleaseBuffer();
		}

	private:
		Inode*	fInode;
};


//	#pragma mark -


Inode::Inode(Disk* disk, bfs_inode* inode, bool ownBuffer)
	:
	fDisk(disk),
	fInode(inode),
	fOwnBuffer(ownBuffer),
	fPath(NULL),
	fRefCount(1),
	fCurrentSmallData(NULL),
	fAttributes(NULL),
	fAttributeBuffer(NULL)
{
	if (inode != NULL)
		fBlockRun = inode->inode_num;
}


Inode::Inode(const Inode& inode)
	:
	fDisk(inode.fDisk),
	fInode(inode.fInode),
	fOwnBuffer(false),
	fPath(NULL),
	fBlockRun(inode.fBlockRun),
	fRefCount(1),
	fCurrentSmallData(NULL),
	fAttributes(NULL),
	fAttributeBuffer(NULL)
{
}


Inode::~Inode()
{
	_Unset();
}


void
Inode::_Unset()
{
	if (fOwnBuffer)
		free(fInode);

	fInode = NULL;
	fBlockRun.SetTo(0, 0, 0);

	free(fPath);
	fPath = NULL;

	delete fAttributes; 
	fAttributes = NULL;
}


status_t
Inode::SetTo(bfs_inode *inode)
{
	_Unset();

	fInode = inode;
	fBlockRun = inode->inode_num;
	return B_OK;
}


status_t
Inode::InitCheck()
{
	if (!fInode)
		return B_ERROR;

	// test inode magic and flags
	if (fInode->magic1 != INODE_MAGIC1
		|| !(fInode->flags & INODE_IN_USE)
		|| fInode->inode_num.length != 1)
		return B_ERROR;

	if (fDisk->BlockSize()) {
			// matches known block size?
		if (fInode->inode_size != fDisk->SuperBlock()->inode_size
			// parent resides on disk?
			|| fInode->parent.allocation_group > fDisk->SuperBlock()->num_ags
			|| fInode->parent.allocation_group < 0
			|| fInode->parent.start > (1L << fDisk->SuperBlock()->ag_shift)
			|| fInode->parent.length != 1
			// attributes, too?
			|| fInode->attributes.allocation_group > fDisk->SuperBlock()->num_ags
			|| fInode->attributes.allocation_group < 0
			|| fInode->attributes.start > (1L << fDisk->SuperBlock()->ag_shift))
			return B_ERROR;
	} else {
		// is inode size one of the valid values?
		switch (fInode->inode_size) {
			case 1024:
			case 2048:
			case 4096:
			case 8192:
				break;
			default:
				return B_ERROR;
		}
	}
	return B_OK;
	// is inode on a boundary matching it's size?
	//return (Offset() % fInode->inode_size) == 0 ? B_OK : B_ERROR;
}


status_t
Inode::CopyBuffer()
{
	if (!fInode)
		return B_ERROR;

	bfs_inode *buffer = (bfs_inode *)malloc(fInode->inode_size);
	if (!buffer)
		return B_NO_MEMORY;

	memcpy(buffer, fInode, fInode->inode_size);
	fInode = buffer;
	fOwnBuffer = true;
	BufferClobbered();
		// this must not be deleted anymore

	return B_OK;
}


/*static*/ bool
Inode::_LowMemory()
{
	static bigtime_t lastChecked;
	static int32 percentUsed;

	if (system_time() > lastChecked + 1000000LL) {
		system_info info;
		get_system_info(&info);
		percentUsed = 100 * info.used_pages / info.max_pages;
	}

	return percentUsed > 75;
}


void
Inode::ReleaseBuffer()
{
	if (atomic_add(&fRefCount, -1) != 1)
		return;

	if (fOwnBuffer) {
		if (!_LowMemory())
			return;

		free(fInode);
		fInode = NULL;
	}
}


status_t
Inode::AcquireBuffer()
{
	if (atomic_add(&fRefCount, 1) != 0)
		return B_OK;

	if (!fOwnBuffer || fInode != NULL)
		return B_OK;

	fInode = (bfs_inode*)malloc(fDisk->BlockSize());
	if (fInode == NULL)
		return B_NO_MEMORY;

	ssize_t bytesRead = fDisk->ReadAt(Offset(), fInode, fDisk->BlockSize());
	if (bytesRead < B_OK)
		return bytesRead;

	return B_OK;
}


void
Inode::BufferClobbered()
{
	AcquireBuffer();
}


void
Inode::SetParent(const block_run& run)
{
	fInode->parent = run;
	BufferClobbered();
}


void
Inode::SetBlockRun(const block_run& run)
{
	fInode->inode_num = run;
	fBlockRun = run;
	BufferClobbered();
}


void
Inode::SetMode(uint32 mode)
{
	fInode->mode = mode;
	BufferClobbered();
}


status_t
Inode::SetName(const char *name)
{
	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	small_data *data = fInode->small_data_start, *nameData = NULL;
	BufferClobbered();

	while (!data->IsLast(fInode)) {
		if (data->type == FILE_NAME_TYPE
			&& data->name_size == FILE_NAME_NAME_LENGTH
			&& *data->Name() == FILE_NAME_NAME)
			nameData = data;

		data = data->Next();
	}

	int32 oldLength = nameData == NULL ? 0 : nameData->data_size;
	int32 newLength = strlen(name) + (nameData == NULL ? sizeof(small_data) + 5 : 0);

	if (int32(data) + newLength - oldLength >= (int32)(fInode + fDisk->BlockSize()))
		return B_NO_MEMORY;

	if (nameData == NULL) {
		memmove(newLength + (uint8 *)fInode->small_data_start,
			fInode->small_data_start,
			int32(data) - int32(fInode->small_data_start));
		nameData = fInode->small_data_start;
	} else {
		memmove(newLength + (uint8 *)nameData, nameData,
			int32(data) - int32(fInode->small_data_start));
	}

	memset(nameData, 0, sizeof(small_data) + 5 + strlen(name));
	nameData->type = FILE_NAME_TYPE;
	nameData->name_size = FILE_NAME_NAME_LENGTH;
	nameData->data_size = strlen(name);
	*nameData->Name() = FILE_NAME_NAME;
	strcpy((char *)nameData->Data(),name);

	return B_OK;
}


const char *
Inode::Name() const
{
	small_data *data = fInode->small_data_start;
	while (!data->IsLast(fInode)) {
		if (data->type == FILE_NAME_TYPE
			&& data->name_size == FILE_NAME_NAME_LENGTH
			&& *data->Name() == FILE_NAME_NAME)
			return (const char *)data->Data();

		data = data->Next();
	}
	return NULL;
}


status_t
Inode::GetNextSmallData(small_data **smallData)
{
	if (!fInode)
		return B_ERROR;

	small_data *data = *smallData;

	// begin from the start?
	if (data == NULL)
		data = fInode->small_data_start;
	else
		data = data->Next();

	// is already last item?
	if (data->IsLast(fInode))
		return B_ENTRY_NOT_FOUND;

	*smallData = data;
	return B_OK;
}


status_t
Inode::RewindAttributes()
{
	fCurrentSmallData = NULL;

	if (fAttributes != NULL)
		fAttributes->Rewind();

	return B_OK;
}


status_t
Inode::GetNextAttribute(char *name, uint32 *type, void **data, size_t *length)
{
	// read attributes out of the small data section

	if (fCurrentSmallData == NULL || !fCurrentSmallData->IsLast(fInode)) {
		if (fCurrentSmallData == NULL)
			fCurrentSmallData = fInode->small_data_start;
		else
			fCurrentSmallData = fCurrentSmallData->Next();

		// skip name attribute
		if (!fCurrentSmallData->IsLast(fInode)
			&& fCurrentSmallData->name_size == FILE_NAME_NAME_LENGTH
			&& *fCurrentSmallData->Name() == FILE_NAME_NAME)
			fCurrentSmallData = fCurrentSmallData->Next();

		if (!fCurrentSmallData->IsLast(fInode)) {
			strncpy(name,fCurrentSmallData->Name(), B_FILE_NAME_LENGTH);
			*type = fCurrentSmallData->type;
			*data = fCurrentSmallData->Data();
			*length = fCurrentSmallData->data_size;

			return B_OK;
		}
	}

	// read attributes out of the attribute directory

	if (Attributes().IsZero())
		return B_ENTRY_NOT_FOUND;

	if (fAttributes == NULL)
		fAttributes = (Directory *)Inode::Factory(fDisk, Attributes());

	status_t status = fAttributes ? fAttributes->InitCheck() : B_ERROR;
	if (status < B_OK)
		return status;

	block_run run;
	status = fAttributes->GetNextEntry(name, &run);
	if (status < B_OK) {
		free(fAttributeBuffer);
		fAttributeBuffer = NULL;
		return status;
	}

	Attribute *attribute = (Attribute *)Inode::Factory(fDisk, run);
	if (attribute == NULL || attribute->InitCheck() < B_OK)
		return B_IO_ERROR;

	*type = attribute->Type();

	void *buffer = realloc(fAttributeBuffer, attribute->Size());
	if (buffer == NULL) {
		free(fAttributeBuffer);
		fAttributeBuffer = NULL;
		delete attribute;
		return B_NO_MEMORY;
	}
	fAttributeBuffer = buffer;

	ssize_t size =  attribute->Read(fAttributeBuffer, attribute->Size());
	delete attribute;

	*length = size;
	*data = fAttributeBuffer;

	return size < B_OK ? size : B_OK;
}


status_t
Inode::_FindPath(Inode::Source *source)
{
	BString path;

	block_run parent = Parent();
	while (!parent.IsZero() && parent != fDisk->Root()) {
		Inode *inode;
		if (source)
			inode = source->InodeAt(parent);
		else
			inode = Inode::Factory(fDisk, parent);

		if (inode == NULL
			|| inode->InitCheck() < B_OK
			|| inode->Name() == NULL
			|| !*inode->Name()) {
			BString sub;
			sub << "__recovered " << parent.allocation_group << ":"
				<< (int32)parent.start << "/";
			path.Prepend(sub);

			delete inode;
			break;
		}
		parent = inode->Parent();
		path.Prepend("/");
		path.Prepend(inode->Name());

		delete inode;
	}
	fPath = strdup(path.String());

	return B_OK;
}


const char *
Inode::Path(Inode::Source *source)
{
	if (fPath == NULL)
		_FindPath(source);

	return fPath;
}


status_t
Inode::CopyTo(const char *root, bool fullPath, Inode::Source *source)
{
	if (root == NULL)
		return B_ENTRY_NOT_FOUND;

	BString path;

	if (fullPath)
		path.Append(Path(source));

	if (*(root + strlen(root) - 1) != '/')
		path.Prepend("/");
	path.Prepend(root);

	return create_directory(path.String(), 0777);
}


status_t
Inode::CopyAttributesTo(BNode *node)
{
	// copy attributes

	RewindAttributes();

	char name[B_FILE_NAME_LENGTH];
	const uint32 kMaxBrokenAttributes = 64;
		// sanity max value
	uint32 count = 0;
	uint32 type;
	void *data;
	size_t size;

	status_t status;
	while ((status = GetNextAttribute(name, &type, &data, &size))
			!= B_ENTRY_NOT_FOUND) {
		if (status != B_OK) {
			printf("could not open attribute (possibly: %s): %s!\n",
				name, strerror(status));
			if (count++ > kMaxBrokenAttributes)
				break;

			continue;
		}

		ssize_t written = node->WriteAttr(name, type, 0, data, size);
		if (written < B_OK) {
			printf("could not write attribute \"%s\": %s\n", name,
				strerror(written));
		} else if ((size_t)written < size) {
			printf("could only write %ld bytes (from %ld) at attribute \"%s\"\n",
				written, size, name); 
		}
	}

	// copy stats

	node->SetPermissions(fInode->mode);
	node->SetOwner(fInode->uid);
	node->SetGroup(fInode->gid);
	node->SetModificationTime(fInode->last_modified_time >> 16);
	node->SetCreationTime(fInode->create_time >> 16);

	return B_OK;
}


Inode *
Inode::Factory(Disk *disk, bfs_inode *inode, bool ownBuffer)
{
	// attributes (of a file)
	if ((inode->mode & (S_ATTR | S_ATTR_DIR)) == S_ATTR)
		return new Attribute(disk, inode, ownBuffer);

	// directories, attribute directories, indices
	if (S_ISDIR(inode->mode) || inode->mode & S_ATTR_DIR)
		return new Directory(disk, inode, ownBuffer);

	// regular files
	if (S_ISREG(inode->mode))
		return new File(disk, inode, ownBuffer);

	// symlinks (short and link in data-stream)
	if (S_ISLNK(inode->mode))
		return new Symlink(disk, inode, ownBuffer);

	return NULL;
}


Inode *
Inode::Factory(Disk *disk, block_run run)
{
	bfs_inode *inode = (bfs_inode *)malloc(disk->BlockSize());
	if (!inode)
		return NULL;

	if (disk->ReadAt(disk->ToOffset(run), inode, disk->BlockSize()) <= 0)
		return NULL;

	Inode *object = Factory(disk, inode);
	if (object == NULL)
		free(inode);

	return object;
}


Inode *
Inode::Factory(Disk *disk, Inode *inode, bool copyBuffer)
{
	bfs_inode *inodeBuffer = inode->fInode;

	if (copyBuffer) {
		bfs_inode *inodeCopy = (bfs_inode *)malloc(inodeBuffer->inode_size);
		if (!inodeCopy)
			return NULL;

		memcpy(inodeCopy, inodeBuffer, inodeBuffer->inode_size);
		inodeBuffer = inodeCopy;
	}
	return Factory(disk, inodeBuffer, copyBuffer);
}


Inode *
Inode::EmptyInode(Disk *disk, const char *name, int32 mode)
{
	bfs_inode *inode = (bfs_inode *)malloc(disk->BlockSize());
	if (!inode)
		return NULL;

	memset(inode, 0, sizeof(bfs_inode));

	inode->magic1 = INODE_MAGIC1;
	inode->inode_size = disk->BlockSize();
	inode->mode = mode;
	inode->flags = INODE_IN_USE | (mode & S_IFDIR ? INODE_LOGGED : 0);

	if (name) {
		small_data *data = inode->small_data_start;
		data->type = FILE_NAME_TYPE;
		data->name_size = FILE_NAME_NAME_LENGTH;
		*data->Name() = FILE_NAME_NAME;
		data->data_size = strlen(name);
		strcpy((char *)data->Data(), name);
	}

	Inode *object = new (std::nothrow) Inode(disk, inode);
	if (object == NULL)
		free(inode);

	object->AcquireBuffer();
		// this must not be deleted anymore!
	return object;
}


//	#pragma mark -


DataStream::DataStream(Disk *disk, bfs_inode *inode, bool ownBuffer)
	: Inode(disk,inode,ownBuffer),
	fCurrent(-1),
	fPosition(0LL)
{
}


DataStream::DataStream(const Inode &inode)
	: Inode(inode),
	fCurrent(-1),
	fPosition(0LL)
{
}


DataStream::~DataStream()
{
}


status_t
DataStream::FindBlockRun(off_t pos)
{
	NodeGetter _(this);

	if (pos > fInode->data.size)
		return B_ENTRY_NOT_FOUND;

	if (fCurrent < 0)
		fLevel = 0;

	fRunBlockEnd = fCurrent >= 0
		? fRunFileOffset + (fRun.length << fDisk->BlockShift()) : 0LL;

	// access in current block run?

	if (fCurrent >= 0 && pos >= fRunFileOffset && pos < fRunBlockEnd)
		return B_OK;

	// find matching block run

	if (fInode->data.max_direct_range > 0
		&& pos >= fInode->data.max_direct_range) {
		if (fInode->data.max_double_indirect_range > 0
			&& pos >= fInode->data.max_indirect_range) {
			// read from double indirect blocks

			//printf("find double indirect block: %ld,%d!\n",fInode->data.double_indirect.allocation_group,fInode->data.double_indirect.start);
			block_run *indirect = (block_run *)fDisk->ReadBlockRun(fInode->data.double_indirect);
			if (indirect == NULL)
				return B_ERROR;

			off_t start = pos - fInode->data.max_indirect_range;
			int32 indirectSize = fDisk->BlockSize() * 16 * (fDisk->BlockSize() / sizeof(block_run));
			int32 directSize = fDisk->BlockSize() * 4;
			int32 index = start / indirectSize;

			//printf("\tstart = %Ld, indirectSize = %ld, directSize = %ld, index = %ld\n",start,indirectSize,directSize,index);
			//printf("\tlook for indirect block at %ld,%d\n",indirect[index].allocation_group,indirect[index].start);
			indirect = (block_run *)fDisk->ReadBlockRun(indirect[index]);
			if (indirect == NULL)
				return B_ERROR;

			fCurrent = (start % indirectSize) / directSize;
			fRunFileOffset = fInode->data.max_indirect_range + (index * indirectSize) + (fCurrent * directSize);
			fRunBlockEnd = fRunFileOffset + directSize;
			fRun = indirect[fCurrent];
			//printf("\tfCurrent = %ld, fRunFileOffset = %Ld, fRunBlockEnd = %Ld, fRun = %ld,%d\n",fCurrent,fRunFileOffset,fRunBlockEnd,fRun.allocation_group,fRun.start);
		} else {
			// access from indirect blocks

			block_run *indirect = (block_run *)fDisk->ReadBlockRun(fInode->data.indirect);
			if (!indirect)
				return B_ERROR;

			int32 indirectRuns = (fInode->data.indirect.length << fDisk->BlockShift()) / sizeof(block_run);

			if (fLevel != 1 || pos < fRunFileOffset) {
				fRunBlockEnd = fInode->data.max_direct_range;
				fCurrent = -1;
				fLevel = 1;
			}

			while (++fCurrent < indirectRuns) {
				if (indirect[fCurrent].IsZero())
					break;

				fRunFileOffset = fRunBlockEnd;
				fRunBlockEnd += indirect[fCurrent].length << fDisk->BlockShift();
				if (fRunBlockEnd > pos)
					break;
			}
			if (fCurrent == indirectRuns || indirect[fCurrent].IsZero())
				return B_ERROR;

			fRun = indirect[fCurrent];
			//printf("reading from indirect block: %ld,%d\n",fRun.allocation_group,fRun.start);
			//printf("### indirect-run[%ld] = (%ld,%d,%d), offset = %Ld\n",fCurrent,fRun.allocation_group,fRun.start,fRun.length,fRunFileOffset);
		}
	} else {
		// access from direct blocks
		if (fRunFileOffset > pos) {
			fRunBlockEnd = 0LL;
			fCurrent = -1;
		}
		fLevel = 0;

		while (++fCurrent < NUM_DIRECT_BLOCKS) {
			if (fInode->data.direct[fCurrent].IsZero())
				break;

			fRunFileOffset = fRunBlockEnd;
			fRunBlockEnd += fInode->data.direct[fCurrent].length << fDisk->BlockShift();
			if (fRunBlockEnd > pos)
				break;
		}
		if (fCurrent == NUM_DIRECT_BLOCKS || fInode->data.direct[fCurrent].IsZero())
			return B_ERROR;

		fRun = fInode->data.direct[fCurrent];
		//printf("### run[%ld] = (%ld,%d,%d), offset = %Ld\n",fCurrent,fRun.allocation_group,fRun.start,fRun.length,fRunFileOffset);
	}
	return B_OK;
}


ssize_t
DataStream::ReadAt(off_t pos, void *buffer, size_t size)
{
	NodeGetter _(this);

	//printf("DataStream::ReadAt(pos = %Ld,buffer = %p,size = %ld);\n",pos,buffer,size);
	// truncate size to read
	if (pos + size > fInode->data.size) {
		if (pos > fInode->data.size)	// reading outside the file
			return B_ERROR;

		size = fInode->data.size - pos;
		if (!size)	// there is nothing left to read
			return 0;
	}
	ssize_t read = 0;

	//printf("### read %ld bytes at %Ld\n",size,pos);
	while (size > 0) {
		status_t status = FindBlockRun(pos);
		if (status < B_OK)
			return status;

		ssize_t bytes = min_c(size, fRunBlockEnd - pos);

		//printf("### read %ld bytes from %Ld\n### --\n",bytes,fDisk->ToOffset(fRun) + pos - fRunFileOffset);
		bytes = fDisk->ReadAt(fDisk->ToOffset(fRun) + pos - fRunFileOffset,
			buffer, bytes);
		if (bytes <= 0) {
			if (bytes == 0) {
				printf("could not read bytes at: %ld,%d\n",
					fRun.allocation_group, fRun.start);
			}
			return bytes < 0 ? bytes : B_BAD_DATA;
		}

		buffer = (void *)((uint8 *)buffer + bytes);
		size -= bytes;
		pos += bytes;
		read += bytes;
	}
	if (read >= 0)
		return read;

	return B_IO_ERROR;
}


ssize_t
DataStream::WriteAt(off_t pos, const void *buffer, size_t size)
{
	NodeGetter _(this);

	// FIXME: truncate size -> should enlargen the file
	if (pos + size > fInode->data.size) {
		if (pos > fInode->data.size)	// writing outside the file
			return B_ERROR;

		size = fInode->data.size - pos;
		if (!size)	// there is nothing left to write
			return 0;
	}
	ssize_t written = 0;

	//printf("### write %ld bytes at %Ld\n",size,pos);
	while (size > 0) {
		status_t status = FindBlockRun(pos);
		if (status < B_OK)
			return status;

		ssize_t bytes = min_c(size,fRunBlockEnd - pos);

		//printf("### write %ld bytes to %Ld\n### --\n",bytes,fDisk->ToOffset(fRun) + pos - fRunFileOffset);
		bytes = fDisk->WriteAt(fDisk->ToOffset(fRun) + pos - fRunFileOffset,buffer,bytes);
		if (bytes < 0)
			return bytes;

		buffer = (void *)((uint8 *)buffer + bytes);
		size -= bytes;
		pos += bytes;
		written += bytes;
	}
	if (written >= 0)
		return written;

	return B_IO_ERROR;
}


off_t
DataStream::Seek(off_t position, uint32 seekMode)
{
	NodeGetter _(this);

	if (seekMode == SEEK_SET)
		fPosition = position;
	else if (seekMode == SEEK_END)
		fPosition = fInode->data.size + position;
	else
		fPosition += position;

	return fPosition;
}


off_t
DataStream::Position() const
{
	return fPosition;
}


status_t
DataStream::SetSize(off_t size)
{
	NodeGetter _(this);

	// FIXME: not yet supported
	if (size > fInode->data.size || size > fInode->data.max_direct_range)
		return B_ERROR;

	if (size == fInode->data.size)
		return B_OK;

	BufferClobbered();

	fInode->data.size = size;
	fInode->data.max_direct_range = size;
	fInode->data.max_indirect_range = 0;
	fInode->data.max_double_indirect_range = 0;

	for (int32 i = 0;i < NUM_DIRECT_BLOCKS;i++) {
		if (size <= 0)
			fInode->data.direct[i].SetTo(0, 0, 0);
		else if ((fInode->data.direct[i].length << fDisk->BlockShift()) >= size) {
			off_t blocks = (size + fDisk->BlockSize() - 1) / fDisk->BlockSize();
			fInode->data.direct[i].length = blocks;
			size = 0;
		} else
			size -= fInode->data.direct[i].length << fDisk->BlockShift();
	}

	return B_OK;
}


//	#pragma mark -


File::File(Disk *disk, bfs_inode *inode,bool ownBuffer)
	: DataStream(disk,inode,ownBuffer)
{
}


File::File(const Inode &inode)
	: DataStream(inode)
{
}


File::~File()
{
}


status_t
File::InitCheck()
{
	status_t status = DataStream::InitCheck();
	if (status == B_OK)
		return IsFile() ? B_OK : B_ERROR;

	return status;
}


status_t
File::CopyTo(const char *root, bool fullPath, Inode::Source *source)
{
	status_t status = Inode::CopyTo(root, fullPath, source);
	if (status < B_OK)
		return status;

	BPath path(root);
	if (fullPath && Path(source))
		path.Append(Path(source));

	char *name = (char *)Name();
	if (name != NULL) {
		// changes the filename in the inode buffer (for deleted entries)
		if (!*name)
			*name = '_';
		path.Append(name);
	} else {
		BString sub;
		sub << "__untitled " << BlockRun().allocation_group << ":"
			<< (int32)BlockRun().start;
		path.Append(sub.String());
	}
	printf("%ld,%d -> %s\n", BlockRun().allocation_group, BlockRun().start,
		path.Path());

	BFile file;
	status = file.SetTo(path.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if (status < B_OK)
		return status;

	char buffer[fDisk->BlockSize()];
	ssize_t size;
	Seek(0, SEEK_SET);

	while ((size = Read(buffer, sizeof(buffer))) > B_OK) {
		ssize_t written = file.Write(buffer, size);
		if (written < B_OK)
			return written;
	}

	return CopyAttributesTo(&file);
}


//	#pragma mark -


Attribute::Attribute(Disk *disk, bfs_inode *inode, bool ownBuffer)
	: File(disk, inode, ownBuffer)
{
}


Attribute::Attribute(const Inode &inode)
	: File(inode)
{
}


Attribute::~Attribute()
{
}


status_t
Attribute::InitCheck()
{
	status_t status = DataStream::InitCheck();
	if (status == B_OK)
		return IsAttribute() ? B_OK : B_ERROR;

	return status;
}


status_t
Attribute::CopyTo(const char */*path*/, bool /*fullPath*/,
	Inode::Source */*source*/)
{
	// files and directories already copy all attributes

	// eventually, this method should be implemented to recover lost
	// attributes on the disk

	return B_OK;
}


//	#pragma mark -


Directory::Directory(Disk *disk, bfs_inode *inode, bool ownBuffer)
	: DataStream(disk, inode, ownBuffer),
	fTree(NULL)
{
}


Directory::Directory(const Inode &inode)
	: DataStream(inode),
	fTree(NULL)
{
}


Directory::~Directory()
{
	delete fTree;
}


status_t
Directory::InitCheck()
{
	status_t status = DataStream::InitCheck();
	if (status == B_OK)
		return (IsDirectory() || IsAttributeDirectory()) ? B_OK : B_ERROR;

	return status;
}


status_t
Directory::CopyTo(const char *root, bool fullPath, Inode::Source *source)
{
	// don't copy attributes or indices
	// the recovery program should make empty files to recover lost attributes
	if (IsAttributeDirectory() || IsIndex())
		return B_OK;

	status_t status = Inode::CopyTo(root, fullPath, source);
	if (status < B_OK)
		return status;

	BPath path(root);
	if (fullPath && Path(source))
		path.Append(Path(source));
	
	char *name = (char *)Name();
	if (name != NULL) {
		// changes the filename in the inode buffer (for deleted entries)
		if (!*name)
			*name = '_';
		path.Append(name);
	} else {
		// create unique name
		BString sub;
		sub << "__untitled " << BlockRun().allocation_group << ":"
			<< (int32)BlockRun().start;
		path.Append(sub.String());
	}

	BEntry entry(path.Path());
	BDirectory directory;
	if ((status = entry.GetParent(&directory)) < B_OK)
		return status;
	
	status = directory.CreateDirectory(path.Leaf(), NULL);
	if (status < B_OK && status != B_FILE_EXISTS)
		return status;

	if ((status = directory.SetTo(&entry)) < B_OK)
		return status;

	return CopyAttributesTo(&directory);
}


status_t
Directory::Rewind()
{
	if (!fTree) {
		status_t status = CreateTree();
		if (status < B_OK)
			return status;
	}
	return fTree->Rewind();
}


status_t
Directory::GetNextEntry(char *name, block_run *run)
{
	status_t status;

	if (!fTree) {
		if ((status = Rewind()) < B_OK)
			return status;
	}
	uint16 length;
	off_t offset;

	if ((status = fTree->GetNextEntry(name, &length, B_FILE_NAME_LENGTH - 1,
			&offset)) < B_OK)
		return status;

	*run = fDisk->ToBlockRun(offset);

	return B_OK;
}


status_t
Directory::GetNextEntry(block_run *run)
{
	char name[B_FILE_NAME_LENGTH];

	return GetNextEntry(name, run);
}


status_t
Directory::Contains(const block_run *run)
{
	status_t status;

	if (!fTree) {
		if ((status = Rewind()) < B_OK)
			return status;
	}

	block_run searchRun;
	while (GetNextEntry(&searchRun) == B_OK) {
		if (searchRun == *run)
			return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
Directory::Contains(const Inode *inode)
{
	status_t status;

	if (!fTree) {
		if ((status = CreateTree()) < B_OK)
			return status;
	}

	off_t value;
	const char *name = inode->Name();
	status = B_ENTRY_NOT_FOUND;

	if (name && (status = fTree->Find((uint8 *)name, (uint16)strlen(name),
			&value)) == B_OK) {
		if (fDisk->ToBlockRun(value) == inode->InodeBuffer()->inode_num)
			return B_OK;

		printf("inode address do not match (%s)!\n", inode->Name());
	}

	if (status != B_OK && status != B_ENTRY_NOT_FOUND)
		return status;

	return Contains(&inode->InodeBuffer()->inode_num);
}


status_t
Directory::FindEntry(const char *name, block_run *run)
{
	status_t status;

	if (!name)
		return B_BAD_VALUE;

	if (!fTree) {
		if ((status = CreateTree()) < B_OK)
			return status;
	}

	off_t value;

	if ((status = fTree->Find((uint8 *)name, (uint16)strlen(name),
			&value)) >= B_OK) {
		if (run)
			*run = fDisk->ToBlockRun(value);
		return B_OK;
	}
	return status;
}


status_t
Directory::AddEntry(Inode *inode)
{
	status_t status;
	bool created = false;

	if (!fTree) {
		status = CreateTree();
		if (status == B_OK)
			status = fTree->Validate();

		if (status == B_BAD_DATA) {
			//puts("bplustree corrupted!");
			fTree = new BPlusTree(BPLUSTREE_STRING_TYPE, BPLUSTREE_NODE_SIZE,
				false);
			if ((status = fTree->InitCheck()) < B_OK) {
				delete fTree;
				fTree = NULL;
			} else
				created = true;
		}

		if (status < B_OK)
			return status;
	}
	// keep all changes in memory
	fTree->SetHoldChanges(true);

	if (created) {
		// add . and ..
		fTree->Insert(".", Block());
		fTree->Insert("..", fDisk->ToBlock(Parent()));
	}

	if (inode->Flags() & INODE_DELETED)
		return B_ENTRY_NOT_FOUND;

	BString name = inode->Name();
	if (name == "") {
		name << "__file " << inode->BlockRun().allocation_group << ":"
			<< (int32)inode->BlockRun().start;
	}

	return fTree->Insert(name.String(), inode->Block());
}


status_t
Directory::CreateTree()
{
	fTree = new BPlusTree(this);

	status_t status = fTree->InitCheck();
	if (status < B_OK) {
		delete fTree;
		fTree = NULL;
		return status;
	}
	return B_OK;
}


status_t
Directory::GetTree(BPlusTree **tree)
{
	if (!fTree) {
		status_t status = CreateTree();
		if (status < B_OK)
			return status;
	}
	*tree = fTree;
	return B_OK;
}


//	#pragma mark -


Symlink::Symlink(Disk *disk, bfs_inode *inode,bool ownBuffer)
	: Inode(disk,inode,ownBuffer)
{
}


Symlink::Symlink(const Inode &inode)
	: Inode(inode)
{
}


Symlink::~Symlink()
{
}


status_t
Symlink::InitCheck()
{
	status_t status = Inode::InitCheck();
	if (status == B_OK)
		return IsSymlink() ? B_OK : B_ERROR;

	return status;
}


status_t
Symlink::CopyTo(const char *root, bool fullPath,Inode::Source *source)
{
	status_t status = Inode::CopyTo(root,fullPath,source);
	if (status < B_OK)
		return status;

	BPath path(root);
	if (fullPath && Path(source))
		path.Append(Path(source));

	char *name = (char *)Name();
	if (name != NULL) {
		// changes the filename in the inode buffer (for deleted entries)
		if (!*name)
			*name = '_';
		path.Append(name);
	} else {
		// create unique name
		BString sub;
		sub << "__symlink " << BlockRun().allocation_group << ":"
			<< (int32)BlockRun().start;
		path.Append(sub.String());
	}

	BEntry entry(path.Path());
	BDirectory directory;
	if ((status = entry.GetParent(&directory)) < B_OK)
		return status;

	char to[2048];
	if (LinksTo(to,sizeof(to)) < B_OK)
		return B_ERROR;

	BSymLink link;
	status = directory.CreateSymLink(path.Leaf(),to,&link);
	if (status < B_OK && status != B_FILE_EXISTS)
		return status;

	if ((status = link.SetTo(&entry)) < B_OK)
		return status;

	return CopyAttributesTo(&link);
}


status_t
Symlink::LinksTo(char *to,size_t maxLength)
{
	if ((fInode->flags & INODE_LONG_SYMLINK) == 0) {
		strcpy(to,fInode->short_symlink);
		return B_OK;
	}

	DataStream stream(*this);
	status_t status = stream.InitCheck();
	if (status < B_OK)
		return status;

	status = stream.Read(to,maxLength);

	return status < B_OK ? status : B_OK;
}

