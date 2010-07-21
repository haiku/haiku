/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_H
#define FILE_H


#include "Node.h"


struct file_io_vec;


class File : public Node {
public:
								File(Volume* volume, uint64 blockIndex,
									const checksumfs_node& nodeData);
								File(Volume* volume, mode_t mode);
	virtual						~File();

	virtual	status_t			InitForVFS();
	virtual	void				DeletingNode();

	virtual	status_t			Resize(uint64 newSize, bool fillWithZeroes,
									Transaction& transaction);
	virtual	status_t			Read(off_t pos, void* buffer, size_t size,
									size_t& _bytesRead);
	virtual	status_t			Write(off_t pos, const void* buffer,
									size_t size, size_t& _bytesWritten,
									bool& _sizeChanged);
	virtual	status_t			Sync();

	virtual	void				RevertNodeData(const checksumfs_node& nodeData);

			status_t			GetFileVecs(uint64 offset, size_t size,
									file_io_vec* vecs, size_t count,
									size_t& _count);

			void*				FileMap() const	{ return fFileMap; }

private:
			struct LevelInfo;

private:
	static	uint32				_DepthForBlockCount(uint64 blockCount);
	static	void				_UpdateLevelInfos(LevelInfo* infos,
									int32 levelCount, uint64 blockCount);
	static	LevelInfo*			_GetLevelInfos(uint64 blockCount,
									int32& _levelCount);

			status_t			_ShrinkTree(uint64 blockCount,
									uint64 newBlockCount,
									Transaction& transaction);
			status_t			_GrowTree(uint64 blockCount,
									uint64 newBlockCount,
									Transaction& transaction);

			status_t			_WriteZeroes(uint64 offset, uint64 size);
			status_t			_WriteData(uint64 offset, const void* buffer,
									size_t size, size_t& _bytesWritten);

private:
			void*				fFileCache;
			void*				fFileMap;
};


#endif	// FILE_H
