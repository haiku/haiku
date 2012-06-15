/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "FileDevice.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include <fs_interface.h>

#include <vfs.h>


static const uint32 kBlockSize = 512;


struct FileDevice::Cookie {
	int	fd;

	Cookie(int fd)
		:
		fd(fd)
	{
	}

	~Cookie()
	{
		if (fd >= 0)
			close(fd);
	}
};


FileDevice::FileDevice()
	:
	fFD(-1),
	fFileSize(0)
{
}


FileDevice::~FileDevice()
{
	if (fFD >= 0)
		close(fFD);
}


status_t
FileDevice::Init(const char* path)
{
	fFD = open(path, O_RDONLY | O_NOTRAVERSE);
	if (fFD < 0)
		return errno;

	struct stat st;
	if (fstat(fFD, &st) != 0)
		return errno;

	if (!S_ISREG(st.st_mode))
		return B_BAD_TYPE;

	fFileSize = st.st_size / kBlockSize * kBlockSize;

	return B_OK;
}


status_t
FileDevice::InitDevice()
{
	return B_OK;
}


void
FileDevice::UninitDevice()
{
}


void
FileDevice::Removed()
{
	delete this;
}


bool
FileDevice::HasSelect() const
{
	return false;
}


bool
FileDevice::HasDeselect() const
{
	return false;
}


bool
FileDevice::HasRead() const
{
	return true;
}


bool
FileDevice::HasWrite() const
{
	return true;
}


bool
FileDevice::HasIO() const
{
	// TODO: Support!
	return false;
}


status_t
FileDevice::Open(const char* path, int openMode, void** _cookie)
{
	// get the vnode
	struct vnode* vnode;
	status_t error = vfs_get_vnode_from_fd(fFD, true, &vnode);
	if (error != B_OK)
		return error;

	// open it
	int fd = vfs_open_vnode(vnode, openMode, true);
	if (fd < 0) {
		vfs_put_vnode(vnode);
		return fd;
	}
	// our vnode reference does now belong to the FD

	Cookie* cookie = new(std::nothrow) Cookie(fd);
	if (cookie == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	*_cookie = cookie;
	return B_OK;
}


status_t
FileDevice::Read(void* _cookie, off_t pos, void* buffer, size_t* _length)
{
	Cookie* cookie = (Cookie*)_cookie;

	ssize_t bytesRead = pread(cookie->fd, buffer, *_length, pos);
	if (bytesRead < 0) {
		*_length = 0;
		return errno;
	}

	*_length = bytesRead;
	return B_OK;
}


status_t
FileDevice::Write(void* _cookie, off_t pos, const void* buffer, size_t* _length)
{
	Cookie* cookie = (Cookie*)_cookie;

	ssize_t bytesWritten = pwrite(cookie->fd, buffer, *_length, pos);
	if (bytesWritten < 0) {
		*_length = 0;
		return errno;
	}

	*_length = bytesWritten;
	return B_OK;
}


status_t
FileDevice::IO(void* _cookie, io_request* request)
{
//	Cookie* cookie = (Cookie*)_cookie;
//	return do_fd_io(cookie->fd, request);
// TODO: The implementation is fine in principle, but do_fd_io() requires either
// the io() hook or the {read,write}_pages() hooks of the underlying FS to be
// implemented, which we can't guarantee. do_fd_io() should work around by using
// read() and write(), but it's all quite of a mess, since we mix up the io()
// hook -- which ATM has the semantics of uncached_io() hook (i.e. ignoring the
// file cache) -- with the actual io() hook semantics (i.e. using the file
// cache).
	return B_UNSUPPORTED;
}


template<typename ResultType>
static status_t
set_ioctl_result(const ResultType& result, void* buffer, size_t length)
{
	// NOTE: We omit the buffer size check for sake of callers (e.g. BFS) not
	// specifying a length argument.
//	if (sizeof(ResultType) < length)
//		return B_BAD_VALUE;

	if (buffer == NULL)
		return B_BAD_ADDRESS;

	if (!IS_USER_ADDRESS(buffer))
		return user_memcpy(buffer, &result, sizeof(ResultType));

	memcpy(buffer, &result, sizeof(ResultType));
	return B_OK;
}


status_t
FileDevice::Control(void* _cookie, int32 op, void* buffer, size_t length)
{
	Cookie* cookie = (Cookie*)_cookie;

	switch (op) {
		case B_GET_DEVICE_SIZE:
			return set_ioctl_result(
				(uint64)fFileSize > (uint64)(~(size_t)0) ? ~(size_t)0 : (size_t)fFileSize,
				buffer, length);

		case B_SET_BLOCKING_IO:
		case B_SET_NONBLOCKING_IO:
			// TODO: Translate to O_NONBLOCK and pass on!
			return B_OK;

		case B_GET_READ_STATUS:
		case B_GET_WRITE_STATUS:
			// TODO: poll() the FD!
			return set_ioctl_result(true, buffer, length);

		case B_GET_ICON:
			return B_UNSUPPORTED;

		case B_GET_GEOMETRY:
		case B_GET_BIOS_GEOMETRY:
		{
			// fill in the geometry
			// Optimally we have only 1 block per sector and only one head.
			// Since we have only a uint32 for the cylinder count, this won't
			// work for files > 2TB. So, we set the head count to the minimally
			// possible value.
			off_t blocks = fFileSize / kBlockSize;
			uint32 heads = (blocks + 0xfffffffe) / 0xffffffff;
			if (heads == 0)
				heads = 1;

			device_geometry geometry;
			geometry.bytes_per_sector = kBlockSize;
		    geometry.sectors_per_track = 1;
		    geometry.cylinder_count = blocks / heads;
		    geometry.head_count = heads;
		    geometry.device_type = B_DISK;
		    geometry.removable = false;
		    geometry.read_only = false;
		    geometry.write_once = false;

			return set_ioctl_result(geometry, buffer, length);
		}

		case B_GET_MEDIA_STATUS:
			return set_ioctl_result((status_t)B_OK, buffer, length);

		case B_SET_INTERRUPTABLE_IO:
		case B_SET_UNINTERRUPTABLE_IO:
			return B_OK;

		case B_FLUSH_DRIVE_CACHE:
			return fsync(cookie->fd) == 0 ? B_OK : errno;

		case B_GET_BIOS_DRIVE_ID:
			return set_ioctl_result((uint8)0xf8, buffer, length);

		case B_GET_DRIVER_FOR_DEVICE:
		case B_SET_DEVICE_SIZE:
		case B_SET_PARTITION:
		case B_FORMAT_DEVICE:
		case B_EJECT_DEVICE:
		case B_LOAD_MEDIA:
		case B_GET_NEXT_OPEN_DEVICE:
		default:
			return B_BAD_VALUE;
	}

	return B_OK;
}


status_t
FileDevice::Select(void* _cookie, uint8 event, selectsync* sync)
{
	// TODO: Support (select_fd())!
	return B_UNSUPPORTED;
}


status_t
FileDevice::Deselect(void* cookie, uint8 event, selectsync* sync)
{
	// TODO: Support (deselect_fd())!
	return B_UNSUPPORTED;
}


status_t
FileDevice::Close(void* cookie)
{
	// TODO: This should probably really close the FD. Depending on the
	// underlying FS operations could block and close() would be needed to
	// unblock them.
	return B_OK;
}


status_t
FileDevice::Free(void* _cookie)
{
	delete (Cookie*)_cookie;
	return B_OK;
}
