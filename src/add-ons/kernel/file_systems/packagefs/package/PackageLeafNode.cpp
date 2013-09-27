/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLeafNode.h"


PackageLeafNode::PackageLeafNode(Package* package, mode_t mode)
	:
	PackageNode(package, mode)
{
}


PackageLeafNode::~PackageLeafNode()
{
}


String
PackageLeafNode::SymlinkPath() const
{
	return String();
}


status_t
PackageLeafNode::Read(off_t offset, void* buffer, size_t* bufferSize)
{
	return EBADF;
}


status_t
PackageLeafNode::Read(io_request* request)
{
	return EBADF;
}
