/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_H
#define FILE_H


#include "Node.h"


class File : public Node {
public:
								File(ino_t id);
	virtual						~File();

	virtual	status_t			Init(Directory* parent, const char* name);

	virtual	status_t			AddPackageNode(PackageNode* packageNode);
};


#endif	// FILE_H
