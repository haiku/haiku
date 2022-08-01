/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include "HttpBuffer.h"

#include <DataIO.h>
#include <NetServicesDefs.h>
#include <String.h>

using namespace BPrivate::Network;


/*!
	\brief Newline sequence

	As per the RFC, defined as \r\n
*/
static constexpr std::array<std::byte, 2> kNewLine = {std::byte('\r'), std::byte('\n')};


/*!
	\brief Create a new HTTP buffer with \a capacity.
*/
HttpBuffer::HttpBuffer(size_t capacity)
{
	fBuffer.reserve(capacity);
};


/*!
	\brief Load data from \a source into the spare capacity of this buffer.

	\exception BNetworkRequestError When BDataIO::Read() returns any error other than B_WOULD_BLOCK

	\retval B_WOULD_BLOCK The read call on the \a source was unsuccessful because it would block.
	\retval >=0 The actual number of bytes read.
*/
ssize_t
HttpBuffer::ReadFrom(BDataIO* source)
{
	// Remove any unused bytes at the beginning of the buffer
	Flush();

	auto currentSize = fBuffer.size();
	auto remainingBufferSize = fBuffer.capacity() - currentSize;

	// Adjust the buffer to the maximum size
	fBuffer.resize(fBuffer.capacity());

	ssize_t bytesRead = B_INTERRUPTED;
	while (bytesRead == B_INTERRUPTED)
		bytesRead = source->Read(fBuffer.data() + currentSize, remainingBufferSize);

	if (bytesRead == B_WOULD_BLOCK || bytesRead == 0) {
		fBuffer.resize(currentSize);
		return bytesRead;
	} else if (bytesRead < 0) {
		throw BNetworkRequestError("BDataIO::Read()", BNetworkRequestError::NetworkError,
			bytesRead);
	}

	// Adjust the buffer to the current size
	fBuffer.resize(currentSize + bytesRead);

	return bytesRead;
}


/*!
	\brief Use BDataIO::WriteExactly() on target to write the contents of the buffer.
*/
void
HttpBuffer::WriteExactlyTo(BDataIO* target)
{
	if (RemainingBytes() == 0)
		return;

	auto status = target->WriteExactly(fBuffer.data() + fCurrentOffset, RemainingBytes());
	if (status != B_OK) {
		throw BNetworkRequestError("BDataIO::WriteExactly()", BNetworkRequestError::SystemError,
			status);
	}

	// Entire buffer is written; reset internal buffer
	Clear();
}


/*!
	\brief Write the contents of the buffer through the helper \a func.

	\param func Handle the actual writing. The function accepts a pointer and a size as inputs
		and should return the number of actual written bytes, which may be fewer than the number
		of available bytes.
*/
void
HttpBuffer::WriteTo(HttpTransferFunction func , std::optional<size_t> maxSize)
{
	if (RemainingBytes() == 0)
		return;

	auto size = RemainingBytes();
	if (maxSize.has_value() && *maxSize < size)
		size = *maxSize;

	auto bytesWritten = func(fBuffer.data() + fCurrentOffset, size);
	if (bytesWritten > size)
		throw BRuntimeError(__PRETTY_FUNCTION__, "More bytes written than were made available");

	fCurrentOffset += bytesWritten;
}


/*!
	\brief Get the next line from this buffer.

	This can be called iteratively until all lines in the current data are read. After using this
	method, you should use Flush() to make sure that the read lines are cleared from the beginning
	of the buffer.

	\retval std::nullopt There are no more lines in the buffer.
	\retval BString The next line.
*/
std::optional<BString>
HttpBuffer::GetNextLine()
{
	auto offset = fBuffer.cbegin() + fCurrentOffset;
	auto result = std::search(offset, fBuffer.cend(), kNewLine.cbegin(), kNewLine.cend());
	if (result == fBuffer.cend())
		return std::nullopt;

	BString line(reinterpret_cast<const char*>(std::addressof(*offset)), std::distance(offset, result));
	fCurrentOffset = std::distance(fBuffer.cbegin(), result) + 2;
	return line;
}


/*!
	\brief Get the number of remaining bytes in this buffer.
*/
size_t
HttpBuffer::RemainingBytes() noexcept
{
	return fBuffer.size() - fCurrentOffset;
}


/*!
	\brief Move data to the beginning of the buffer to clear at the back.

	The GetNextLine() increases the offset of the internal buffer. This call moves remaining data
	to the beginning of the buffer sets the correct size, making the remainder of the capacity
	available for further reading.
*/
void
HttpBuffer::Flush() noexcept
{
	if (fCurrentOffset > 0) {
		auto end = fBuffer.cbegin() + fCurrentOffset;
		fBuffer.erase(fBuffer.cbegin(), end);
		fCurrentOffset = 0;
	}
}


/*!
	\brief Clear the internal buffer
*/
void
HttpBuffer::Clear() noexcept
{
	fBuffer.clear();
	fCurrentOffset = 0;
}
