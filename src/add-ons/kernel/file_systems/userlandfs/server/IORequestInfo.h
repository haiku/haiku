/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_IO_REQUEST_INFO_H
#define USERLAND_FS_IO_REQUEST_INFO_H

#include <util/OpenHashTable.h>


namespace UserlandFS {

struct IORequestInfo : HashTableLink<IORequestInfo> {
	int32	id;
	bool	isWrite;

	IORequestInfo(int32 id, bool isWrite)
		:
		id(id),
		isWrite(isWrite)
	{
	}
};

}	// namespace UserlandFS


using UserlandFS::IORequestInfo;


#endif	// USERLAND_FS_IO_REQUEST_INFO_H
