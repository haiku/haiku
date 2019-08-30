/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SYMLINK_H
#define SYMLINK_H

#include "Node.h"
#include "String.h"

class SymLink : public Node {
public:
	SymLink(Volume *volume);
	virtual ~SymLink();

	virtual status_t SetSize(off_t newSize);
	virtual off_t GetSize() const;

	status_t SetLinkedPath(const char *path);
	const char *GetLinkedPath() const { return fLinkedPath.GetString(); }
	size_t GetLinkedPathLength() const { return fLinkedPath.GetLength(); }

	// debugging
	virtual void GetAllocationInfo(AllocationInfo &info);

private:
	String	fLinkedPath;
};

#endif	// SYMLINK_H
