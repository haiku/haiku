/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DIRECTORY_H
#define DIRECTORY_H


#include "Node.h"


class Directory : public Node {
public:
								Directory(Volume* volume, uint64 blockIndex,
									const checksumfs_node& nodeData);
								Directory(Volume* volume, uint64 blockIndex,
									mode_t mode);
	virtual						~Directory();
};


#endif	// DIRECTORY_H
