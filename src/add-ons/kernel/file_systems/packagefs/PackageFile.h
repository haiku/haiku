/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_FILE_H
#define PACKAGE_FILE_H


#include "PackageData.h"

#include "PackageLeafNode.h"


class PackageFile : public PackageLeafNode {
public:
								PackageFile(mode_t mode,
									const PackageData& data);
	virtual						~PackageFile();

private:
			PackageData			fData;
};


#endif	// PACKAGE_FILE_H
