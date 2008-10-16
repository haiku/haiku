/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Directory.h"
#include "Volume.h"
#include "File.h"

#include <StorageDefs.h>
#include <util/kernel_cpp.h>

#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

//#define TRACE(x) dprintf x
#define TRACE(x) do {} while (0)

namespace FATFS {

struct dir_entry {
	void *Buffer() const { return (void *)fName; };
	const char *BaseName() const { return fName; };
	const char *Extension() const { return fExt; };
	uint8		Flags() const { return fFlags; };
	uint32		Cluster(int32 fatBits) const;
	uint32		Size() const { return B_LENDIAN_TO_HOST_INT32(fSize); };
	bool		IsFile() const;
	bool		IsDir() const;
	char fName[8];
	char fExt[3];
	uint8 fFlags;
	uint8 fReserved1;
	uint8 fCreateTime10ms;
	uint16 fCreateTime;
	uint16 fCreateDate;
	uint16 fAccessDate;
	uint16 fClusterMSB;
	uint16 fModifiedTime;
	uint16 fModifiedDate;
	uint16 fClusterLSB;
	uint32 fSize;
} _PACKED;


uint32
dir_entry::Cluster(int32 fatBits) const
{
	uint32 c = B_LENDIAN_TO_HOST_INT16(fClusterLSB);
	if (fatBits == 32)
		c += ((uint32)B_LENDIAN_TO_HOST_INT16(fClusterMSB) << 16);
	return c;
}

bool
dir_entry::IsFile() const
{
	return ((Flags() & (FAT_VOLUME|FAT_SUBDIR)) == 0);
}


bool
dir_entry::IsDir() const
{
	return ((Flags() & (FAT_VOLUME|FAT_SUBDIR)) == FAT_SUBDIR);
}


struct dir_cookie {
	int32	index;
	struct dir_entry entry;
	off_t	Offset() const { return index * sizeof(struct dir_entry); }
};


Directory::Directory(Volume &volume, uint32 cluster, const char *name)
	:
	fVolume(volume),
	fStream(volume, cluster, UINT32_MAX, name)
{
	TRACE(("FASFS::Directory::(, %lu, %s)\n", cluster, name));
}


Directory::~Directory()
{
	TRACE(("FASFS::Directory::~()\n"));
}


status_t 
Directory::InitCheck()
{
	status_t err;
	err = fStream.InitCheck();
	if (err < B_OK)
		return err;
	return B_OK;
}


status_t 
Directory::Open(void **_cookie, int mode)
{
	TRACE(("FASFS::Directory::%s(, %d)\n", __FUNCTION__, mode));
	_inherited::Open(_cookie, mode);

	struct dir_cookie *c = new struct dir_cookie;
	if (c == NULL)
		return B_NO_MEMORY;

	c->index = -1;

	*_cookie = (void *)c;
	return B_OK;
}


status_t 
Directory::Close(void *cookie)
{
	TRACE(("FASFS::Directory::%s()\n", __FUNCTION__));
	_inherited::Close(cookie);

	delete (struct dir_cookie *)cookie;
	return B_OK;
}


Node *
Directory::Lookup(const char *name, bool traverseLinks)
{
	TRACE(("FASFS::Directory::%s('%s', %d)\n", __FUNCTION__, name, traverseLinks));
	if (!strcmp(name, ".")) {
		Acquire();
		return this;
	}
	char *dot = strchr(name, '.');
	int baselen = strlen(name);
	if (baselen > FATFS_BASENAME_LENGTH) // !?
		return NULL;
	char *ext = NULL;
	int extlen = 0;
	if (dot) {
		baselen = dot - name;
		ext = dot + 1;
		if (strlen(ext) > FATFS_EXTNAME_LENGTH) // !?
			return NULL;
		extlen = strlen(ext);
	}

	status_t err;
	struct dir_cookie cookie;
	struct dir_cookie *c = &cookie;
	c->index = -1;

	do {
		err = GetNextEntry(c);
		if (err < B_OK)
			return NULL;
		TRACE(("FASFS::Directory::%s: %s <> '%8.8s.%3.3s'\n", __FUNCTION__, 
			name, c->entry.BaseName(), c->entry.Extension()));
		int i;
		for (i = 0; i < FATFS_BASENAME_LENGTH; i++)
			if (c->entry.BaseName()[i] == ' ')
				break;
		int nlen = MIN(i,FATFS_BASENAME_LENGTH);
		for (i = 0; i < FATFS_EXTNAME_LENGTH; i++)
			if (c->entry.Extension()[i] == ' ')
				break;
		int elen = MIN(i,FATFS_EXTNAME_LENGTH);
		if (nlen != baselen)
			continue;
		if (elen != extlen)
			continue;
		if (strncasecmp(name, c->entry.BaseName(), nlen))
			continue;
		if (strncasecmp(ext, c->entry.Extension(), elen))
			continue;
		TRACE(("GOT IT!\n"));
		break;
	} while (true);

	if (c->entry.IsFile()) {
		TRACE(("IS FILE\n"));
		return new File(fVolume, c->entry.Cluster(fVolume.FatBits()), 
			c->entry.Size(), name);
	}
	if (c->entry.IsDir()) {
		TRACE(("IS DIR\n"));
		return new Directory(fVolume, c->entry.Cluster(fVolume.FatBits()), 
			name);
	}
	return NULL;
}


status_t 
Directory::GetNextEntry(void *cookie, char *name, size_t size)
{
	TRACE(("FASFS::Directory::%s()\n", __FUNCTION__));
	struct dir_cookie *c = (struct dir_cookie *)cookie;
	status_t err;

	err = GetNextEntry(cookie);
	if (err < B_OK)
		return err;

	strlcpy(name, c->entry.fName, MIN(size, FATFS_BASENAME_LENGTH));
	strlcpy(name, ".", size);
	strlcpy(name, c->entry.fExt, MIN(size, FATFS_EXTNAME_LENGTH));
	return B_OK;
}


status_t 
Directory::GetNextNode(void *cookie, Node **_node)
{
	return B_ERROR;
}


status_t
Directory::Rewind(void *cookie)
{
	TRACE(("FASFS::Directory::%s()\n", __FUNCTION__));
	struct dir_cookie *c = (struct dir_cookie *)cookie;
	c->index = -1;

	return B_OK;
}


bool
Directory::IsEmpty()
{
	TRACE(("FASFS::Directory::%s()\n", __FUNCTION__));
	struct dir_cookie cookie;
	struct dir_cookie *c = &cookie;
	c->index = -1;
	if (GetNextEntry(c) == B_OK)
		return false;
	return true;
}


status_t
Directory::GetName(char *name, size_t size) const
{
	TRACE(("FASFS::Directory::%s()\n", __FUNCTION__));
	if (this == fVolume.Root())
		return fVolume.GetName(name, size);
	return fStream.GetName(name, size);
}


ino_t
Directory::Inode() const
{
	TRACE(("FASFS::Directory::%s()\n", __FUNCTION__));
	return fStream.FirstCluster() << 16;
}

status_t 
Directory::GetNextEntry(void *cookie, uint8 mask, uint8 match = 0)
{
	TRACE(("FASFS::Directory::%s(, %02x, %02x)\n", __FUNCTION__, mask, match));
	struct dir_cookie *c = (struct dir_cookie *)cookie;

	do {
		c->index++;
		size_t len = sizeof(c->entry);
		if (fStream.ReadAt(c->Offset(), (uint8 *)&c->entry, &len) < B_OK)
			return B_ENTRY_NOT_FOUND;
		TRACE(("FASFS::Directory::%s: got one entry\n", __FUNCTION__));
		if ((uint8)c->entry.fName[0] == 0x00) // last one
			return B_ENTRY_NOT_FOUND;
		if ((uint8)c->entry.fName[0] == 0xe5) // deleted
			continue;
		if (c->entry.Flags() == 0x0f) // LFN entry
			continue;
		if (c->entry.Flags() & (FAT_VOLUME|FAT_SUBDIR) == FAT_VOLUME) {
			// TODO handle Volume name (set fVolume's name)
			continue;
		}
		TRACE(("FASFS::Directory::%s: checking '%8.8s.%3.3s', %02x\n", __FUNCTION__, 
			c->entry.BaseName(), c->entry.Extension(), c->entry.Flags()));
		if ((c->entry.Flags() & mask) == match)
			break;
	} while (true);
	TRACE(("FATFS::Directory::%s: '%8.8s.%3.3s'\n", __FUNCTION__,
		c->entry.BaseName(), c->entry.Extension()));
	return B_OK;
}

}	// namespace FATFS
