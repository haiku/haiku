/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNPACKING_DIRECTORY_H
#define UNPACKING_DIRECTORY_H


#include "Directory.h"
#include "PackageDirectory.h"
#include "UnpackingNode.h"


class UnpackingDirectory : public Directory, public UnpackingNode {
public:
								UnpackingDirectory(ino_t id);
	virtual						~UnpackingDirectory();

	virtual	status_t			Init(Directory* parent, const char* name);

	virtual	mode_t				Mode() const;
	virtual	uid_t				UserID() const;
	virtual	gid_t				GroupID() const;
	virtual	timespec			ModifiedTime() const;
	virtual	off_t				FileSize() const;

	virtual	Node*				GetNode();

	virtual	status_t			AddPackageNode(PackageNode* packageNode);
	virtual	void				RemovePackageNode(PackageNode* packageNode);

	virtual	PackageNode*		GetPackageNode();

	virtual	status_t			Read(off_t offset, void* buffer,
									size_t* bufferSize);
	virtual	status_t			Read(io_request* request);

	virtual	status_t			ReadSymlink(void* buffer,
									size_t* bufferSize);

	virtual	status_t			OpenAttributeDirectory(
									AttributeDirectoryCookie*& _cookie);
	virtual	status_t			OpenAttribute(const char* name, int openMode,
									AttributeCookie*& _cookie);

private:
			PackageDirectoryList fPackageDirectories;
};


class RootDirectory : public UnpackingDirectory {
public:
								RootDirectory(ino_t id,
									const timespec& modifiedTime);

	virtual	timespec			ModifiedTime() const;

private:
			timespec			fModifiedTime;
};


#endif	// UNPACKING_DIRECTORY_H
