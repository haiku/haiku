/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <fs_info.h>
#include <fs_interface.h>
#include <fs_volume.h>

#include <lock.h>


class BlockAllocator;
class Directory;
class File;
class Node;
class SymLink;
class Transaction;


class Volume {
public:
								Volume(uint32 flags);
								~Volume();

			status_t			Init(const char* device);
			status_t			Init(int fd, uint64 totalBlocks);

			status_t			Mount(fs_volume* fsVolume);
			void				Unmount();

			status_t			Initialize(const char* name);

			void				GetInfo(fs_info& info);

			status_t			NewNode(Node* node);
			status_t			PublishNode(Node* node, uint32 flags);
			status_t			GetNode(uint64 blockIndex, Node*& _node);
			status_t			PutNode(Node* node);
			status_t			RemoveNode(Node* node);
			status_t			UnremoveNode(Node* node);

			status_t			ReadNode(uint64 blockIndex, Node*& _node);

			status_t			CreateDirectory(mode_t mode,
									Transaction& transaction,
									Directory*& _directory);
			status_t			CreateFile(mode_t mode,
									Transaction& transaction, File*& _file);
			status_t			CreateSymLink(mode_t mode,
									Transaction& transaction,
									SymLink*& _symLink);
			status_t			DeleteNode(Node* node);

			status_t			SetName(const char* name);

	inline	void				TransactionStarted();
	inline	void				TransactionFinished();

	inline	dev_t				ID() const			{ return fFSVolume->id; }
	inline	int					FD() const			{ return fFD; }
	inline	bool				IsReadOnly() const;
	inline	uint64				TotalBlocks() const	{ return fTotalBlocks; }
	inline	void*				BlockCache() const	{ return fBlockCache; }
	inline	const char*			Name() const		{ return fName; }
	inline	BlockAllocator*		GetBlockAllocator() const
									{ return fBlockAllocator; }
	inline	Directory*			RootDirectory() const
									{ return fRootDirectory; }

private:
			status_t			_Init(uint64 totalBlocks);

			status_t			_CreateNode(Node* node,
									Transaction& transaction);
			status_t			_DeleteDirectoryEntries(Directory* directory);

private:
			fs_volume*			fFSVolume;
			int					fFD;
			uint32				fFlags;
			void*				fBlockCache;
			uint64				fTotalBlocks;
			char*				fName;
			BlockAllocator*		fBlockAllocator;
			Directory*			fRootDirectory;
			mutex				fLock;
			mutex				fTransactionLock;
};


void
Volume::TransactionStarted()
{
	mutex_lock(&fTransactionLock);
}


void
Volume::TransactionFinished()
{
	mutex_unlock(&fTransactionLock);
}


bool
Volume::IsReadOnly() const
{
	return (fFlags & B_FS_IS_READONLY) != 0;
}


#endif	// VOLUME_H
