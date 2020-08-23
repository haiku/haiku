/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef INODE_H
#define INODE_H

#include "ufs2.h"
#include "Volume.h"

#define	UFS2_ROOT	((ino_t)2)


struct timeval32
{
	int tv_sec, tv_usec;
};


struct ufs2_inode {
	u_int16_t	fileMode;
	int16_t		linkCount;
	int32_t		userId;
	int32_t		groupId;
	int32_t		inodeBlockSize;
	int64_t		size;
	int64_t		byteHeld;
	struct		timeval32	accessTime;
	struct		timeval32	modifiedTime;
	struct		timeval32	changeTime;
	struct		timeval32	createTime;
	/* time in nano seconds */
	int32_t		nsModifiedTime;
	int32_t		nsAccessTime;
	int32_t		nsChangeTime;
	int32_t		nsCreateTime;
	int32_t		generationNumber;
	int32_t		kernelFlags;
	int32_t		statusFlags;
	int32_t		extendedAttSize; /* Extended attribute size */
	/* 2 Direct extended attribute block pointers */
	int64_t		extendedBklPtr1;
	int64_t		extendedBklPtr2;

	union {
		struct {
			int64_t		directBlockPointer[12];
			int64_t		indirectBlockPointer;
			int64_t		doubleIndriectBlockPointer;
			int64_t		tripleIndriectBlockPointer;
		};
		char	symlinkpath[120];
	};
	/* Unused but need to figure out what are they */
	int64_t		unused1;
	int64_t		unused2;
	int64_t		unused3;


	static void _DecodeTime(struct timespec& timespec, timeval32 time)
	{
		timespec.tv_sec = time.tv_sec;
		timespec.tv_nsec = time.tv_usec * 1000;
	}
	void GetAccessTime(struct timespec& timespec) const
		{ _DecodeTime(timespec, accessTime); }
	void GetChangeTime(struct timespec& timespec) const
		{ _DecodeTime(timespec, changeTime); }
	void GetModificationTime(struct timespec& timespec) const
		{ _DecodeTime(timespec, modifiedTime); }
	void GetCreationTime(struct timespec& timespec) const
		{ _DecodeTime(timespec, createTime); }
};

class Inode {
	public:
						Inode(Volume* volume, ino_t id);
						Inode(Volume* volume, ino_t id,
							const ufs2_inode& item);
						~Inode();

			status_t	InitCheck();

			ino_t		ID() const { return fID; }
//
			rw_lock*	Lock() { return& fLock; }
//
//			status_t	UpdateNodeFromDisk();

			bool		IsDirectory() const
							{ return S_ISDIR(Mode()); }
			bool		IsFile() const
							{ return S_ISREG(Mode()); }
			bool		IsSymLink() const
							{ return S_ISLNK(Mode()); }
//			status_t	CheckPermissions(int accessMode) const;

			mode_t		Mode() const { return fNode.fileMode; }
			off_t		Size() const { return fNode.size; }
			uid_t		UserID() const { return fNode.userId; }
			gid_t		GroupID() const { return fNode.groupId; }
			void		GetChangeTime(struct timespec& timespec) const
							{ fNode.GetChangeTime(timespec); }
			void		GetModificationTime(struct timespec& timespec) const
							{ fNode.GetModificationTime(timespec); }
			void		GetCreationTime(struct timespec& timespec) const
							{ fNode.GetCreationTime(timespec); }
			void		GetAccessTime(struct timespec& timespec) const
							{ fNode.GetAccessTime(timespec); }

			Volume*		GetVolume() const { return fVolume; }

			ino_t		Parent();

			off_t   	FindBlock(off_t block_number, off_t block_offset);
			status_t	ReadAt(off_t pos, uint8* buffer, size_t* length);
			status_t	ReadLink(char* buffer, size_t *_bufferSize);
//			status_t	FillGapWithZeros(off_t start, off_t end);

			void*		FileCache() const { return fCache; }
			void*		Map() const { return fMap; }

//			status_t	FindParent(ino_t* id);
	//static	Inode*		Create(Transaction& transaction, ino_t id, Inode* parent, int32 mode, uint64 size = 0, uint64 flags = 0);
private:
						Inode(Volume* volume);
						Inode(const Inode&);
						Inode& operator=(const Inode&);
			int64_t		GetBlockPointer(int ptrNumber) {
						return fNode.directBlockPointer[ptrNumber]; }

			int64_t 	GetIndirectBlockPointer() {
						return fNode.indirectBlockPointer; }

			int64_t 	GetDoubleIndirectBlockPtr() {
						return fNode.doubleIndriectBlockPointer; }

			int64_t 	GetTripleIndirectBlockPtr() {
						return fNode.tripleIndriectBlockPointer; }

//			uint64		_NumBlocks();
private:

			rw_lock		fLock;
			::Volume*	fVolume;
			ino_t		fID;
			void*		fCache;
			void*		fMap;
			status_t	fInitStatus;
			ufs2_inode	fNode;
};

#endif	// INODE_H
