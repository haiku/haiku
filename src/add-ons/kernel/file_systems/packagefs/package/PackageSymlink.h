/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_SYMLINK_H
#define PACKAGE_SYMLINK_H


#include "PackageLeafNode.h"


class PackageSymlink : public PackageLeafNode {
public:
								PackageSymlink(Package* package, mode_t mode);
	virtual						~PackageSymlink();

			void				SetSymlinkPath(const String& path);

	virtual	String				SymlinkPath() const;

private:
				String			fSymlinkPath;
};


#endif	// PACKAGE_SYMLINK_H
