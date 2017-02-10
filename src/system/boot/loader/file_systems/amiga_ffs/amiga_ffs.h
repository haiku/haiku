/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef AMIGA_FFS_H
#define AMIGA_FFS_H


#include <SupportDefs.h>
#include <ByteOrder.h>


namespace FFS {

/** The base class for all FFS blocks */

class BaseBlock{
	public:
		BaseBlock() : fData(NULL) {}
		BaseBlock(void *data, int32 blockSize) { SetTo(data, blockSize); }
		BaseBlock(int32 blockSize) { fSize = blockSize >> 2; }

		void SetTo(void *data, int32 blockSize) { fData = (int32 *)data; fSize = blockSize >> 2; }
		void SetTo(void *data) { fData = (int32 *)data; }

		int32 *BlockData() const { return fData; }
		int32 BlockSize() const { return fSize << 2; }
		int32 LongWords() const { return fSize; }

		int32 PrimaryType() const { return Offset(0); }
		int32 SecondaryType() const { return BackOffset(1); }
		int32 CheckSum() const { return Offset(5); }

		inline bool IsRootBlock() const;
		inline bool IsDirectory() const;
		inline bool IsFile() const;
		inline bool IsExtensionBlock() const;
		inline bool IsDirectoryLink() const;
		inline bool IsFileLink() const;
		inline bool IsSymbolicLink() const;

		status_t ValidateCheckSum() const;

	protected:
		int32 Offset(int16 i) const { return B_BENDIAN_TO_HOST_INT32(fData[i]); }
		int32 BackOffset(int16 i) const { return B_BENDIAN_TO_HOST_INT32(fData[fSize - i]); }

		status_t GetNameBackOffset(int32 offset, char *name, size_t size) const;

	private:
		int32	fSize;
		int32	*fData;
};


/** The base class for all blocks that represent files and directories
 *	(all blocks that are accessible to the user)
 */

class NodeBlock : public BaseBlock {
	public:
		NodeBlock() {}
		NodeBlock(int32 blockSize) : BaseBlock(blockSize) {}
		NodeBlock(void *data, int32 blockSize) : BaseBlock(data, blockSize) {}

		int32 HeaderKey() const { return Offset(1); }
		int32 Protection() const { return BackOffset(48); }

		int32 Days() const { return BackOffset(23); }
		int32 Minute() const { return BackOffset(22); }
		int32 Ticks() const { return BackOffset(21); }

		status_t GetName(char *name, size_t size) const { return GetNameBackOffset(20, name, size); }

		int32 LinkChain() const { return BackOffset(10); }
		int32 HashChain() const { return BackOffset(4); }

		int32 Parent() const { return BackOffset(3); }
};


/** A standard user directory block */

class DirectoryBlock : public NodeBlock {
	public:
		DirectoryBlock() : NodeBlock() {}
		DirectoryBlock(int32 blockSize) : NodeBlock(blockSize) {}
		DirectoryBlock(void *data, int32 blockSize) : NodeBlock(data, blockSize) {}

		char ToUpperChar(int32 type, char c) const;
		int32 HashIndexFor(int32 type, const char *name) const;

		int32 HashValueAt(int32 index) const;
		int32 NextHashValue(int32 &index) const;
		int32 FirstHashValue(int32 &index) const;

	protected:
		int32 *HashTable() const { return BlockData() + 6; }
		int32 HashSize() const { return LongWords() - 56; }
};


/** The root block of the device and at the same time the root directory */

class RootBlock : public DirectoryBlock {
	public:
		RootBlock() : DirectoryBlock() {}
		RootBlock(int32 blockSize) : DirectoryBlock(blockSize) {}
		RootBlock(void *data, int32 blockSize) : DirectoryBlock(data, blockSize) {}

		int32 BitmapFlag() const { return BackOffset(50); }
		int32 BitmapExtension() const { return BackOffset(24); }

		int32 VolumeDays() const { return BackOffset(10); }
		int32 VolumeMinutes() const { return BackOffset(9); }
		int32 VolumeTicks() const { return BackOffset(8); }

		int32 CreationDays() const { return BackOffset(7); }
		int32 CreationMinutes() const { return BackOffset(6); }
		int32 CreationTicks() const { return BackOffset(5); }
};


/** A standard user file block */

class FileBlock : public NodeBlock {
	public:
		FileBlock() : NodeBlock() {}
		FileBlock(int32 blockSize) : NodeBlock(blockSize) {}
		FileBlock(void *data, int32 blockSize) : NodeBlock(data, blockSize) {}

		int32 BlockCount() const { return Offset(2); }
		int32 FirstData() const { return Offset(4); }
		int32 Size() const { return BackOffset(47); }
		int32 NextExtension() const { return BackOffset(2); }
			// The extension block is handled by this class as well

		int32 DataBlock(int32 index) const { return BackOffset(51 + index); }
		int32 NumDataBlocks() const { return LongWords() - 56; }
};

class HashIterator {
	public:
		HashIterator(int32 device, DirectoryBlock &node);
		~HashIterator();

		status_t InitCheck();
		void Goto(int32 index);
		NodeBlock *GetNext(int32 &block);
		void Rewind();

	private:
		DirectoryBlock	&fDirectory;
		int32		fDevice;
		int32		fCurrent;
		int32		fBlock;
		NodeBlock	fNode;
		int32		*fData;
};

enum primary_types {
	PT_SHORT			= 2,
	PT_DATA				= 8,
	PT_LIST				= 16,
};

enum secondary_types {
	ST_ROOT				= 1,
	ST_DIRECTORY		= 2,
	ST_FILE				= -3,
	ST_DIRECTORY_LINK	= 4,
	ST_FILE_LINK		= -4,
	ST_SOFT_LINK		= 3
};

enum dos_types {
	DT_AMIGA_OFS			= 'DOS\0',
	DT_AMIGA_FFS			= 'DOS\1',
	DT_AMIGA_FFS_INTL		= 'DOS\2',
	DT_AMIGA_FFS_DCACHE		= 'DOS\3',
};

enum protection_flags {
	FILE_IS_DELETABLE	= 1,
	FILE_IS_EXECUTABLE	= 2,
	FILE_IS_READABLE	= 4,
	FILE_IS_WRITABLE	= 8,
	FILE_IS_ARCHIVED	= 16,
	FILE_IS_PURE		= 32,
	FILE_IS_SCRIPT		= 64,
	FILE_IS_HOLD		= 128,
};

enum name_lengths {
	FFS_NAME_LENGTH		= 30,
	COMMENT_LENGTH		= 79,
};

status_t get_root_block(int fDevice, char *buffer, int32 blockSize, off_t partitionSize);

//	inline methods

inline bool 
BaseBlock::IsRootBlock() const
{
	return PrimaryType() == PT_SHORT && SecondaryType() == ST_ROOT;
}


inline bool 
BaseBlock::IsDirectory() const
{
	return PrimaryType() == PT_SHORT && SecondaryType() == ST_DIRECTORY;
}


inline bool 
BaseBlock::IsFile() const
{
	return PrimaryType() == PT_SHORT && SecondaryType() == ST_FILE;
}


inline bool 
BaseBlock::IsExtensionBlock() const
{
	return PrimaryType() == PT_LIST && SecondaryType() == ST_FILE;
}


inline bool 
BaseBlock::IsDirectoryLink() const
{
	return PrimaryType() == PT_SHORT && SecondaryType() == ST_DIRECTORY_LINK;
}


inline bool 
BaseBlock::IsFileLink() const
{
	return PrimaryType() == PT_SHORT && SecondaryType() == ST_FILE_LINK;
}


inline bool 
BaseBlock::IsSymbolicLink() const
{
	return PrimaryType() == PT_SHORT && SecondaryType() == ST_SOFT_LINK;
}

}	// namespace FFS

#endif	/* AMIGA_FFS_H */

