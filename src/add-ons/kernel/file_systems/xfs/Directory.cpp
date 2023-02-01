/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "BPlusTree.h"
#include "Directory.h"
#include "Extent.h"
#include "LeafDirectory.h"
#include "Node.h"
#include "ShortDirectory.h"


DirectoryIterator::~DirectoryIterator()
{
}


DirectoryIterator*
DirectoryIterator::Init(Inode* inode)
{
	if (inode->Format() == XFS_DINODE_FMT_LOCAL) {
		TRACE("Iterator:Init: LOCAL");
		ShortDirectory* shortDir = new(std::nothrow) ShortDirectory(inode);
		return shortDir;
	}

	if (inode->Format() == XFS_DINODE_FMT_EXTENTS) {
		TRACE("Iterator:Init: EXTENTS");
		status_t status;

		// Check if it is extent based directory
		Extent* extentDir = new(std::nothrow) Extent(inode);
		if (extentDir == NULL)
			return NULL;

		if (extentDir->IsBlockType()) {
			status = extentDir->Init();
			if (status == B_OK)
				return extentDir;
		}

		delete extentDir;

		// Check if it is leaf based directory
		LeafDirectory* leafDir = new(std::nothrow) LeafDirectory(inode);
		if (leafDir == NULL)
			return NULL;

		if (leafDir->IsLeafType()) {
			status = leafDir->Init();
			if (status == B_OK)
				return leafDir;
		}

		delete leafDir;

		// Check if it is node based directory
		NodeDirectory* nodeDir = new(std::nothrow) NodeDirectory(inode);
		if (nodeDir == NULL)
			return NULL;

		if (nodeDir->IsNodeType()) {
			status = nodeDir->Init();
			if (status == B_OK)
				return nodeDir;
		}

		delete nodeDir;
	}

	if (inode->Format() == XFS_DINODE_FMT_BTREE) {
		TRACE("Iterator:Init(): B+TREE");
		TreeDirectory* treeDir = new(std::nothrow) TreeDirectory(inode);
		if (treeDir == NULL)
			return NULL;

		status_t status = treeDir->InitCheck();

		if (status == B_OK)
			return treeDir;
	}

	// Invalid format return NULL
	return NULL;
}