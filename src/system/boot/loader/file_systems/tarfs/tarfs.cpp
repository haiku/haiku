/*
 * Copyright 2005, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "tarfs.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <AutoDeleter.h>
#include <OS.h>

#include <zlib.h>

#include <boot/partitions.h>
#include <boot/platform.h>
#include <util/kernel_cpp.h>
#include <util/DoublyLinkedList.h>


//#define TRACE_TARFS
#ifdef TRACE_TARFS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static const uint32 kCompressedArchiveOffset = 192 * 1024;	// at 192 kB
static const size_t kTarRegionSize = 4 * 1024 * 1024;		// 4 MB

namespace TarFS {

struct RegionDelete {
	inline void operator()(void *memory)
	{
		if (memory != NULL)
			platform_free_region(memory, kTarRegionSize);
	}
};

struct RegionDeleter : BPrivate::AutoDeleter<void, RegionDelete> {
	RegionDeleter() : BPrivate::AutoDeleter<void, RegionDelete>() {}
	RegionDeleter(void *memory) : BPrivate::AutoDeleter<void, RegionDelete>(memory) {}
};

class Directory;

class Entry : public DoublyLinkedListLinkImpl<Entry> {
	public:
		Entry(const char *name);
		virtual ~Entry() {}

		const char *Name() const { return fName; }
		virtual ::Node *ToNode() = 0;
		virtual TarFS::Directory *ToTarDirectory() { return NULL; }

	protected:
		const char	*fName;
		int32		fID;
};


typedef DoublyLinkedList<TarFS::Entry>	EntryList;
typedef EntryList::Iterator	EntryIterator;


class File : public ::Node, public Entry {
	public:
		File(tar_header *header, const char *name);
		virtual ~File();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;

		virtual int32 Type() const;
		virtual off_t Size() const;
		virtual ino_t Inode() const;

		virtual ::Node *ToNode() { return this; }

	private:
		tar_header	*fHeader;
		off_t		fSize;
};


class Directory : public ::Directory, public Entry {
	public:
		Directory(const char *name);
		virtual ~Directory();

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;

		virtual TarFS::Entry *LookupEntry(const char *name);
		virtual ::Node *Lookup(const char *name, bool traverseLinks);

		virtual status_t GetNextEntry(void *cookie, char *nameBuffer, size_t bufferSize);
		virtual status_t GetNextNode(void *cookie, Node **_node);
		virtual status_t Rewind(void *cookie);
		virtual bool IsEmpty();

		virtual ino_t Inode() const;

		virtual ::Node *ToNode() { return this; };
		virtual TarFS::Directory *ToTarDirectory()	{ return this; }

		status_t AddDirectory(char *dirName, TarFS::Directory **_dir = NULL);
		status_t AddFile(tar_header *header);

	private:
		typedef ::Directory _inherited;

		EntryList	fEntries;
};

class Volume : public TarFS::Directory {
	public:
		Volume();
		~Volume();

		status_t			Init(boot::Partition *partition);

		TarFS::Directory	*Root() { return this; }
};

}	// namespace TarFS


static int32 sNextID = 1;


// #pragma mark -


bool
skip_gzip_header(z_stream *stream)
{
	uint8 *buffer = (uint8 *)stream->next_in;
	
	// check magic and skip method
	if (buffer[0] != 0x1f || buffer[1] != 0x8b)
		return false;

	uint32 offset = 3;

	// we need the flags field to determine the length of the header
	int flags = buffer[offset++];

	offset += 6;

	if ((flags & 0x04) != 0) {
		// skip extra field
		offset += buffer[offset++] | (buffer[offset++] << 8);
		if (offset >= stream->avail_in)
			return false;
	}
	if ((flags & 0x08) != 0) {
		// skip original name
		while (buffer[offset++])
			;
	}
	if ((flags & 0x10) != 0) {
		// skip comment
		while (buffer[offset++])
			;
	}
	if ((flags & 0x02) != 0) {
		// skip CRC
		offset += 2;
	}

	if (offset >= stream->avail_in)
		return false;

	stream->next_in += offset;
	stream->avail_in -= offset;
	return true;
}


// #pragma mark -


TarFS::Entry::Entry(const char *name)
	:
	fName(name),
	fID(sNextID++)
{
}


// #pragma mark -


TarFS::File::File(tar_header *header, const char *name)
	: TarFS::Entry(name),
	fHeader(header)
{
	fSize = strtol(header->size, NULL, 8);
}


TarFS::File::~File()
{
}


ssize_t
TarFS::File::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	TRACE(("tarfs: read at %Ld, %lu bytes, fSize = %Ld\n", pos, bufferSize, fSize));

	if (pos < 0 || !buffer)
		return B_BAD_VALUE;

	if (pos >= fSize || bufferSize == 0)
		return 0;

	size_t toRead = fSize - pos;
	if (toRead > bufferSize)
		toRead = bufferSize;

	memcpy(buffer, (char*)fHeader + BLOCK_SIZE + pos, toRead);

	return toRead;
}


ssize_t
TarFS::File::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	return B_NOT_ALLOWED;
}


status_t
TarFS::File::GetName(char *nameBuffer, size_t bufferSize) const
{
	return strlcpy(nameBuffer, Name(), bufferSize) >= bufferSize ? B_BUFFER_OVERFLOW : B_OK;
}


int32
TarFS::File::Type() const
{
	return S_IFREG;
}


off_t
TarFS::File::Size() const
{
	return fSize;
}


ino_t
TarFS::File::Inode() const
{
	return fID;
}


// #pragma mark -

TarFS::Directory::Directory(const char *name)
	: TarFS::Entry(name)
{
}


TarFS::Directory::~Directory()
{
	while (TarFS::Entry *entry = fEntries.Head()) {
		fEntries.Remove(entry);
		delete entry;
	}
}


status_t 
TarFS::Directory::Open(void **_cookie, int mode)
{
	_inherited::Open(_cookie, mode);

	EntryIterator *iterator = new EntryIterator(fEntries.GetIterator());
	if (iterator == NULL)
		return B_NO_MEMORY;

	*_cookie = iterator;

	return B_OK;
}


status_t 
TarFS::Directory::Close(void *cookie)
{
	_inherited::Close(cookie);

	delete (EntryIterator *)cookie;
	return B_OK;
}


status_t
TarFS::Directory::GetName(char *nameBuffer, size_t bufferSize) const
{
	return strlcpy(nameBuffer, Name(), bufferSize) >= bufferSize ? B_BUFFER_OVERFLOW : B_OK;
}


TarFS::Entry *
TarFS::Directory::LookupEntry(const char *name)
{
	EntryIterator iterator(fEntries.GetIterator());

	while (iterator.HasNext()) {
		TarFS::Entry *entry = iterator.Next();
		if (strcmp(name, entry->Name()) == 0)
			return entry;
	}

	return NULL;
}


::Node *
TarFS::Directory::Lookup(const char *name, bool /*traverseLinks*/)
{
	if (TarFS::Entry *entry = LookupEntry(name)) {
		entry->ToNode()->Acquire();
			// our entries are not supposed to be deleted after use
		return entry->ToNode();
	}

	return NULL;
}


status_t 
TarFS::Directory::GetNextEntry(void *_cookie, char *name, size_t size)
{
	EntryIterator *iterator = (EntryIterator *)_cookie;
	TarFS::Entry *entry = iterator->Next();

	if (entry != NULL) {
		strlcpy(name, entry->Name(), size);
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t 
TarFS::Directory::GetNextNode(void *_cookie, Node **_node)
{
	EntryIterator *iterator = (EntryIterator *)_cookie;
	TarFS::Entry *entry = iterator->Next();

	if (entry != NULL) {
		*_node = entry->ToNode();
		return B_OK;
	}
	return B_ENTRY_NOT_FOUND;
}


status_t 
TarFS::Directory::Rewind(void *_cookie)
{
	EntryIterator *iterator = (EntryIterator *)_cookie;
	*iterator = fEntries.GetIterator();
	return B_OK;	
}


status_t
TarFS::Directory::AddDirectory(char *dirName, TarFS::Directory **_dir)
{
	char *subDir = strchr(dirName, '/');
	if (subDir) {
		// skip slashes
		while (*subDir == '/') {
			*subDir = '\0';
			subDir++;
		}

		if (*subDir == '\0') {
			// a trailing slash
			subDir = NULL;
		}
	}

	// check, whether the directory does already exist
	Entry *entry = LookupEntry(dirName);
	TarFS::Directory *dir = (entry ? entry->ToTarDirectory() : NULL);
	if (entry) {
		if (!dir)
			return B_ERROR;
	} else {
		// doesn't exist yet -- create it
		dir = new TarFS::Directory(dirName);
		if (!dir)
			return B_NO_MEMORY;

		fEntries.Add(dir);
	}

	// recursively create the subdirectories
	if (subDir) {
		status_t error = dir->AddDirectory(subDir, &dir);
		if (error != B_OK)
			return error;
	}

	if (_dir)
		*_dir = dir;

	return B_OK;
}


status_t
TarFS::Directory::AddFile(tar_header *header)
{
	char *leaf = strrchr(header->name, '/');
	char *dirName = NULL;
	if (leaf) {
		dirName = header->name;
		*leaf = '\0';
		leaf++;
	} else
		leaf = header->name;

	// create the parent directory
	TarFS::Directory *dir = this;
	if (dirName) {
		status_t error = AddDirectory(dirName, &dir);
		if (error != B_OK)
			return error;
	}

	// create the file
	TarFS::File *file = new TarFS::File(header, leaf);
	if (!file)
		return B_NO_MEMORY;

	dir->fEntries.Add(file);

	return B_OK;
}


bool 
TarFS::Directory::IsEmpty()
{
	return fEntries.IsEmpty();
}


ino_t
TarFS::Directory::Inode() const
{
	return fID;
}


// #pragma mark -


TarFS::Volume::Volume()
	: TarFS::Directory("Boot from CD-ROM")
{
}


TarFS::Volume::~Volume()
{
}


status_t
TarFS::Volume::Init(boot::Partition *partition)
{
	void *cookie;
	status_t error = partition->Open(&cookie, O_RDONLY);
	if (error != B_OK)
		return error;

	struct PartitionCloser {
		boot::Partition	*partition;
		void			*cookie;

		PartitionCloser(boot::Partition *partition, void *cookie)
			: partition(partition),
			  cookie(cookie)
		{
		}

		~PartitionCloser()
		{
			partition->Close(cookie);
		}
	} _(partition, cookie);

	RegionDeleter regionDeleter;

	char *out = NULL;

	char in[2048];
	z_stream zStream = {
		(Bytef*)in,		// next in
		sizeof(in),		// avail in
		0,				// total in
		NULL,			// next out
		0,				// avail out
		0,				// total out
		0,				// msg
		0,				// state
		Z_NULL,			// zalloc
		Z_NULL,			// zfree
		Z_NULL,			// opaque
		0,				// data type
		0,				// adler
		0,				// reserved
	};

	int status;
	uint32 offset = kCompressedArchiveOffset;

	do {
		if (partition->ReadAt(cookie, offset, in, sizeof(in)) != sizeof(in)) {
			status = Z_STREAM_ERROR;
			break;
		}

		zStream.avail_in = sizeof(in);
		zStream.next_in = (Bytef *)in;

		if (offset == kCompressedArchiveOffset) {
			// check and skip gzip header
			if (!skip_gzip_header(&zStream))
				return B_BAD_DATA;

			if (platform_allocate_region((void **)&out, kTarRegionSize,
					B_READ_AREA | B_WRITE_AREA, false) != B_OK) {
				TRACE(("tarfs: allocating region failed!\n"));
				return B_NO_MEMORY;
			}

			regionDeleter.SetTo(out);
			zStream.avail_out = kTarRegionSize;
			zStream.next_out = (Bytef *)out;

			status = inflateInit2(&zStream, -15);
			if (status != Z_OK)
				return B_ERROR;
		}

		status = inflate(&zStream, Z_SYNC_FLUSH);
		offset += sizeof(in);

		if (zStream.avail_in != 0 && status != Z_STREAM_END)
			dprintf("tarfs: didn't read whole block!\n");
	} while (status == Z_OK);

	inflateEnd(&zStream);

	if (status != Z_STREAM_END) {
		TRACE(("tarfs: inflating failed: %d!\n", status));
		return B_BAD_DATA;
	}

	status = B_OK;

	// parse the tar file
	char *block = out;
	int blockCount = zStream.total_out / BLOCK_SIZE;
	int blockIndex = 0;

	while (blockIndex < blockCount) {
		// check header
		tar_header *header = (tar_header*)(block + blockIndex * BLOCK_SIZE);
		//dump_header(*header);

		if (header->magic[0] == '\0')
			break;

		if (strcmp(header->magic, kTarHeaderMagic) != 0) {
			if (strcmp(header->magic, kOldTarHeaderMagic) != 0) {
				dprintf("Bad tar header magic in block %d.\n", blockIndex);
				status = B_BAD_DATA;
				break;
			}
		}

		off_t size = strtol(header->size, NULL, 8);

		TRACE(("tarfs: \"%s\", %Ld bytes\n", header->name, size));

		// TODO: this is old-style GNU tar which probably won't work with newer ones...
		switch (header->type) {
			case TAR_FILE:
			case TAR_FILE2:
				status = AddFile(header);
				break;

			case TAR_DIRECTORY:
				status = AddDirectory(header->name, NULL);
				break;

			case TAR_LONG_NAME:
				// this is a long file name
				// TODO: read long name
			default:
				// unsupported type
				status = B_ERROR;
				break;
		}

		if (status != B_OK)
			return status;

		// next block
		blockIndex += (size + 2 * BLOCK_SIZE - 1) / BLOCK_SIZE;
	}

	if (status != B_OK)
		return status;

	regionDeleter.Detach();
	return B_OK;
}


//	#pragma mark -


static status_t
tarfs_get_file_system(boot::Partition *partition, ::Directory **_root)
{
	TarFS::Volume *volume = new TarFS::Volume;
	if (volume == NULL)
		return B_NO_MEMORY;

	if (volume->Init(partition) < B_OK) {
		TRACE(("Initializing tarfs failed\n"));
		delete volume;
		return B_ERROR;
	}

	*_root = volume->Root();
	return B_OK;
}


file_system_module_info gTarFileSystemModule = {
	"file_systems/tarfs/v1",
	kPartitionTypeTarFS,
	tarfs_get_file_system
};

