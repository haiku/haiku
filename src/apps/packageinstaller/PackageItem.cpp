/*
 * Copyright (c) 2007-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "PackageItem.h"

#include <Alert.h>
#include <ByteOrder.h>
#include <Directory.h>
#include <NodeInfo.h>
#include <SymLink.h>
#include <Volume.h>

#include <fs_info.h>
#include "zlib.h"

// Macro reserved for later localization
#define T(x) x

enum {
	P_CHUNK_SIZE = 256
};

static const uint32 kDefaultMode = 0777;
static const uint8 padding[7] = { 0, 0, 0, 0, 0, 0, 0 };

enum {
	P_DATA = 0,
	P_ATTRIBUTE
};


status_t
inflate_data(uint8 *in, uint32 inSize, uint8 *out, uint32 outSize)
{		
	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = inSize;
	stream.next_in = in;
	status_t ret;

	ret = inflateInit(&stream);
	if (ret != Z_OK) {
		parser_debug("inflatInit failed\n");
		return B_ERROR;
	}

	stream.avail_out = outSize;
	stream.next_out = out;

	ret = inflate(&stream, Z_NO_FLUSH);
	if (ret != Z_STREAM_END) {
		// Uncompressed file size in package info corrupted
		parser_debug("Left: %d\n", stream.avail_out);
		return B_ERROR;
	}

	inflateEnd(&stream);
	return B_OK;
}


static inline int
inflate_file_to_file(BFile *in, uint64 in_size, BFile *out, uint64 out_size)
{
	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = 0;
	stream.next_in = Z_NULL;
	status_t ret;

	uint8 buffer_out[P_CHUNK_SIZE], buffer_in[P_CHUNK_SIZE];
	uint64 bytes_read = 0, read = P_CHUNK_SIZE, write = 0;

	ret = inflateInit(&stream);
	if (ret != Z_OK) {
		parser_debug("inflate_file_to_file: inflateInit failed\n");
		return B_ERROR;
	}

	do {
		bytes_read += P_CHUNK_SIZE;
		if (bytes_read > in_size) {
			read = in_size - (bytes_read - P_CHUNK_SIZE);
			bytes_read = in_size;
		}

		stream.avail_in = in->Read(buffer_in, read);
		if (stream.avail_in != read) {
			parser_debug("inflate_file_to_file: read failed\n");
			(void)inflateEnd(&stream);
			return B_ERROR;
		}
		stream.next_in = buffer_in;

		do {
			stream.avail_out = P_CHUNK_SIZE;
			stream.next_out = buffer_out;

			ret = inflate(&stream, Z_NO_FLUSH);
			if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
				parser_debug("inflate_file_to_file: inflate failed\n");
				(void)inflateEnd(&stream);
				return B_ERROR;
			}

			write = P_CHUNK_SIZE - stream.avail_out;
			if (static_cast<uint64>(out->Write(buffer_out, write)) != write) {
				parser_debug("inflate_file_to_file: write failed\n");
				(void)inflateEnd(&stream);
				return B_ERROR;
			}
		}
		while (stream.avail_out == 0);
	}
	while (bytes_read != in_size);

	(void)inflateEnd(&stream);

	return B_OK;
}


// #pragma mark - PackageItem


PackageItem::PackageItem(BFile *parent, const BString &path, uint8 type,
	uint32 ctime, uint32 mtime, uint64 offset, uint64 size)
{
	SetTo(parent, path, type, ctime, mtime, offset, size);
}


PackageItem::~PackageItem()
{
}


void
PackageItem::SetTo(BFile *parent, const BString &path, uint8 type, uint32 ctime, 
	uint32 mtime, uint64 offset, uint64 size)
{
	fPackage = parent;
	fPath = path;

	fOffset = offset;
	fSize = size;
	fPathType = type;
	fCreationTime = ctime;
	fModificationTime = mtime;
}


int32
PackageItem::ItemExists(const char *name)
{
	// TODO: this function doesn't really fit in, the GUI should be separated
	// from the package engine completely

	BString alertString = "The ";

	alertString << ItemKind() << " named \'" << name << "\' ";
	alertString << T("already exists in the given path. Should I replace "
		"the existing file with the one from this package?");

	BAlert *alert = new BAlert(T("file_exists"), alertString.String(),
		T("Yes"), T("No"), T("Abort"));

	return alert->Go();
}


status_t
PackageItem::InitPath(const char *path, BPath *destination)
{
	status_t ret = B_OK;

	if (fPathType == P_INSTALL_PATH) {
		if (!path) {
			parser_debug("InitPath path is NULL\n");
			return B_ERROR;
		}
		ret = destination->SetTo(path, fPath.String());
	}
	else if (fPathType == P_SYSTEM_PATH)
		ret = destination->SetTo(fPath.String());
	else {
		if (!path) {
			parser_debug("InitPath path is NULL\n");
			return B_ERROR;
		}

		BVolume volume(dev_for_path(path));
		ret = volume.InitCheck();
		if (ret != B_OK)
			return ret;

		BDirectory temp;
		ret = volume.GetRootDirectory(&temp);
		if (ret != B_OK)
			return ret;

		BPath mountPoint(&temp, NULL);
		ret = destination->SetTo(mountPoint.Path(), fPath.String());
	}

	return ret;
}


status_t
PackageItem::HandleAttributes(BPath *destination, BNode *node, 
	const char *header)
{
	status_t ret = B_OK;

	BVolume volume(dev_for_path(destination->Path()));
	if (volume.KnowsAttr()) {
		parser_debug("We have an offset\n");
		if (!fPackage)
			return B_ERROR;

		ret = fPackage->InitCheck();
		if (ret != B_OK)
			return ret;

		// We need to parse the data section now
		fPackage->Seek(fOffset, SEEK_SET);
		uint8 buffer[7];
		if (fPackage->Read(buffer, 7) != 7 || memcmp(buffer, header, 5))
			return B_ERROR;
		parser_debug("Header validated!\n");

		char *attrName = 0;
		uint32 nameSize = 0;
		uint8 *attrData = new uint8[P_CHUNK_SIZE];
		uint64 dataSize = P_CHUNK_SIZE;
		uint8 *temp = new uint8[P_CHUNK_SIZE];
		uint64 tempSize = P_CHUNK_SIZE;

		uint64 attrCSize = 0, attrOSize = 0;
		uint32 attrType = 0; // type_code type
		bool attrStarted = false, done = false;

		while (fPackage->Read(buffer, 7) == 7) {
			if (!memcmp(buffer, "FBeA", 5))
				continue;

			ret = ParseAttribute(buffer, node, &attrName, &nameSize, &attrType,
				&attrData, &dataSize, &temp, &tempSize, &attrCSize, &attrOSize, 
				&attrStarted, &done);
			if (ret != B_OK || done) {
				if (ret != B_OK) {
					parser_debug("_ParseAttribute failed for %s\n",
						destination->Path());
				}
				break;
			}
		}

		delete[] attrData;
		delete[] temp;
	}

	return ret;
}


status_t
PackageItem::ParseAttribute(uint8 *buffer, BNode *node, char **attrName, 
	uint32 *nameSize, uint32 *attrType, uint8 **attrData, uint64 *dataSize, 
	uint8 **temp, uint64 *tempSize, uint64 *attrCSize, uint64 *attrOSize, 
	bool *attrStarted, bool *done)
{
	status_t ret = B_OK;
	uint32 length;

	if (!memcmp(buffer, "BeAI", 5)) {
		parser_debug(" Attribute started.\n");
		if (*attrName)
			*attrName[0] = 0;
		*attrCSize = 0;
		*attrOSize = 0;

		*attrStarted = true;
	} else if (!memcmp(buffer, "BeAN", 5)) {
		if (!*attrStarted) {
			ret = B_ERROR;
			return ret;
		}

		parser_debug(" BeAN.\n");
		fPackage->Read(&length, 4);
		swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
			B_SWAP_BENDIAN_TO_HOST);

		if (*nameSize < (length + 1)) {
			delete *attrName;
			*nameSize = length + 1;
			*attrName = new char[*nameSize];
		}
		fPackage->Read(*attrName, length);
		(*attrName)[length] = 0;

		parser_debug(" (%ld) = %s\n", length, *attrName);
	} else if (!memcmp(buffer, "BeAT", 5)) {
		if (!*attrStarted) {
			ret = B_ERROR;
			return ret;
		}

		parser_debug(" BeAT.\n");
		fPackage->Read(attrType, 4);
		swap_data(B_UINT32_TYPE, attrType, sizeof(*attrType), 
				B_SWAP_BENDIAN_TO_HOST);
	} else if (!memcmp(buffer, "BeAD", 5)) {
		if (!*attrStarted) {
			ret = B_ERROR;
			return ret;
		}

		parser_debug(" BeAD.\n");
		fPackage->Read(attrCSize, 8);
		swap_data(B_UINT64_TYPE, attrCSize, sizeof(*attrCSize), 
				B_SWAP_BENDIAN_TO_HOST);

		fPackage->Read(attrOSize, 8);
		swap_data(B_UINT64_TYPE, attrOSize, sizeof(*attrOSize), 
				B_SWAP_BENDIAN_TO_HOST);

		fPackage->Seek(4, SEEK_CUR); // TODO: Check what this means

		if (*tempSize < *attrCSize) {
			delete *temp;
			*tempSize = *attrCSize;
			*temp = new uint8[*tempSize];
		}
		if (*dataSize < *attrOSize) {
			delete *attrData;
			*dataSize = *attrOSize;
			*attrData = new uint8[*dataSize];
		}

		if (fPackage->Read(*temp, *attrCSize) 
				!= static_cast<ssize_t>(*attrCSize)) {
			ret = B_ERROR;
			return ret;
		}

		parser_debug("  Data read successfuly. Inflating!\n");
		ret = inflate_data(*temp, *tempSize, *attrData, *dataSize);
		if (ret != B_OK)
			return ret;
	} else if (!memcmp(buffer, padding, 7)) {
		if (!*attrStarted) {
			*done = true;
			return ret;
		}

		parser_debug(" Padding.\n");
		ssize_t wrote = node->WriteAttr(*attrName, *attrType, 0, *attrData, 
			*attrOSize);
		if (wrote != static_cast<ssize_t>(*attrOSize)) {
			parser_debug("Failed to write attribute %s %s\n", *attrName, strerror(wrote));
			return B_ERROR;
		}

		*attrStarted = false;
		if (*attrName)
			*attrName[0] = 0;
		*attrCSize = 0;
		*attrOSize = 0;

		parser_debug(" > Attribute added.\n");
	} else {
		parser_debug(" Unknown attribute\n");
		ret = B_ERROR;
	}

	return ret;
}


status_t
PackageItem::ParseData(uint8 *buffer, BFile *file, uint64 originalSize, 
	bool *done)
{
	status_t ret = B_OK;

	if (!memcmp(buffer, "FiMF", 5)) {
		parser_debug(" Found file data.\n");
		uint64 compressed, original;
		fPackage->Read(&compressed, 8);
		swap_data(B_UINT64_TYPE, &compressed, sizeof(uint64), 
				B_SWAP_BENDIAN_TO_HOST);

		fPackage->Read(&original, 8);
		swap_data(B_UINT64_TYPE, &original, sizeof(uint64), 
				B_SWAP_BENDIAN_TO_HOST);
		parser_debug(" Still good... (%llu : %llu)\n", original,
				originalSize);

		if (original != originalSize) {
			parser_debug(" File size mismatch\n");
			return B_ERROR; // File size mismatch
		}
		parser_debug(" Still good...\n");

		if (fPackage->Read(buffer, 4) != 4) {
			parser_debug(" Read(buffer, 4) failed\n");
			return B_ERROR;
		}
		parser_debug(" Still good...\n");

		ret = inflate_file_to_file(fPackage, compressed, file, original);
		if (ret != B_OK) {
			parser_debug(" inflate_file_to_file failed\n");
			return ret;
		}
		parser_debug(" File data inflation complete!\n");
	}
	else if (!memcmp(buffer, padding, 7)) {
		*done = true;
		return ret;
	}
	else {
		parser_debug("_ParseData unknown tag\n");
		ret = B_ERROR;
	}

	return ret;
}


// #pragma mark - PackageDirectory


PackageDirectory::PackageDirectory(BFile *parent, const BString &path,
		uint8 type, uint32 ctime, uint32 mtime, uint64 offset, uint64 size)
	: PackageItem(parent, path, type, ctime, mtime, offset, size)
{
}


status_t
PackageDirectory::WriteToPath(const char *path, BPath *final)
{
	BPath destination;
	status_t ret;
	parser_debug("Directory: %s WriteToPath() called!\n", fPath.String());

	ret = InitPath(path, &destination);
	if (ret != B_OK)
		return ret;

	// Since Haiku is single-user right now, we give the newly
	// created directory default permissions
	ret = create_directory(destination.Path(), kDefaultMode);
	if (ret != B_OK)
		return ret;
	BDirectory dir(destination.Path());
	parser_debug("Directory created!\n");

	if (fCreationTime)
		dir.SetCreationTime(static_cast<time_t>(fCreationTime));
	
	if (fModificationTime)
		dir.SetModificationTime(static_cast<time_t>(fModificationTime));

	// Since directories can only have attributes in the offset section,
	// we can check here whether it is necessary to continue
	if (fOffset)
		ret = HandleAttributes(&destination, &dir, "FoDa");
	
	if (final)
		*final = destination;

	return ret;
}


const char*
PackageDirectory::ItemKind()
{
	return "directory";
}


//	#pragma mark - PackageFile


PackageFile::PackageFile(BFile *parent, const BString &path, uint8 type,
		uint32 ctime, uint32 mtime, uint64 offset, uint64 size,
		uint64 originalSize, uint32 platform, const BString &mime,
		const BString &signature, uint32 mode)
	: PackageItem(parent, path, type, ctime, mtime, offset, size),
	fOriginalSize(originalSize),
	fPlatform(platform),
	fMode(mode),
	fMimeType(mime),
	fSignature(signature)
{
}


status_t
PackageFile::WriteToPath(const char *path, BPath *final)
{
	BPath destination;
	status_t ret;
	parser_debug("File: %s WriteToPath() called!\n", fPath.String());

	ret = InitPath(path, &destination);
	if (ret != B_OK)
		return ret;

	BFile file(destination.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	ret = file.InitCheck();
	if (ret == B_FILE_EXISTS) {
		int32 selection = ItemExists(destination.Leaf());
		switch (selection) {
			case 0:
				ret = file.SetTo(destination.Path(),
					B_WRITE_ONLY | B_ERASE_FILE);
				if (ret != B_OK)
					return ret;
				break;
			case 1:
				return B_OK;
			default:
				return B_FILE_EXISTS;
		}
	} else if (ret == B_ENTRY_NOT_FOUND) {
		BPath directory;
		destination.GetParent(&directory);
		if (create_directory(directory.Path(), kDefaultMode) != B_OK)
			return B_ERROR;

		ret = file.SetTo(destination.Path(), B_WRITE_ONLY | B_CREATE_FILE);
		if (ret != B_OK)
			return ret;
	} else if (ret != B_OK)
		return ret;

	parser_debug(" File created!\n");

	// Set the file permissions, creation and modification times
	ret = file.SetPermissions(static_cast<mode_t>(fMode));
	if (fCreationTime && ret == B_OK)
		ret = file.SetCreationTime(static_cast<time_t>(fCreationTime));
	if (fModificationTime && ret == B_OK)
		ret = file.SetModificationTime(static_cast<time_t>(fModificationTime));

	if (ret != B_OK)
		return ret;
	
	// Set the mimetype and application signature if present
	BNodeInfo info(&file);
	if (fMimeType.Length() > 0) {
		ret = info.SetType(fMimeType.String());
		if (ret != B_OK)
			return ret;
	}
	if (fSignature.Length() > 0) {
		ret = info.SetPreferredApp(fSignature.String());
		if (ret != B_OK)
			return ret;
	}

	if (fOffset) {
		parser_debug("We have an offset\n");
		if (!fPackage)
			return B_ERROR;

		ret = fPackage->InitCheck();
		if (ret != B_OK)
			return ret;

		// We need to parse the data section now
		fPackage->Seek(fOffset, SEEK_SET);
		uint8 buffer[7];

		char *attrName = 0;
		uint32 nameSize = 0;
		uint8 *attrData = new uint8[P_CHUNK_SIZE];
		uint64 dataSize = P_CHUNK_SIZE;
		uint8 *temp = new uint8[P_CHUNK_SIZE];
		uint64 tempSize = P_CHUNK_SIZE;

		uint64 attrCSize = 0, attrOSize = 0;
		uint32 attrType = 0; // type_code type
		bool attrStarted = false, done = false;
		
		uint8 section = P_ATTRIBUTE;

		while (fPackage->Read(buffer, 7) == 7) {
			if (!memcmp(buffer, "FBeA", 5)) {
				parser_debug("-> Attribute\n");
				section = P_ATTRIBUTE;
				continue;
			} else if (!memcmp(buffer, "FiDa", 5)) {
				parser_debug("-> File data\n");
				section = P_DATA;
				continue;
			}

			switch (section) {
				case P_ATTRIBUTE:
					ret = ParseAttribute(buffer, &file, &attrName, &nameSize,
						&attrType, &attrData, &dataSize, &temp, &tempSize,
						&attrCSize, &attrOSize, &attrStarted, &done);
					break;

				case P_DATA:
					ret = ParseData(buffer, &file, fOriginalSize, &done);
					break;

				default:
					return B_ERROR;
			}

			if (ret != B_OK || done)
				break;
		}

		delete[] attrData;
		delete[] temp;
	}

	if (final)
		*final = destination;

	return ret;
}


const char*
PackageFile::ItemKind()
{
	return "file";
}


//	#pragma mark -


PackageLink::PackageLink(BFile *parent, const BString &path,
		const BString &link, uint8 type, uint32 ctime, uint32 mtime,
		uint32 mode, uint64 offset, uint64 size)
	: PackageItem(parent, path, type, ctime, mtime, offset, size),
	fMode(mode),
	fLink(link)
{
}


status_t
PackageLink::WriteToPath(const char *path, BPath *final)
{
	parser_debug("Symlink: %s WriteToPath() called!\n", fPath.String());

	BPath destination;
	status_t ret = InitPath(path, &destination);
	if (ret != B_OK)
		return ret;

	BString linkName(destination.Leaf());
	parser_debug("%s:%s:%s\n", fPath.String(), destination.Path(),
		linkName.String());

	BPath dirPath;
	ret = destination.GetParent(&dirPath);
	BDirectory dir(dirPath.Path());

	ret = dir.InitCheck();
	if (ret == B_ENTRY_NOT_FOUND) {
		if ((ret = create_directory(dirPath.Path(), kDefaultMode)) != B_OK) {
			parser_debug("create_directory()) failed\n");
			return B_ERROR;
		}
	}
	if (ret != B_OK) {
		parser_debug("destination InitCheck failed %s for %s\n", strerror(ret), dirPath.Path());
		return ret;
	}

	BSymLink symlink;
	ret = dir.CreateSymLink(destination.Path(), fLink.String(), &symlink);
	if (ret == B_FILE_EXISTS) {
		// We need to check if the existing symlink is pointing at the same path
		// as our new one - if not, let's prompt the user
		symlink.SetTo(destination.Path());
		BPath oldLink;

		ret = symlink.MakeLinkedPath(&dir, &oldLink);
		chdir(dirPath.Path());

		if (ret == B_BAD_VALUE || oldLink != fLink.String()) {
			// The old symlink is different (or not a symlink) - ask the user
			int32 selection = ItemExists(destination.Leaf());
			switch (selection) {
				case 0:
				{
					symlink.Unset();
					BEntry entry;
					ret = entry.SetTo(destination.Path());
					if (ret != B_OK)
						return ret;

					entry.Remove();
					ret = dir.CreateSymLink(destination.Path(), fLink.String(),
						&symlink);
					break;
				}
				case 1:
					parser_debug("Skipping already existent SymLink\n");
					return B_OK;
				default:
					ret = B_FILE_EXISTS;
			}
		} else {
			ret = B_OK;
		}
	}
	if (ret != B_OK) {
		parser_debug("CreateSymLink failed\n");
		return ret;
	}

	parser_debug(" Symlink created!\n");

	ret = symlink.SetPermissions(static_cast<mode_t>(fMode));

	if (fCreationTime && ret == B_OK)
		ret = symlink.SetCreationTime(static_cast<time_t>(fCreationTime));
	
	if (fModificationTime && ret == B_OK) {
		ret = symlink.SetModificationTime(static_cast<time_t>(
			fModificationTime));
	}

	if (ret != B_OK) {
		parser_debug("Failed to set symlink attributes\n");
		return ret;
	}

	if (fOffset) {
		// Symlinks also seem to have attributes - so parse them
		ret = HandleAttributes(&destination, &symlink, "LnDa");
	}

	if (final) {
		*final = destination;
	}

	return ret;
}


const char*
PackageLink::ItemKind()
{
	return "symbolic link";
}
