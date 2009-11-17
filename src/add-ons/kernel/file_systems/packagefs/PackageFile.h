/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_FILE_H
#define PACKAGE_FILE_H


#include "PackageNode.h"


class PackageFile : public PackageNode {
public:
								PackageFile();
	virtual						~PackageFile();
};


#endif	// PACKAGE_FILE_H
