/*
 * Copyright 2007-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "PackageItem.h"

#include <string.h>

#include <Alert.h>
#include <ByteOrder.h>
#include <Catalog.h>
#include <Directory.h>
#include <fs_info.h>
#include <Locale.h>
#include <NodeInfo.h>
#include <OS.h>
#include <SymLink.h>
#include <Volume.h>

#include "zlib.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageItem"

enum {
	P_CHUNK_SIZE = 256
};

static const uint32 kDefaultMode = 0777;
static const uint8 padding[7] = { 0, 0, 0, 0, 0, 0, 0 };

enum {
	P_DATA = 0,
	P_ATTRIBUTE
};

extern const char **environ;


status_t
inflate_data(uint8 *in, uint32 inSize, uint8 *out, uint32 outSize)
{
	parser_debug("inflate_data() called - input_size: %ld, output_size: %ld\n",
		inSize, outSize);
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
			delete[] *attrName;
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
			delete[] *temp;
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
PackageItem::SkipAttribute(uint8 *buffer, bool *attrStarted, bool *done)
{
	status_t ret = B_OK;
	uint32 length;

	if (!memcmp(buffer, "BeAI", 5)) {
		parser_debug(" Attribute started.\n");
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

		fPackage->Seek(length, SEEK_CUR);
	} else if (!memcmp(buffer, "BeAT", 5)) {
		if (!*attrStarted) {
			ret = B_ERROR;
			return ret;
		}

		parser_debug(" BeAT.\n");
		fPackage->Seek(4, SEEK_CUR);
	} else if (!memcmp(buffer, "BeAD", 5)) {
		if (!*attrStarted) {
			ret = B_ERROR;
			return ret;
		}

		parser_debug(" BeAD.\n");
		uint64 length64;
		fPackage->Read(&length64, 8);
		swap_data(B_UINT64_TYPE, &length64, sizeof(length64),
			B_SWAP_BENDIAN_TO_HOST);

		fPackage->Seek(12 + length64, SEEK_CUR);

		parser_debug("  Data skipped successfuly.\n");
	} else if (!memcmp(buffer, padding, 7)) {
		if (!*attrStarted) {
			*done = true;
			return ret;
		}

		parser_debug(" Padding.\n");
		*attrStarted = false;
		parser_debug(" > Attribute skipped.\n");
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


// #pragma mark - PackageScript


PackageScript::PackageScript(BFile *parent, uint64 offset, uint64 size,
		uint64 originalSize)
	:
	PackageItem(parent, NULL, 0, 0, 0, offset, size),
	fOriginalSize(originalSize),
	fThreadId(-1)
{
}


status_t
PackageScript::DoInstall(const char *path, ItemState *state)
{
	status_t ret = B_OK;
	parser_debug("Script: DoInstall() called!\n");

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
					ret = SkipAttribute(buffer, &attrStarted, &done);
					break;

				case P_DATA:
					ret = _ParseScript(buffer, fOriginalSize, &done);
					break;

				default:
					return B_ERROR;
			}

			if (ret != B_OK || done)
				break;
		}
	}

	parser_debug("Ret: %ld %s\n", ret, strerror(ret));
	return ret;
}


const uint32
PackageScript::ItemKind()
{
	return P_KIND_SCRIPT;
}


status_t
PackageScript::_ParseScript(uint8 *buffer, uint64 originalSize, bool *done)
{
	status_t ret = B_OK;

	if (!memcmp(buffer, "FiMF", 5)) {
		parser_debug(" Found file (script) data.\n");
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

		uint8 *temp = new uint8[compressed];
		if (fPackage->Read(temp, compressed) != (int64)compressed) {
			parser_debug(" Read(temp, compressed) failed\n");
			delete[] temp;
			return B_ERROR;
		}

		uint8 *script = new uint8[original];
		ret = inflate_data(temp, compressed, script, original);
		if (ret != B_OK) {
			parser_debug(" inflate_data failed\n");
			delete[] temp;
			delete[] script;
			return ret;
		}

		ret = _RunScript(script, originalSize);
		delete[] script;
		delete[] temp;
		parser_debug(" Script data inflation complete!\n");
	} else if (!memcmp(buffer, padding, 7)) {
		*done = true;
		return ret;
	} else {
		parser_debug("_ParseData unknown tag\n");
		ret = B_ERROR;
	}

	return ret;
}


status_t
PackageScript::_RunScript(uint8 *script, uint32 len)
{
	// This function written by Peter Folk <pfolk@uni.uiuc.edu>
	// and published in the BeDevTalk FAQ, modified for use in the
	// PackageInstaller
	// http://www.abisoft.com/faq/BeDevTalk_FAQ.html#FAQ-209

	// Save current FDs
	int old_in  =  dup(0);
	int old_out  =  dup(1);
	int old_err  =  dup(2);

	int filedes[2];

	/* Create new pipe FDs as stdin, stdout, stderr */
	pipe(filedes);  dup2(filedes[0], 0); close(filedes[0]);
	int in = filedes[1];  // Write to in, appears on cmd's stdin
	pipe(filedes);  dup2(filedes[1], 1); close(filedes[1]);
	pipe(filedes);  dup2(filedes[1], 2); close(filedes[1]);

	const char **argv = new const char * [3];
	argv[0] = strdup("/bin/sh");
	argv[1] = strdup("-s");
	argv[2] = NULL;

	// "load" command.
	fThreadId = load_image(2, argv, environ);

	int i;
	for (i = 0; i < 2; i++)
		delete argv[i];
	delete [] argv;

	if (fThreadId < B_OK)
		return fThreadId;

	// thread id is now suspended.
	setpgid(fThreadId, fThreadId);

	// Restore old FDs
	close(0); dup(old_in); close(old_in);
	close(1); dup(old_out); close(old_out);
	close(2); dup(old_err); close(old_err);

	set_thread_priority(fThreadId, B_LOW_PRIORITY);
	resume_thread(fThreadId);

	// Write the script
	if (write(in, script, len) != (int32)len || write(in, "\nexit\n", 6) != 6) {
		parser_debug("Writing script failed\n");
		kill_thread(fThreadId);
		return B_ERROR;
	}

	return B_OK;
}


// #pragma mark - PackageDirectory


PackageDirectory::PackageDirectory(BFile *parent, const BString &path,
		uint8 type, uint32 ctime, uint32 mtime, uint64 offset, uint64 size)
	:
	PackageItem(parent, path, type, ctime, mtime, offset, size)
{
}


status_t
PackageDirectory::DoInstall(const char *path, ItemState *state)
{
	BPath &destination = state->destination;
	status_t ret;
	parser_debug("Directory: %s DoInstall() called!\n", fPath.String());

	ret = InitPath(path, &destination);
	parser_debug("Ret: %ld %s\n", ret, strerror(ret));
	if (ret != B_OK)
		return ret;

	// Since Haiku is single-user right now, we give the newly
	// created directory default permissions
	ret = create_directory(destination.Path(), kDefaultMode);
	parser_debug("Create dir ret: %ld %s\n", ret, strerror(ret));
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

	parser_debug("Ret: %ld %s\n", ret, strerror(ret));
	return ret;
}


const uint32
PackageDirectory::ItemKind()
{
	return P_KIND_DIRECTORY;
}


//	#pragma mark - PackageFile


PackageFile::PackageFile(BFile *parent, const BString &path, uint8 type,
		uint32 ctime, uint32 mtime, uint64 offset, uint64 size,
		uint64 originalSize, uint32 platform, const BString &mime,
		const BString &signature, uint32 mode)
	:
	PackageItem(parent, path, type, ctime, mtime, offset, size),
	fOriginalSize(originalSize),
	fPlatform(platform),
	fMode(mode),
	fMimeType(mime),
	fSignature(signature)
{
}


status_t
PackageFile::DoInstall(const char *path, ItemState *state)
{
	if (state == NULL)
		return B_ERROR;

	BPath &destination = state->destination;
	status_t ret = B_OK;
	parser_debug("File: %s DoInstall() called!\n", fPath.String());

	BFile file;
	if (state->status == B_NO_INIT || destination.InitCheck() != B_OK) {
		ret = InitPath(path, &destination);
		if (ret != B_OK)
			return ret;

		ret = file.SetTo(destination.Path(),
			B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
		if (ret == B_ENTRY_NOT_FOUND) {
			BPath directory;
			destination.GetParent(&directory);
			if (create_directory(directory.Path(), kDefaultMode) != B_OK)
				return B_ERROR;

			ret = file.SetTo(destination.Path(), B_WRITE_ONLY | B_CREATE_FILE);
		} else if (ret == B_FILE_EXISTS)
			state->status = B_FILE_EXISTS;

		if (ret != B_OK)
			return ret;
	}

	if (state->status == B_FILE_EXISTS) {
		switch (state->policy) {
			case P_EXISTS_OVERWRITE:
				ret = file.SetTo(destination.Path(),
					B_WRITE_ONLY | B_ERASE_FILE);
				break;

			case P_EXISTS_NONE:
			case P_EXISTS_ASK:
				ret = B_FILE_EXISTS;
				break;

			case P_EXISTS_SKIP:
				return B_OK;
		}
	}

	if (ret != B_OK)
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

	return ret;
}


const uint32
PackageFile::ItemKind()
{
	return P_KIND_FILE;
}


//	#pragma mark -


PackageLink::PackageLink(BFile *parent, const BString &path,
		const BString &link, uint8 type, uint32 ctime, uint32 mtime,
		uint32 mode, uint64 offset, uint64 size)
	:
	PackageItem(parent, path, type, ctime, mtime, offset, size),
	fMode(mode),
	fLink(link)
{
}


status_t
PackageLink::DoInstall(const char *path, ItemState *state)
{
	if (state == NULL)
		return B_ERROR;

	status_t ret = B_OK;
	BSymLink symlink;
	parser_debug("Symlink: %s DoInstall() called!\n", fPath.String());

	BPath &destination = state->destination;
	BDirectory *dir = &state->parent;

	if (state->status == B_NO_INIT || destination.InitCheck() != B_OK
		|| dir->InitCheck() != B_OK) {
		// Not yet initialized
		ret = InitPath(path, &destination);
		if (ret != B_OK)
			return ret;

		BString linkName(destination.Leaf());
		parser_debug("%s:%s:%s\n", fPath.String(), destination.Path(),
			linkName.String());

		BPath dirPath;
		ret = destination.GetParent(&dirPath);
		ret = dir->SetTo(dirPath.Path());

		if (ret == B_ENTRY_NOT_FOUND) {
			ret = create_directory(dirPath.Path(), kDefaultMode);
			if (ret != B_OK) {
				parser_debug("create_directory()) failed\n");
				return B_ERROR;
			}
		}
		if (ret != B_OK) {
			parser_debug("destination InitCheck failed %s for %s\n",
				strerror(ret), dirPath.Path());
			return ret;
		}

		ret = dir->CreateSymLink(destination.Path(), fLink.String(), &symlink);
		if (ret == B_FILE_EXISTS) {
			// We need to check if the existing symlink is pointing at the same path
			// as our new one - if not, let's prompt the user
			symlink.SetTo(destination.Path());
			BPath oldLink;

			ret = symlink.MakeLinkedPath(dir, &oldLink);
			chdir(dirPath.Path());

			if (ret == B_BAD_VALUE || oldLink != fLink.String())
				state->status = ret = B_FILE_EXISTS;
			else
				ret = B_OK;
		}
	}

	if (state->status == B_FILE_EXISTS) {
		switch (state->policy) {
			case P_EXISTS_OVERWRITE:
			{
				BEntry entry;
				ret = entry.SetTo(destination.Path());
				if (ret != B_OK)
					return ret;

				entry.Remove();
				ret = dir->CreateSymLink(destination.Path(), fLink.String(),
					&symlink);
				break;
			}

			case P_EXISTS_NONE:
			case P_EXISTS_ASK:
				ret = B_FILE_EXISTS;
				break;

			case P_EXISTS_SKIP:
				return B_OK;
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

	return ret;
}


const uint32
PackageLink::ItemKind()
{
	return P_KIND_SYM_LINK;
}

