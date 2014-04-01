/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGES_DIRECTORY_H
#define PACKAGES_DIRECTORY_H


#include <sys/stat.h>

#include <Referenceable.h>

#include "NodeRef.h"


class PackagesDirectory : public BReferenceable {
public:
								PackagesDirectory();
								~PackagesDirectory();

			const char*			Path() const
									{ return fPath; }
			int					DirectoryFD() const
									{ return fDirFD; }
			const node_ref&		NodeRef() const
									{ return fNodeRef; }
			dev_t				DeviceID() const
									{ return fNodeRef.device; }
			ino_t				NodeID() const
									{ return fNodeRef.node; }

			status_t			Init(const char* path, dev_t mountPointDeviceID,
									ino_t mountPointNodeID, struct stat& _st);

private:
			char*				fPath;
			int					fDirFD;
			node_ref			fNodeRef;
};


#endif	// PACKAGES_DIRECTORY_H
