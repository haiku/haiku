/*******************************************************************************
/
/	File:			FileUtils.cpp
/
/   Description:	Utility functions for copying file data and attributes.
/
/	Copyright 1998-1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#include <stdio.h>
#include <fs_attr.h>
#include "array_delete.h"
#include "FileUtils.h"

status_t CopyFileData(BFile& dst, BFile& src)
{
	struct stat src_stat;
	status_t err = src.GetStat(&src_stat);
	if (err != B_OK) {
		printf("couldn't get stat: %#010lx\n", err);
		return err;
	}
		
	size_t bufSize = src_stat.st_blksize;
	if (! bufSize) {
		bufSize = 32768;
	}
	
	char* buf = new char[bufSize];
	array_delete<char> bufDelete(buf);
	
	printf("copy data, bufSize = %ld\n", bufSize);
	// copy data
	while (true) {
		ssize_t bytes = src.Read(buf, bufSize);
		if (bytes > 0) {
			ssize_t result = dst.Write(buf, bytes);
			if (result != bytes) {
				printf("result = %#010lx, bytes = %#010lx\n", (uint32) result,
					(uint32) bytes);
				return B_ERROR;
			}
		} else {
			if (bytes < 0) {
				printf(" bytes = %#010lx\n", (uint32) bytes);
				return bytes;
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


status_t CopyAttributes(BNode& dst, BNode& src)
{
	// copy attributes
	src.RewindAttrs();
	char name[B_ATTR_NAME_LENGTH];
	while (src.GetNextAttrName(name) == B_OK) {
		attr_info info;
		if (src.GetAttrInfo(name, &info) == B_OK) {
			size_t bufSize = info.size;
			char* buf = new char[bufSize];
			array_delete<char> bufDelete = buf;
			
			// copy one attribute
			ssize_t bytes = src.ReadAttr(name, info.type, 0, buf, bufSize);
			if (bytes > 0) {
				dst.WriteAttr(name, info.type, 0, buf, bufSize);
			} else {
				return bytes;
			}
		}
	}

	return B_OK;		
}


status_t CopyFile(BFile& dst, BFile& src)
{
	status_t err = CopyFileData(dst, src);
	if (err != B_OK)
		return err;
		
	return CopyAttributes(dst, src);
}
