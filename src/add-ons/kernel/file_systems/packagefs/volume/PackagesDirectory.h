/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGES_DIRECTORY_H
#define PACKAGES_DIRECTORY_H


#include <sys/stat.h>

#include <Referenceable.h>


class PackagesDirectory : public BReferenceable {
public:
								PackagesDirectory();
								~PackagesDirectory();

			const char*			Path() const
									{ return fPath; }
			int					DirectoryFD() const
									{ return fDirFD; }
			dev_t				DeviceID() const
									{ return fDeviceID; }
			ino_t				NodeID() const
									{ return fNodeID; }

			status_t			Init(const char* path, dev_t mountPointDeviceID,
									ino_t mountPointNodeID, struct stat& _st);

private:
			char*				fPath;
			int					fDirFD;
			dev_t				fDeviceID;
			ino_t				fNodeID;
};


#endif	// PACKAGES_DIRECTORY_H
