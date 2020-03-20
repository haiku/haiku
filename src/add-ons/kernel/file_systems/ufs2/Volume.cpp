/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Volume.h"

//#define TRACE_UFS2
#ifdef TRACE_UFS2
#	define TRACE(x...) dprintf("\33[34mufs2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#   define ERROR(x...) dprintf("\33[34mexfat:\33[0m " x)

class DeviceOpner {
	public:
								DeviceOpner(int fd, int mode);
								DeviceOpner(const char* device, int mode);
								~DeviceOpner();

				int				Open(const char* device, int mode);
				int				Open(int fd, int mode);
				void*			InitCache(off_t numBlocks, uint32 blockSize);
				void			RemoveCache(bool allowWrites);

				void			Keep();

				int				Device() const { return fDevice; }
				int				Mode() const { return fMode; }
				bool			IsReadOnly() const { return _IsReadOnly(fMode); }

				status_t		GetSize(off_t* _size, uint32* _blockSize = NULL);

	private:
			static	bool		_IsReadOnly(int mode)
									{ return (mode & O_RWMASK) == O_RDONLY; }
			static	bool		_IsReadWrite(int mode)
									{ return (mode & O_RWMASK) == O_RDWR; }

					int			fDevice;
					int			fMode;
					void*		fBlockCache;
};

bool
ufs2_super_block::IsValid()
{
	if (ufs2_magic != UFS2_SUPER_BLOCK_MAGIC)
		return false;

	return true;
}

status_t
Volume::Identify(int fd, ufs2_super_block *superBlock)
{
	if (read_pos(fd, UFS2_SUPER_BLOCK_OFFSET, superBlock,
		sizeof(ufs2_super_block)) != sizeof(ufs2_super_block))
		return B_IO_ERROR;

	if (!superBlock->IsValid()) {
		ERROR("invalid superblock! Identify failed!!\n");
		return B_BAD_VALUE;
	}

	return B_OK;
}
