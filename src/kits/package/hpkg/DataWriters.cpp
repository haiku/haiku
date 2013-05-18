/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/DataWriters.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <package/hpkg/ErrorOutput.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


// #pragma mark - AbstractDataWriter


AbstractDataWriter::AbstractDataWriter()
	:
	fBytesWritten(0)
{
}


AbstractDataWriter::~AbstractDataWriter()
{
}


// #pragma mark - FDDataWriter


FDDataWriter::FDDataWriter(int fd, off_t offset, BErrorOutput* errorOutput)
	:
	fFD(fd),
	fOffset(offset),
	fErrorOutput(errorOutput)
{
}


status_t
FDDataWriter::WriteDataNoThrow(const void* buffer, size_t size)
{
	ssize_t bytesWritten = pwrite(fFD, buffer, size, fOffset);
	if (bytesWritten < 0) {
		fErrorOutput->PrintError(
			"WriteDataNoThrow(%p, %lu) failed to write data: %s\n", buffer,
			size, strerror(errno));
		return errno;
	}
	if ((size_t)bytesWritten != size) {
		fErrorOutput->PrintError(
			"WriteDataNoThrow(%p, %lu) failed to write all data\n", buffer,
			size);
		return B_ERROR;
	}

	fOffset += size;
	fBytesWritten += size;
	return B_OK;
}


// #pragma mark - ZlibDataWriter


ZlibDataWriter::ZlibDataWriter(AbstractDataWriter* dataWriter)
	:
	fDataWriter(dataWriter),
	fCompressor(this)
{
}


void
ZlibDataWriter::Init()
{
	status_t error = fCompressor.Init();
	if (error != B_OK)
		throw status_t(error);
}


void
ZlibDataWriter::Finish()
{
	status_t error = fCompressor.Finish();
	if (error != B_OK)
		throw status_t(error);
}


status_t
ZlibDataWriter::WriteDataNoThrow(const void* buffer,
	size_t size)
{
	status_t error = fCompressor.CompressNext(buffer, size);
	if (error == B_OK)
		fBytesWritten += size;
	return error;
}


status_t
ZlibDataWriter::WriteData(const void* buffer, size_t size)
{
	return fDataWriter->WriteDataNoThrow(buffer, size);
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
