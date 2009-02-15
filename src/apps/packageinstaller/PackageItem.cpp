/*
 * Copyright (c) 2007, Haiku, Inc.
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
inflate_data(uint8 *in, uint32 in_size, uint8 *out, uint32 out_size)
{		
	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = in_size;
	stream.next_in = in;
	status_t ret;

	ret = inflateInit(&stream);
	if (ret != Z_OK)
		return B_ERROR;

	stream.avail_out = out_size;
	stream.next_out = out;

	ret = inflate(&stream, Z_NO_FLUSH);
	if (ret != Z_STREAM_END) {
		parser_debug("Left: %d\n", stream.avail_out);
		return B_ERROR; // Uncompressed file size in package info corrupted
	}

	(void)inflateEnd(&stream);

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
	if (ret != Z_OK)
		return B_ERROR;

	do {
		bytes_read += P_CHUNK_SIZE;
		if (bytes_read > in_size) {
			read = in_size - (bytes_read - P_CHUNK_SIZE);
			bytes_read = in_size;
		}

		stream.avail_in = in->Read(buffer_in, read);
		if (stream.avail_in != read) {
			(void)inflateEnd(&stream);
			return B_ERROR;
		}
		stream.next_in = buffer_in;

		do {
			stream.avail_out = P_CHUNK_SIZE;
			stream.next_out = buffer_out;

			ret = inflate(&stream, Z_NO_FLUSH);
			if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
				(void)inflateEnd(&stream);
				return B_ERROR;
			}

			write = P_CHUNK_SIZE - stream.avail_out;
			if (static_cast<uint64>(out->Write(buffer_out, write)) != write) {
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


// #pragma mark -


PkgDirectory::PkgDirectory(BFile *parent, BString path, uint8 type, uint32 ctime, 
			uint32 mtime, uint64 offset, uint64 size)
	:	
	fPath(path),
	fOffset(offset),
	fSize(size),
	fPathType(type),
	fCreationTime(ctime),
	fModificationTime(mtime),
	fPackage(parent)
{
}


void
PkgDirectory::SetTo(BFile *parent, BString path, uint8 type, uint32 ctime, 
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


PkgDirectory::~PkgDirectory()
{
}


status_t
PkgDirectory::WriteToPath(const char *path, BPath *final)
{
	BPath destination;
	status_t ret;
	parser_debug("Directory: %s WriteToPath() called!\n", fPath.String());

	ret = _InitPath(path, &destination);
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
	if (fOffset) {
		ret = _HandleAttributes(&destination, &dir, "FoDa");
	}
	
	if (final) {
		*final = destination;
	}

	return ret;
}


int32
PkgDirectory::_ItemExists(const char *name)
{
	BString alertString = T("The file named");
	alertString << " \'" << name << "\' ";
	alertString << T("already exists in the given path. Should I replace "
		"the existing file with the one from this package?");

	BAlert *alert = new BAlert(T("file_exists"), alertString.String(),
		T("Yes"), T("No"), T("Abort"));

	return alert->Go();
}


status_t
PkgDirectory::_InitPath(const char *path, BPath *destination)
{
	status_t ret = B_OK;

	if (fPathType == P_INSTALL_PATH) {
		if (!path)
			return B_ERROR;
		ret = destination->SetTo(path, fPath.String());
	}
	else if (fPathType == P_SYSTEM_PATH)
		ret = destination->SetTo(fPath.String());
	else {
		if (!path)
			return B_ERROR;

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
PkgDirectory::_HandleAttributes(BPath *destination, BNode *node, 
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

			ret = _ParseAttribute(buffer, node, &attrName, &nameSize, &attrType,
					&attrData, &dataSize, &temp, &tempSize, &attrCSize, &attrOSize, 
					&attrStarted, &done);
			if (ret != B_OK || done)
				break;
		}

		delete[] attrData;
		delete[] temp;
	}

	return ret;
}


inline status_t
PkgDirectory::_ParseAttribute(uint8 *buffer, BNode *node, char **attrName, 
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
	}
	else if (!memcmp(buffer, "BeAN", 5)) {
		if (!*attrStarted) {
			ret = B_ERROR;
			return ret;
		}

		parser_debug(" BeAN.\n");
		fPackage->Read(&length, 4);
		swap_data(B_UINT32_TYPE, &length, sizeof(uint32), B_SWAP_BENDIAN_TO_HOST);

		if (*nameSize < (length + 1)) {
			delete *attrName;
			*nameSize = length + 1;
			*attrName = new char[*nameSize];
		}
		fPackage->Read(*attrName, length);
		(*attrName)[length] = 0;

		parser_debug(" (%ld) = %s\n", length, *attrName);
	}
	else if (!memcmp(buffer, "BeAT", 5)) {
		if (!*attrStarted) {
			ret = B_ERROR;
			return ret;
		}

		parser_debug(" BeAT.\n");
		fPackage->Read(attrType, 4);
		swap_data(B_UINT32_TYPE, attrType, sizeof(*attrType), 
				B_SWAP_BENDIAN_TO_HOST);
	}
	else if (!memcmp(buffer, "BeAD", 5)) {
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
	}
	else if (!memcmp(buffer, padding, 7)) {
		if (!*attrStarted) {
			*done = true;
			return ret;
		}

		parser_debug(" Padding.\n");
		ssize_t wrote = node->WriteAttr(*attrName, *attrType, 0, *attrData, 
			*attrOSize);
		if(wrote != static_cast<ssize_t>(*attrOSize)) {
			ret = B_ERROR;
			return ret;
		}

		*attrStarted = false;
		if (*attrName)
			*attrName[0] = 0;
		*attrCSize = 0;
		*attrOSize = 0;

		parser_debug(" > Attribute added.\n");
	}
	else {
		ret = B_ERROR;
	}

	return ret;
}


inline status_t
PkgDirectory::_ParseData(uint8 *buffer, BFile *file, uint64 originalSize, 
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
			ret = B_ERROR; // File size missmatch
			return ret;
		}
		parser_debug(" Still good...\n");

		if (fPackage->Read(buffer, 4) != 4) {
			ret = B_ERROR;
			return ret;
		}
		parser_debug(" Still good...\n");

		ret = inflate_file_to_file(fPackage, compressed, file, original);
		if (ret != B_OK)
			return ret;
		parser_debug(" File data inflation complete!\n");
	}
	else if (!memcmp(buffer, padding, 7)) {
		*done = true;
		return ret;
	}
	else {
		ret = B_ERROR;
	}

	return ret;
}



PkgFile::PkgFile(BFile *parent, BString path, uint8 type, uint32 ctime, 
			uint32 mtime, uint64 offset, uint64 size, uint64 originalSize, 
			uint32 platform, BString mime, BString signature, uint32 mode)
	:	PkgItem(parent, path, type, ctime, mtime, offset, size),
	fOriginalSize(originalSize),
	fPlatform(platform),
	fMode(mode),
	fMimeType(mime),
	fSignature(signature)
{
}


PkgFile::~PkgFile()
{
}


status_t
PkgFile::WriteToPath(const char *path, BPath *final)
{
	BPath destination;
	status_t ret;
	parser_debug("File: %s WriteToPath() called!\n", fPath.String());

	ret = _InitPath(path, &destination);
	if (ret != B_OK)
		return ret;

	BFile file(destination.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	ret = file.InitCheck();
	if (ret == B_FILE_EXISTS) {
		int32 selection = _ItemExists(destination.Leaf());
		switch (selection) {
			case 0:
				ret = file.SetTo(destination.Path(), B_WRITE_ONLY | B_ERASE_FILE);
				if (ret != B_OK)
					return ret;
				break;
			case 1:
				return B_OK;
			default:
				return B_FILE_EXISTS;
		}
	}
	else if (ret == B_ENTRY_NOT_FOUND) {
		BPath directory;
		destination.GetParent(&directory);
		if (create_directory(directory.Path(), kDefaultMode) != B_OK)
			return B_ERROR;

		ret = file.SetTo(destination.Path(), B_WRITE_ONLY | B_CREATE_FILE);
		if (ret != B_OK)
			return ret;
	}
	else if (ret != B_OK)
		return ret;

	parser_debug(" File created!\n");

	// Set the file permissions, creation and modification times
	ret = file.SetPermissions(static_cast<mode_t>(fMode));
	if (fCreationTime)
		ret |= file.SetCreationTime(static_cast<time_t>(fCreationTime));
	if (fModificationTime)
		ret |= file.SetModificationTime(static_cast<time_t>(fModificationTime));

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
			}
			else if (!memcmp(buffer, "FiDa", 5)) {
				parser_debug("-> File data\n");
				section = P_DATA;
				continue;
			}

			switch (section) {
				case P_ATTRIBUTE:
				{
					ret = _ParseAttribute(buffer, &file, &attrName, &nameSize, &attrType,
						&attrData, &dataSize, &temp, &tempSize, &attrCSize, &attrOSize, 
						&attrStarted, &done);
					break;
				}
				case P_DATA:
				{
					ret = _ParseData(buffer, &file, fOriginalSize, &done);
					break;
				}
				default:
					return B_ERROR;
			}

			if (ret != B_OK || done)
				break;
		}

		delete[] attrData;
		delete[] temp;
	}

	if (final) {
		*final = destination;
	}

	return ret;
}


PkgLink::PkgLink(BFile *parent, BString path, BString link, uint8 type, 
		uint32 ctime, uint32 mtime, uint32 mode, uint64 offset, uint64 size)
	:	PkgItem(parent, path, type, ctime, mtime, offset, size),
	fMode(mode),
	fLink(link)
{
}


PkgLink::~PkgLink()
{
}


status_t
PkgLink::WriteToPath(const char *path, BPath *final)
{
	BPath destination;
	status_t ret;
	parser_debug("Symlink: %s WriteToPath() called!\n", fPath.String());

	ret = _InitPath(path, &destination);
	if (ret != B_OK)
		return ret;

	BString linkName(destination.Leaf());
	parser_debug("%s:%s:%s\n", fPath.String(), destination.Path(), linkName.String());
	BPath dirPath;
	ret = destination.GetParent(&dirPath);
	BDirectory dir(dirPath.Path());

	ret = dir.InitCheck();
	if (ret == B_ENTRY_NOT_FOUND) {
		if (create_directory(destination.Path(), kDefaultMode) != B_OK)
			return B_ERROR;
	}
	if (ret != B_OK)
		return ret;

	BSymLink symlink;
	ret = dir.CreateSymLink(linkName.String(), fLink.String(), &symlink);
	if (ret != B_OK)
		return ret;

	parser_debug(" Symlink created!\n");

	ret = symlink.SetPermissions(static_cast<mode_t>(fMode));

	if (fCreationTime)
		ret |= symlink.SetCreationTime(static_cast<time_t>(fCreationTime));
	
	if (fModificationTime)
		ret |= symlink.SetModificationTime(static_cast<time_t>(fModificationTime));

	if (ret != B_OK)
		return ret;

	if (fOffset) {
		// Simlinks also seem to have attributes - so parse them
		ret = _HandleAttributes(&destination, &dir, "LnDa");
	}

	if (final) {
		*final = destination;
	}

	return ret;
}

