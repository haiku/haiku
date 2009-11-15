/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DIRECTORY_H
#define DIRECTORY_H


#include "Node.h"


class Directory : public Node {
public:
								Directory(ino_t id);
	virtual						~Directory();
};


#endif	// DIRECTORY_H
