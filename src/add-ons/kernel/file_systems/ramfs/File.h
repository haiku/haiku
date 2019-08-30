/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FILE_H
#define FILE_H

#include "DataContainer.h"
#include "Node.h"

class File : public Node, public DataContainer {
public:
	File(Volume *volume);
	virtual ~File();

	Volume *GetVolume() const	{ return Node::GetVolume(); }

	virtual status_t ReadAt(off_t offset, void *buffer, size_t size,
							size_t *bytesRead);
	virtual status_t WriteAt(off_t offset, const void *buffer, size_t size,
							 size_t *bytesWritten);

	virtual status_t SetSize(off_t newSize);
	virtual off_t GetSize() const;

	// debugging
	virtual void GetAllocationInfo(AllocationInfo &info);
};

#endif	// FILE_H
