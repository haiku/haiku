/******************************************************************************
/
/	File:			FileUtils.cpp
/
/   Description:	Utility functions for copying file data and attributes.
/
/	Copyright 1998-1999, Be Incorporated, All Rights Reserved
/
******************************************************************************/
#include "FileUtils.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <fs_attr.h>

#include "AutoDeleter.h"


using std::nothrow;


status_t
CopyFileData(BFile& dst, BFile& src)
{
	struct stat src_stat;
	status_t err = src.GetStat(&src_stat);
	if (err != B_OK) {
		printf("couldn't get stat: %#010lx\n", err);
		return err;
	}
		
	size_t bufSize = src_stat.st_blksize;
	if (bufSize == 0)
		bufSize = 32768;
	
	char* buf = new (nothrow) char[bufSize];
	if (buf == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<char> _(buf);
	
	printf("copy data, bufSize = %ld\n", bufSize);
	// copy data
	while (true) {
		ssize_t bytes = src.Read(buf, bufSize);
		if (bytes > 0) {
			ssize_t result = dst.Write(buf, bytes);
			if (result != bytes) {
				fprintf(stderr, "Failed to write %ld bytes: %s\n", bytes,
					strerror((status_t)result));
				if (result < 0)
					return (status_t)result;
				else
					return B_IO_ERROR;
			}
		} else {
			if (bytes < 0) {
				fprintf(stderr, "Failed to read file: %s\n", strerror(
					(status_t)bytes));
				return (status_t)bytes;
			} else {
				// EOF
				break;
			}
		}
	}

	// finish up miscellaneous stat stuff
	dst.SetPermissions(src_stat.st_mode);
	dst.SetOwner(src_stat.st_uid);
	dst.SetGroup(src_stat.st_gid);
	dst.SetModificationTime(src_stat.st_mtime);
	dst.SetCreationTime(src_stat.st_crtime);

	return B_OK;
}


status_t
CopyAttributes(BNode& dst, BNode& src)
{
	// copy attributes
	src.RewindAttrs();
	char attrName[B_ATTR_NAME_LENGTH];
	while (src.GetNextAttrName(attrName) == B_OK) {
		attr_info info;
		if (src.GetAttrInfo(attrName, &info) != B_OK) {
			fprintf(stderr, "Failed to read info for attribute '%s'\n",
				attrName);
			continue;
		}
		// copy one attribute in chunks of 4096 bytes
		size_t size = 4096;
		uint8 buffer[size];
		off_t offset = 0;
		ssize_t read = src.ReadAttr(attrName, info.type, offset, buffer,
			min_c(size, info.size));
		if (read < 0) {
			fprintf(stderr, "Error reading attribute '%s'\n", attrName);
			return (status_t)read;
		}
		// NOTE: Attributes of size 0 are perfectly valid!
		while (read >= 0) {
			ssize_t written = dst.WriteAttr(attrName, info.type, offset, 
				buffer, read);
			if (written != read) {
				fprintf(stderr, "Error writing attribute '%s'\n", attrName);
				if (written < 0)
					return (status_t)written;
				else
					return B_IO_ERROR;
			}
			offset += read;
			read = src.ReadAttr(attrName, info.type, offset, buffer,
				min_c(size, info.size - offset));
			if (read < 0) {
				fprintf(stderr, "Error reading attribute '%s'\n", attrName);
				return (status_t)read;
			}
			if (read == 0)
				break;
		}
	}

	return B_OK;		
}


status_t
CopyFile(BFile& dst, BFile& src)
{
	status_t err = CopyFileData(dst, src);
	if (err != B_OK)
		return err;
		
	return CopyAttributes(dst, src);
}
