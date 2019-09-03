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

static const uint8 kDeviceIcon[] = {
	0x6e, 0x63, 0x69, 0x66, 0x08, 0x05, 0x00, 0x04, 0x00, 0x54, 0x02, 0x00,
	0x06, 0x02, 0x3b, 0x01, 0x9b, 0x3a, 0xa2, 0x35, 0xbc, 0x24, 0x3e, 0x3c,
	0x71, 0xd2, 0x48, 0xd1, 0x7c, 0x49, 0x84, 0x91, 0x00, 0xe7, 0xbb, 0x8f,
	0xff, 0xc9, 0x98, 0x67, 0x02, 0x00, 0x06, 0x02, 0x3b, 0xa7, 0x11, 0x38,
	0xd0, 0xc8, 0xbb, 0xf4, 0xb8, 0x3e, 0x90, 0xe6, 0x4a, 0xed, 0x7c, 0x48,
	0x5b, 0xd7, 0x00, 0x8a, 0x56, 0x1d, 0xff, 0xb5, 0x7a, 0x3a, 0x02, 0x00,
	0x06, 0x02, 0xbb, 0x6f, 0xcb, 0xb8, 0xd4, 0xc8, 0x39, 0xaa, 0x71, 0xbc,
	0x39, 0x92, 0x49, 0x2f, 0xf1, 0x48, 0xd9, 0x6a, 0x00, 0xff, 0xc7, 0x90,
	0xff, 0xff, 0xf4, 0xea, 0x03, 0x66, 0x33, 0x00, 0x03, 0xff, 0xdf, 0xc0,
	0x03, 0xad, 0x72, 0x38, 0x11, 0x0a, 0x06, 0x26, 0x54, 0x3a, 0x46, 0x4c,
	0x45, 0x5c, 0x4b, 0x4c, 0x60, 0x3e, 0x60, 0x0a, 0x06, 0x38, 0x22, 0x26,
	0x2e, 0x26, 0x4f, 0x3c, 0x5a, 0x4e, 0x48, 0x4e, 0x2a, 0x0a, 0x04, 0x26,
	0x2e, 0x26, 0x4f, 0x3c, 0x5a, 0x3c, 0x37, 0x0a, 0x04, 0x3c, 0x37, 0x3c,
	0x5a, 0x4e, 0x48, 0x4e, 0x2a, 0x0a, 0x04, 0x38, 0x22, 0x26, 0x2e, 0x3c,
	0x37, 0x4e, 0x2a, 0x0a, 0x04, 0x28, 0x32, 0x28, 0x4e, 0x3a, 0x57, 0x3a,
	0x39, 0x0a, 0x04, 0x2a, 0x4d, 0x2b, 0x46, 0x38, 0x49, 0x2a, 0x43, 0x0a,
	0x04, 0x2a, 0x4d, 0x36, 0x52, 0x38, 0x49, 0x2b, 0x46, 0x0a, 0x04, 0x2a,
	0x4d, 0x38, 0x54, 0x38, 0x49, 0x36, 0x52, 0x0a, 0x04, 0x2e, 0x4c, 0xbb,
	0x2b, 0xc5, 0xd3, 0xbb, 0x2b, 0xc5, 0x07, 0x2e, 0x4a, 0x0a, 0x04, 0x2c,
	0x49, 0x34, 0x4d, 0x34, 0x4b, 0x2c, 0x47, 0x0a, 0x04, 0x2a, 0x35, 0x2a,
	0x40, 0x2b, 0x38, 0x38, 0x3b, 0x0a, 0x04, 0x36, 0x44, 0x2a, 0x40, 0x38,
	0x46, 0x38, 0x3b, 0x0a, 0x04, 0x2b, 0x38, 0x2a, 0x40, 0x36, 0x44, 0x38,
	0x3b, 0x0a, 0x04, 0x2e, 0xbe, 0x67, 0x2e, 0xbf, 0x33, 0xbb, 0x2d, 0xc0,
	0x3f, 0xbb, 0x2d, 0xbf, 0x73, 0x0a, 0x04, 0x2c, 0xbd, 0x29, 0x2c, 0xbd,
	0xf5, 0x34, 0x3f, 0x34, 0x3d, 0x08, 0x02, 0x2a, 0x4e, 0x2a, 0x54, 0x0e,
	0x0a, 0x01, 0x01, 0x00, 0x00, 0x0a, 0x00, 0x01, 0x10, 0x10, 0x01, 0x17,
	0x84, 0x20, 0x04, 0x0a, 0x00, 0x01, 0x10, 0x30, 0x30, 0x29, 0x01, 0x17,
	0x84, 0x20, 0x04, 0x0a, 0x00, 0x01, 0x10, 0x30, 0x40, 0x1b, 0x01, 0x17,
	0x84, 0x20, 0x04, 0x0a, 0x00, 0x01, 0x01, 0x10, 0x01, 0x17, 0x84, 0x00,
	0x04, 0x0a, 0x02, 0x01, 0x02, 0x00, 0x0a, 0x03, 0x01, 0x03, 0x00, 0x0a,
	0x04, 0x01, 0x04, 0x00, 0x0a, 0x05, 0x01, 0x05, 0x00, 0x0a, 0x06, 0x02,
	0x0b, 0x06, 0x00, 0x0a, 0x02, 0x02, 0x07, 0x0d, 0x00, 0x0a, 0x07, 0x02,
	0x0c, 0x08, 0x00, 0x0a, 0x03, 0x02, 0x09, 0x0e, 0x08, 0x15, 0xff, 0x0a,
	0x00, 0x02, 0x0a, 0x0f, 0x08, 0x15, 0xff
};


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

	if (IS_USER_ADDRESS(buffer))
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

		case B_GET_ICON_NAME:
			return user_strlcpy((char *)buffer, "devices/device-volume",
				B_FILE_NAME_LENGTH);

		case B_GET_VECTOR_ICON:
		{
			if (length != sizeof(device_icon)) {
				return B_BAD_VALUE;
			}

			device_icon iconData;
			if (user_memcpy(&iconData, buffer, sizeof(device_icon)) != B_OK) {
				return B_BAD_ADDRESS;
			}

			if (iconData.icon_size >= (int32)sizeof(kDeviceIcon)) {
				if (user_memcpy(iconData.icon_data, kDeviceIcon,
						sizeof(kDeviceIcon)) != B_OK) {
					return B_BAD_ADDRESS;
				}
			}

			iconData.icon_size = sizeof(kDeviceIcon);
			return user_memcpy(buffer, &iconData, sizeof(device_icon));
		}

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
