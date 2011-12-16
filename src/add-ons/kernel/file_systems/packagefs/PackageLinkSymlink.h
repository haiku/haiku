/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LINK_SYMLINK_H
#define PACKAGE_LINK_SYMLINK_H


#include "Node.h"


class Package;
class PackageLinksListener;


class PackageLinkSymlink : public Node {
public:
								PackageLinkSymlink(Package* package);
	virtual						~PackageLinkSymlink();

			void				Update(Package* package,
									PackageLinksListener* listener);

	virtual	mode_t				Mode() const;
	virtual	timespec			ModifiedTime() const;
	virtual	off_t				FileSize() const;

	virtual	status_t			Read(off_t offset, void* buffer,
									size_t* bufferSize);
	virtual	status_t			Read(io_request* request);

	virtual	status_t			ReadSymlink(void* buffer, size_t* bufferSize);

private:
			struct OldAttributes;

private:
			timespec			fModifiedTime;
			const char*			fLinkPath;
};


#endif	// PACKAGE_LINK_SYMLINK_H
