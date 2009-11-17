/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_SYMLINK_H
#define PACKAGE_SYMLINK_H


#include "PackageNode.h"


class PackageSymlink : public PackageNode {
public:
								PackageSymlink();
	virtual						~PackageSymlink();
};


#endif	// PACKAGE_SYMLINK_H
