/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Directory.h"


DirectoryIterator::DirectoryIterator(Inode* inode)
	:
	fInode(inode),
	fShortDir(NULL),
	fExtentDir(NULL),
	fLeafDir(NULL),
	fNodeDir(NULL),
	fTreeDir(NULL)
{
}


DirectoryIterator::~DirectoryIterator()
{
	delete fShortDir;
	delete fLeafDir;
	delete fExtentDir;
	delete fNodeDir;
	delete fTreeDir;
}


status_t
DirectoryIterator::Init()
{
	if (fInode->Format() == XFS_DINODE_FMT_LOCAL)
	{
		fShortDir = new(std::nothrow) ShortDirectory(fInode);
		if (fShortDir == NULL)
			return B_NO_MEMORY;
		return B_OK;
	}
	if (fInode->Format() == XFS_DINODE_FMT_EXTENTS) {
		// TODO: Only working with Block directories, not leaf.
		fExtentDir = new(std::nothrow) Extent(fInode);
		if (fExtentDir == NULL)
			return B_NO_MEMORY;
		if (fExtentDir->IsBlockType())
			return fExtentDir->Init();

		delete fExtentDir;
		fExtentDir = NULL;

		fLeafDir = new(std::nothrow) LeafDirectory(fInode);
		if (fLeafDir == NULL)
			return B_NO_MEMORY;
		status_t status = fLeafDir->Init();
		if (status != B_OK)
			return status;
		if (fLeafDir->IsLeafType())
			return B_OK;
		delete fLeafDir;
		fLeafDir = NULL;

		fNodeDir = new(std::nothrow) NodeDirectory(fInode);
		if (fNodeDir == NULL)
			return B_NO_MEMORY;
		status = fNodeDir->Init();
		if (status != B_OK)
			return status;
		if (fNodeDir->IsNodeType())
			return B_OK;
		delete fNodeDir;
		fNodeDir = NULL;
	}

	/* Return B_OK so even if the shortform directory has an extent directory
	 * we can atleast still list the shortform directory
	 */

	//TODO: Reading from B+Trees based directories
	if (fInode->Format() == XFS_DINODE_FMT_BTREE) {
		TRACE("Iterator:GetNext: B+TREE");
		fTreeDir = new(std::nothrow) TreeDirectory(fInode);
		if (fTreeDir == NULL)
			return B_NO_MEMORY;
		return fTreeDir->InitCheck();
	}

	return B_BAD_VALUE;
}


status_t
DirectoryIterator::GetNext(char* name, size_t* length, xfs_ino_t* ino)
{
	status_t status;
	if (fInode->Format() == XFS_DINODE_FMT_LOCAL) {
		status = fShortDir->GetNext(name, length, ino);
		return status;
	}

	//TODO: Reading from extent based directories
	if (fInode->Format() == XFS_DINODE_FMT_EXTENTS) {
		TRACE("Iterator:GetNext: EXTENTS");
		if (fExtentDir != NULL)
			status = fExtentDir->GetNext(name, length, ino);
		else if (fLeafDir != NULL)
			status = fLeafDir->GetNext(name, length, ino);
		else
			status = fNodeDir->GetNext(name, length, ino);
		return status;
	}

	//TODO: Reading from B+Trees based directories
	if (fInode->Format() == XFS_DINODE_FMT_BTREE) {
		TRACE("Iterator:GetNext: B+TREE");
		return B_NOT_SUPPORTED;
	}

	// Only reaches here if Inode is a device or is corrupt.
	return B_BAD_VALUE;
}


status_t
DirectoryIterator::Lookup(const char* name, size_t length, xfs_ino_t* ino)
{
	status_t status;
	if (fInode->Format() == XFS_DINODE_FMT_LOCAL) {
		status = fShortDir->Lookup(name, length, ino);
		return status;
	}

	//TODO: Reading from extent based dirs
	if (fInode->Format() == XFS_DINODE_FMT_EXTENTS) {
		TRACE("Iterator:Lookup: EXTENTS");
		if (fExtentDir != NULL)
			status = fExtentDir->Lookup(name, length, ino);
		else if (fLeafDir != NULL)
			status = fLeafDir->Lookup(name, length, ino);
		else
			status = fNodeDir->Lookup(name, length, ino);
		return status;
	}

	//TODO: Reading from B+Tree based dirs
	if (fInode->Format() == XFS_DINODE_FMT_BTREE) {
		TRACE("Iterator:Lookup: B+TREE");
		return B_NOT_SUPPORTED;
	}

	return B_BAD_VALUE;
}
