/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYM_LINK_H
#define SYM_LINK_H


#include "Node.h"


class SymLink : public Node {
public:
								SymLink(Volume* volume, uint64 blockIndex,
									const checksumfs_node& nodeData);
								SymLink(Volume* volume, mode_t mode);
	virtual						~SymLink();

			status_t			ReadSymLink(char* buffer, size_t toRead,
									size_t& _bytesRead);
			status_t			WriteSymLink(const char* buffer, size_t toWrite,
									Transaction& transaction);
};


#endif	// SYM_LINK_H
