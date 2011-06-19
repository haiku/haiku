/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Directory.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include <StorageDefs.h>
#include <util/kernel_cpp.h>

#include "CachedBlock.h"
#include "File.h"
#include "Volume.h"


//#define TRACE(x) dprintf x
#define TRACE(x) do {} while (0)

namespace FATFS {

struct dir_entry {
	void *Buffer() const { return (void *)fName; };
	const char *BaseName() const { return fName; };
	const char *Extension() const { return fExt; };
	uint8		Flags() const { return fFlags; };
	uint32		Cluster(int32 fatBits) const;
	void		SetCluster(uint32 cluster, int32 fatBits);
	uint32		Size() const { return B_LENDIAN_TO_HOST_INT32(fSize); };
	void		SetSize(uint32 size);
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


void
dir_entry::SetCluster(uint32 cluster, int32 fatBits)
{
	fClusterLSB = B_HOST_TO_LENDIAN_INT16((uint16)cluster);
	if (fatBits == 32)
		fClusterMSB = B_HOST_TO_LENDIAN_INT16(cluster >> 16);
}


void
dir_entry::SetSize(uint32 size)
{
	fSize = B_HOST_TO_LENDIAN_INT32(size);
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
	enum {
		MAX_UTF16_NAME_LENGTH = 255
	};

	int32	index;
	struct dir_entry entry;
	off_t	entryOffset;
	uint16	nameBuffer[MAX_UTF16_NAME_LENGTH];
	uint32	nameLength;

	off_t	Offset() const { return index * sizeof(struct dir_entry); }
	char*	Name()	{ return (char*)nameBuffer; }

	void	ResetName();
	bool	AddNameChars(const uint16* chars, uint32 count);
	bool	ConvertNameToUTF8();
	void	Set8_3Name(const char* baseName, const char* extension);
};


void
dir_cookie::ResetName()
{
	nameLength = 0;
}


bool
dir_cookie::AddNameChars(const uint16* chars, uint32 count)
{
	// If there is a null character, we ignore it and all subsequent characters.
	for (uint32 i = 0; i < count; i++) {
		if (chars[i] == 0) {
			count = i;
			break;
		}
	}

	if (count > 0) {
		if (count > (MAX_UTF16_NAME_LENGTH - nameLength))
			return false;

		nameLength += count;
		memcpy(nameBuffer + (MAX_UTF16_NAME_LENGTH - nameLength),
			chars, count * 2);
	}

	return true;
}


bool
dir_cookie::ConvertNameToUTF8()
{
	char name[B_FILE_NAME_LENGTH];
	uint32 nameOffset = 0;

	const uint16* utf16 = nameBuffer + (MAX_UTF16_NAME_LENGTH - nameLength);

	for (uint32 i = 0; i < nameLength; i++) {
		uint8 utf8[4];
		uint32 count;
		uint16 c = B_LENDIAN_TO_HOST_INT16(utf16[i]);
		if (c < 0x80) {
			utf8[0] = c;
			count = 1;
		} else if (c < 0xff80) {
			utf8[0] = 0xc0 | (c >> 6);
			utf8[1] = 0x80 | (c & 0x3f);
			count = 2;
		} else if ((c & 0xfc00) != 0xd800) {
			utf8[0] = 0xe0 | (c >> 12);
			utf8[1] = 0x80 | ((c >> 6) & 0x3f);
			utf8[2] = 0x80 | (c & 0x3f);
			count = 3;
		} else {
			// surrogate pair
			if (i + 1 >= nameLength)
				return false;

			uint16 c2 = B_LENDIAN_TO_HOST_INT16(utf16[++i]);
			if ((c2 & 0xfc00) != 0xdc00)
				return false;

			uint32 value = ((c - 0xd7c0) << 10) | (c2 & 0x3ff);
			utf8[0] = 0xf0 | (value >> 18);
			utf8[1] = 0x80 | ((value >> 12) & 0x3f);
			utf8[2] = 0x80 | ((value >> 6) & 0x3f);
			utf8[3] = 0x80 | (value & 0x3f);
			count = 4;
		}

		if (nameOffset + count >= sizeof(name))
			return false;

		memcpy(name + nameOffset, utf8, count);
		nameOffset += count;
	}

	name[nameOffset] = '\0';
	strlcpy(Name(), name, sizeof(nameBuffer));
	return true;
}


void
dir_cookie::Set8_3Name(const char* baseName, const char* extension)
{
	// trim base name
	uint32 baseNameLength = 8;
	while (baseNameLength > 0 && baseName[baseNameLength - 1] == ' ')
		baseNameLength--;

	// trim extension
	uint32 extensionLength = 3;
	while (extensionLength > 0 && extension[extensionLength - 1] == ' ')
		extensionLength--;

	// compose the name
	char* name = Name();
	memcpy(name, baseName, baseNameLength);

	if (extensionLength > 0) {
		name[baseNameLength] = '.';
		memcpy(name + baseNameLength + 1, extension, extensionLength);
		name[baseNameLength + 1 + extensionLength] = '\0';
	} else
		name[baseNameLength] = '\0';
}


// #pragma mark -


static bool
is_valid_8_3_file_name_char(char c)
{
	if ((uint8)c >= 128)
		return true;

	if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
		return true;

	return strchr("*!#$%&'()-@^_`{}~ ", c) != NULL;
}


static bool
check_valid_8_3_file_name(const char* name, const char*& _baseName,
	uint32& _baseNameLength, const char*& _extension, uint32& _extensionLength)
{
	// check length of base name and extension
	size_t nameLength = strlen(name);
	const char* extension = strchr(name, '.');
	size_t baseNameLength;
	size_t extensionLength;
	if (extension != NULL) {
		baseNameLength = extension - name;
		extensionLength = nameLength - baseNameLength - 1;
		if (extensionLength > 0)
			extension++;
		else
			extension = NULL;
	} else {
		baseNameLength = nameLength;
		extensionLength = 0;
	}

	// trim trailing space
	while (baseNameLength > 0 && name[baseNameLength - 1] == ' ')
		baseNameLength--;
	while (extensionLength > 0 && extension[extensionLength - 1] == ' ')
		extensionLength--;

	if (baseNameLength == 0 || baseNameLength > 8 || extensionLength > 3)
		return false;

	// check the chars
	for (size_t i = 0; i < baseNameLength; i++) {
		if (!is_valid_8_3_file_name_char(name[i]))
			return false;
	}

	for (size_t i = 0; i < extensionLength; i++) {
		if (!is_valid_8_3_file_name_char(extension[i]))
			return false;
	}

	_baseName = name;
	_baseNameLength = baseNameLength;
	_extension = extension;
	_extensionLength = extensionLength;

	return true;
}


// #pragma mark - Directory

Directory::Directory(Volume &volume, off_t dirEntryOffset, uint32 cluster,
	const char *name)
	:
	fVolume(volume),
	fStream(volume, cluster, UINT32_MAX, name),
	fDirEntryOffset(dirEntryOffset)
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
	c->entryOffset = 0;

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


Node*
Directory::LookupDontTraverse(const char* name)
{
	TRACE(("FASFS::Directory::%s('%s')\n", __FUNCTION__, name));
	if (!strcmp(name, ".")) {
		Acquire();
		return this;
	}

	status_t err;
	struct dir_cookie cookie;
	struct dir_cookie *c = &cookie;
	c->index = -1;
	c->entryOffset = 0;

	do {
		err = GetNextEntry(c);
		if (err < B_OK)
			return NULL;
		TRACE(("FASFS::Directory::%s: %s <> '%s'\n", __FUNCTION__,
			name, c->Name()));
		if (strcasecmp(name, c->Name()) == 0) {
			TRACE(("GOT IT!\n"));
			break;
		}
	} while (true);

	if (c->entry.IsFile()) {
		TRACE(("IS FILE\n"));
		return new File(fVolume, c->entryOffset,
			c->entry.Cluster(fVolume.FatBits()), c->entry.Size(), name);
	}
	if (c->entry.IsDir()) {
		TRACE(("IS DIR\n"));
		return new Directory(fVolume, c->entryOffset,
			c->entry.Cluster(fVolume.FatBits()), name);
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

	strlcpy(name, c->Name(), size);
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
	c->entryOffset = 0;

	return B_OK;
}


bool
Directory::IsEmpty()
{
	TRACE(("FASFS::Directory::%s()\n", __FUNCTION__));
	struct dir_cookie cookie;
	struct dir_cookie *c = &cookie;
	c->index = -1;
	c->entryOffset = 0;
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
Directory::CreateFile(const char* name, mode_t permissions, Node** _node)
{
	if (Node* node = Lookup(name, false)) {
		node->Release();
		return B_FILE_EXISTS;
	}

	// We only support 8.3 file names ATM.
	const char* baseName;
	const char* extension;
	uint32 baseNameLength;
	uint32 extensionLength;
	if (!check_valid_8_3_file_name(name, baseName, baseNameLength, extension,
			extensionLength)) {
		return B_UNSUPPORTED;
	}

	// prepare a directory entry for the new file
	dir_entry entry;

	memset(entry.fName, ' ', 11);
		// clear both base name and extension
	memcpy(entry.fName, baseName, baseNameLength);
	if (extensionLength > 0)
		memcpy(entry.fExt, extension, extensionLength);

	entry.fFlags = 0;
	entry.fReserved1 = 0;
	entry.fCreateTime10ms = 199;
	entry.fCreateTime = B_HOST_TO_LENDIAN_INT16((23 << 11) | (59 << 5) | 29);
		// 23:59:59.9
	entry.fCreateDate = B_HOST_TO_LENDIAN_INT16((127 << 9) | (12 << 5) | 31);
		// 2107-12-31
	entry.fAccessDate = entry.fCreateDate;
	entry.fClusterMSB = 0;
	entry.fModifiedTime = entry.fCreateTime;
	entry.fModifiedDate = entry.fCreateDate;
	entry.fClusterLSB = 0;
	entry.fSize = 0;

	// add the entry to the directory
	off_t entryOffset;
	status_t error = _AddEntry(entry, entryOffset);
	if (error != B_OK)
		return error;

	// create a File object
	File* file = new(std::nothrow) File(fVolume, entryOffset,
		entry.Cluster(fVolume.FatBits()), entry.Size(), name);
	if (file == NULL)
		return B_NO_MEMORY;

	*_node = file;
	return B_OK;
}


/*static*/ status_t
Directory::UpdateDirEntry(Volume& volume, off_t dirEntryOffset,
	uint32 firstCluster, uint32 size)
{
	if (dirEntryOffset == 0)
		return B_BAD_VALUE;

	CachedBlock cachedBlock(volume);
	off_t block = volume.ToBlock(dirEntryOffset);

	status_t error = cachedBlock.SetTo(block, CachedBlock::READ);
	if (error != B_OK)
		return error;

	dir_entry* entry = (dir_entry*)(cachedBlock.Block()
		+ dirEntryOffset % volume.BlockSize());

	entry->SetCluster(firstCluster, volume.FatBits());
	entry->SetSize(size);

	return cachedBlock.Flush();
}


status_t
Directory::GetNextEntry(void *cookie, uint8 mask, uint8 match)
{
	TRACE(("FASFS::Directory::%s(, %02x, %02x)\n", __FUNCTION__, mask, match));
	struct dir_cookie *c = (struct dir_cookie *)cookie;

	bool hasLongName = false;
	bool longNameValid = false;

	do {
		c->index++;
		size_t len = sizeof(c->entry);
		if (fStream.ReadAt(c->Offset(), (uint8 *)&c->entry, &len,
				&c->entryOffset) != B_OK || len != sizeof(c->entry)) {
			return B_ENTRY_NOT_FOUND;
		}

		TRACE(("FASFS::Directory::%s: got one entry\n", __FUNCTION__));
		if ((uint8)c->entry.fName[0] == 0x00) // last one
			return B_ENTRY_NOT_FOUND;
		if ((uint8)c->entry.fName[0] == 0xe5) // deleted
			continue;
		if (c->entry.Flags() == 0x0f) { // LFN entry
			uint8* nameEntry = (uint8*)&c->entry;
			if ((*nameEntry & 0x40) != 0) {
				c->ResetName();
				hasLongName = true;
				longNameValid = true;
			}

			uint16 nameChars[13];
			memcpy(nameChars, nameEntry + 0x01, 10);
			memcpy(nameChars + 5, nameEntry + 0x0e, 12);
			memcpy(nameChars + 11, nameEntry + 0x1c, 4);
			longNameValid |= c->AddNameChars(nameChars, 13);
			continue;
		}
		if ((c->entry.Flags() & (FAT_VOLUME|FAT_SUBDIR)) == FAT_VOLUME) {
			// TODO handle Volume name (set fVolume's name)
			continue;
		}
		TRACE(("FASFS::Directory::%s: checking '%8.8s.%3.3s', %02x\n", __FUNCTION__,
			c->entry.BaseName(), c->entry.Extension(), c->entry.Flags()));
		if ((c->entry.Flags() & mask) == match) {
			if (longNameValid)
				longNameValid = c->ConvertNameToUTF8();
			if (!longNameValid) {
				// copy 8.3 name to name buffer
				c->Set8_3Name(c->entry.BaseName(), c->entry.Extension());
			}
			break;
		}
	} while (true);
	TRACE(("FATFS::Directory::%s: '%8.8s.%3.3s'\n", __FUNCTION__,
		c->entry.BaseName(), c->entry.Extension()));
	return B_OK;
}


status_t
Directory::_AddEntry(dir_entry& entry, off_t& _entryOffset)
{
	off_t dirSize = _GetStreamSize();
	if (dirSize < 0)
		return dirSize;

	uint32 firstCluster = fStream.FirstCluster();

	// First null-terminate the new entry list, so we don't leave the list in
	// a broken state, if writing the actual entry fails. We only need to do
	// that when the entry is not at the end of a cluster.
	if ((dirSize + sizeof(entry)) % fVolume.ClusterSize() != 0) {
		// TODO: Rather zero the complete remainder of the cluster?
		size_t size = 1;
		char terminator = 0;
		status_t error = fStream.WriteAt(dirSize + sizeof(entry), &terminator,
			&size);
		if (error != B_OK)
			return error;
		if (size != 1)
			return B_ERROR;
	}

	// write the entry
	size_t size = sizeof(entry);
	status_t error = fStream.WriteAt(dirSize, &entry, &size, &_entryOffset);
	if (error != B_OK)
		return error;
	if (size != sizeof(entry))
		return B_ERROR;
		// TODO: Undo changes!

	fStream.SetSize(dirSize + sizeof(entry));

	// If the directory cluster has changed (which should only happen, if the
	// directory was empty before), we need to adjust the directory entry.
	if (firstCluster != fStream.FirstCluster()) {
		error = UpdateDirEntry(fVolume, fDirEntryOffset, fStream.FirstCluster(),
			0);
		if (error != B_OK)
			return error;
			// TODO: Undo changes!
	}

	return B_OK;
}


off_t
Directory::_GetStreamSize()
{
	off_t size = fStream.Size();
	if (size != UINT32_MAX)
		return size;

	// iterate to the end of the directory
	size = 0;
	while (true) {
		dir_entry entry;
		size_t entrySize = sizeof(entry);
		status_t error = fStream.ReadAt(size, &entry, &entrySize);
		if (error != B_OK)
			return error;

		if (entrySize != sizeof(entry) || entry.fName[0] == 0)
			break;

		size += sizeof(entry);
	}

	fStream.SetSize(size);
	return size;
}


}	// namespace FATFS
