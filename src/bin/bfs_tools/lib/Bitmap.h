/*
 * Copyright 2001-2008, pinc Software. All Rights Reserved.
 */
#ifndef BITMAP_H
#define BITMAP_H


#include <SupportDefs.h>

class Disk;
class Inode;


class Bitmap {
	public:
		Bitmap(Disk *disk);
		Bitmap();
		~Bitmap();

		status_t	SetTo(Disk *disk);
		status_t	InitCheck();

		off_t		FreeBlocks() const;
		off_t		UsedBlocks() const { return fUsedBlocks; }

		bool		UsedAt(off_t block) const;
		bool		BackupUsedAt(off_t block) const;
		bool		BackupSetAt(off_t block,bool used);
		void		BackupSet(Inode *inode,bool used);

		status_t	Validate();
		status_t	CompareWithBackup();
		bool		TrustBlockContaining(off_t block) const;

		size_t		Size() const { return fSize; }

	private:
		Disk	*fDisk;
		uint32	*fBitmap;
		uint32	*fBackupBitmap;
		size_t	fSize;
		size_t	fByteSize;
		off_t	fUsedBlocks;
};

#endif	/* BITMAP_H */
