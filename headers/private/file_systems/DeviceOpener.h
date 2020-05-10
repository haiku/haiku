/*
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef DEVICEOPENER_H
#define DEVICEOPENER_H

#ifdef FS_SHELL
#	include "fssh_api_wrapper.h"
#else
#	include <fcntl.h>
#	include <sys/types.h>
#	include <SupportDefs.h>
#endif


class DeviceOpener {
	public:
								DeviceOpener(int fd, int mode);
								DeviceOpener(const char* device, int mode);
								~DeviceOpener();

				int				Open(const char* device, int mode);
				int				Open(int fd, int mode);
				void*			InitCache(off_t numBlocks, uint32 blockSize);
				void			RemoveCache(bool allowWrites);

				void			Keep();

				int				Device() const { return fDevice; }
				int				Mode() const { return fMode; }
				bool			IsReadOnly() const {
												return _IsReadOnly(fMode); }

				status_t		GetSize(off_t* _size,
										uint32* _blockSize = NULL);

	private:
			static	bool		_IsReadOnly(int mode)
									{ return (mode & O_RWMASK) == O_RDONLY; }
			static	bool		_IsReadWrite(int mode)
									{ return (mode & O_RWMASK) == O_RDWR; }

					int			fDevice;
					int			fMode;
					void*		fBlockCache;
};

#endif	// DEVICEOPENER_H
