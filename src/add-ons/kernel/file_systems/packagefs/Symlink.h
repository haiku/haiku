/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYMLINK_H
#define SYMLINK_H


#include "Node.h"


class Symlink : public Node {
public:
								Symlink(ino_t id);
	virtual						~Symlink();

	virtual	status_t			Init(Directory* parent, const char* name);

	virtual	status_t			AddPackageNode(PackageNode* packageNode);
};


#endif	// SYMLINK_H
