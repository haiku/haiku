/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "File.h"

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>


using std::nothrow;


namespace FFS {


class Stream {
	public:
		Stream(int device, FileBlock &node);
		~Stream();

		status_t InitCheck();
		ssize_t ReadAt(off_t offset, uint8 *buffer, size_t size);

	private:
		int32 BlockOffset(off_t offset) const;
		int32 BlockIndex(off_t offset) const;
		int32 ExtensionBlockOffset(off_t offset) const;
		status_t ReadNextExtension();

		int			fDevice;
		FileBlock	&fNode;
		FileBlock	fBlock;
		int32		fExtensionBlockOffset;
};


Stream::Stream(int device, FileBlock &node)
	:
	fDevice(device),
	fNode(node)
{
	void *buffer = malloc(fNode.BlockSize());
	if (buffer == NULL)
		return;

	fExtensionBlockOffset = 0;
	fBlock.SetTo(buffer, fNode.BlockSize());
}


Stream::~Stream()
{
	free(fBlock.BlockData());
}


status_t
Stream::InitCheck()
{
	return fBlock.BlockData() != NULL ? B_OK : B_NO_MEMORY;
}


int32
Stream::BlockOffset(off_t offset) const
{
	return offset % fNode.BlockSize();
}


int32
Stream::BlockIndex(off_t offset) const
{
	return (offset % (fNode.BlockSize() * fNode.NumDataBlocks())) / fNode.BlockSize();
}


int32
Stream::ExtensionBlockOffset(off_t offset) const
{
	return offset / (fNode.BlockSize() * fNode.NumDataBlocks());
}


status_t
Stream::ReadNextExtension()
{
	int32 next;
	if (fExtensionBlockOffset == 0)
		next = fNode.NextExtension();
	else
		next = fBlock.NextExtension();

	if (read_pos(fDevice, next * fNode.BlockSize(), fBlock.BlockData(), fNode.BlockSize()) < B_OK)
		return B_ERROR;

	return fBlock.ValidateCheckSum();
}


ssize_t
Stream::ReadAt(off_t offset, uint8 *buffer, size_t size)
{
	if (offset < 0)
		return B_BAD_VALUE;
	if (offset + (off_t)size > fNode.Size())
		size = fNode.Size() - offset;

	ssize_t bytesLeft = (ssize_t)size;

	while (bytesLeft != 0) {
		int32 extensionBlock = ExtensionBlockOffset(offset);

		// get the right extension block

		if (extensionBlock < fExtensionBlockOffset)
			fExtensionBlockOffset = 1;

		while (fExtensionBlockOffset < extensionBlock) {
			if (ReadNextExtension() != B_OK)
				return B_ERROR;

			fExtensionBlockOffset++;
		}

		// read the data block into memory

		int32 block;
		if (extensionBlock == 0)
			block = fNode.DataBlock(BlockIndex(offset));
		else
			block = fBlock.DataBlock(BlockIndex(offset));

		int32 blockOffset = BlockOffset(offset);
		int32 toRead = fNode.BlockSize() - blockOffset;
		if (toRead > bytesLeft)
			toRead = bytesLeft;

		ssize_t bytesRead = read_pos(fDevice, block * fNode.BlockSize() + blockOffset,
			buffer, toRead);
		if (bytesRead < 0)
			return errno;

		bytesLeft -= bytesRead;
		buffer += bytesRead;
		offset += fNode.BlockSize() - blockOffset;
	}

	return size;
}


//	#pragma mark -


File::File(Volume &volume, int32 block)
	:
	fVolume(volume)
{
	void *data = malloc(volume.BlockSize());
	if (data == NULL)
		return;

	if (read_pos(volume.Device(), block * volume.BlockSize(), data, volume.BlockSize()) == volume.BlockSize())
		fNode.SetTo(data, volume.BlockSize());
}


File::~File()
{
}


status_t
File::InitCheck()
{
	if (!fNode.IsFile())
		return B_BAD_TYPE;

	return fNode.ValidateCheckSum();
}


status_t
File::Open(void **_cookie, int mode)
{
	Stream *stream = new(nothrow) Stream(fVolume.Device(), fNode);
	if (stream == NULL)
		return B_NO_MEMORY;

	if (stream->InitCheck() != B_OK) {
		delete stream;
		return B_NO_MEMORY;
	}

	*_cookie = (void *)stream;
	return B_OK;
}


status_t
File::Close(void *cookie)
{
	Stream *stream = (Stream *)cookie;

	delete stream;
	return B_OK;
}


ssize_t
File::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	Stream *stream = (Stream *)cookie;
	if (stream == NULL)
		return B_BAD_VALUE;

	return stream->ReadAt(pos, (uint8 *)buffer, bufferSize);
}


ssize_t
File::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	return EROFS;
}


status_t
File::GetName(char *nameBuffer, size_t bufferSize) const
{
	return fNode.GetName(nameBuffer, bufferSize);
}


int32
File::Type() const
{
	return S_IFREG;
}


off_t
File::Size() const
{
	return fNode.Size();
}


ino_t
File::Inode() const
{
	return fNode.HeaderKey();
}


}	// namespace FFS
